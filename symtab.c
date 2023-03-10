#include "symtab.h" // also includes `pool.h'
#include "debug.h"

// TODO: implement lexical scoping

#define SORT_FUNC_FOR(x) \
  static inline bool in_between__##x(struct lisp_sym* ppm,             \
                                     struct lisp_hash hash,            \
                                     uint lower, uint upper) {         \
    return hash.x >= ppm[lower].hash.x && hash.x <= ppm[upper].hash.x; \
  } \
  static inline bool ex_between__##x(struct lisp_sym* ppm,             \
                                     struct lisp_hash hash,            \
                                     uint lower, uint upper) {         \
    return hash.x > ppm[lower].hash.x && hash.x < ppm[upper].hash.x;   \
  } \
  static inline bool repeats__##x(const struct sort_t* sort) {         \
    return sort->mask & __LISP_HASH_##x;                               \
  } \
  static inline uint yield__##x(struct lisp_sym* ppm, uint i) {        \
    return (uint) ppm[i].hash.x;                                       \
  } \
  static inline bool eq__##x(uint n, struct lisp_hash hash) {          \
    return n == hash.x;                                                \
  } \
  static inline bool lt__##x(uint n, struct lisp_hash hash) {          \
    return n < hash.x;                                                 \
  } \
  static struct sort_t sort_##x = {  \
    .in_between  = in_between__##x,  \
    .ex_between  = ex_between__##x,  \
    .repeats     = repeats__##x,     \
    .yield       = yield__##x,       \
    .eq          = eq__##x,          \
    .lt          = lt__##x,          \
                                     \
    .mask        = 0,                \
    .next        = NULL,             \
  }

// what lack of classes does to a mfr...

SORT_FUNC_FOR(len);
SORT_FUNC_FOR(sum);
SORT_FUNC_FOR(psum);
SORT_FUNC_FOR(com_part);

static const struct sort_t* sort_entry = &sort_len;

////////////////////////////////////////////////////////////////////////////////

static uint hash_i = 1;

struct lisp_symtab_pp symtab_pp[SYMTAB_CELL] = {0};

// TODO: stub
static inline void pool_clean(POOL_T* pp) {
  return;
}

static inline int s(int t) {
  return t*(t+1)/2;
}

static inline int mod_norm(int val, int len) {
  return val - s(len);
}

static inline bool hash_eq(struct lisp_hash hash_a, struct lisp_hash hash_b) {
  return (hash_a.sum == hash_b.sum)  &&
    (hash_a.psum     == hash_b.psum) &&
    (hash_a.len      == hash_b.len)  &&
    (hash_a.com_part == hash_b.com_part);
}

////////////////////////////////////////////////////////////////////////////////

/**              to_pp        from_pp
   [ . . . . | . . . . | . . . . ]
                 ^to_idx(2)    ^from_idx(1)
                      <------
*/
static void lisp_symtab_sort_backprog(POOL_T* from_pp, uint from_idx,
                                      POOL_T* to_pp,   uint to_idx) {
  struct lisp_sym tmp = {0}, * tmpp = NULL;
  struct lisp_sym* mem = NULL;
  POOL_T* pp = from_pp;
  uint iter = 0;

  from_idx = IDX_HM(from_idx);
  to_idx   = IDX_HM(to_idx);

  do {
    mem = pp->mem;

    // swap the last one's element with the first element of the current one
    if (iter > 0) {
      *tmpp  = mem[0];
      mem[0] = tmp;
    }

    DB_FMT("[ == ] lisp_symtab_sort_backprog: i = %d",
           ((pp == from_pp)? from_idx: SYMPOOL));
    DB_FMT("[ == ] lisp_symtab_sort_backprog: lim = %d",
           ((pp == to_pp)? to_idx: 0));

    // always go 0->SYMPOOL (a -> b), unless in the edges:
    //   from-edge: 0      -> from_idx
    //   to-edge:   to_idx -> SYMPOOL

    for (uint a = ((pp == from_pp)? from_idx: SYMPOOL),
              b = ((pp == to_pp)?   to_idx:   0);
         a > b; --a) {
      tmp      = mem[a];
      mem[a]   = mem[a-1];
      mem[a-1] = tmp;
    }

    tmpp = mem;

    // we're done :)
    if (pp == to_pp) {
      return;
    }
  } while (++iter, pp->prev);
}

struct lisp_sort_same_ret {
  POOL_T* pp;  /** @pp: the pool thread */
  uint    idx; /** @idx: the index in @pp::mem */
};

// find the last one in a thread that's the same as us
static struct lisp_sort_same_ret
lisp_symtab_sort_lsame(POOL_T* base_pp, uint base_idx, struct lisp_hash hash,
                       const struct sort_t* sort) {
  struct lisp_sort_same_ret ret = {
    .pp  = base_pp,
    .idx = base_idx,
  };

  POOL_T* pp = ret.pp;
  uint    i  = ret.idx;

  base_idx = IDX_HM(base_idx);

  do {
    struct lisp_sym* mem = pp->mem;

    for (i = ((pp == base_pp)? base_idx: 0); i < SYMPOOL; ++i) {
      uint yie = sort->yield(mem, i);

      if (!sort->eq(yie, hash)) {
        goto done;
      }
    }

    pp = pp->next;
  } while (pp);

done:
  ret.pp  = pp;
  ret.idx = IDX_MH(i);

  return ret;
}

static struct lisp_sort_ret
lisp_symtab_sort_lsmall(const uint pp_idx, struct lisp_sym* mem,
                        struct lisp_hash hash, const struct sort_t* sort) {
  register int ret = 0;

  struct lisp_sort_ret ret_t = {
    .master = -1u,
  };

  uint i = 0;

  // find the least small element with respect to `hash'
  for (uint lower = 0; i < pp_idx; ++i) {
    uint yie = sort->yield(mem, i);

    if (sort->lt(yie, hash)) {
      if (yie > lower) {
        ret_t.master = IDX_MH(i);
      }

      continue;
    }

    assert(!sort->eq(yie, hash), __SORT_NEXT);

    // greater-than: we're done
    break;
  }

  // we didn't find anything: the thread is probably not sorted, it's safe to
  // put ourselves at the first position
  if (ret_t.master == -1u) {
    ret_t.master = 1;
  }

  defer((ret_t.master == pp_idx)? __SORT_RETURN: __SORT_OK);

  done_for_with(ret_t, ret_t.slave = ret);
}

////////////////////////////////////////////////////////////////////////////////

static int lisp_symtab_sort_base(POOL_T* pp, uint idx, struct lisp_hash hash,
                                 const struct sort_t* sort) {
  register int ret = 0;

  POOL_T* cpp  = NULL,
        * base = pp;

  struct lisp_sym* mem       = pp->mem,
                 * base_mem  = base->mem;
  struct lisp_sort_ret sstat = {0};
  struct lisp_sort_same_ret smstat = {0};

  const uint base_idx = idx;

  // we're the biggest: the good ending
  if (IDX_HM(idx) == 0) {
    cpp = pp;
    pp  = pp->prev;

    // nothing to sort
    assert(pp, 0);

    // check against the previous thread
    assert(!sort->lt(sort->yield(pp->mem, IDX_HM(SYMPOOL)), hash), 0);

    pp = cpp;
  }
  else if (sort->lt(sort->yield(mem, IDX_HM(idx) - 1), hash)) {
    defer_as(0);
  }

  do {
    cpp = pp;
    mem = pp->mem;
    idx = pp->p_idx;

    // inclusively in between its base and its neighbour: find the smallest, put
    // ourselves one entry after it
    if (sort->in_between(mem, hash, 0, IDX_HM(idx))) {
      sstat = lisp_symtab_sort_lsmall(pp->p_idx, mem, hash, sort);

      switch (sstat.slave) {
      case __SORT_NEXT:
        goto next;
      case __SORT_RETURN:
        defer_as(0);
      default:
        // we found something smaller than us with no repetition: the ok ending
        lisp_symtab_sort_backprog(base, base_idx, pp, sstat.master);
        defer_as(0);
      }
    }

    pp = pp->prev;
  } while (pp);

  /** at the first pool thread */

  pp  = cpp;
  mem = pp->mem;

  sstat = lisp_symtab_sort_lsmall(pp->p_idx, mem, hash, sort);

  switch (sstat.slave) {
  case __SORT_NEXT:
    goto next;
  case __SORT_RETURN:
    defer_as(0);
  default:
    // we tried to find something smaller us, if we hit this function we're
    // the smallest in the thread, so we swap with the base. this is the *worst*
    // case, and is O(n) time-wise: the bad ending
    lisp_symtab_sort_backprog(base, base_idx, pp, sstat.master);
    defer_as(0);
  }

next:
  base_mem->hash.rep |= sort->mask;
  smstat = lisp_symtab_sort_lsame(pp, sstat.master, hash, sort);

  // if we're here, we have to back-prog either way, so might as well do it now
  lisp_symtab_sort_backprog(base, base_idx, smstat.pp, smstat.idx);

  assert(sort->next, err(EHASHERR));

  ret = lisp_symtab_sort_base(smstat.pp, smstat.idx, hash, sort->next);
  assert(ret == 0, OR_ERR());

  done_for(ret);
}

/** the sort fields are (by priority):

    1. ::len
    2. ::sum
    3. ::psum
    4. ::com_part

    if any of the fields above are the same, the next one in line will sort the
    entries. these functions are called on top of one another
*/
static int lisp_symtab_sort(POOL_T* pp, uint idx, struct lisp_hash hash) {
  return lisp_symtab_sort_base(pp, idx, hash, sort_entry);
}

static struct lisp_sym_ret
lisp_symtab_get_sorted(POOL_T* pp, struct lisp_hash hash,
                       const struct sort_t* sort) {
  register int ret = 0;

  POOL_T* cpp = NULL;
  struct lisp_sym_ret ret_t = {0};

  struct lisp_sym* mem = NULL;

  uint idx = 0;

  do {
    cpp = pp;
    mem = pp->mem;
    idx = pp->p_idx;

    // in between the current chunk
    if (sort->in_between(mem, hash, 0, IDX_HM(idx))) {
      // find the smallest instance, or the only instance
      for (uint i = 0; i < idx; ++i) {
        if (sort->eq(sort->yield(mem, i), hash)) {
          if (sort->repeats(sort)) {
            goto next;
          }

          ret_t.master = (mem+i);
          defer();
        }
      }
    }

    // `mem[pp->idx - 1].x < hash.x' means we're way out of range
    assert(!sort->lt(sort->yield(mem, IDX_HM(pp->p_idx)), hash), 1);

    pp = pp->next;
  } while (pp);

  assert(ret_t.master != NULL, 1);

next:
  pp = cpp;

  if (sort->next) {
    ret_t = lisp_symtab_get_sorted(pp, hash, sort->next);
    ret   = ret_t.slave;
    assert(ret == 0, 1);
  }
  else {
    assert(ret == 0, 1);
  }

  done_for_with(ret_t, ret_t.slave = ret);
}

////////////////////////////////////////////////////////////////////////////////

struct lisp_hash_ret inc_hash(struct lisp_hash hash, char c) {
  register int ret = 0;

  struct lisp_hash_ret hash_ret = {
    .master = hash,
    .slave  = 0,
  };

  assert(hash.len < SYMTAB_MAX_SYM, err(EIDTOOBIG));

  ASCII_NORM(c);

  uchar prt_pre  = hash.psum % SYMTAB_CELL;
  hash.sum      += c*hash_i;
  hash.psum     += c;
  uchar prt_post = c;

  hash_i *= SYMTAB_PRIM;
  hash_i %= SYMTAB_CELL;

  ++hash.len;

  // TODO: another field, `::com_mod', that contains similar information to
  // `::com_part', but uses the `SYMTAB_CELL` module

  // cache if there was a roll-over
  if ((prt_pre + prt_post) > SYMTAB_MAX_SYM) {
    hash.com_part += c*hash_i;
  }

  hash_ret.master = hash;

  done_for_with(hash_ret, hash_ret.slave = ret);
}

void inc_hash_done(struct lisp_hash* hash) {
  hash->sum = mod_norm(hash->sum, hash->len);

  hash_i = 1;
}

void hash_done(struct lisp_hash* hash) {
  hash->sum      = 0;
  hash->psum     = 0;
  hash->len      = 0;
  hash->com_part = 0;
}

struct lisp_hash_ret str_hash(const char* str) {
  register int ret = 0;

  struct lisp_hash_ret hash_ret = {
    .master = {0},
    .slave  = 0,
  };

  char c = '\0';

  DB_FMT("[ == ] symtab: for string: %s", str);

  for (uint i = 0; (c = str[i]); ++i) {
    hash_ret = inc_hash(hash_ret.master, c);
    ret      = hash_ret.slave;

    DB_FMT("[ == ] symtab: character (%c) (%d)", c, hash_ret.master.sum);

    assert(ret == 0, OR_ERR());
  }

  inc_hash_done(&hash_ret.master);
  done_for_with(hash_ret, hash_ret.slave = ret);
}

static struct lisp_sym_ret lisp_symtab_get_for_set(struct lisp_hash hash) {
  register int ret = 0;

  struct lisp_sym_ret ret_t = {0};
  const uint idx = HASH_IDX(hash);

  POOL_T* base_pp = symtab_pp[idx].base;

  // no symbols yet: good to go!
  if (!base_pp->p_idx) {
    defer();
  }

  DB_FMT("[ == ] symtab(get-for-set): trying to get index %d", idx);

  ret_t = lisp_symtab_get_sorted(base_pp, hash, sort_entry);
  ret   = ret_t.slave;

  assert(ret == 0 && hash_eq(ret_t.master->hash, hash), 1);

  done_for_with(ret_t, ret_t.slave = ret);
}

// TODO: we could probably return `struct lisp_sym_ret' like `get' does
int lisp_symtab_set(struct lisp_sym sym) {
  register int ret = 0;

  const uint idx = HASH_IDX(sym.hash);
  uint pp_idx    = 0;

  struct lisp_sym_ret sym_ret = lisp_symtab_get_for_set(sym.hash);

  // what we're trying to set already exists
  if (sym_ret.master) {
    // TODO: stub; should overwrite the existing symbol
    DB_MSG("[ == ] symtab: symbol to be set already exists");
    defer();
  }

  DB_FMT("[ == ] symtab: adding at index %d\n---", idx);

  POOL_T*    mpp = symtab_pp[idx].mem;
  POOL_RET_T pr  = pool_add_node(mpp);
  assert(pr.stat == 0, OR_ERR());

  if (pr.new != pr.base) {
    mpp = symtab_pp[idx].mem = pr.new;
  }

  pp_idx = mpp->p_idx;

  mpp->mem[IDX_HM(pp_idx)] = sym;

  ret = lisp_symtab_sort(mpp, pp_idx, sym.hash);
  assert(ret == 0, OR_ERR());

  done_for(ret);
}

struct lisp_sym_ret lisp_symtab_get(struct lisp_hash hash) {
  register int ret = 0;

  struct lisp_sym_ret ret_t = {0};
  const uint idx = HASH_IDX(hash);

  POOL_T* base_pp = symtab_pp[idx].base;

  assert(base_pp->p_idx, err(ENOTFOUND));

  DB_FMT("[ == ] symtab: trying to get index %d", idx);

  ret_t = lisp_symtab_get_sorted(base_pp, hash, sort_entry);
  ret   = ret_t.slave;

  assert(ret == 0 && hash_eq(ret_t.master->hash, hash), err(ENOTFOUND));

  done_for_with(ret_t, ret_t.slave = ret);
}

void symtab_init(void) {
  for (uint i = 0; i < SYMTAB_CELL; ++i) {
    symtab_pp[i].mem = symtab_pp[i].base = (symtab + i);
  }

  sort_len.next  = &sort_sum;
  sort_sum.next  = &sort_psum;
  sort_psum.next = &sort_com_part;
}

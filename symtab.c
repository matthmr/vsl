#include "symtab.h"
#include "err.h"

// TODO: ensure that the hash algorithm doesn't trigger false positives
// TODO: implement lexical scoping

static uint hash_i = 0;

//static POOL_T symtab[SYMTAB_PRIM];

static inline void pool_clean(POOL_T* pp) {
  return;
}

struct lisp_hash_ret inc_hash(struct lisp_hash hash, char c) {
  struct lisp_hash_ret hash_ret = {
    .master = hash,
    .slave  = 0,
  };

  hash_i          = (hash_i >= SYMTAB_PRIM? 1: (hash_i + 1));

  uchar pre_mod   = (hash.sum % SYMTAB_PRIM);
  hash.sum       += (c - ASCII_FAC)*hash_i;
  hash.psum      += (c - ASCII_FAC);
  uchar post_mod  = (hash.sum % SYMTAB_PRIM);

  ++hash.len;

  /** we bound the length of a symbol to the square of its prime factor.
      for the default prime, this makes this LISP understand symbols
      up to 9409 characters
   */
  if (hash.len > SYMTAB_MAX_SYM) {
    defer_for_as(hash_ret.slave, err(EIDTOOBIG));
  }

  if (pre_mod && post_mod) {
    hash.com_pos  = hash.len;
    hash.com_part = (c - ASCII_FAC);
  }

  hash_ret.master = hash;

  done_for(hash_ret);
}

void inc_hash_done(void) {
  hash_i = 0;
}

void hash_done(struct lisp_hash* hash) {
  hash->sum      = 0;
  hash->psum     = 0;
  hash->len      = 0;
  hash->com_part = 0;
  hash->com_pos  = 0;
}

// TODO: stub
struct lisp_hash_ret str_hash(const char* str) {
  struct lisp_hash_ret hash_ret = {
    .master = {0},
    .slave  = 0,
  };

  for (uint i = 0;; ++i) {
    char c = str[i];

    if (!c || (hash_ret = inc_hash(hash_ret.master, c)).slave != 0) {
      break;
    }
  }

  inc_hash_done();
  done_for(hash_ret);
}

// TODO: the functions below are stubs

struct lisp_sym_ret lisp_symtab_set(struct lisp_hash hash) {
  struct lisp_sym_ret ret = {0};
  return ret;
}

struct lisp_sym_ret lisp_symtab_get(struct lisp_hash hash) {
  struct lisp_sym_ret ret = {0};
  return ret;
}

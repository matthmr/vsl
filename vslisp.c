// vslisp: a very simple lisp implementation;
//         not bootstrapped

#include <unistd.h>
#include <stdlib.h>

#ifdef DEBUG
#  include <stdio.h>
#endif

#include "vslisp.h"

static uint paren = 0;
static char iobuf[IOBLOCK] = {0};

static struct hash chash = {0};
static struct lisp_cps lcps = {0};
static struct lisp_sexp* head = NULL,
                       * root = NULL;

struct MEMPOOL(lisp_sexp, SEXPPOOL);
//struct MEMPOOL(lisp_sym_hash, HASHPOOL);

static struct MEMPOOL_TMPL(lisp_sexp) sexpmp = {0};
static struct MEMPOOL_TMPL(lisp_sexp)* sexpmpp = {0};
//static struct MEMPOOL_TMPL(lisp_sym_hash) hashmp = {0};

static inline struct pool_ret_t pool_add_node(struct MEMPOOL_TMPL(lisp_sexp)* mpp) {
  // TODO: make this an `int' funcion because of
  //       ENOMEM in malloc
  struct pool_ret_t ret = {
    .mem = NULL,
    .new = 0,
  };

  if (mpp->used == mpp->total) {
    mpp->next = malloc(mpp->total * sizeof(struct MEMPOOL_TMPL(lisp_sexp)));

    mpp->next->total = mpp->total;
    mpp->next->used  = 0;
    mpp->next->next  = NULL;

    mpp = mpp->next;
    ret.new = true;
  }

  ret.mem = mpp->mem + mpp->used;
  ++mpp->used;

  return ret;
}
static inline void lisp_sexp_node(struct MEMPOOL_TMPL(lisp_sexp)* mpp) {
  struct pool_ret_t pr    = pool_add_node(mpp);
  struct lisp_sexp* nhead = pr.mem;

  if (pr.new) {
    nhead->root = (struct root_off_t) {
      .local = false,
      .am    = 0,
    };
  }
  else {
    nhead->root = (struct root_off_t) {
      .local = true,
      .am    = (nhead - mpp->mem),
    };
  }

  if (head->left.sexp == false) {
    head->left.t.local = !pr.new;
    head->left.t.am    = (pr.new)? 0: 1;
    head->left.sexp    = true;
  }
  else if (head->right.sexp == false) {
    head->right.t.local = !pr.new;
    //head->right.t.am    = (pr.new)? 0: 1;
    head->right.sexp    = true;
  }

  head = nhead;
}
static inline void lisp_sexp_sym(void) {
  if (head->left.sexp == neither) {
    head->left.sexp     = false;
    head->left.sym.hash = chash;
  }
  else if (head->right.sexp == neither) {
    head->right.sexp     = false;
    head->right.sym.hash = chash;
  }
  else {} // <- TODO

  chash.internal.i = 0;
  chash.len        = 0;
  chash.hash       = 0L;
}
static inline void lisp_sexp_end(void) {
  // TODO: get the parent address, point to it
}
static inline long do_chash(int i, char c) {
  long ret = 0;

  ret   = ((c + i) % 0x100);
  ret <<= ((i*8)   % 0xffffffffffffffff);

  return ret;
}
static inline struct lisp_cps lisp_ev(struct lisp_cps pstat,
                                      enum lisp_pev sev) {
  if (pstat.master.stat & __LISP_SYMBOL_IN) {
    pstat.master.stat &= ~__LISP_SYMBOL_IN;
    sev |= __LISP_SYMBOL_OUT;
  }
  if (sev & __LISP_PAREN_OUT) {
    if (paren) {
      pstat.master.stat &= ~__LISP_PAREN_IN;
      --paren;
    }
    else {
      pstat.slave = 1;
      goto done;
    }
  }

  pstat.master.ev |= sev;

 done:
  return pstat;
}
static inline struct lisp_cps lisp_stat(struct lisp_cps pstat,
                                        enum lisp_pstat sstat) {
  if (sstat & __LISP_PAREN_IN) {
    lisp_sexp_node(sexpmpp);
    ++paren;
  }
  if (pstat.master.stat & __LISP_SYMBOL_IN) {
    pstat = lisp_ev(pstat, __LISP_SYMBOL_OUT);
  }

  pstat.master.stat |= sstat;

  return pstat;
}
static inline struct lisp_cps lisp_whitespace(struct lisp_cps pstat) {
  if (pstat.master.stat & __LISP_SYMBOL_IN) {
    pstat = lisp_ev(pstat, __LISP_SYMBOL_OUT);
  }

  return pstat;
}
static inline struct lisp_cps lisp_csym(struct lisp_cps pstat, char c) {
  pstat.master.stat |= __LISP_SYMBOL_IN;

  if (!__LISP_ALLOWED_IN_NAME(c)) {
    pstat.slave = 1;
    goto done;
  }

  ++chash.len;
  chash.hash += do_chash(chash.internal.i, c);
  ++chash.internal.i;

#ifdef DEBUG
      fprintf(stderr, "vslisp: character (%c) (0x%lx)\n", c, chash.hash);
#endif

 done:
  return pstat;
}
static inline struct lisp_cps ev_listen(struct lisp_cps pstat) {
  enum lisp_pev ev = pstat.master.ev;
  if (ev & __LISP_SYMBOL_OUT) {
#ifdef DEBUG
    fputs("EVENT: symbol out\n", stderr);
#endif
    pstat.master.ev &= ~__LISP_SYMBOL_OUT;
    lisp_sexp_sym();
  }
  if (ev & __LISP_PAREN_OUT) {
#ifdef DEBUG
    fputs("EVENT: paren out\n", stderr);
#endif
    pstat.master.ev &= ~__LISP_PAREN_OUT;
    lisp_sexp_end();
  }
  return pstat;
}
static int parse_ioblock(char* buf, uint size) {
  int ret = 0;

  for (uint i = 0; i < size; i++) {
    char c = buf[i];

    switch (c) {
    case __LISP_PAREN_OPEN:
#ifdef DEBUG
      fputs("vslisp: paren_open\n", stderr);
#endif
      lcps = lisp_stat(lcps, __LISP_PAREN_IN);
      break;
    case __LISP_PAREN_CLOSE:
#ifdef DEBUG
      fputs("vslisp: paren_close\n", stderr);
#endif
      lcps = lisp_ev(lcps, __LISP_PAREN_OUT);
      break;
    case __LISP_WHITESPACE:
#ifdef DEBUG
      fputs("vslisp: whitespace\n", stderr);
#endif
      lcps = lisp_whitespace(lcps);
      break;
    default:
      lcps = lisp_csym(lcps, c);
      break;
    }

    if (lcps.slave) {
      ret = 1;
      goto done;
    }
    else {
      lcps = ev_listen(lcps);
    }
  }

#ifdef DEBUG
  printf("vslisp: [ret = %d, paren = %d]\n", ret, paren);
#endif

 done:
  return ret;
}

static int parse_bytstream(int fd) {
  int ret = 0;

  uint r;

  do {
    r = read(fd, iobuf, IOBLOCK);
    int s = parse_ioblock(iobuf, r);
    if (s) {
      ret = s;
      goto done;
    }
  } while (r == IOBLOCK);

 done:
  return ret;
}

int main(void) {
  sexpmp.total = SEXPPOOL;
  sexpmp.used  = 1;

  sexpmpp = &sexpmp;

  root = head = &sexpmpp->mem[0];
  ++sexpmpp->used;

  head->root.local  = true;
  head->left.sexp   = neither;
  head->right.sexp  = neither;

  int ret = parse_bytstream(STDIN_FILENO);

  if ((ret = (ret || paren))) {
    fputs("[ !! ] vslisp: error while parsing file\n", stderr);
  }

  return ret;
}

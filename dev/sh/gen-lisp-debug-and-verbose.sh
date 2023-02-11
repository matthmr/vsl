#!/usr/bin/sh -ex

make -B -k $@ CFLAGSADD=-ggdb libvsl.a &&      \
  clang -L. -lvsl dev/c/lisp.c -o lisp-debug

make -B -k $@ CFLAGSADD=-DDEBUG libvsl.a &&    \
  clang -L. -lvsl dev/c/lisp.c -o lisp-verbose

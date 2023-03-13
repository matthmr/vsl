#!/usr/bin/sh -ex

make -B -k $@ CFLAGSADD=-ggdb libvsl.a libvslprim.a libcgen.a && \
  clang -I. -L. -lvsl -lvslprim dev/c/lisp.c -o lisp-debug

make -B -k $@ CFLAGSADD=-DDEBUG libvsl.a libvslprim.a libcgen.a && \
  clang -I. -L. -lvsl -lvslprim dev/c/lisp.c -o lisp-verbose

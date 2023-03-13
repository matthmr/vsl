#!/usr/bin/sh -ex

make $@ -k libvsl.a libvslprim.a libcgen.a && \
  clang -I. -L. -lvsl -lvslprim dev/c/lisp.c -o lisp

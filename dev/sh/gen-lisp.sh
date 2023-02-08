#!/usr/bin/sh -ex

make -k libvsl.a && clang -L. -lvsl dev/c/lisp.c -o lisp

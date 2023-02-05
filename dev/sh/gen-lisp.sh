#!/usr/bin/sh -ex

make -C ~p/vsl -k libvsl.a && clang -L/home/p/vsl -lvsl /home/p/vsl/dev/c/lisp.c -o /home/p/vsl/lisp

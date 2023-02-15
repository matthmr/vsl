#!/usr/bin/sh -ex

DEV_C='lisp.c'
DEV_SH="gen-include.sh gen-lisp-debug-and-verbose.sh \
gen-lisp.sh gen-todo.sh"
DEV_TST='tst0.vsl'

if [[ "$(git branch | grep '^*' | cut -d' ' -f2)" == "dev" ]]; then
  echo "[ !! ] Cannot run this script while in \`dev'" 1>&2
  exit 1
fi

mkdir -p dev/c dev/sh dev/tst

IFS=' '

for _c in $DEV_C; do
  git sh dev:dev/c/$_c > dev/c/$_c
done

for _sh in $DEV_SH; do
  git sh dev:dev/sh/$_sh > dev/sh/$_sh
  chmod +x dev/sh/$_sh
done

for _tst in $DEV_TST; do
  git sh dev:dev/tst/$_tst > dev/tst/$_tst
done

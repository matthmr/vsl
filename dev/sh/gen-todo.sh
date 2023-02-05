#!/usr/bin/sh -x

grep --include='*.[ch]' -rniH 'TODO' | \
sed 's:^:../:' > dev/TODO

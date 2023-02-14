#!/usr/bin/sh -x

cinclude2dot > dev/vsl.dot 2> /dev/null
dot -Tpng dev/vsl.dot -o dev/vsl.png

rm dev/vsl.dot

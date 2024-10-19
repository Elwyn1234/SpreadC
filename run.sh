#!/usr/bin/bash

[[ -f src/dArray.c ]] && mv src/dArray.c /tmp/

cat >> src/dArray.c << HEREDOC
#include "dArray.h"

#include <stdlib.h>

HEREDOC

cat src/templates/dArrayBody.c.template | \
    sed 's/<TYPE_NAME>/String/g' | \
    sed 's/<TYPE>/String/g' \
    >> src/dArray.c
cat src/templates/dArrayBody.c.template | \
    sed 's/<TYPE_NAME>/Int/g' | \
    sed 's/<TYPE>/int/g' \
    >> src/dArray.c



[[ -f src/dArray.h ]] && mv src/dArray.h /tmp/

cat >> src/dArray.h << HEREDOC
#include "types.h"
#ifndef _DARRAY_H
#define _DARRAY_H

HEREDOC

cat src/templates/dArrayHead.h.template | \
    sed 's/<TYPE_NAME>/String/g' | \
    sed 's/<TYPE>/String/g' \
    >> src/dArray.h
cat src/templates/dArrayHead.h.template | \
    sed 's/<TYPE_NAME>/Int/g' | \
    sed 's/<TYPE>/int/g' \
    >> src/dArray.h

cat >> src/dArray.h << HEREDOC
#endif

HEREDOC

echo Compilation timings
time clang -g -O0 src/*.c -o build/a.out `pkg-config --cflags --libs pango x11 pangocairo`

echo -e "\nStarting Application...\n"
if [[ "$1" == d ]]; then
  gdb ./build/a.out
else
  ./build/a.out
fi


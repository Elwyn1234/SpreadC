#!/usr/bin/bash

echo Compilation timings
time clang -g -O0 src/*.c -o build/a.out `pkg-config --cflags --libs pango x11 pangocairo`

echo -e "\nStarting Application...\n"
if [[ "$1" == d ]]; then
  gdb ./build/a.out
else
  ./build/a.out
fi


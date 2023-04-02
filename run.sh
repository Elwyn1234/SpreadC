#!/usr/bin/bash
clang src/linux.c -o build/a.out `pkg-config --cflags --libs pango x11 pangocairo`
./build/a.out

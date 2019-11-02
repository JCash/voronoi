#!/usr/bin/env bash
mkdir -p build

if [ "$USE_ASAN" != "" ]; then
    ASAN="-fsanitize=address -fno-omit-frame-pointer -fsanitize-address-use-after-scope -fsanitize=undefined"
    ASAN_LDFLAGS="-fsanitize=address "

    echo Using ASAN
fi

clang $ASAN -c src/stb_wrapper.c -o build/stb_wrapper.o
clang $ASAN -o build/main -g -O0 -m64 -std=c99 -Wall -Weverything -Wno-float-equal -pedantic -lm -Isrc build/stb_wrapper.o src/main.c

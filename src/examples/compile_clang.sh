#!/usr/bin/env bash
BUILD_DIR=../../build
mkdir -p $BUILD_DIR

ASAN_FLAGS="-O1 -fsanitize=address -fno-omit-frame-pointer -fsanitize-address-use-after-scope"
CFLAGS="$CFLAGS -Wall -Weverything -pedantic -Wno-float-equal"
clang $CFLAGS $ASAN_FLAGS simple.c -I.. -lm -o $BUILD_DIR/simple

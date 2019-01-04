#!/usr/bin/env bash

ASAN_FLAGS="-fsanitize=address -fno-omit-frame-pointer -fsanitize-address-use-after-scope "
CFLAGS="$CFLAGS $ASAN_FLAGS -std=c99 -g -O1 -m64 -Wall -Weverything -Wno-float-equal -Wno-unused-function -Wno-double-promotion -pedantic -I../src"
LINKFLAGS="-lm -fsanitize=address"
DOUBLEDEFINES="-Wno-double-promotion -DTEST_USE_DOUBLE -DJCV_REAL_TYPE=double -DJCV_ATAN2=atan2 -DJCV_SQRT=sqrt"

clang -o ../build/test $CFLAGS $LINKFLAGS test.c
clang -o ../build/test_double $CFLAGS $LINKFLAGS $DOUBLEDEFINES test.c

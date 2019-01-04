#!/usr/bin/env bash

CFLAGS="$CFLAGS -std=c99 -g -O0 -m64 -Wall -Weverything -Wno-float-equal -Wno-unused-function -Wno-double-promotion -pedantic -I../src"
LINKFLAGS="-lm"
DOUBLEDEFINES="-Wno-double-promotion -DTEST_USE_DOUBLE -DJCV_REAL_TYPE=double -DJCV_ATAN2=atan2 -DJCV_SQRT=sqrt"

clang -o ../build/test $CFLAGS $LINKFLAGS test.c
clang -o ../build/test_double $CFLAGS $LINKFLAGS $DOUBLEDEFINES test.c

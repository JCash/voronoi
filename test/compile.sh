#!/usr/bin/env bash

if [ "$USE_ASAN" != "" ]; then
    ASAN_OPTIONS=detect_stack_use_after_return=1
    ASAN_FLAGS="-fsanitize=address -fno-omit-frame-pointer -fsanitize-address-use-after-scope "
    ASAN_LDFLAGS="-fsanitize=address "
    echo Using ASAN
fi

if [ "${CC}" == "" ]; then
    CC=clang
fi

if [ "${STDVERSION}" == "" ]; then
    STDVERSION=c99
fi

CFLAGS="$CFLAGS ${ARCH} ${ASAN_FLAGS} -std=${STDVERSION} -g -O1 -Wall -Weverything -Wno-float-equal -Wno-unused-function -Wno-double-promotion -Wno-declaration-after-statement -pedantic -I../src"
LINKFLAGS="-lm ${ASAN_LDFLAGS}"
DOUBLEDEFINES="-Wno-double-promotion -DTEST_USE_DOUBLE -DJCV_REAL_TYPE=double -DJCV_ATAN2=atan2 -DJCV_SQRT=sqrt"

${CC} -o ../build/test $CFLAGS $LINKFLAGS test.c
${CC} -o ../build/test_double $CFLAGS $LINKFLAGS $DOUBLEDEFINES test.c

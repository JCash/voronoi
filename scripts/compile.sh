#!/usr/bin/env bash
mkdir -p build

OPT="-g -O2"

if [ "$USE_ASAN" != "" ]; then
    ASAN_OPTIONS=detect_stack_use_after_return=1
    ASAN="-fsanitize=address -fno-omit-frame-pointer -fsanitize-address-use-after-scope -fsanitize=undefined"
    ASAN_LDFLAGS="-fsanitize=address "
    OPT="-g -O0"
    echo Using ASAN=${ASAN}
fi

if [ "${CC}" == "" ]; then
    CC=clang
fi

if [ "${STDVERSION}" == "" ]; then
    STDVERSION=c99
fi

if [ "Darwin" == "$(uname)" ]; then
    if [ -e "/Library/Developer/CommandLineTools/SDKs/MacOSX.sdk" ]; then
        SYSROOT="-isysroot /Library/Developer/CommandLineTools/SDKs/MacOSX.sdk"
    fi
    if [ "" == "${SYSROOT}" ]; then
        SYSROOT="-isysroot $(xcode-select --print-path)/Platforms/MacOSX.platform/Developer/SDKs/MacOSX.sdk"
    fi
fi

CFLAGS="-g -O2 -std=c11 -Wall -Weverything -Wno-double-promotion -Wno-float-equal -pedantic -Wno-declaration-after-statement -Isrc"
DOUBLEDEFINES="-Wno-double-promotion -DTEST_USE_DOUBLE -DJCV_USE_DOUBLE=1 -DJCV_REAL_TYPE=double -DJCV_ATAN2=atan2 -DJCV_SQRT=sqrt -DJCV_REAL_TYPE_EPSILON=DBL_EPSILON"
#CFLAGS="${CFLAGS} ${DOUBLEDEFINES}"

${CC} ${ASAN} ${ARCH} -c src/stb_wrapper.c -o build/stb_wrapper.o
${CC} ${ASAN} -o build/main ${ARCH} -std=${STDVERSION} ${OPT} ${CFLAGS} -lm  ${SYSROOT} build/stb_wrapper.o src/main.c

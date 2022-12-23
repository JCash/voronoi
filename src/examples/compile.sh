#!/usr/bin/env bash
BUILD_DIR=../../build
mkdir -p $BUILD_DIR

if [ "${CC}" == "" ]; then
    CC=clang
fi

if [ "${STDVERSION}" == "" ]; then
    STDVERSION=c99
fi

if [ "$USE_ASAN" != "" ]; then
    ASAN_OPTIONS=detect_stack_use_after_return=1
    ASAN="-fsanitize=address -fno-omit-frame-pointer -fsanitize-address-use-after-scope -fsanitize=undefined"
    ASAN_LDFLAGS="-fsanitize=address "
    echo Using ASAN=${ASAN}
fi

if [ "Darwin" == "$(uname)" ]; then
    if [ -e "/Library/Developer/CommandLineTools/SDKs/MacOSX.sdk" ]; then
        SYSROOT="-isysroot /Library/Developer/CommandLineTools/SDKs/MacOSX.sdk"
    fi
    if [ "" == "${SYSROOT}" ]; then
        SYSROOT="-isysroot $(xcode-select --print-path)/Platforms/MacOSX.platform/Developer/SDKs/MacOSX.sdk"
    fi
fi

CFLAGS="${CFLAGS} -std=${STDVERSION} -Wall -Weverything -pedantic -Wno-float-equal -Wno-declaration-after-statement"
${CC} ${CFLAGS} ${ASAN} ${SYSROOT} simple.c -I.. -lm -o ${BUILD_DIR}/simple

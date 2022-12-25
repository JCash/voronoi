#!/usr/bin/env bash

if [ "$USE_ASAN" != "" ]; then
    ASAN_OPTIONS=detect_stack_use_after_return=1
    ASAN_FLAGS="-fsanitize=address -fno-omit-frame-pointer -fsanitize-address-use-after-scope "
    ASAN_LDFLAGS="-fsanitize=address "
    echo Using ASAN
fi
if [ "$USE_UBSAN" != "" ]; then
    UBSAN_FLAGS="-fsanitize=undefined"
    UBSAN_LDFLAGS="-fsanitize=undefined"
    echo Using UBSAN
fi

if [ "Darwin" == "$(uname)" ]; then
    if [ -e "/Library/Developer/CommandLineTools/SDKs/MacOSX.sdk" ]; then
        SYSROOT="-isysroot /Library/Developer/CommandLineTools/SDKs/MacOSX.sdk"
    fi
    if [ "" == "${SYSROOT}" ]; then
        SYSROOT="-isysroot $(xcode-select --print-path)/Platforms/MacOSX.platform/Developer/SDKs/MacOSX.sdk"
    fi
fi

if [ "${CC}" == "" ]; then
    CC=clang
fi

if [ "${CC}" == "clang" ]; then
    CXX=clang++
fi
if [ "${CC}" == "gcc" ]; then
    CXX=g++
fi

if [ "${STDVERSION}" == "" ]; then
    STDVERSION=c99
fi

CCFLAGS="${ARCH} ${ASAN_FLAGS} -g -O1 -DJC_TEST_USE_COLORS -Wall -Weverything -Wno-float-equal -Wno-unused-function -Wno-double-promotion -Wno-declaration-after-statement -pedantic -I../src ${SYSROOT}"
CFLAGS="-c $CFLAGS -std=${STDVERSION} ${CCFLAGS}"
CXXFLAGS="$CXXFLAGS -std=c++11 -Wno-global-constructors -Wno-weak-vtables -Wno-old-style-cast -Wno-zero-as-null-pointer-constant -Wno-c++98-compat -Wno-c++98-compat-pedantic -Wno-suggest-override"
LINKFLAGS="-lm ${ASAN_LDFLAGS}"
DOUBLEDEFINES="-Wno-double-promotion -DTEST_USE_DOUBLE -DJCV_REAL_TYPE=double -DJCV_ATAN2=atan2 -DJCV_SQRT=sqrt -DJCV_REAL_TYPE_EPSILON=DBL_EPSILON"

${CXX} -o ../build/test $CCFLAGS $CXXFLAGS $LINKFLAGS test.cpp
${CXX} -o ../build/test_double $CCFLAGS $CXXFLAGS $LINKFLAGS $DOUBLEDEFINES test.cpp

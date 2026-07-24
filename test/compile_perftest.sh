#!/usr/bin/env bash
set -e

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

if [ "${CXX}" == "" ]; then
    CXX=clang++
fi

OPT="-O2"
#CCFLAGS="${ARCH} ${ASAN_FLAGS} -g -DJC_TEST_USE_COLORS -Wall -Weverything -Wno-float-equal -Wno-unused-function -Wno-double-promotion -Wno-declaration-after-statement -pedantic -I../src ${SYSROOT}"
CCFLAGS="${ARCH} ${ASAN_FLAGS} -g ${OPT} -DJC_TEST_USE_COLORS -I../src ${SYSROOT}"
CFLAGS="-c $CFLAGS -std=${STDVERSION} ${CCFLAGS}"
CXXFLAGS="$CXXFLAGS -std=c++11 -Wno-global-constructors -Wno-weak-vtables -Wno-old-style-cast -Wno-zero-as-null-pointer-constant -Wno-c++98-compat -Wno-c++98-compat-pedantic -Wno-suggest-override"
LINKFLAGS="-lm ${ASAN_LDFLAGS}"
# DOUBLEDEFINES="-Wno-double-promotion -DTEST_USE_DOUBLE -DJCV_REAL_TYPE=double -DJCV_ATAN2=atan2 -DJCV_SQRT=sqrt -DJCV_REAL_TYPE_EPSILON=DBL_EPSILON"

NAME=jc_voronoi
ALGORITHM=USE_JC_VORONOI
${CXX} -o ../build/perftest_${NAME} $CCFLAGS $CXXFLAGS $LINKFLAGS -D${ALGORITHM} -I.. perftest.cpp


NAME=boost
ALGORITHM=USE_BOOST
BOOST_ROOT=${BOOST_ROOT:-/Users/mathiaswesterdahl/notwork/external/boost}
${CXX} -o ../build/perftest_${NAME} $CCFLAGS $CXXFLAGS $LINKFLAGS -D${ALGORITHM} -I${BOOST_ROOT}/libs/polygon/include -I${BOOST_ROOT}/libs/config/include perftest.cpp

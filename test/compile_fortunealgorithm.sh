#! /usr/bin/env bash

DIR=./external/FortuneAlgorithm/src
ASAN_FLAGS="-fsanitize=address -fno-omit-frame-pointer -fsanitize-address-use-after-scope "

clang++ -std=c++14 -g -O0 ${ASAN_FLAGS} -I${DIR} ${DIR}/*.cpp  -o ./build/fortunealgorithm

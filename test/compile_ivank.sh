#! /usr/bin/env bash

DIR=./external/ivank/source
ASAN_FLAGS="-fsanitize=address -fno-omit-frame-pointer -fsanitize-address-use-after-scope "

clang++ -g -O0 ${ASAN_FLAGS} -I${DIR} ${DIR}/VParabola.cpp ${DIR}/Voronoi.cpp ${DIR}/main.cpp -o ./build/ivank

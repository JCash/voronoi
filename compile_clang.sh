mkdir -p build
clang -o build/main -g -O0 -m64 -std=c99 -Wall -Weverything -Wno-float-equal -pedantic -lm src/main.c

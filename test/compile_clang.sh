#!/usr/bin/env bash

clang -o ../build/test -std=c99 -g -O0 -m64 -Wall -Weverything -Wno-float-equal -pedantic -I../src -lm test.c

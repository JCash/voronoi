#!/usr/bin/env bash
BUILD_DIR=../../build
mkdir -p $BUILD_DIR

clang -Wall -Weverything -pedantic -Wno-float-equal simple.c -I.. -o $BUILD_DIR/simple

---
title: Build
weight: 1
---

The library itself does not need to be built. The repository includes `src/main.c`, an example app that writes a Voronoi diagram to PNG or SVG.

## macOS and Linux

With `clang` installed, run from the repository root:

```sh
./scripts/compile.sh
./build/main -n 100 -r 3 -w 1024 -h 768 -o example.svg
```

## Windows

Install Visual Studio or the Visual Studio Build Tools with the **Desktop development with C++** workload. From an **x64 Native Tools Command Prompt for VS**:

```bat
scripts\compile_cl.bat
build\main.exe -n 100 -r 3 -w 1024 -h 768 -o example.svg
```

This generates 100 random sites, applies three relaxation passes, and writes a 1024 × 768 SVG. Run the executable with `--help` for all image size, input, relaxation, and output options.

# Benchmarks

This performance test is running 1k, 10k and 100k random (same seed) point generation.
It also uses a special case of 100k points which really pushes the beachline operations.

*Tests ran on an Apple M5 Max MacBook Pro with 128 GB RAM, using Apple clang
21.0.0 with `-O3 -DNDEBUG` on arm64 macOS. The 0.10 timings are medians of five
benchmark-batch medians; each batch used 201 iterations for 1k, 51 for 10k,
and 31 for 100k and the pathological case. Boost used 21 iterations. 0.9 used
21 iterations for the smaller random cases and 5 for the pathological case,
where each run took about nine seconds. Input setup, image and output
generation, and output allocation were excluded from the timed region; diagram
destruction was included.*

## Graphs

Here is the resulting runtime performance:

<img src="images/benchmark/release-0.10.0-voronoi.svg" alt="Voronoi diagram generation" width="350">

And here is the peak memory useage, and what's retained once the diagram is generated:

<img src="images/benchmark/release-0.10.0-memory.svg" alt="Memory" width="350">

Code metrics:

<img src="images/benchmark/release-0.10.0-code-build.svg" alt="Code size and build time" width="350">

## Tables

### Diagram generation

| Case | 0.9 | 0.10 | Boost | 0.10/Boost |
|---|---:|---:|---:|---:|
| 10k | 5.04 ms | 4.87 ms | 6.82 ms | 71.5% |
| 100k | 51.22 ms | 53.95 ms | 80.21 ms | 67.3% |
| 100k pathological | 9.15 s | 18.63 ms | 39.25 ms | 47.5% |

### Memory

Each entry is peak heap KB / retained heap KB / allocation count.

| Case | 0.9 | 0.10 | Boost | 0.10/Boost |
|---|---:|---:|---:|---:|
| 10k | 4,832 / 4,832 / 281 | 4,181 / 2,699 / 126 | 4,394 / 3,672 / 55,815 | 95.2% |
| 100k | 48,060 / 48,060 / 2,785 | 41,434 / 26,666 / 1,187 | 42,104 / 36,719 / 557,371 | 98.4% |
| 100k pathological | 46,236 / 46,236 / 2,671 | 45,467 / 35,290 / 2,041 | 57,999 / 36,718 / 400,010 | 78.4% |

### Code and build statistics

| Metric | 0.9 | 0.10 | Boost | 0.10/Boost |
|---|---:|---:|---:|---:|
| Stripped executable | 33,912 B | 33,720 B | 150,616 B | 22.4% |
| Implementation code | 1,254 LOC | 1,961 LOC | 9,047 LOC | 21.7% |
| Clean build time | 0.13 s | 0.17 s | 0.97 s | 17.5% |

<!-- wasm-benchmarks:start -->
## WebAssembly and JavaScript Voronoi performance

Times are medians and lower is better. The benchmark exercises each library's public API. Point generation, comparison-library input conversion, WebAssembly initialization, garbage collection, and report generation are outside the timed regions. JCV's public `generate` call copies the prepared `Float32Array` into WebAssembly memory, and that copy is included.

The random cases use a deterministic seed. `100k pathological` is the issue48 input with 99,998 symmetric diagonal-pair sites. Each measurement has two untimed warmups followed by 10 samples for 10k and five samples for 100k and the pathological case.

JCV copies one packed result from WebAssembly into a JavaScript-owned `ArrayBuffer` and disposes each result inside the timed operation. Site access materializes ergonomic `Site` objects; edge rendering reads packed vertex arrays without creating edge objects; Delauney access reads the packed adjacency count. Other libraries expose different public output forms: d3-voronoi and voronoi eagerly provide arrays, while d3-delaunay renders its Voronoi mesh. These rows therefore compare public access workflows, not identical post-processing algorithms.

### Voronoi Diagram Generation

| Case | JCV 0.10 | d3-delaunay | d3-voronoi | gorhill/voronoi |
|---|---:|---:|---:|---:|
| 10k | 7.43 ms | 3.09 ms | 28.75 ms | 30.97 ms |
| 100k | 98.90 ms | 30.46 ms | 339.07 ms | 330.20 ms |
| 100k pathological | 31.88 ms | 9.12 ms | 129.05 ms | 3.17 s |

### Generate + Get Sites

| Case | JCV 0.10 | d3-delaunay | d3-voronoi | gorhill/voronoi |
|---|---:|---:|---:|---:|
| 10k | 7.90 ms | 6.78 ms | 28.17 ms | 34.86 ms |
| 100k | 101.83 ms | 48.16 ms | 317.60 ms | 324.23 ms |
| 100k pathological | 34.86 ms | 27.35 ms | 113.89 ms | 2.53 s |

### Generate + Render Edges

| Case | JCV 0.10 | d3-delaunay | d3-voronoi | gorhill/voronoi |
|---|---:|---:|---:|---:|
| 10k | 8.43 ms | 11.19 ms | 28.14 ms | 31.56 ms |
| 100k | 102.08 ms | 107.60 ms | 317.79 ms | 380.50 ms |
| 100k pathological | 36.70 ms | 47.42 ms | 115.17 ms | 2.69 s |

<img src="images/benchmark/wasm-voronoi-edges.svg" alt="Render Voronoi Edges" width="350">

### Generate + Get Delauney

| Case | JCV 0.10 | d3-delaunay | d3-voronoi | gorhill/voronoi |
|---|---:|---:|---:|---:|
| 10k | 7.90 ms | 5.44 ms | 31.02 ms | — |
| 100k | 100.37 ms | 46.93 ms | 388.69 ms | — |
| 100k pathological | 33.76 ms | 23.41 ms | 119.44 ms | — |

<img src="images/benchmark/wasm-delauney-edges.svg" alt="Get Delauney Edges" width="350">

### Retained runtime memory

| Case | JCV 0.10 | d3-delaunay | d3-voronoi | gorhill/voronoi |
|---|---:|---:|---:|---:|
| 10k | 9.9 MiB | 5.8 MiB | 31.4 MiB | 39.8 MiB |
| 100k | 56.0 MiB | 17.9 MiB | 165.1 MiB | 173.0 MiB |
| 100k pathological | 60.2 MiB | 15.9 MiB | 162.0 MiB | 181.5 MiB |

<img src="images/benchmark/wasm-memory.svg" alt="Retained WebAssembly and JavaScript memory" width="350">

Each memory sample starts a fresh Node.js process, prepares the library-specific input, forces garbage collection, records a baseline, generates and retains one public diagram, forces garbage collection again, and records the resident-memory delta. Values are medians of 3 isolated samples. Input arrays and initialized library runtimes are part of the baseline. Resident memory is runtime- and operating-system-dependent, so compare entries only within the environment reported below.

<!-- wasm-code-size:start -->
### Code size

| Library | Dependencies | Compound module | Brotli | Compound LOC |
|---|---:|---:|---:|---:|
| JCV 0.10 | 0 | 27.8 KiB | 10.2 KiB | 2,730 |
| d3-delaunay | 2 (delaunator, robust-predicates) | 18.6 KiB | 6.1 KiB | 1,258 |
| d3-voronoi | 0 | 9.0 KiB | 3.4 KiB | 864 |
| gorhill/voronoi | 0 | 16.0 KiB | 4.1 KiB | 1,072 |

<img src="images/benchmark/wasm-code-size.svg" alt="WebAssembly and JavaScript module size" width="350">

Compound module size is the JCV `.wasm` payload or the library’s published minified browser module including runtime dependencies. Brotli uses quality 11. Compound LOC counts nonblank, non-comment implementation lines including runtime dependency sources. d3-delaunay depends directly on delaunator, which depends on robust-predicates; the other modules have no runtime package dependencies.

<!-- wasm-code-size:end -->

### Environment

- Apple M5 Max
- Darwin 27.0.0 (arm64)
- Node.js v26.5.0
- Wasm compiled with Emscripten `-O3 -flto`
- d3-delaunay 6.0.4, d3-voronoi 1.1.4, gorhill/voronoi 1.0.0

The `gorhill/voronoi` package has no direct Delauney retrieval operation, so that entry is not applicable.

<!-- wasm-benchmarks:end -->

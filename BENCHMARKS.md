# Benchmarks

This performance test is running 1k, 10k and 100k random (same seed) point generation.
It also uses a special case of 100k points which really pushes the beachline operations.

*Tests ran on an Apple M5 Max MacBook Pro with 128 GB RAM, using Apple clang
21.0.0 with `-O3 -DNDEBUG` on arm64 macOS. Timings are medians: Current used
paired runs with 201 iterations for 1k, 51 for 10k, and 31 for 100k and the
pathological case; Boost used 21 iterations. Dev used 21 iterations for the
smaller random cases and 5 for the pathological case, where each run took about
nine seconds. Input setup, diagram destruction, image and output generation,
and output allocation were excluded from the timed region.*

## Graphs

Here is the resulting runtime performance:

<img src="images/benchmark/release-0.10.0-performance.svg" alt="Performance" width="350">

And here is the peak memory useage, and what's retained once the diagram is generated:

<img src="images/benchmark/release-0.10.0-memory.svg" alt="Memory" width="350">

Code metrics:

<img src="images/benchmark/release-0.10.0-code-build.svg" alt="Memory" width="350">

## Tables

### Diagram generation

| Case | 0.9 | 0.10 | Boost | 0.10/Boost |
|---|---:|---:|---:|---:|
| 10k | 5.04 ms | 5.13 ms | 6.82 ms | 75.2% |
| 100k | 51.22 ms | 55.98 ms | 80.21 ms | 69.8% |
| issue48 | 9.15 s | 19.43 ms | 39.25 ms | 49.5% |

### Memory

Each entry is peak heap KB / retained heap KB / allocation count.

| Case | 0.9 | 0.10 | Boost | 0.10/Boost |
|---|---:|---:|---:|---:|
| 10k | 4,832 / 4,832 / 281 | 4,181 / 2,699 / 126 | 4,394 / 3,672 / 55,815 | 95.2% |
| 100k | 48,060 / 48,060 / 2,785 | 41,434 / 26,666 / 1,187 | 42,104 / 36,719 / 557,371 | 98.4% |
| issue48 | 46,236 / 46,236 / 2,671 | 45,676 / 35,343 / 2,041 | 57,999 / 36,718 / 400,010 | 78.8% |

### Code and build statistics

| Metric | 0.9 | 0.10 | Boost | 0.10/Boost |
|---|---:|---:|---:|---:|
| Stripped executable | 33,912 B | 33,696 B | 150,616 B | 22.4% |
| Implementation code | 1,254 LOC | 1,776 LOC | 9,047 LOC | 19.6% |
| Clean build time | 0.13 s | 0.14 s | 0.97 s | 14.4% |

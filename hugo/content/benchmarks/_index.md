---
title: Benchmarks
weight: 3
---

The benchmark suite compares native diagram generation, peak and retained memory, allocation count, build time, and WebAssembly output workflows.

## C Runtime

![Voronoi diagram generation](../images/benchmark/release-0.10.0-voronoi.svg)

![Peak and retained memory](../images/benchmark/release-0.10.0-memory.svg)

![Code size and build time](../images/benchmark/release-0.10.0-code-build.svg)

## WebAssembly

The offline suite compares `jc_voronoi` with d3-delaunay, d3-voronoi, and gorhill/voronoi for 10k, 100k, and a pathological 100k-point input.

![Render Voronoi edges](../images/benchmark/wasm-voronoi-edges.svg)

![Get Delauney edges](../images/benchmark/wasm-delauney-edges.svg)

![Worker-backed JCV peak and retained memory](../images/benchmark/wasm-memory.svg)

![WebAssembly and JavaScript module size](../images/benchmark/wasm-code-size.svg)

[Read the complete methodology and tables on GitHub](https://github.com/JCash/voronoi/blob/dev/BENCHMARKS.md), or [run the offline browser benchmark](../benchmark/).

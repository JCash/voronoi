#!/usr/bin/env bash
set -euo pipefail

OUTPUT_DIR="${1:-build/wasm}"
mkdir -p "${OUTPUT_DIR}"

emcc wasm/voronoi_wasm.c \
  -Isrc \
  -O3 \
  -flto \
  -sWASM=1 \
  -sMODULARIZE=1 \
  -sEXPORT_ES6=1 \
  -sEXPORT_NAME=createVoronoiModule \
  -sENVIRONMENT=web,worker,node \
  -sFILESYSTEM=0 \
  -sALLOW_MEMORY_GROWTH=1 \
  -sEXPORTED_FUNCTIONS='["_jcv_voronoi_edges","_jcv_delauney_edges","_jcv_benchmark_generate","_jcv_benchmark_generate_sites","_jcv_benchmark_generate_edges","_jcv_benchmark_generate_delauney","_malloc","_free"]' \
  -sEXPORTED_RUNTIME_METHODS='["HEAP32","HEAPF32"]' \
  -o "${OUTPUT_DIR}/jc_voronoi.js"

cp wasm/voronoi.js "${OUTPUT_DIR}/voronoi.js"
cp wasm/package.json "${OUTPUT_DIR}/package.json"

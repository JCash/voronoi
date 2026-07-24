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
  -sEXPORTED_FUNCTIONS='["_jcv_wasm_diagram_create","_jcv_wasm_diagram_destroy","_jcv_wasm_diagram_input_count","_jcv_wasm_diagram_site_count","_jcv_wasm_diagram_vertex_count","_jcv_wasm_diagram_site","_jcv_wasm_diagram_site_at","_jcv_wasm_diagram_edge_count","_jcv_wasm_diagram_edge","_jcv_wasm_cell_edge_count","_jcv_wasm_cell_edge","_jcv_wasm_cell_edge_flags","_jcv_wasm_site_point","_jcv_wasm_site_index","_jcv_wasm_site_boundary","_jcv_wasm_point_x","_jcv_wasm_point_y","_jcv_wasm_edge_site","_jcv_wasm_edge_position","_jcv_wasm_edge_vertex","_jcv_voronoi_edges","_jcv_delauney_edges","_malloc","_free"]' \
  -sEXPORTED_RUNTIME_METHODS='["HEAP32","HEAPF32"]' \
  -o "${OUTPUT_DIR}/jc_voronoi.js"

cp wasm/voronoi.js "${OUTPUT_DIR}/voronoi.js"
cp wasm/package.json "${OUTPUT_DIR}/package.json"

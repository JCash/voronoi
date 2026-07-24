import assert from "node:assert/strict";
const outputDirectory = process.env.WASM_OUTPUT_DIR || "../build/wasm";
const { loadVoronoi } = await import(`${outputDirectory}/voronoi.js`);

const voronoi = await loadVoronoi();
const edges = voronoi.edges([
  { x: 10, y: 10 },
  { x: 90, y: 10 },
  { x: 50, y: 90 },
], 100, 100);

assert.ok(edges instanceof Float32Array);
assert.ok(edges.length > 0);
assert.equal(edges.length % 4, 0);
for (const coordinate of edges) {
  assert.ok(Number.isFinite(coordinate));
  assert.ok(coordinate >= 0 && coordinate <= 100);
}

assert.equal(voronoi.edges([], 100, 100).length, 0);
const delauneyEdges = voronoi.delauneyEdges([
  { x: 10, y: 10 },
  { x: 90, y: 10 },
  { x: 50, y: 90 },
], 100, 100);
assert.ok(delauneyEdges.length > 0);
assert.equal(delauneyEdges.length % 4, 0);
for (const coordinate of delauneyEdges) assert.ok(Number.isFinite(coordinate));

console.log(`Generated ${edges.length / 4} Voronoi and ${delauneyEdges.length / 4} Delauney edges`);

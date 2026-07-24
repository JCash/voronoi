import assert from "node:assert/strict";
import { diagramToJSON } from "../site/diagram_json.js";
const outputDirectory = process.env.WASM_OUTPUT_DIR || "../build/wasm";
const { loadVoronoi } = await import(`${outputDirectory}/voronoi.js`);

const voronoi = await loadVoronoi();
const points = [
  { x: 10, y: 10 },
  { x: 90, y: 10 },
  { x: 50, y: 90 },
];
const edges = voronoi.edges(points, 100, 100);

assert.ok(edges instanceof Float32Array);
assert.ok(edges.length > 0);
assert.equal(edges.length % 4, 0);
for (const coordinate of edges) {
  assert.ok(Number.isFinite(coordinate));
  assert.ok(coordinate >= 0 && coordinate <= 100);
}

assert.equal(voronoi.edges([], 100, 100).length, 0);
const delauneyEdges = voronoi.delauneyEdges(points, 100, 100);
assert.ok(delauneyEdges.length > 0);
assert.equal(delauneyEdges.length % 4, 0);
for (const coordinate of delauneyEdges) assert.ok(Number.isFinite(coordinate));

const diagram = voronoi.generate(points, { bounds: [0, 0, 100, 100] });
assert.equal(diagram.inputCount, 3);
assert.equal(diagram.numSites, 3);
assert.ok(diagram.numVertices > 0);
assert.equal(diagram.numEdges, diagram.edges.length);
assert.equal(
  diagram.numDelauneyEdges,
  diagram.edges.filter((edge) => edge.sites[0] && edge.sites[1]).length,
);
assert.deepEqual(diagram.bounds, [0, 0, 100, 100]);
assert.equal(diagram.sites.length, 3);
assert.ok(diagram.edges.length > 0);

const voronoiPath = [];
const pathContext = {
  moveTo: (x, y) => voronoiPath.push(["M", x, y]),
  lineTo: (x, y) => voronoiPath.push(["L", x, y]),
};
assert.equal(diagram.render(pathContext), pathContext);
assert.equal(voronoiPath.length, diagram.numEdges * 2);
const delauneyPath = [];
const delauneyContext = {
  moveTo: (x, y) => delauneyPath.push(["M", x, y]),
  lineTo: (x, y) => delauneyPath.push(["L", x, y]),
};
assert.equal(diagram.renderDelauney(delauneyContext), delauneyContext);
assert.equal(delauneyPath.length, diagram.numDelauneyEdges * 2);

const cell = diagram.cell(0);
assert.ok(cell);
assert.equal(cell.site, diagram.site(0));
assert.equal(cell.site.index, 0);
assert.deepEqual(cell.site.p.toJSON(), points[0]);
assert.equal(cell.site.boundary, true);
assert.ok(cell.edges.length > 0);
assert.equal(cell.polygon.length, cell.edges.length + 1);
assert.deepEqual([...cell.site.p], [10, 10]);
assert.equal(cell.polygon[0].x, cell.polygon.at(-1).x);
assert.equal(cell.polygon[0].y, cell.polygon.at(-1).y);
for (const edge of cell.edges) {
  assert.equal(edge.sites[0], cell.site);
  assert.equal(edge.pos.length, 2);
  assert.equal(edge.vertices.length, 2);
  assert.equal("a" in edge, false);
  assert.equal("b" in edge, false);
  assert.equal("c" in edge, false);
}
assert.ok(cell.neighbors.length > 0);
assert.equal(diagram.neighbors(0), cell.neighbors);
for (const neighbor of cell.neighbors) assert.ok(neighbor instanceof Object);

const json = diagramToJSON(diagram);
assert.deepEqual(json.bounds, [0, 0, 100, 100]);
assert.equal(json.inputCount, 3);
assert.equal(json.sites.length, 3);
assert.equal(json.vertices.length, diagram.numVertices);
assert.ok(json.vertices.every(Boolean));
assert.equal(json.edges.length, diagram.edges.length);
assert.equal(json.cells.length, 3);
assert.equal(json.cells[0].site, 0);
assert.equal(json.cells[0].edges.length, cell.edges.length);
assert.deepEqual(json.cells[0].neighbors, cell.neighbors.map((site) => site.index));
assert.equal(json.cells[0].polygon.length, cell.polygon.length);
assert.ok(json.delauneyEdges.length > 0);
assert.deepEqual(Object.keys(json.edges[0]).sort(), ["pos", "sites", "vertices"]);

assert.throws(() => diagram.cell(-1), RangeError);
assert.throws(() => diagram.cell(3), RangeError);

const pruned = voronoi.generate([
  { x: 20, y: 20 },
  { x: 20, y: 20 },
  { x: 80, y: 80 },
  { x: 200, y: 200 },
], 100, 100);
assert.equal(pruned.numSites, 2);
assert.equal([pruned.cell(0), pruned.cell(1)].filter((value) => value === null).length, 1);
assert.equal(pruned.cell(3), null);
assert.deepEqual(pruned.neighbors(3), []);
const prunedJSON = diagramToJSON(pruned);
assert.equal(prunedJSON.cells.filter((value) => value === null).length, 2);
pruned.dispose();

const manyPoints = Array.from({ length: 128 }, (_, index) => ({
  x: 2 + ((index * 73) % 997) / 997 * 96,
  y: 2 + ((index * 193) % 991) / 991 * 96,
}));
const many = voronoi.generate(manyPoints, 100, 100);
const globalVertexPairs = new Set(many.edges.map((edge) =>
  [...edge.vertices].sort((a, b) => a - b).join(":"),
));
for (let inputIndex = 0; inputIndex < many.inputCount; ++inputIndex) {
  const manyCell = many.cell(inputIndex);
  if (!manyCell) continue;
  assert.equal(new Set(manyCell.neighbors.map((site) => site.index)).size, manyCell.neighbors.length);
  for (let edgeIndex = 0; edgeIndex < manyCell.edges.length; ++edgeIndex) {
    const edge = manyCell.edges[edgeIndex];
    const next = manyCell.edges[(edgeIndex + 1) % manyCell.edges.length];
    assert.equal(edge.sites[0], manyCell.site);
    assert.equal(edge.pos[1].x, next.pos[0].x);
    assert.equal(edge.pos[1].y, next.pos[0].y);
    assert.ok(globalVertexPairs.has([...edge.vertices].sort((a, b) => a - b).join(":")));
  }
}
many.dispose();

const empty = voronoi.generate([], 100, 100);
assert.equal(empty.inputCount, 0);
assert.equal(empty.numSites, 0);
assert.equal(empty.numVertices, 0);
assert.deepEqual(empty.sites, []);
assert.deepEqual(empty.edges, []);
assert.throws(() => empty.cell(0), RangeError);
empty.dispose();

const retainedEdge = cell.edges[0];
diagram.dispose();
diagram.dispose();
assert.throws(() => diagram.numSites, /disposed/);
assert.throws(() => retainedEdge.pos, /disposed/);

console.log(`Generated ${edges.length / 4} Voronoi and ${delauneyEdges.length / 4} Delauney edges`);

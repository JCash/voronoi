import path from "node:path";
import { pathToFileURL } from "node:url";

const [library, caseName, wasmDirectory] = process.argv.slice(2);
const WIDTH = 4096;

function randomPoints(count) {
  let state = 0x12345678;
  const points = new Float32Array(count * 2);
  for (let i = 0; i < points.length; ++i) {
    state = (Math.imul(state, 1664525) + 1013904223) >>> 0;
    points[i] = 10 + (state / 0x100000000) * (WIDTH - 20);
  }
  return { points, bounds: [0, 0, WIDTH, WIDTH] };
}

function issue48Points() {
  const count = 99998;
  const points = new Float32Array(count * 2);
  for (let i = 0; i < count; ++i) {
    const value = Math.floor(i / 2) + 1;
    points[i * 2] = i & 1 ? -value : value;
    points[i * 2 + 1] = -value;
  }
  return { points, bounds: [-50000, -50000, 50000, 0] };
}

function inputForCase(name) {
  if (name === "10k") return randomPoints(10000);
  if (name === "100k") return randomPoints(100000);
  if (name === "100k pathological") return issue48Points();
  throw new Error(`Unknown memory benchmark case: ${name}`);
}

async function collectGarbage() {
  for (let i = 0; i < 3; ++i) {
    globalThis.gc?.();
    await new Promise((resolve) => setImmediate(resolve));
  }
}

const { points, bounds } = inputForCase(caseName);
let generate;

if (library === "JCV 0.11") {
  const { loadVoronoi } = await import(pathToFileURL(path.join(wasmDirectory, "voronoi.js")));
  const voronoi = await loadVoronoi();
  generate = () => voronoi.generate(points, { bounds });
} else if (library === "d3-delaunay") {
  const { Delaunay } = await import("d3-delaunay");
  const prepared = new Float64Array(points);
  generate = () => new Delaunay(prepared).voronoi(bounds);
} else if (library === "d3-voronoi") {
  const { voronoi } = await import("d3-voronoi");
  const prepared = Array.from({ length: points.length / 2 }, (_, index) => [
    points[index * 2],
    points[index * 2 + 1],
  ]);
  const generator = voronoi().extent([
    [bounds[0], bounds[1]],
    [bounds[2], bounds[3]],
  ]);
  generate = () => generator(prepared);
} else if (library === "gorhill/voronoi") {
  const { default: Voronoi } = await import("voronoi");
  const prepared = Array.from({ length: points.length / 2 }, (_, index) => ({
    x: points[index * 2],
    y: points[index * 2 + 1],
  }));
  const box = { xl: bounds[0], xr: bounds[2], yt: bounds[1], yb: bounds[3] };
  generate = () => new Voronoi().compute(prepared, box);
} else {
  throw new Error(`Unknown memory benchmark library: ${library}`);
}

await collectGarbage();
const baseline = process.memoryUsage().rss;
const baselineMaximum = process.resourceUsage().maxRSS * 1024;
globalThis.retainedDiagram = generate();
const generatedMaximum = process.resourceUsage().maxRSS * 1024;
await collectGarbage();
const retained = process.memoryUsage().rss;
const retainedBytes = Math.max(0, retained - baseline);

process.stdout.write(JSON.stringify({
  peakBytes: Math.max(retainedBytes, generatedMaximum - baselineMaximum),
  retainedBytes,
}));

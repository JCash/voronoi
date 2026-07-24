import path from "node:path";
import { pathToFileURL } from "node:url";

const [caseName, wasmDirectory] = process.argv.slice(2);
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
  throw new Error(`Unknown worker memory benchmark case: ${name}`);
}

async function collectGarbage() {
  for (let i = 0; i < 3; ++i) {
    globalThis.gc?.();
    await new Promise((resolve) => setImmediate(resolve));
  }
}

const { points, bounds } = inputForCase(caseName);
const { loadVoronoiWorker } = await import(pathToFileURL(path.join(wasmDirectory, "voronoi.js")));
const voronoi = await loadVoronoiWorker();

await collectGarbage();
const baseline = process.memoryUsage().rss;
let maximum = baseline;
const sampler = setInterval(() => {
  maximum = Math.max(maximum, process.memoryUsage().rss);
}, 1);

const diagram = await voronoi.generate(points, { bounds });
maximum = Math.max(maximum, process.memoryUsage().rss);
clearInterval(sampler);

await collectGarbage();
const retained = process.memoryUsage().rss;
const generationMemory = diagram._generationMemory;
globalThis.retainedDiagram = diagram;

process.stdout.write(JSON.stringify({
  peakBytes: Math.max(0, maximum - baseline),
  retainedBytes: Math.max(0, retained - baseline),
  inputBytes: generationMemory.inputBytes,
  wasmHeapBytes: generationMemory.wasmHeapBytes,
  packedBytes: diagram.byteLength,
}));

import { loadVoronoi } from "../vendor/voronoi.js";
import { prepareComparisons } from "./vendor/comparisons.js";

const WIDTH = 4096;
const operations = [
  ["generate", "Generate diagram"],
  ["sites", "Generate + get sites"],
  ["edges", "Generate + render edges"],
  ["delaunay", "Generate + get Delaunay"],
];
const libraries = ["JCV 0.10", "d3-delaunay", "d3-voronoi", "gorhill/voronoi"];
const runButton = document.querySelector("#run");
const status = document.querySelector("#status");
const resultsElement = document.querySelector("#results");
let voronoi;
let sink;

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

function prepareJcv(points, bounds) {
  let checksum = 0;
  const context = {
    moveTo: (x, y) => { checksum += x + y; },
    lineTo: (x, y) => { checksum += x + y; },
  };
  const withDiagram = (read) => {
    const diagram = voronoi.generate(points, { bounds });
    try {
      return read(diagram);
    } finally {
      diagram.dispose();
    }
  };
  return {
    generate: () => withDiagram((diagram) => diagram.numSites),
    sites: () => withDiagram((diagram) => diagram.sites.length),
    edges: () => withDiagram((diagram) => {
      checksum = 0;
      diagram.render(context);
      return checksum;
    }),
    delaunay: () => voronoi.delaunayEdges(points, { bounds }).length / 4,
  };
}

function median(values) {
  const ordered = [...values].sort((a, b) => a - b);
  const middle = Math.floor(ordered.length / 2);
  return ordered.length % 2 ? ordered[middle] : (ordered[middle - 1] + ordered[middle]) / 2;
}

async function measure(operation, samples) {
  if (!operation) return null;
  for (let i = 0; i < 2; ++i) sink = operation();
  const times = [];
  for (let i = 0; i < samples; ++i) {
    const start = performance.now();
    const result = operation();
    const elapsed = performance.now() - start;
    sink = result;
    times.push(elapsed);
    await new Promise(requestAnimationFrame);
  }
  return median(times);
}

function renderTables(data) {
  resultsElement.replaceChildren(...operations.map(([key, label]) => {
    const section = document.createElement("section");
    section.className = "result";
    const heading = document.createElement("h2");
    heading.textContent = `${label} · median ms`;
    const table = document.createElement("table");
    const head = table.createTHead().insertRow();
    ["Case", ...libraries].forEach((name) => {
      const cell = document.createElement("th");
      cell.scope = "col";
      cell.textContent = name;
      head.append(cell);
    });
    const body = table.createTBody();
    for (const [caseName, result] of Object.entries(data)) {
      const row = body.insertRow();
      [caseName, ...libraries.map((library) => result[key][library])].forEach((value, index) => {
        const cell = row.insertCell();
        cell.textContent = index === 0
          ? value
          : value == null
            ? "—"
            : typeof value === "number"
              ? value.toFixed(2)
              : value;
      });
    }
    section.append(heading, table);
    return section;
  }));
}

async function run() {
  runButton.disabled = true;
  const cases = {
    "10k": randomPoints(10000),
    "100k": randomPoints(100000),
    "100k pathological": issue48Points(),
  };
  const output = {};
  try {
    for (const [caseName, input] of Object.entries(cases)) {
      status.textContent = `Preparing ${caseName}…`;
      const comparisons = prepareComparisons(input.points, input.bounds);
      const prepared = {
        "JCV 0.10": prepareJcv(input.points, input.bounds),
        ...comparisons,
      };
      output[caseName] = Object.fromEntries(operations.map(([key]) => [key, {}]));
      const samples = caseName === "10k" ? 10 : 5;
      for (const [operationKey, operationLabel] of operations) {
        for (const library of libraries) {
          status.textContent = `${caseName} · ${operationLabel} · ${library}`;
          try {
            output[caseName][operationKey][library] = await measure(prepared[library][operationKey], samples);
          } catch (error) {
            output[caseName][operationKey][library] = "error";
            console.error(`${caseName} / ${operationLabel} / ${library}`, error);
          }
        }
      }
      renderTables(output);
    }
    status.textContent = "Benchmark complete";
  } catch (error) {
    status.textContent = "Benchmark failed — see console";
    console.error(error);
  } finally {
    runButton.disabled = false;
  }
}

try {
  voronoi = await loadVoronoi();
  status.textContent = "Ready";
  runButton.disabled = false;
  runButton.addEventListener("click", run);
} catch (error) {
  status.textContent = "Libraries failed to load";
  console.error(error);
}

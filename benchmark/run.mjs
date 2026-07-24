import { mkdir, readFile, writeFile } from "node:fs/promises";
import os from "node:os";
import path from "node:path";
import { pathToFileURL } from "node:url";
import { prepareComparisons } from "./src/comparisons.js";
import { collectCodeSizes, codeSizeChart, codeSizeMarkdown } from "./code_size.mjs";

const repository = path.resolve(import.meta.dirname, "..");
const wasmDirectory = process.env.WASM_OUTPUT_DIR
  ? path.resolve(import.meta.dirname, process.env.WASM_OUTPUT_DIR)
  : path.join(repository, "build", "wasm");
const { default: createVoronoiModule } = await import(
  pathToFileURL(path.join(wasmDirectory, "jc_voronoi.js"))
);

const WIDTH = 4096;
const CASES = [
  ["10k", () => randomPoints(10000), 10],
  ["100k", () => randomPoints(100000), 5],
  ["100k pathological", issue48Points, 5],
];
const OPERATIONS = [
  [null, "Voronoi Diagram Generation", "generate", null],
  [null, "Generate + Get Sites", "sites", null],
  ["voronoi-edges", "Generate + Get Edges", "edges", "Get Voronoi Edges"],
  ["delauney-edges", "Generate + Get Delauney", "delauney", "Get Delauney Edges"],
];
const LIBRARIES = [
  ["JCV 0.10", "#5267d9"],
  ["d3-delaunay", "#14a38b"],
  ["d3-voronoi", "#e08b35"],
  ["gorhill/voronoi", "#b85bb4"],
];
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

function prepareWasm(module, points, bounds) {
  const pointer = module._malloc(points.byteLength);
  module.HEAPF32.set(points, pointer / Float32Array.BYTES_PER_ELEMENT);
  const args = [pointer, points.length / 2, ...bounds];
  return {
    operations: {
      generate: () => module._jcv_benchmark_generate(...args),
      sites: () => module._jcv_benchmark_generate_sites(...args),
      edges: () => module._jcv_benchmark_generate_edges(...args),
      delauney: () => module._jcv_benchmark_generate_delauney(...args),
    },
    dispose: () => module._free(pointer),
  };
}

function median(values) {
  const ordered = [...values].sort((a, b) => a - b);
  const middle = Math.floor(ordered.length / 2);
  return ordered.length % 2
    ? ordered[middle]
    : (ordered[middle - 1] + ordered[middle]) / 2;
}

function measure(operation, samples) {
  if (!operation) return null;
  for (let i = 0; i < 2; ++i) sink = operation();
  const times = [];
  for (let i = 0; i < samples; ++i) {
    globalThis.gc?.();
    const start = performance.now();
    const result = operation();
    const elapsed = performance.now() - start;
    sink = result;
    times.push(elapsed);
  }
  return median(times);
}

function formatTime(milliseconds) {
  if (milliseconds == null) return "—";
  if (milliseconds < 1) return `${(milliseconds * 1000).toFixed(2)} us`;
  if (milliseconds >= 1000) return `${(milliseconds / 1000).toFixed(2)} s`;
  return `${milliseconds.toFixed(2)} ms`;
}

function escapeXml(value) {
  return String(value)
    .replaceAll("&", "&amp;")
    .replaceAll("<", "&lt;")
    .replaceAll(">", "&gt;")
    .replaceAll('"', "&quot;");
}

function niceMaximum(value) {
  if (value <= 0) return 1;
  const magnitude = 10 ** Math.floor(Math.log10(value));
  const normalized = value / magnitude;
  const nice = normalized <= 1 ? 1 : normalized <= 2 ? 2 : normalized <= 4 ? 4 : normalized <= 5 ? 5 : 10;
  return nice * magnitude;
}

function chart(operationTitle, operationKey, results) {
  const chartLibraries = operationKey === "delauney"
    ? LIBRARIES.filter(([library]) => library !== "gorhill/voronoi")
    : LIBRARIES;
  const width = 1200;
  const height = 720;
  const left = 120;
  const right = 50;
  const top = 110;
  const bottom = 575;
  const plotHeight = bottom - top;
  const measuredMaximum = Math.max(...Object.values(results).flatMap((result) =>
    chartLibraries.map(([library]) => result[operationKey][library]).filter(Number.isFinite),
  ));
  const maximum = operationKey === "delauney"
    ? niceMaximum(measuredMaximum * 1.1)
    : 500;
  const caseWidth = (width - left - right) / CASES.length;
  const barWidth = 48;
  const barGap = 8;
  const groupWidth = chartLibraries.length * barWidth + (chartLibraries.length - 1) * barGap;
  const lines = [];
  const subtitle = operationKey === "edges" || operationKey === "delauney"
    ? "Diagram generation + complete edge retrieval · median time · lower is better"
    : "Median time · linear scale · lower is better";

  lines.push(`<svg xmlns="http://www.w3.org/2000/svg" width="${width}" height="${height}" viewBox="0 0 ${width} ${height}" role="img" aria-labelledby="title desc">`);
  lines.push(`  <title id="title">${escapeXml(operationTitle)}</title>`);
  lines.push(`  <desc id="desc">Grouped bar chart comparing WebAssembly and JavaScript Voronoi libraries for 10k, 100k, and the 100k pathological case. Times are medians in milliseconds; lower is better. Bars above 500 milliseconds are truncated with a jagged top.</desc>`);
  lines.push("  <style>.stat-hit{fill:transparent;pointer-events:all}</style>");
  lines.push('  <rect width="1200" height="720" fill="#f8fafc"/>');
  lines.push(`  <text x="70" y="49" font-family="system-ui, sans-serif" font-size="27" font-weight="700" fill="#152238">${escapeXml(operationTitle)}</text>`);
  lines.push(`  <text x="70" y="76" font-family="system-ui, sans-serif" font-size="15" fill="#536174">${escapeXml(subtitle)}</text>`);

  for (let tick = 0; tick <= 5; ++tick) {
    const value = maximum * tick / 5;
    const y = bottom - plotHeight * tick / 5;
    lines.push(`  <line x1="${left}" y1="${y}" x2="${width - right}" y2="${y}" stroke="#d8dee8"/>`);
    lines.push(`  <text x="${left - 12}" y="${y + 5}" font-family="system-ui, sans-serif" font-size="13" fill="#536174" text-anchor="end">${escapeXml(tick === 0 ? "0" : formatTime(value))}</text>`);
  }

  CASES.forEach(([caseName], caseIndex) => {
    const center = left + caseWidth * (caseIndex + 0.5);
    const startX = center - groupWidth / 2;
    lines.push(`  <text x="${center}" y="612" font-family="system-ui, sans-serif" font-size="15" font-weight="600" fill="#26354a" text-anchor="middle">${caseName}</text>`);
    chartLibraries.forEach(([library, color], libraryIndex) => {
      const value = results[caseName][operationKey][library];
      const x = startX + libraryIndex * (barWidth + barGap);
      if (value == null) {
        lines.push(`  <text x="${x + barWidth / 2}" y="565" font-family="system-ui, sans-serif" font-size="11" fill="#536174" text-anchor="middle">N/A</text>`);
        return;
      }
      const overflow = value > maximum;
      const barHeight = Math.max(1, Math.min(plotHeight, value / maximum * plotHeight));
      const y = bottom - barHeight;
      const labelY = Math.max(top - 5, y - 8 - (libraryIndex % 2) * 14);
      const formatted = formatTime(value);
      if (overflow) {
        lines.push(`  <path d="M${x} ${bottom} V${top + 10} L${x + 8} ${top} L${x + 16} ${top + 10} L${x + 24} ${top} L${x + 32} ${top + 10} L${x + 40} ${top} L${x + barWidth} ${top + 10} V${bottom} Z" fill="${color}"/>`);
      } else {
        lines.push(`  <rect x="${x}" y="${y.toFixed(2)}" width="${barWidth}" height="${barHeight.toFixed(2)}" rx="4" fill="${color}"/>`);
      }
      lines.push(`  <text x="${x + barWidth / 2}" y="${labelY.toFixed(2)}" font-family="system-ui, sans-serif" font-size="10" font-weight="650" fill="#35445a" text-anchor="middle">${escapeXml(formatted)}</text>`);
      lines.push(`  <g class="stat-bar" tabindex="0"><rect class="stat-hit" x="${x}" y="${Math.min(y, bottom - 15).toFixed(2)}" width="${barWidth}" height="${Math.max(15, barHeight).toFixed(2)}"><title>${escapeXml(`${library}, ${caseName}: median ${formatted}`)}</title></rect></g>`);
    });
  });

  const legendWidth = 210;
  const legendStart = (width - legendWidth * chartLibraries.length) / 2;
  chartLibraries.forEach(([library, color], index) => {
    const x = legendStart + index * legendWidth;
    lines.push(`  <rect x="${x}" y="660" width="16" height="16" rx="3" fill="${color}"/><text x="${x + 24}" y="674" font-family="system-ui, sans-serif" font-size="14" fill="#344258">${escapeXml(library)}</text>`);
  });
  lines.push("</svg>");
  return `${lines.join("\n")}\n`;
}

function markdown(results, codeSizes) {
  const cpu = os.cpus()[0]?.model ?? "Unknown CPU";
  const lines = [
    "<!-- wasm-benchmarks:start -->",
    "## WebAssembly and JavaScript Voronoi performance",
    "",
    "Times are medians and lower is better. The benchmark compares only calls into each library. Point generation, input conversion, WebAssembly initialization, copying points into WebAssembly memory, garbage collection, and report generation are outside the timed regions.",
    "",
    "The random cases use a deterministic seed. `100k pathological` is the issue48 input with 99,998 symmetric diagonal-pair sites. Each measurement has two untimed warmups followed by 10 samples for 10k and five samples for 100k and the pathological case.",
    "",
    "For JCV, site access calls `jcv_diagram_get_sites`, an O(1) pointer lookup. Edge and Delauney access consume their complete public iterators. Other libraries expose different public output forms: d3-voronoi and voronoi eagerly provide arrays, while d3-delaunay renders its Voronoi mesh. These rows therefore compare public access workflows, not identical post-processing algorithms.",
    "",
  ];

  for (const [fileKey, title, operationKey, chartTitle] of OPERATIONS) {
    lines.push(`### ${title}`, "", `| Case | ${LIBRARIES.map(([name]) => name).join(" | ")} |`, `|---|${LIBRARIES.map(() => "---:").join("|")}|`);
    for (const [caseName] of CASES) {
      lines.push(`| ${caseName} | ${LIBRARIES.map(([library]) => formatTime(results[caseName][operationKey][library])).join(" | ")} |`);
    }
    if (fileKey) {
      lines.push("", `<img src="images/benchmark/wasm-${fileKey}.svg" alt="${chartTitle}" width="350">`, "");
    } else {
      lines.push("");
    }
  }

  lines.push(codeSizeMarkdown(codeSizes), "");

  lines.push(
    "### Environment",
    "",
    `- ${cpu}`,
    `- ${os.type()} ${os.release()} (${os.arch()})`,
    `- Node.js ${process.version}`,
    "- Wasm compiled with Emscripten `-O3 -flto`",
    "- d3-delaunay 6.0.4, d3-voronoi 1.1.4, gorhill/voronoi 1.0.0",
    "",
    "The `gorhill/voronoi` package has no direct Delauney retrieval operation, so that entry is not applicable.",
    "",
    "<!-- wasm-benchmarks:end -->",
  );
  return lines.join("\n");
}

const module = await createVoronoiModule();
const results = {};
for (const [caseName, createInput, samples] of CASES) {
  process.stdout.write(`Benchmarking ${caseName}\n`);
  const { points, bounds } = createInput();
  const wasm = prepareWasm(module, points, bounds);
  const prepared = {
    "JCV 0.10": wasm.operations,
    ...prepareComparisons(points, bounds),
  };
  results[caseName] = Object.fromEntries(OPERATIONS.map(([, , key]) => [key, {}]));
  try {
    for (const [, title, operationKey] of OPERATIONS) {
      for (const [library] of LIBRARIES) {
        process.stdout.write(`  ${title}: ${library}\n`);
        results[caseName][operationKey][library] = measure(prepared[library][operationKey], samples);
      }
    }
  } finally {
    wasm.dispose();
  }
}

const imageDirectory = path.join(repository, "images", "benchmark");
await mkdir(imageDirectory, { recursive: true });
for (const [fileKey, , operationKey, chartTitle] of OPERATIONS) {
  if (fileKey) {
    await writeFile(path.join(imageDirectory, `wasm-${fileKey}.svg`), chart(chartTitle, operationKey, results));
  }
}
const codeSizes = await collectCodeSizes(wasmDirectory);
await writeFile(path.join(imageDirectory, "wasm-code-size.svg"), codeSizeChart(codeSizes));
const reportPath = path.join(repository, "Benchmarks.md");
const generatedSection = markdown(results, codeSizes);
let report;
try {
  report = await readFile(reportPath, "utf8");
} catch (error) {
  if (error.code !== "ENOENT") throw error;
  report = "";
}
const startMarker = "<!-- wasm-benchmarks:start -->";
const endMarker = "<!-- wasm-benchmarks:end -->";
const start = report.indexOf(startMarker);
const end = report.indexOf(endMarker);
if (start >= 0 && end >= start) {
  report = `${report.slice(0, start)}${generatedSection}${report.slice(end + endMarker.length)}`;
} else {
  report = `${report.trimEnd()}${report.trim() ? "\n\n" : ""}${generatedSection}\n`;
}
await writeFile(reportPath, report);
process.stdout.write("Wrote Benchmarks.md and images/benchmark/wasm-*.svg\n");

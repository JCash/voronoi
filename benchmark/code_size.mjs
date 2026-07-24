import { readdir, readFile, writeFile } from "node:fs/promises";
import path from "node:path";
import { pathToFileURL } from "node:url";
import { brotliCompressSync, constants as zlibConstants } from "node:zlib";

const repository = path.resolve(import.meta.dirname, "..");
const modules = path.join(import.meta.dirname, "node_modules");
const COLORS = {
  "JCV 0.10": "#5267d9",
  "d3-delaunay": "#14a38b",
  "d3-voronoi": "#e08b35",
  "gorhill/voronoi": "#b85bb4",
};

async function javascriptFiles(directory) {
  const output = [];
  for (const entry of await readdir(directory, { withFileTypes: true })) {
    const file = path.join(directory, entry.name);
    if (entry.isDirectory()) output.push(...await javascriptFiles(file));
    else if (entry.name.endsWith(".js") || entry.name.endsWith(".mjs")) output.push(file);
  }
  return output;
}

function countCodeLines(source) {
  let inBlockComment = false;
  let count = 0;
  for (const originalLine of source.split(/\r?\n/)) {
    let line = originalLine;
    while (line.length) {
      if (inBlockComment) {
        const end = line.indexOf("*/");
        if (end < 0) { line = ""; break; }
        line = line.slice(end + 2);
        inBlockComment = false;
      }
      const block = line.indexOf("/*");
      const single = line.indexOf("//");
      if (single >= 0 && (block < 0 || single < block)) {
        line = line.slice(0, single);
        break;
      }
      if (block < 0) break;
      const end = line.indexOf("*/", block + 2);
      if (end >= 0) line = `${line.slice(0, block)} ${line.slice(end + 2)}`;
      else { line = line.slice(0, block); inBlockComment = true; break; }
    }
    if (line.trim()) ++count;
  }
  return count;
}

async function sourceLines(files) {
  let count = 0;
  for (const file of files) count += countCodeLines(await readFile(file, "utf8"));
  return count;
}

export async function collectCodeSizes(wasmDirectory = path.join(repository, "build", "wasm")) {
  const definitions = {
    "JCV 0.10": {
      module: path.join(wasmDirectory, "jc_voronoi.wasm"),
      sources: [path.join(repository, "src", "jc_voronoi.h"), path.join(repository, "wasm", "voronoi_wasm.c"), path.join(repository, "wasm", "voronoi.js")],
      dependencies: [],
    },
    "d3-delaunay": {
      module: path.join(modules, "d3-delaunay", "dist", "d3-delaunay.min.js"),
      sources: [
        ...await javascriptFiles(path.join(modules, "d3-delaunay", "src")),
        path.join(modules, "delaunator", "index.js"),
        path.join(modules, "robust-predicates", "esm", "orient2d.js"),
        path.join(modules, "robust-predicates", "esm", "util.js"),
      ],
      dependencies: ["delaunator", "robust-predicates"],
    },
    "d3-voronoi": {
      module: path.join(modules, "d3-voronoi", "dist", "d3-voronoi.min.js"),
      sources: await javascriptFiles(path.join(modules, "d3-voronoi", "src")),
      dependencies: [],
    },
    "gorhill/voronoi": {
      module: path.join(modules, "voronoi", "rhill-voronoi-core.min.js"),
      sources: [path.join(modules, "voronoi", "rhill-voronoi-core.js")],
      dependencies: [],
    },
  };
  const output = {};
  for (const [library, definition] of Object.entries(definitions)) {
    const payload = await readFile(definition.module);
    output[library] = {
      bytes: payload.byteLength,
      brotli: brotliCompressSync(payload, { params: { [zlibConstants.BROTLI_PARAM_QUALITY]: 11 } }).byteLength,
      lines: await sourceLines(definition.sources),
      dependencies: definition.dependencies,
    };
  }
  return output;
}

function formatBytes(bytes) {
  return `${(bytes / 1024).toFixed(1)} KiB`;
}

export function codeSizeMarkdown(metrics) {
  const lines = [
    "<!-- wasm-code-size:start -->",
    "### Code size",
    "",
    "| Library | Dependencies | Compound module | Brotli | Compound LOC |",
    "|---|---:|---:|---:|---:|",
  ];
  for (const [library, values] of Object.entries(metrics)) {
    const dependencyList = values.dependencies.length
      ? `${values.dependencies.length} (${values.dependencies.join(", ")})`
      : "0";
    lines.push(`| ${library} | ${dependencyList} | ${formatBytes(values.bytes)} | ${formatBytes(values.brotli)} | ${values.lines.toLocaleString("en-US")} |`);
  }
  lines.push(
    "",
    '<img src="images/benchmark/wasm-code-size.svg" alt="WebAssembly and JavaScript module size" width="350">',
    "",
    "Compound module size is the JCV `.wasm` payload or the library’s published minified browser module including runtime dependencies. Brotli uses quality 11. Compound LOC counts nonblank, non-comment implementation lines including runtime dependency sources. d3-delaunay depends directly on delaunator, which depends on robust-predicates; the other modules have no runtime package dependencies.",
    "",
    "<!-- wasm-code-size:end -->",
  );
  return lines.join("\n");
}

export function codeSizeChart(metrics) {
  const libraries = Object.keys(metrics);
  const panels = [
    ["Compound module", "bytes", formatBytes],
    ["Brotli", "brotli", formatBytes],
    ["Compound source LOC", "lines", (value) => value.toLocaleString("en-US")],
  ];
  const lines = [
    '<svg xmlns="http://www.w3.org/2000/svg" width="1200" height="720" viewBox="0 0 1200 720" role="img" aria-labelledby="title desc">',
    '  <title id="title">WebAssembly and JavaScript Code Size</title>',
    '  <desc id="desc">Three independent horizontal bar groups compare compound module bytes, Brotli-compressed bytes, and compound source lines of code including dependencies for four Voronoi libraries. Lower is better.</desc>',
    '  <rect width="1200" height="720" fill="#f8fafc"/>',
    '  <text x="70" y="49" font-family="system-ui, sans-serif" font-size="27" font-weight="700" fill="#152238">WebAssembly and JavaScript Code Size</text>',
    '  <text x="70" y="76" font-family="system-ui, sans-serif" font-size="15" fill="#536174">Runtime dependencies included · independent scale for each metric · lower is better</text>',
  ];

  const legendStart = 150;
  libraries.forEach((library, index) => {
    const x = legendStart + index * 235;
    lines.push(`  <rect x="${x}" y="93" width="16" height="16" rx="3" fill="${COLORS[library]}"/><text x="${x + 24}" y="107" font-family="system-ui, sans-serif" font-size="14" fill="#344258">${library}</text>`);
  });

  panels.forEach(([title, key, format], panelIndex) => {
    const panelY = 125 + panelIndex * 170;
    const maximum = Math.max(...libraries.map((library) => metrics[library][key]));
    const unit = key === "lines" ? "nonblank, non-comment LOC" : key === "brotli" ? "Brotli quality 11" : "production payload";
    lines.push(`  <rect x="70" y="${panelY}" width="1060" height="155" rx="9" fill="#ffffff" stroke="#e0e6ef"/>`);
    lines.push(`  <text x="95" y="${panelY + 30}" font-family="system-ui, sans-serif" font-size="17" font-weight="700" fill="#26354a">${title}</text>`);
    lines.push(`  <text x="1085" y="${panelY + 30}" font-family="system-ui, sans-serif" font-size="12" fill="#6b7789" text-anchor="end">${unit}</text>`);
    libraries.forEach((library, index) => {
      const value = metrics[library][key];
      const barWidth = value / maximum * 700;
      const y = panelY + 43 + index * 25;
      lines.push(`  <rect x="280" y="${y}" width="${barWidth.toFixed(2)}" height="18" rx="4" fill="${COLORS[library]}"/>`);
      lines.push(`  <text x="${(290 + barWidth).toFixed(2)}" y="${y + 14}" font-family="system-ui, sans-serif" font-size="11" font-weight="650" fill="#35445a">${format(value)}</text>`);
    });
  });
  lines.push('  <text x="600" y="685" font-family="system-ui, sans-serif" font-size="12" fill="#6b7789" text-anchor="middle">d3-delaunay: 2 dependencies · all others: 0 · JCV Wasm payload excludes generated JavaScript glue</text>');
  lines.push("</svg>");
  return `${lines.join("\n")}\n`;
}

async function updateCodeSizeReport() {
  const wasmDirectory = process.env.WASM_OUTPUT_DIR
    ? path.resolve(import.meta.dirname, process.env.WASM_OUTPUT_DIR)
    : path.join(repository, "build", "wasm");
  const metrics = await collectCodeSizes(wasmDirectory);
  await writeFile(path.join(repository, "images", "benchmark", "wasm-code-size.svg"), codeSizeChart(metrics));
  const reportPath = path.join(repository, "Benchmarks.md");
  let report = await readFile(reportPath, "utf8");
  const section = codeSizeMarkdown(metrics);
  const startMarker = "<!-- wasm-code-size:start -->";
  const endMarker = "<!-- wasm-code-size:end -->";
  const start = report.indexOf(startMarker);
  const end = report.indexOf(endMarker);
  if (start >= 0 && end >= start) {
    report = `${report.slice(0, start)}${section}${report.slice(end + endMarker.length)}`;
  } else {
    const wasmEnd = report.indexOf("<!-- wasm-benchmarks:end -->");
    report = `${report.slice(0, wasmEnd)}${section}\n\n${report.slice(wasmEnd)}`;
  }
  await writeFile(reportPath, report);
}

if (process.argv[1] && import.meta.url === pathToFileURL(process.argv[1]).href) {
  await updateCodeSizeReport();
  process.stdout.write("Wrote code-size table and images/benchmark/wasm-code-size.svg\n");
}

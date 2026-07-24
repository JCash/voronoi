import { diagramToJSON } from "./diagram_json.js";
import { readPointFile } from "./point_file.js";

const canvas = document.querySelector("#diagram");
const context = canvas.getContext("2d");
const countInput = document.querySelector("#site-count");
const countOutput = document.querySelector("#site-count-output");
const status = document.querySelector("#status");
const delauneyButton = document.querySelector("#delauney");
const copyButton = document.querySelector("#copy-json");
const pointForm = document.querySelector("#point-form");
const pointXInput = document.querySelector("#point-x");
const pointYInput = document.querySelector("#point-y");
const playground = document.querySelector(".voronoi-playground");
const openButton = document.querySelector("#open-file");
const openInput = document.querySelector("#open-file-input");
let points = [];
let voronoi;
let diagram;
let draggedPoint = -1;
let showDelauney = false;

function readBoolean(value) {
  return ["1", "true", "yes", "on"].includes((value || "").toLowerCase());
}

function readInteger(value, fallback, minimum, maximum, name) {
  if (value === null) return fallback;
  const parsed = Number(value);
  if (!Number.isInteger(parsed) || parsed < minimum || parsed > maximum) {
    throw new Error(`${name} must be an integer from ${minimum} to ${maximum}`);
  }
  return parsed;
}

function readPoints(value) {
  if (!value) return null;
  const parsed = value.split(";").filter(Boolean).map((pair) => {
    const coordinates = pair.split(",").map(Number);
    if (coordinates.length !== 2 || coordinates.some((coordinate) => !Number.isFinite(coordinate) || coordinate < 0 || coordinate > 1)) {
      throw new Error("points must contain normalized x,y pairs");
    }
    return { x: coordinates[0], y: coordinates[1] };
  });
  if (parsed.length < 2 || parsed.length > 250) {
    throw new Error("points must contain between 2 and 250 sites");
  }
  return parsed;
}

async function openFile(file) {
  if (!file) return;
  try {
    points = readPointFile(await file.text());
    updateCount();
    draw();
    status.textContent = `Opened ${points.length} sites from ${file.name}`;
  } catch (error) {
    status.textContent = `Open failed: ${error.message}`;
  }
}

function readConfiguration() {
  const parameters = new URLSearchParams(window.location.search);
  return {
    points: readPoints(parameters.get("points")),
    count: readInteger(parameters.get("count"), 42, 2, 250, "count"),
    seed: readInteger(parameters.get("seed"), null, 0, 4294967295, "seed"),
    delauney: readBoolean(parameters.get("delauney")),
  };
}

function seededRandom(seed) {
  let state = seed >>> 0;
  return () => {
    state = (Math.imul(state, 1664525) + 1013904223) >>> 0;
    return state / 4294967296;
  };
}

function updateCount() {
  if (countOutput) countOutput.value = String(points.length);
  if (countInput && points.length >= Number(countInput.min) && points.length <= Number(countInput.max)) {
    countInput.value = String(points.length);
  }
}

function randomize(count, random = Math.random) {
  points = Array.from({ length: count }, () => ({
    x: .035 + random() * .93,
    y: .055 + random() * .89,
  }));
  updateCount();
  draw();
}

function resize() {
  const ratio = window.devicePixelRatio || 1;
  const bounds = canvas.getBoundingClientRect();
  canvas.width = Math.round(bounds.width * ratio);
  canvas.height = Math.round(bounds.height * ratio);
  draw();
}

function draw() {
  if (!canvas.width || !voronoi) return;
  const { width, height } = canvas;
  const scaled = points.map(({ x, y }) => ({ x: x * width, y: y * height }));
  diagram?.dispose();
  diagram = voronoi.generate(scaled, width, height);
  context.fillStyle = "#1a1c19";
  context.fillRect(0, 0, width, height);
  if (showDelauney) {
    context.strokeStyle = "#ff9367aa";
    context.lineWidth = Math.max(1, window.devicePixelRatio || 1);
    context.beginPath();
    diagram.renderDelauney(context);
    context.stroke();
  }
  context.strokeStyle = "#72796c";
  context.lineWidth = Math.max(1, window.devicePixelRatio || 1);
  context.beginPath();
  diagram.render(context);
  context.stroke();
  const radius = 3.25 * (window.devicePixelRatio || 1);
  context.fillStyle = "#d8ff57";
  for (const point of scaled) {
    context.beginPath();
    context.arc(point.x, point.y, radius, 0, Math.PI * 2);
    context.fill();
  }
  const delauneyCount = showDelauney ? diagram.numDelauneyEdges : 0;
  const delauneyStatus = showDelauney ? ` · ${delauneyCount} Delauney` : "";
  status.textContent = `${diagram.numSites} sites · ${diagram.numEdges} edges${delauneyStatus}`;
}

async function copyJSON() {
  if (!diagram) return;
  const json = JSON.stringify(diagramToJSON(diagram), null, 2);
  try {
    await navigator.clipboard.writeText(json);
  } catch {
    const textarea = document.createElement("textarea");
    textarea.value = json;
    textarea.style.position = "fixed";
    textarea.style.opacity = "0";
    document.body.append(textarea);
    textarea.select();
    document.execCommand("copy");
    textarea.remove();
  }
  status.textContent = "Full diagram JSON copied";
}

function eventPoint(event) {
  const bounds = canvas.getBoundingClientRect();
  return {
    x: Math.min(1, Math.max(0, (event.clientX - bounds.left) / bounds.width)),
    y: Math.min(1, Math.max(0, (event.clientY - bounds.top) / bounds.height)),
  };
}

function nearest(point) {
  let best = -1;
  const bounds = canvas.getBoundingClientRect();
  let distance = 18 ** 2;
  points.forEach((candidate, index) => {
    const deltaX = (candidate.x - point.x) * bounds.width;
    const deltaY = (candidate.y - point.y) * bounds.height;
    const candidateDistance = deltaX ** 2 + deltaY ** 2;
    if (candidateDistance < distance) { best = index; distance = candidateDistance; }
  });
  return best;
}

canvas.addEventListener("pointerdown", (event) => {
  const point = eventPoint(event);
  draggedPoint = nearest(point);
  if (draggedPoint < 0) {
    if (points.length >= 250) return;
    points.push(point);
    draggedPoint = points.length - 1;
  } else {
    points[draggedPoint] = point;
  }
  canvas.setPointerCapture(event.pointerId);
  updateCount();
  draw();
});
canvas.addEventListener("pointermove", (event) => {
  if (draggedPoint < 0) return;
  points[draggedPoint] = eventPoint(event);
  draw();
});
canvas.addEventListener("pointerup", () => { draggedPoint = -1; });
canvas.addEventListener("pointercancel", () => { draggedPoint = -1; });
canvas.addEventListener("dblclick", (event) => {
  const index = nearest(eventPoint(event));
  if (index >= 0) points.splice(index, 1);
  draggedPoint = -1;
  updateCount();
  draw();
});
if (countInput) countInput.addEventListener("input", () => randomize(Number(countInput.value)));
document.querySelector("#randomize")?.addEventListener("click", () => randomize(Number(countInput.value)));
delauneyButton?.addEventListener("click", () => {
  showDelauney = !showDelauney;
  delauneyButton.setAttribute("aria-pressed", String(showDelauney));
  draw();
});
document.querySelector("#clear")?.addEventListener("click", () => {
  points = [];
  updateCount();
  draw();
});
if (pointForm) {
  pointForm.addEventListener("submit", (event) => {
    event.preventDefault();
    const x = Number(pointXInput.value);
    const y = Number(pointYInput.value);
    if (!Number.isFinite(x) || !Number.isFinite(y) || x < 0 || x > 1 || y < 0 || y > 1 || points.length >= 250) return;
    points.push({ x, y });
    updateCount();
    draw();
  });
}
if (copyButton) {
  copyButton.addEventListener("click", async () => {
    try {
      await copyJSON();
      copyButton.textContent = "Copied JSON";
      window.setTimeout(() => { copyButton.textContent = "Copy JSON"; }, 1600);
    } catch (error) {
      copyButton.textContent = "Copy failed";
      console.error(error);
    }
  });
}
if (openButton && openInput) {
  openButton.addEventListener("click", () => openInput.click());
  openInput.addEventListener("change", async () => {
    await openFile(openInput.files[0]);
    openInput.value = "";
  });
  let dragDepth = 0;
  playground.addEventListener("dragenter", (event) => {
    if (!Array.from(event.dataTransfer?.types || []).includes("Files")) return;
    event.preventDefault();
    dragDepth += 1;
    playground.classList.add("voronoi-drop-target");
  });
  playground.addEventListener("dragover", (event) => {
    if (!Array.from(event.dataTransfer?.types || []).includes("Files")) return;
    event.preventDefault();
    event.dataTransfer.dropEffect = "copy";
  });
  playground.addEventListener("dragleave", () => {
    dragDepth -= 1;
    if (dragDepth <= 0) {
      dragDepth = 0;
      playground.classList.remove("voronoi-drop-target");
    }
  });
  playground.addEventListener("drop", async (event) => {
    event.preventDefault();
    dragDepth = 0;
    playground.classList.remove("voronoi-drop-target");
    await openFile(event.dataTransfer?.files[0]);
  });
}
new ResizeObserver(resize).observe(canvas);
window.addEventListener("pagehide", () => diagram?.dispose());

let configuration;
let configurationError;
try {
  configuration = readConfiguration();
} catch (error) {
  configuration = { points: null, count: 42, seed: null, delauney: false };
  configurationError = error;
}

try {
  const { loadVoronoi } = await import("./vendor/voronoi.js");
  voronoi = await loadVoronoi();
  showDelauney = configuration.delauney;
  delauneyButton?.setAttribute("aria-pressed", String(showDelauney));
  const embeddedPoints = readPoints(canvas.dataset.points);
  if (embeddedPoints || configuration.points) {
    points = embeddedPoints || configuration.points;
    updateCount();
    draw();
  } else {
    const random = configuration.seed === null ? Math.random : seededRandom(configuration.seed);
    randomize(configuration.count, random);
  }
  resize();
  if (configurationError) {
    status.textContent = `URL error: ${configurationError.message}`;
  }
} catch (error) {
  status.textContent = "Wasm failed to load";
  console.error(error);
}

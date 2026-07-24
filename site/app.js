import { diagramToJSON } from "./diagram_json.js";

const canvas = document.querySelector("#diagram");
const context = canvas.getContext("2d");
const countInput = document.querySelector("#site-count");
const countOutput = document.querySelector("#site-count-output");
const status = document.querySelector("#status");
const delauneyButton = document.querySelector("#delauney");
const copyButton = document.querySelector("#copy-json");
let points = [];
let voronoi;
let diagram;
let draggedPoint = -1;
let showDelauney = false;

function randomize(count) {
  points = Array.from({ length: count }, () => ({ x: .035 + Math.random() * .93, y: .055 + Math.random() * .89 }));
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
    for (const edge of diagram.edges) {
      if (!edge.sites[0] || !edge.sites[1]) continue;
      context.moveTo(edge.sites[0].p.x, edge.sites[0].p.y);
      context.lineTo(edge.sites[1].p.x, edge.sites[1].p.y);
    }
    context.stroke();
  }
  context.strokeStyle = "#72796c";
  context.lineWidth = Math.max(1, window.devicePixelRatio || 1);
  context.beginPath();
  for (const edge of diagram.edges) {
    context.moveTo(edge.pos[0].x, edge.pos[0].y);
    context.lineTo(edge.pos[1].x, edge.pos[1].y);
  }
  context.stroke();
  const radius = 3.25 * (window.devicePixelRatio || 1);
  context.fillStyle = "#d8ff57";
  for (const point of scaled) {
    context.beginPath();
    context.arc(point.x, point.y, radius, 0, Math.PI * 2);
    context.fill();
  }
  const delauneyCount = showDelauney
    ? diagram.edges.filter((edge) => edge.sites[0] && edge.sites[1]).length
    : 0;
  const delauneyStatus = showDelauney ? ` · ${delauneyCount} Delauney` : "";
  status.textContent = `${diagram.numSites} sites · ${diagram.edges.length} edges${delauneyStatus}`;
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
  let distance = .001;
  points.forEach((candidate, index) => {
    const candidateDistance = (candidate.x - point.x) ** 2 + (candidate.y - point.y) ** 2;
    if (candidateDistance < distance) { best = index; distance = candidateDistance; }
  });
  return best;
}

canvas.addEventListener("pointerdown", (event) => {
  const point = eventPoint(event);
  draggedPoint = nearest(point);
  if (draggedPoint < 0) { points.push(point); draggedPoint = points.length - 1; }
  canvas.setPointerCapture(event.pointerId);
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
  draw();
});
countInput.addEventListener("input", () => {
  countOutput.value = countInput.value;
  randomize(Number(countInput.value));
});
document.querySelector("#randomize").addEventListener("click", () => randomize(Number(countInput.value)));
delauneyButton.addEventListener("click", () => {
  showDelauney = !showDelauney;
  delauneyButton.setAttribute("aria-pressed", String(showDelauney));
  draw();
});
document.querySelector("#clear").addEventListener("click", () => { points = []; draw(); });
copyButton.addEventListener("click", copyJSON);
new ResizeObserver(resize).observe(canvas);
window.addEventListener("pagehide", () => diagram?.dispose());

try {
  const { loadVoronoi } = await import("./vendor/voronoi.js");
  voronoi = await loadVoronoi();
  randomize(Number(countInput.value));
  resize();
} catch (error) {
  status.textContent = "Wasm failed to load";
  console.error(error);
}

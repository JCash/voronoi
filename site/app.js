const canvas = document.querySelector("#diagram");
const context = canvas.getContext("2d");
const countInput = document.querySelector("#site-count");
const countOutput = document.querySelector("#site-count-output");
const status = document.querySelector("#status");
const delauneyButton = document.querySelector("#delauney");
let points = [];
let voronoi;
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
  const edges = voronoi.edges(scaled, width, height);
  const delauneyEdges = showDelauney
    ? voronoi.delauneyEdges(scaled, width, height)
    : new Float32Array();
  context.fillStyle = "#1a1c19";
  context.fillRect(0, 0, width, height);
  if (showDelauney) {
    context.strokeStyle = "#ff9367aa";
    context.lineWidth = Math.max(1, window.devicePixelRatio || 1);
    context.beginPath();
    for (let i = 0; i < delauneyEdges.length; i += 4) {
      context.moveTo(delauneyEdges[i], delauneyEdges[i + 1]);
      context.lineTo(delauneyEdges[i + 2], delauneyEdges[i + 3]);
    }
    context.stroke();
  }
  context.strokeStyle = "#72796c";
  context.lineWidth = Math.max(1, window.devicePixelRatio || 1);
  context.beginPath();
  for (let i = 0; i < edges.length; i += 4) {
    context.moveTo(edges[i], edges[i + 1]);
    context.lineTo(edges[i + 2], edges[i + 3]);
  }
  context.stroke();
  const radius = 3.25 * (window.devicePixelRatio || 1);
  context.fillStyle = "#d8ff57";
  for (const point of scaled) {
    context.beginPath();
    context.arc(point.x, point.y, radius, 0, Math.PI * 2);
    context.fill();
  }
  const delauneyStatus = showDelauney ? ` · ${delauneyEdges.length / 4} Delauney` : "";
  status.textContent = `${points.length} sites · ${edges.length / 4} edges${delauneyStatus}`;
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
new ResizeObserver(resize).observe(canvas);

try {
  const { loadVoronoi } = await import("./vendor/voronoi.js");
  voronoi = await loadVoronoi();
  randomize(Number(countInput.value));
  resize();
} catch (error) {
  status.textContent = "Wasm failed to load";
  console.error(error);
}

# jc-voronoi

Fast Voronoi and Delaunay generation for JavaScript, powered by WebAssembly.
This package wraps the header-only
[`jc_voronoi`](https://github.com/JCash/voronoi) C implementation as an ES
module and includes TypeScript declarations.

## Install

```sh
npm install jc-voronoi
```

## Generate a diagram

```js
import { loadVoronoi } from "jc-voronoi";

const voronoi = await loadVoronoi();
const points = [
  { x: 10, y: 20 },
  { x: 80, y: 30 },
  { x: 40, y: 90 },
];

const diagram = voronoi.generate(points, {
  bounds: [0, 0, 100, 100],
});

for (const edge of diagram.edges) {
  console.log(edge.pos[0], edge.pos[1]);
}

const cell = diagram.cell(0);
if (cell) {
  console.log(cell.polygon);
  console.log(cell.neighbors);
}

// Optional: release the JavaScript-owned result buffer immediately.
diagram.dispose();
```

Points can also be supplied as a flat `Float32Array` containing x/y pairs.
Use `generate(points, width, height)` when the bounds start at `(0, 0)`.

## Get flat edge coordinates

If you only need edges, these methods avoid creating diagram objects and return
a `Float32Array` containing `x0, y0, x1, y1` for each edge:

```js
const voronoiEdges = voronoi.edges(points, 100, 100);
const delaunayEdges = voronoi.delaunayEdges(points, {
  bounds: [0, 0, 100, 100],
});
```

## Generate in a worker

For a large one-off diagram, use the included worker. It transfers the packed
result back and terminates before the promise resolves:

```js
import { loadVoronoiWorker } from "jc-voronoi";

const voronoi = await loadVoronoiWorker();
const diagram = await voronoi.generate(points, {
  bounds: [0, 0, 100, 100],
});
```

The worker API supports browsers and Node.js. See the
[project README](https://github.com/JCash/voronoi#webassembly) for more API
details, the interactive demo, and benchmarks.

## License

[MIT](https://github.com/JCash/voronoi/blob/master/LICENSE)

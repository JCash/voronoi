---
title: Generate and inspect a diagram
weight: 1
aliases:
  - /examples/webassembly/
  - /examples/c/webassembly/
---

Tagged releases include `jc_voronoi.wasm`, Emscripten's ES-module loader, and a small browser/Node.js wrapper.

```js
import { loadVoronoi } from "./voronoi.js";

const voronoi = await loadVoronoi();
const diagram = voronoi.generate(
  [{ x: 10, y: 20 }, { x: 80, y: 30 }, { x: 40, y: 90 }],
  100,
  100,
);

for (const edge of diagram.edges) {
  console.log(edge.pos[0], edge.pos[1]);
}

const cell = diagram.cell(0);
if (cell) {
  console.log(cell.site, cell.polygon, cell.edges, cell.neighbors);
}

diagram.dispose();
```

The diagram and its sites, edges, cells, neighbors, and polygon points are zero-copy views over one compact WebAssembly allocation. A cell is `null` when its input point was pruned.

Build the package locally with an installed Emscripten SDK:

```sh
./scripts/build_wasm.sh
```

The default output directory is `build/wasm`.

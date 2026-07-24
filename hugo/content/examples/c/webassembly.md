---
title: WebAssembly
weight: 8
aliases:
  - /examples/webassembly/
---

Tagged releases include `jc_voronoi.wasm`, Emscripten's ES-module loader, and a small browser/Node.js wrapper.

```js
import { loadVoronoi } from "./voronoi.js";

const voronoi = await loadVoronoi();
const edges = voronoi.edges(
  [{ x: 10, y: 20 }, { x: 80, y: 30 }, { x: 40, y: 90 }],
  100,
  100,
);
// Float32Array: x0, y0, x1, y1 for each edge
```

Build the package locally with an installed Emscripten SDK:

```sh
./scripts/build_wasm.sh
```

The default output directory is `build/wasm`.

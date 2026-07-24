---
title: JavaScript
weight: 2
---

The JavaScript API loads the WebAssembly module and exposes persistent diagram objects with sites, cells, edges, neighbors, and polygon points.

## Load the module

```js
import { loadVoronoi } from "./voronoi.js";

const voronoi = await loadVoronoi();
```

## `voronoi.generate(points, bounds)`

Generate a persistent diagram using either a width and height or explicit bounds:

```js
const diagram = voronoi.generate(points, 100, 100);

const boundedDiagram = voronoi.generate(points, {
  bounds: [minX, minY, maxX, maxY],
});
```

`points` may be an array of `{ x, y }` objects or a flat `Float32Array`. Call `diagram.dispose()` when the diagram is no longer needed.

## `Diagram`

| Member | Result |
|---|---|
| `bounds` | `[minX, minY, maxX, maxY]` |
| `inputCount` | Number of input points |
| `numSites` | Number of retained sites |
| `numVertices` | Number of unique vertices |
| `sites` | Retained `Site` objects |
| `edges` | All Voronoi `Edge` objects |
| `site(inputIndex)` | Input-order `Site`, or `null` when pruned |
| `cell(inputIndex)` | Input-order `Cell`, or `null` when pruned |
| `neighbors(inputIndex)` | Neighboring `Site` objects |
| `dispose()` | Releases the diagram's WebAssembly allocation |

## `Site`

| Member | Result |
|---|---|
| `p` | Site position as a `Point` |
| `index` | Original input index |
| `boundary` | Whether the site touches the clipping boundary |
| `cell` | The site's `Cell` |

## `Cell`

| Member | Result |
|---|---|
| `site` | The cell's `Site` |
| `edges` | Counter-clockwise cell edges |
| `neighbors` | Adjacent sites |
| `polygon` | Closed polygon of `Point` objects |

## `Edge`

| Member | Result |
|---|---|
| `sites` | Two adjacent sites; boundary sides may be `null` |
| `pos` | Two endpoint `Point` objects |
| `vertices` | Two unique vertex indices |

`Point` exposes `.x` and `.y` and can also be destructured as `[x, y]`.

## Compatibility helpers

`voronoi.edges(points, width, height)` and `voronoi.delauneyEdges(points, width, height)` return flat `Float32Array` coordinate pairs for applications that need the earlier bulk-output API.

See [Examples - JS](../../../examples/js/) for a complete example.

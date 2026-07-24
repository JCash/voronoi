---
title: JavaScript
weight: 2
---

The JavaScript API loads the WebAssembly module and exposes persistent diagram objects with sites, cells, edges, neighbors, and polygon points.

## API overview

### Types

| Type | Purpose |
|---|---|
| [`Voronoi`](#voronoi) | Loaded module used to generate diagrams |
| [`Diagram`](#diagram) | Persistent generated result and entry point for topology |
| [`Site`](#site) | Retained input point and cell metadata |
| [`Cell`](#cell) | One site's polygon, edges, and neighbors |
| [`Edge`](#edge) | Voronoi segment, adjacent sites, and vertex indices |
| [`Point`](#point) | Two-dimensional coordinate |

### Functions and methods

| Function | Result |
|---|---|
| [`loadVoronoi(options?)`](#load-the-module) | Loads and initializes the WebAssembly module |
| [`voronoi.generate(points, width, height)`](#voronoi) | Generates a persistent `Diagram` |
| [`voronoi.generate(points, options)`](#voronoi) | Generates a `Diagram` using explicit bounds or dimensions |
| [`voronoi.edges(points, width, height)`](#compatibility-helpers) | Returns flat Voronoi edge coordinates |
| [`voronoi.delauneyEdges(points, width, height)`](#compatibility-helpers) | Returns flat Delauney edge coordinates |
| [`diagram.site(inputIndex)`](#diagram) | Returns the retained `Site`, or `null` when pruned |
| [`diagram.cell(inputIndex)`](#diagram) | Returns the site's `Cell`, or `null` when pruned |
| [`diagram.neighbors(inputIndex)`](#diagram) | Returns neighboring `Site` objects |
| [`diagram.render(context)`](#diagram) | Adds all Voronoi segments to a path context without creating edge objects |
| [`diagram.renderDelauney(context)`](#diagram) | Adds all Delauney segments to a path context without creating edge objects |
| [`diagram.dispose()`](#diagram) | Eagerly releases the JavaScript-owned result buffer |

## Load the module

```js
import { loadVoronoi } from "./voronoi.js";

const voronoi = await loadVoronoi();
```

## `Voronoi`

Generate a persistent diagram using either a width and height or explicit bounds:

```js
const diagram = voronoi.generate(points, 100, 100);

const boundedDiagram = voronoi.generate(points, {
  bounds: [minX, minY, maxX, maxY],
});
```

`points` may be an array of `{ x, y }` objects or a flat `Float32Array`. Generation performs one bulk copy from WebAssembly into a compact JavaScript-owned `ArrayBuffer`; subsequent access does not call into WebAssembly.

## `Diagram`

| Member | Result |
|---|---|
| `bounds` | `[minX, minY, maxX, maxY]` |
| `inputCount` | Number of input points |
| `numSites` | Number of retained sites |
| `numVertices` | Number of unique vertices |
| `numEdges` | Number of Voronoi edges |
| `numDelauneyEdges` | Number of Delauney adjacency edges |
| `sites` | Retained `Site` objects |
| `edges` | All Voronoi `Edge` objects |
| `site(inputIndex)` | Input-order `Site`, or `null` when pruned |
| `cell(inputIndex)` | Input-order `Cell`, or `null` when pruned |
| `neighbors(inputIndex)` | Neighboring `Site` objects |
| `render(context)` | Adds Voronoi segments through `moveTo` and `lineTo` |
| `renderDelauney(context)` | Adds Delauney segments through `moveTo` and `lineTo` |
| `dispose()` | Eagerly releases the result buffer and invalidates the diagram |

The result buffer is garbage-collected normally; `dispose()` is optional.

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

## `Point`

`Point` exposes `.x` and `.y` and can also be destructured as `[x, y]`.

## Compatibility helpers

`voronoi.edges(points, width, height)` and `voronoi.delauneyEdges(points, width, height)` return flat `Float32Array` coordinate pairs for applications that need the earlier bulk-output API.

See [Examples - JS](../../../examples/js/) for a complete example.

---
title: JavaScript
weight: 2
---

The JavaScript API loads the WebAssembly module and exposes persistent diagram objects with sites, cells, edges, neighbors, and polygon points.

## API overview

### Types

<table class="api-summary"><tbody>
<tr><td><a href="#voronoi"><code>Voronoi</code></a></td><td>Loaded module used to generate diagrams.</td></tr>
<tr><td><a href="#worker-backed-generation"><code>VoronoiWorker</code></a></td><td>One-shot asynchronous generator that releases its WebAssembly runtime.</td></tr>
<tr><td><a href="#diagram"><code>Diagram</code></a></td><td>Persistent generated result and entry point for topology.</td></tr>
<tr><td><a href="#site"><code>Site</code></a></td><td>Retained input point and cell metadata.</td></tr>
<tr><td><a href="#cell"><code>Cell</code></a></td><td>One site's polygon, edges, and neighbors.</td></tr>
<tr><td><a href="#edge"><code>Edge</code></a></td><td>Voronoi segment, adjacent sites, and vertex indices.</td></tr>
<tr><td><a href="#point"><code>Point</code></a></td><td>Two-dimensional coordinate.</td></tr>
</tbody></table>

### Functions and methods

<table class="api-summary"><tbody>
<tr><td><a href="#load-the-module"><code>loadVoronoi(options?)</code></a></td><td>Load and initialize the WebAssembly module.</td></tr>
<tr><td><a href="#worker-backed-generation"><code>loadVoronoiWorker(options?)</code></a></td><td>Create an asynchronous worker-backed generator.</td></tr>
<tr><td><a href="#voronoi"><code>voronoi.generate(points, width, height)</code></a></td><td>Generate a persistent <code>Diagram</code>.</td></tr>
<tr><td><a href="#voronoi"><code>voronoi.generate(points, options)</code></a></td><td>Generate a <code>Diagram</code> using explicit bounds or dimensions.</td></tr>
<tr><td><a href="#worker-backed-generation"><code>workerVoronoi.generate(points, options)</code></a></td><td>Generate a <code>Diagram</code> in a one-shot worker.</td></tr>
<tr><td><a href="#compatibility-helpers"><code>voronoi.edges(points, width, height)</code></a></td><td>Return flat Voronoi edge coordinates.</td></tr>
<tr><td><a href="#compatibility-helpers"><code>voronoi.delauneyEdges(points, width, height)</code></a></td><td>Return flat Delauney edge coordinates.</td></tr>
<tr><td><a href="#compatibility-helpers"><code>voronoi.delauneyEdges(points, options)</code></a></td><td>Return flat Delauney edge coordinates using explicit bounds.</td></tr>
<tr><td><a href="#diagram"><code>diagram.site(inputIndex)</code></a></td><td>Return the retained <code>Site</code>, or <code>null</code> when pruned.</td></tr>
<tr><td><a href="#diagram"><code>diagram.cell(inputIndex)</code></a></td><td>Return the site's <code>Cell</code>, or <code>null</code> when pruned.</td></tr>
<tr><td><a href="#diagram"><code>diagram.neighbors(inputIndex)</code></a></td><td>Return neighboring <code>Site</code> objects.</td></tr>
<tr><td><a href="#diagram"><code>diagram.render(context)</code></a></td><td>Add all Voronoi segments to a path context without creating edge objects.</td></tr>
<tr><td><a href="#diagram"><code>diagram.renderDelauney(context)</code></a></td><td>Add all Delauney segments to a path context without creating edge objects.</td></tr>
<tr><td><a href="#diagram"><code>diagram.dispose()</code></a></td><td>Eagerly release the JavaScript-owned result buffer.</td></tr>
</tbody></table>

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

## Worker-backed generation

Use the asynchronous generator for large, one-off diagrams when retained memory
matters more than worker startup latency:

```js
import { loadVoronoiWorker } from "./voronoi.js";

const voronoi = await loadVoronoiWorker();
const diagram = await voronoi.generate(points, {
  bounds: [minX, minY, maxX, maxY],
});
```

Each call starts a module worker, generates and transfers the packed result, and
terminates the worker before resolving. The returned `Diagram` has the same API
as a synchronous result, but its WebAssembly heap is no longer retained. Input
`Float32Array` values are copied before transfer and are never detached.

## `Diagram`

| Member | Result |
|---|---|
| `bounds` | `[minX, minY, maxX, maxY]` |
| `inputCount` | Number of input points |
| `byteLength` | Exact size of the packed result buffer |
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

`voronoi.edges(points, width, height)` and `voronoi.delauneyEdges(points, width, height)` return flat `Float32Array` coordinate pairs for applications that need the earlier bulk-output API. `delauneyEdges` also accepts `{ bounds: [minX, minY, maxX, maxY] }` or `{ width, height }`.

See [Examples - JS](../../../examples/js/) for a complete example.

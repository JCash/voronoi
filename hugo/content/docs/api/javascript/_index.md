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
<tr><td><a href="#input-and-option-types"><code>PointInput</code></a></td><td>Object points or packed float coordinates accepted by generators.</td></tr>
<tr><td><a href="#input-and-option-types"><code>GenerateOptions</code></a></td><td>Diagram bounds or width and height.</td></tr>
<tr><td><a href="#input-and-option-types"><code>PathContext</code></a></td><td>Minimal drawing context used by render methods.</td></tr>
<tr><td><a href="#module-options"><code>VoronoiModuleOptions</code></a></td><td>WebAssembly module loading options.</td></tr>
<tr><td><a href="#worker-backed-generation"><code>VoronoiWorkerOptions</code></a></td><td>Worker script configuration.</td></tr>
</tbody></table>

### Functions and methods

<table class="api-summary"><tbody>
<tr><td><a href="#load-the-module"><code>loadVoronoi(options?)</code></a></td><td>Load and initialize the WebAssembly module.</td></tr>
<tr><td><a href="#worker-backed-generation"><code>loadVoronoiWorker(options?)</code></a></td><td>Create an asynchronous worker-backed generator.</td></tr>
<tr><td><a href="#voronoi"><code>voronoi.generate(points, width, height)</code></a></td><td>Generate a persistent <code>Diagram</code>.</td></tr>
<tr><td><a href="#voronoi"><code>voronoi.generate(points, options)</code></a></td><td>Generate a <code>Diagram</code> using explicit bounds or dimensions.</td></tr>
<tr><td><a href="#worker-backed-generation"><code>workerVoronoi.generate(points, options)</code></a></td><td>Generate a <code>Diagram</code> in a one-shot worker.</td></tr>
<tr><td><a href="#worker-backed-generation"><code>workerVoronoi.generate(points, width, height)</code></a></td><td>Generate a worker-backed <code>Diagram</code> using dimensions.</td></tr>
<tr><td><a href="#compatibility-helpers"><code>voronoi.edges(points, width, height)</code></a></td><td>Return flat Voronoi edge coordinates.</td></tr>
<tr><td><a href="#compatibility-helpers"><code>voronoi.delaunayEdges(points, width, height)</code></a></td><td>Return flat Delaunay edge coordinates.</td></tr>
<tr><td><a href="#compatibility-helpers"><code>voronoi.delaunayEdges(points, options)</code></a></td><td>Return flat Delaunay edge coordinates using explicit bounds.</td></tr>
<tr><td><a href="#diagram"><code>diagram.site(inputIndex)</code></a></td><td>Return the retained <code>Site</code>, or <code>null</code> when pruned.</td></tr>
<tr><td><a href="#diagram"><code>diagram.cell(inputIndex)</code></a></td><td>Return the site's <code>Cell</code>, or <code>null</code> when pruned.</td></tr>
<tr><td><a href="#diagram"><code>diagram.neighbors(inputIndex)</code></a></td><td>Return neighboring <code>Site</code> objects.</td></tr>
<tr><td><a href="#diagram"><code>diagram.render(context)</code></a></td><td>Add all Voronoi segments to a path context without creating edge objects.</td></tr>
<tr><td><a href="#diagram"><code>diagram.renderDelaunay(context)</code></a></td><td>Add all Delaunay segments to a path context without creating edge objects.</td></tr>
<tr><td><a href="#diagram"><code>diagram.dispose()</code></a></td><td>Eagerly release the JavaScript-owned result buffer.</td></tr>
<tr><td><a href="#point"><code>point.toJSON()</code></a></td><td>Return a plain <code>{ x, y }</code> object.</td></tr>
</tbody></table>

## Load the module

```js
import { loadVoronoi } from "./voronoi.js";

const voronoi = await loadVoronoi();
```

### Module options

`loadVoronoi(options)` accepts `VoronoiModuleOptions`:

| Member | Type | Purpose |
|---|---|---|
| `locateFile` | `(path, scriptDirectory) => string` | Override the URL used to load `jc_voronoi.wasm` or another module file |
| `wasmBinary` | `Uint8Array` | Provide the WebAssembly binary directly |
| `print` | `(...args) => void` | Override standard runtime output |
| `printErr` | `(...args) => void` | Override runtime error output |

Additional Emscripten module options are passed through unchanged.

## Input and option types

`PointInput` is either an array of ordinary `{ x, y }` objects or a flat
`Float32Array` containing `x, y` coordinate pairs. Input objects do not need to
implement the iterator exposed by generated `Point` objects.

`GenerateOptions` accepts one of these forms:

```js
{ bounds: [minX, minY, maxX, maxY] }
{ width: 100, height: 100 }
```

`PathContext` is the minimal drawing interface used by `render()` and
`renderDelaunay()`:

```ts
interface PathContext {
  moveTo(x: number, y: number): void;
  lineTo(x: number, y: number): void;
}
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

`loadVoronoiWorker({ workerUrl })` accepts a string or `URL` overriding the
default `voronoi.worker.js` module URL.

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
| `numDelaunayEdges` | Number of Delaunay adjacency edges |
| `sites` | Retained `Site` objects |
| `edges` | All Voronoi `Edge` objects |
| `site(inputIndex)` | Input-order `Site`, or `null` when pruned |
| `cell(inputIndex)` | Input-order `Cell`, or `null` when pruned |
| `neighbors(inputIndex)` | Neighboring `Site` objects |
| `render(context)` | Adds Voronoi segments through `moveTo` and `lineTo` |
| `renderDelaunay(context)` | Adds Delaunay segments through `moveTo` and `lineTo` |
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

`Point` exposes `.x` and `.y`, can be destructured as `[x, y]`, and provides
`toJSON()` to create a plain `{ x, y }` object.

## Compatibility helpers

`voronoi.edges(points, width, height)` and `voronoi.delaunayEdges(points, width, height)` return flat `Float32Array` coordinate pairs for applications that need the earlier bulk-output API. `delaunayEdges` also accepts `{ bounds: [minX, minY, maxX, maxY] }` or `{ width, height }`.

See [Examples - JS](../../../examples/js/) for a complete example.

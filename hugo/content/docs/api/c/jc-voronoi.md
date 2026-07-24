---
title: jc_voronoi.h
weight: 1
aliases:
  - /docs/api/jc-voronoi/
---

`jc_voronoi.h` contains the complete core API for generating Voronoi cells and
Delauney adjacency, then traversing the result without further allocation.

## Types

<table class="api-summary"><tbody>
<tr><td><a href="#jcv_real"><code>jcv_real</code></a></td><td>Configurable coordinate scalar.</td></tr>
<tr><td><a href="#jcv_point"><code>jcv_point</code></a></td><td>Two-dimensional coordinate.</td></tr>
<tr><td><a href="#jcv_rect"><code>jcv_rect</code></a></td><td>Axis-aligned bounding rectangle.</td></tr>
<tr><td><a href="#jcv_site"><code>jcv_site</code></a></td><td>Input point and its generated cell metadata.</td></tr>
<tr><td><a href="#jcv_edge"><code>jcv_edge</code></a></td><td>Clipped Voronoi edge value.</td></tr>
<tr><td><a href="#jcv_diagram"><code>jcv_diagram</code></a></td><td>Generated result and public counts and bounds.</td></tr>
<tr><td><a href="#jcv_edge_iter"><code>jcv_edge_iter</code></a></td><td>Iterator over diagram or site edges.</td></tr>
<tr><td><a href="#jcv_delauney_iter"><code>jcv_delauney_iter</code></a></td><td>Iterator over adjacent site pairs.</td></tr>
<tr><td><a href="#jcv_delauney_edge"><code>jcv_delauney_edge</code></a></td><td>One Delauney adjacency result.</td></tr>
<tr><td><a href="#fjcvallocfn-and-fjcvfreefn"><code>FJCVAllocFn</code>, <code>FJCVFreeFn</code></a></td><td>Custom allocation callbacks.</td></tr>
<tr><td><a href="#jcv_clipper"><code>jcv_clipper</code></a></td><td>Custom clipping callbacks and context.</td></tr>
<tr><td><a href="#jcv_context_internal"><code>jcv_context_internal</code></a></td><td>Opaque context passed to clipping callbacks.</td></tr>
</tbody></table>

## Functions

<table class="api-summary"><tbody>
<tr><td><a href="#jcv_diagram_generate"><code>jcv_diagram_generate</code></a></td><td>Generate a complete Voronoi diagram.</td></tr>
<tr><td><a href="#jcv_delauney_generate"><code>jcv_delauney_generate</code></a></td><td>Generate Delauney adjacency only.</td></tr>
<tr><td><a href="#jcv_diagram_generate_useralloc"><code>jcv_diagram_generate_useralloc</code></a></td><td>Generate using caller-provided allocation callbacks.</td></tr>
<tr><td><a href="#jcv_diagram_free"><code>jcv_diagram_free</code></a></td><td>Release a generated diagram.</td></tr>
<tr><td><a href="#jcv_diagram_get_sites"><code>jcv_diagram_get_sites</code></a></td><td>Access the diagram-owned site array.</td></tr>
<tr><td><a href="#jcv_get_num_vertices"><code>jcv_get_num_vertices</code></a></td><td>Get the unique vertex count.</td></tr>
<tr><td><a href="#jcv_diagram_get_vertices"><code>jcv_diagram_get_vertices</code></a></td><td>Copy unique vertices into caller storage.</td></tr>
<tr><td><a href="#jcv_diagram_get_edge_count"><code>jcv_diagram_get_edge_count</code></a></td><td>Get the Voronoi edge count.</td></tr>
<tr><td><a href="#jcv_diagram_get_edges"><code>jcv_diagram_get_edges</code></a></td><td>Begin iteration over all counter-clockwise Voronoi edges.</td></tr>
<tr><td><a href="#jcv_site_get_edges"><code>jcv_site_get_edges</code></a></td><td>Begin iteration around one cell.</td></tr>
<tr><td><a href="#jcv_edge_next"><code>jcv_edge_next</code></a></td><td>Advance a Voronoi edge iterator.</td></tr>
<tr><td><a href="#jcv_delauney_get_edge_count"><code>jcv_delauney_get_edge_count</code></a></td><td>Get the Delauney adjacency count.</td></tr>
<tr><td><a href="#jcv_delauney_begin"><code>jcv_delauney_begin</code></a></td><td>Begin Delauney adjacency iteration.</td></tr>
<tr><td><a href="#jcv_delauney_next"><code>jcv_delauney_next</code></a></td><td>Advance a Delauney iterator.</td></tr>
<tr><td><a href="#jcv_boxshape_test"><code>jcv_boxshape_test</code></a></td><td>Built-in rectangle point test.</td></tr>
<tr><td><a href="#jcv_boxshape_clip"><code>jcv_boxshape_clip</code></a></td><td>Built-in rectangle edge clipper.</td></tr>
<tr><td><a href="#jcv_boxshape_fillgaps"><code>jcv_boxshape_fillgaps</code></a></td><td>Close cells along a rectangle boundary.</td></tr>
</tbody></table>

## Include the implementation

Define `JC_VORONOI_IMPLEMENTATION` in exactly one C or C++ translation unit:

```c
#define JC_VORONOI_IMPLEMENTATION
#include "jc_voronoi.h"
```

Include `jc_voronoi.h` without the define everywhere else.

## Configuration

Define configuration macros before including the header, using the same values in
every translation unit.

| Define | Purpose | Default |
|---|---|---|
| `JCV_REAL_TYPE` | Coordinate and calculation type | `float` |
| `JCV_REAL_TYPE_EPSILON` | Epsilon used for scalar comparisons | `FLT_EPSILON` |
| `JCV_ATAN2` | Two-argument arctangent matching `JCV_REAL_TYPE` | `atan2f` |
| `JCV_SQRT` | Square root matching `JCV_REAL_TYPE` | `sqrtf` |
| `JCV_PI` | Pi constant matching `JCV_REAL_TYPE` | Single-precision pi |
| `JCV_FLT_MAX` | Largest supported coordinate magnitude | `3.402823466e+38F` |
| `JCV_EDGE_INTERSECT_THRESHOLD` | Near-parallel edge intersection threshold | `1.0e-10F` |

See the [double-precision example](../../../../examples/c/double-precision/) for the
complete set of overrides required when `JCV_REAL_TYPE` is `double`.

## Data types

### `jcv_real`

```c
typedef JCV_REAL_TYPE jcv_real;
```

The scalar used for coordinates and geometric calculations. It defaults to
`float` and can be configured before including the header.

### `jcv_point`

```c
typedef struct jcv_point_ {
    jcv_real x;
    jcv_real y;
} jcv_point;
```

A two-dimensional coordinate expressed in `jcv_real` values.

### `jcv_rect`

```c
typedef struct jcv_rect_ {
    jcv_point min;
    jcv_point max;
} jcv_rect;
```

`jcv_rect` is an axis-aligned bounding rectangle. Its `min` and `max` values are
also exposed as `diagram.min` and `diagram.max` after generation.

### `jcv_site`

```c
typedef struct jcv_site_ {
    jcv_point p;
    uint32_t index : 31;
    uint32_t boundary : 1;
} jcv_site;
```

| Member | Meaning |
|---|---|
| `p` | The surviving input point represented by this site. |
| `index` | Index of that point in the original input array. |
| `boundary` | Non-zero when the site's cell touches the clipping boundary. |

Generated sites are ordered for Fortune's sweep, not by input index. Duplicate
points, points outside an explicit rectangle, and points rejected by the clipper
do not produce sites. Use `site.index` to map a surviving site back to its input.

### `jcv_edge`

```c
typedef struct jcv_edge_ {
    jcv_site* sites[2];
    jcv_point pos[2];
    int vertices[2];
    jcv_real a;
    jcv_real b;
    jcv_real c;
} jcv_edge;
```

| Member | Meaning |
|---|---|
| `sites` | Sites separated by the edge. `sites[1]` is `NULL` for a clipping-boundary edge. |
| `pos` | Clipped line-segment endpoints. |
| `vertices` | Unique vertex indices corresponding to `pos[0]` and `pos[1]`. |
| `a`, `b`, `c` | Coefficients of the supporting line `a*x + b*y + c = 0`. |

During per-site iteration, `sites[0]` is always the requested site and edges are
oriented counter-clockwise around its cell. The iterator copies each result into
caller-owned `jcv_edge` storage; pointers within that value still refer to the
diagram.

### `jcv_diagram`

```c
typedef struct jcv_diagram_ {
    jcv_context_internal* internal;
    int numsites;
    int numvertices;
    jcv_point min;
    jcv_point max;
} jcv_diagram;
```

Read `numsites`, `numvertices`, `min`, and `max` after generation. `internal` is
opaque and must not be accessed or modified. Always zero-initialize a new diagram;
generation automatically releases an existing generated result when reusing the
same live `jcv_diagram`.

### `jcv_edge_iter`

```c
typedef struct jcv_edge_iter_ jcv_edge_iter;
```

Iterator state for Voronoi edges. Allocate it on the stack, initialize it with
`jcv_diagram_get_edges` or `jcv_site_get_edges`, and retrieve values with
`jcv_edge_next`. Do not access its members directly.

### `jcv_delauney_iter`

```c
typedef struct jcv_delauney_iter_ jcv_delauney_iter;
```

Iterator state for Delauney adjacency. Allocate it on the stack, initialize it
with `jcv_delauney_begin`, and retrieve values with `jcv_delauney_next`. Do not
access its members directly.

### `jcv_delauney_edge`

```c
typedef struct jcv_delauney_edge_ {
    jcv_edge edge;
    const jcv_site* sites[2];
    jcv_point pos[2];
} jcv_delauney_edge;
```

One pair of adjacent sites. `sites` points to the two diagram-owned sites and
`pos` contains their input positions. After `jcv_delauney_generate`, only
`sites` and `pos` are supported output; do not depend on `edge` geometry.

### `FJCVAllocFn` and `FJCVFreeFn`

```c
typedef void* (*FJCVAllocFn)(void* userctx, size_t size);
typedef void (*FJCVFreeFn)(void* userctx, void* p);
```

Allocation callbacks accepted by `jcv_diagram_generate_useralloc`. The generator
passes its `userallocctx` argument unchanged to both callbacks.

### `jcv_clipper`

```c
typedef struct jcv_clipper_ {
    jcv_clip_test_point_fn test_fn;
    jcv_clip_edge_fn clip_fn;
    jcv_clip_fillgap_fn fill_fn;
    jcv_point min;
    jcv_point max;
    void* ctx;
} jcv_clipper;
```

A custom clipping implementation and caller-defined context. `test_fn` accepts
points inside the final shape, `clip_fn` clips edge endpoints, and `fill_fn` adds
boundary edges that close each cell. The generator sets `min` and `max` to the
effective bounding rectangle and passes `ctx` through to every callback.

See [`jc_voronoi_clip.h`](../jc-voronoi-clip/) for the supplied convex-polygon
callbacks.

### `jcv_context_internal`

```c
typedef struct jcv_context_internal_ jcv_context_internal;
```

Opaque diagram context passed to `jcv_clip_fillgap_fn` and the supplied
`fill_fn` implementations. Client code must not access or modify it.

## Generate and release diagrams

### `jcv_diagram_generate`

```c
void jcv_diagram_generate(
    int num_points,
    const jcv_point* points,
    const jcv_rect* rect,
    const jcv_clipper* clipper,
    jcv_diagram* diagram);
```

Generates the complete clipped Voronoi diagram using `malloc` internally.

- `num_points` is the length of `points`.
- `points` is read during generation and remains owned by the caller.
- `rect` may be `NULL`; the library then calculates bounds and adds 10 units of
  padding.
- `clipper` may be `NULL`; the default box clipper uses the supplied or calculated
  rectangle.
- `diagram` must point to a zero-initialized diagram or a diagram containing a
  currently generated result.

Generation prunes duplicate points, points outside the rectangle, and points for
which the clipper's `test_fn` returns zero.

### `jcv_delauney_generate`

```c
void jcv_delauney_generate(
    int num_points,
    const jcv_point* points,
    const jcv_rect* rect,
    const jcv_clipper* clipper,
    jcv_diagram* diagram);
```

Generates only the Delauney adjacency used by `jcv_delauney_begin` and
`jcv_delauney_next`. This avoids constructing Voronoi edge geometry, per-site
edge lists, and unique vertices. `jcv_diagram_get_edge_count` and
`jcv_get_num_vertices` therefore return zero for this result.

The public API retains the historical spelling **Delauney** in its symbol names.

### `jcv_diagram_generate_useralloc`

```c
typedef void* (*FJCVAllocFn)(void* userctx, size_t size);
typedef void (*FJCVFreeFn)(void* userctx, void* p);

void jcv_diagram_generate_useralloc(
    int num_points,
    const jcv_point* points,
    const jcv_rect* rect,
    const jcv_clipper* clipper,
    void* userallocctx,
    FJCVAllocFn allocfn,
    FJCVFreeFn freefn,
    jcv_diagram* diagram);
```

Generates a complete Voronoi diagram like `jcv_diagram_generate`, but routes all
diagram allocations through `allocfn` and releases them through `freefn`.
`userallocctx` is passed unchanged to both callbacks. Both callbacks must remain
valid until the diagram is freed or regenerated.

### `jcv_diagram_free`

```c
void jcv_diagram_free(jcv_diagram* diagram);
```

Releases all internal storage with `free` or the custom free callback. Call it
exactly once for a generated diagram. All sites, edge-site pointers, iterators,
and other diagram-derived data become invalid.

## Access sites and vertices

### `jcv_diagram_get_sites`

```c
const jcv_site* jcv_diagram_get_sites(const jcv_diagram* diagram);
```

Returns a diagram-owned array containing `diagram->numsites` sites. The array is
sweep-ordered; use each site's `index` member to recover input order.

### `jcv_get_num_vertices`

```c
int jcv_get_num_vertices(const jcv_diagram* diagram);
```

Returns the number of unique endpoints in a complete Voronoi diagram.

### `jcv_diagram_get_vertices`

```c
void jcv_diagram_get_vertices(
    const jcv_diagram* diagram,
    jcv_point* vertices);
```

Writes every unique endpoint into caller-owned storage for at least
`jcv_get_num_vertices(diagram)` points. An edge's `vertices[n]` indexes the point
written for `edge.pos[n]`. This API is unavailable on a Delauney-only result.

## Traverse Voronoi edges

### `jcv_diagram_get_edge_count`

```c
int jcv_diagram_get_edge_count(const jcv_diagram* diagram);
```

Returns in constant time the number of non-degenerate edges yielded by an
iterator initialized with `jcv_diagram_get_edges`.

### `jcv_diagram_get_edges`

```c
void jcv_diagram_get_edges(
    const jcv_diagram* diagram,
    jcv_edge_iter* iter);
```

Initializes `iter` over every edge in the diagram. Use `jcv_edge_next` to retrieve
the edges; calling `jcv_diagram_get_edges` alone does not return an edge value.

Iteration yields each edge once. Each edge is oriented counter-clockwise around
`edge.sites[0]`: its endpoints run from `edge.pos[0]` to `edge.pos[1]` in that
direction.

### `jcv_site_get_edges`

```c
void jcv_site_get_edges(
    const jcv_diagram* diagram,
    const jcv_site* site,
    jcv_edge_iter* iter);
```

Initializes an iterator over one site's closed cell boundary. `site` must point
into the site array owned by `diagram`. Results are counter-clockwise and oriented
for that site.

### `jcv_edge_next`

```c
int jcv_edge_next(jcv_edge_iter* iter, jcv_edge* edge);
```

Copies the next edge into `edge` and returns non-zero. Returns zero at the end.
Iteration performs no allocation.

```c
jcv_edge_iter iter;
jcv_edge edge;
jcv_diagram_get_edges(&diagram, &iter);
while (jcv_edge_next(&iter, &edge)) {
    draw_line(edge.pos[0], edge.pos[1]);
}
```

## Traverse Delauney adjacency

### `jcv_delauney_get_edge_count`

```c
int jcv_delauney_get_edge_count(const jcv_diagram* diagram);
```

Returns in constant time the number of adjacency edges yielded by a Delauney
iterator.

### `jcv_delauney_begin`

```c
void jcv_delauney_begin(
    const jcv_diagram* diagram,
    jcv_delauney_iter* iter);
```

Initializes `iter` for either a complete Voronoi diagram or a Delauney-only
result. Retrieve adjacency values with `jcv_delauney_next`.

### `jcv_delauney_next`

```c
int jcv_delauney_next(
    jcv_delauney_iter* iter,
    jcv_delauney_edge* edge);
```

Copies the next adjacent site pair into `edge` and returns non-zero. Returns zero
at the end. See `jcv_delauney_edge` for the output member contract.

## Clipper callback types

```c
typedef int (*jcv_clip_test_point_fn)(
    const jcv_clipper* clipper, jcv_point point);
typedef int (*jcv_clip_edge_fn)(
    const jcv_clipper* clipper, jcv_edge* edge);
typedef void (*jcv_clip_fillgap_fn)(
    const jcv_clipper* clipper,
    jcv_context_internal* allocator,
    jcv_site* site);
```

| Callback | Contract |
|---|---|
| `test_fn` | Return non-zero when an input point is inside the final shape. May be `NULL` to skip shape-based point pruning. |
| `clip_fn` | Clip `edge->pos[0]` and `edge->pos[1]`; return non-zero when an edge remains. |
| `fill_fn` | Add boundary edges that close gaps in each site's clipped polygon. |

Supplying `NULL` for the entire clipper selects the built-in box callbacks.

## Built-in box clipper

### `jcv_boxshape_test`

```c
int jcv_boxshape_test(const jcv_clipper* clipper, jcv_point point);
```

Returns non-zero when `point` is inside the clipper's `min` and `max` bounds.

### `jcv_boxshape_clip`

```c
int jcv_boxshape_clip(const jcv_clipper* clipper, jcv_edge* edge);
```

Clips `edge` to the clipper's rectangular bounds and returns non-zero when a
segment remains.

### `jcv_boxshape_fillgaps`

```c
void jcv_boxshape_fillgaps(
    const jcv_clipper* clipper,
    jcv_context_internal* allocator,
    jcv_site* site);
```

Adds rectangle-boundary edges needed to close the site's cell.

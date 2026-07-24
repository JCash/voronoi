---
title: jc_voronoi_clip.h
weight: 2
aliases:
  - /docs/api/jc-voronoi-clip/
---

`jc_voronoi_clip.h` provides `jcv_clipper` callbacks that clip Voronoi cells to a
caller-supplied convex polygon.

## Types

<table class="api-summary"><tbody>
<tr><td><a href="#jcv_clipping_polygon"><code>jcv_clipping_polygon</code></a></td><td>Caller-owned convex polygon boundary.</td></tr>
</tbody></table>

## Functions

<table class="api-summary"><tbody>
<tr><td><a href="#jcv_clip_polygon_test_point"><code>jcv_clip_polygon_test_point</code></a></td><td>Test whether an input point is inside the polygon.</td></tr>
<tr><td><a href="#jcv_clip_polygon_clip_edge"><code>jcv_clip_polygon_clip_edge</code></a></td><td>Clip a Voronoi edge to the polygon.</td></tr>
<tr><td><a href="#jcv_clip_polygon_fill_gaps"><code>jcv_clip_polygon_fill_gaps</code></a></td><td>Close cells along the polygon boundary.</td></tr>
</tbody></table>

## Include the implementation

Define both implementation macros in exactly one translation unit:

```c
#define JC_VORONOI_IMPLEMENTATION
#include "jc_voronoi.h"

#define JC_VORONOI_CLIP_IMPLEMENTATION
#include "jc_voronoi_clip.h"
```

Other translation units can include `jc_voronoi_clip.h` without either define.
The clipping header includes `jc_voronoi.h` itself.

## `jcv_clipping_polygon`

```c
typedef struct jcv_clipping_polygon_ {
    jcv_point* points;
    int num_points;
} jcv_clipping_polygon;
```

`points` describes a convex polygon with counter-clockwise winding. Supply at
least three vertices in boundary order. The polygon and its point array remain
owned by the caller and must stay valid throughout diagram generation.

## Polygon clipper callbacks

These functions implement the three callback roles declared by
[`jc_voronoi.h`](../jc-voronoi/#clipper-callback-types). Applications normally
assign them to a `jcv_clipper`; they do not call them directly.

### `jcv_clip_polygon_test_point`

```c
int jcv_clip_polygon_test_point(
    const jcv_clipper* clipper,
    jcv_point point);
```

Returns non-zero when `point` is inside the convex polygon referenced by
`clipper->ctx`.

### `jcv_clip_polygon_clip_edge`

```c
int jcv_clip_polygon_clip_edge(
    const jcv_clipper* clipper,
    jcv_edge* edge);
```

Clips `edge` to the convex polygon and returns non-zero when a segment remains.

### `jcv_clip_polygon_fill_gaps`

```c
void jcv_clip_polygon_fill_gaps(
    const jcv_clipper* clipper,
    jcv_context_internal* allocator,
    jcv_site* site);
```

Adds polygon-boundary edges needed to close the site's cell.

## Complete setup

```c
jcv_point boundary[] = {
    { 50, 10 },
    { 90, 90 },
    { 10, 90 }
};

jcv_clipping_polygon polygon = {
    boundary,
    (int)(sizeof(boundary) / sizeof(boundary[0]))
};

jcv_clipper clipper = {0};
clipper.test_fn = jcv_clip_polygon_test_point;
clipper.clip_fn = jcv_clip_polygon_clip_edge;
clipper.fill_fn = jcv_clip_polygon_fill_gaps;
clipper.ctx = &polygon;

jcv_diagram diagram = {0};
jcv_diagram_generate(num_points, points, NULL, &clipper, &diagram);

/* Read clipped cells and edges here. */

jcv_diagram_free(&diagram);
```

The generator derives the clipper's preliminary `min` and `max` bounds, so callers
do not need to initialize those members. Passing an explicit `jcv_rect` still
prunes input points outside that rectangle before producing the polygon-clipped
result.

See the [custom clipping example](../../../../examples/c/custom-clipping/) for the
same setup in context.

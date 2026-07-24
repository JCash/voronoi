---
title: API
weight: 2
---

The public C API is split across two single-header files:

- [`jc_voronoi.h`](jc-voronoi/) generates and traverses Voronoi diagrams and Delauney adjacency.
- [`jc_voronoi_clip.h`](jc-voronoi-clip/) adds a ready-made clipper for convex polygons.

Both headers are C99-compatible and can be included from C++. They allocate no
caller-visible objects: callers provide input arrays and output structures, while
the generated diagram owns its internal storage until `jcv_diagram_free` is called.

## Typical lifecycle

1. Zero-initialize a `jcv_diagram`.
2. Generate a full Voronoi diagram with `jcv_diagram_generate`, or adjacency only
   with `jcv_delauney_generate`.
3. Read sites, vertices, and edges through the accessors and iterators.
4. Call `jcv_diagram_free` exactly once when finished.

```c
jcv_diagram diagram = {0};
jcv_diagram_generate(num_points, points, NULL, NULL, &diagram);

/* Read diagram data here. */

jcv_diagram_free(&diagram);
```

Pointers returned by the API and pointers stored in edge values belong to the
diagram. They become invalid when that diagram is regenerated or freed.

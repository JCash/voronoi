---
title: Custom clipping
weight: 6
aliases:
  - /examples/custom-clipping/
---

The optional `jc_voronoi_clip.h` header clips edges against a convex polygon.

```c
#define JC_VORONOI_CLIP_IMPLEMENTATION
#include "jc_voronoi_clip.h"

jcv_clipping_polygon polygon;
polygon.num_points = 3;
polygon.points = malloc(sizeof(jcv_point) * (size_t)polygon.num_points);

polygon.points[0].x = width / 2;
polygon.points[1].x = width - width / 5;
polygon.points[2].x = width / 5;
polygon.points[0].y = height / 5;
polygon.points[1].y = height - height / 5;
polygon.points[2].y = height - height / 5;

jcv_clipper clipper;
clipper.test_fn = jcv_clip_polygon_test_point;
clipper.clip_fn = jcv_clip_polygon_clip_edge;
clipper.fill_fn = jcv_clip_polygon_fill_gaps;
clipper.ctx = &polygon;

jcv_diagram diagram = {0};
jcv_diagram_generate(count, points, NULL, &clipper, &diagram);
```

The polygon and diagram storage remain owned by the caller. See `src/main.c` for a complete rendering example.

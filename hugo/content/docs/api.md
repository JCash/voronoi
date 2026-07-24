---
title: API
weight: 2
---

```c
void jcv_diagram_generate(int num_points, const jcv_point* points,
    const jcv_rect* rect, const jcv_clipper* clipper, jcv_diagram* diagram);
void jcv_delauney_generate(int num_points, const jcv_point* points,
    const jcv_rect* rect, const jcv_clipper* clipper, jcv_diagram* diagram);
void jcv_diagram_generate_useralloc(int num_points, const jcv_point* points,
    const jcv_rect* rect, const jcv_clipper* clipper, void* userallocctx,
    FJCVAllocFn allocfn, FJCVFreeFn freefn, jcv_diagram* diagram);
void jcv_diagram_free(jcv_diagram* diagram);

const jcv_site* jcv_diagram_get_sites(const jcv_diagram* diagram);
int jcv_get_num_vertices(const jcv_diagram* diagram);
void jcv_diagram_get_vertices(const jcv_diagram* diagram, jcv_point* vertices);
int jcv_diagram_get_edge_count(const jcv_diagram* diagram);
int jcv_delauney_get_edge_count(const jcv_diagram* diagram);
void jcv_diagram_get_edges(const jcv_diagram* diagram, jcv_edge_iter* iter);
void jcv_site_get_edges(const jcv_diagram* diagram, const jcv_site* site,
    jcv_edge_iter* iter);
int jcv_edge_next(jcv_edge_iter* iter, jcv_edge* edge);
```

Pass `NULL` for the bounding rectangle to calculate one automatically. Pass `NULL` for the clipper to use the default box clipper.

Input points are pruned when they are duplicates, outside the bounding box, or rejected by the clipper's test function.

See the [C examples](../../examples/c/) for complete generation and traversal patterns. The header remains the authoritative reference for structures and configuration macros.

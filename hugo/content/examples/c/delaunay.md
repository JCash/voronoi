---
title: Delaunay adjacency
weight: 4
aliases:
  - /examples/delauney/
  - /examples/c/delauney/
---

When only adjacency is needed, skip clipping and Voronoi cell topology.

```c
jcv_diagram diagram = {0};
jcv_delaunay_generate(num_points, points, NULL, NULL, &diagram);
int edge_count = jcv_delaunay_get_edge_count(&diagram);

jcv_delaunay_iter iter;
jcv_delaunay_begin(&diagram, &iter);
jcv_delaunay_edge edge;
while( jcv_delaunay_next(&iter, &edge) )
{
    // Use edge.sites and edge.pos here.
}

jcv_diagram_free(&diagram);
```

Sites and the Delaunay iterator are available on a Delaunay-only diagram. Voronoi edge geometry, per-site edges, and unique vertices are intentionally unavailable.

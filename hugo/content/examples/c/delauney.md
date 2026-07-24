---
title: Delauney adjacency
weight: 4
aliases:
  - /examples/delauney/
---

When only adjacency is needed, skip clipping and Voronoi cell topology.

```c
jcv_diagram diagram = {0};
jcv_delauney_generate(num_points, points, NULL, NULL, &diagram);
int edge_count = jcv_delauney_get_edge_count(&diagram);

jcv_delauney_iter iter;
jcv_delauney_begin(&diagram, &iter);
jcv_delauney_edge edge;
while( jcv_delauney_next(&iter, &edge) )
{
    // Use edge.sites and edge.pos here.
}

jcv_diagram_free(&diagram);
```

Sites and the Delauney iterator are available on a Delauney-only diagram. Voronoi edge geometry, per-site edges, and unique vertices are intentionally unavailable.

---
title: Relax points
weight: 5
aliases:
  - /examples/relaxation/
---

Move each site toward the average of its cell vertices for a simple relaxation pass.

```c
void relax_points(const jcv_diagram* diagram, jcv_point* points)
{
    const jcv_site* sites = jcv_diagram_get_sites(diagram);
    for( int i = 0; i < diagram->numsites; ++i )
    {
        const jcv_site* site = &sites[i];
        jcv_point sum = site->p;
        int count = 1;

        jcv_edge_iter iter;
        jcv_edge edge;
        jcv_site_get_edges(diagram, site, &iter);
        while( jcv_edge_next(&iter, &edge) )
        {
            sum.x += edge.pos[0].x;
            sum.y += edge.pos[0].y;
            ++count;
        }

        points[site->index].x = sum.x / count;
        points[site->index].y = sum.y / count;
    }
}
```

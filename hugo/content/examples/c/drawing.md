---
title: Draw edges and cells
weight: 9
aliases:
  - /examples/drawing/
---

Diagram iteration returns every Voronoi edge once. Site iteration returns each site's edges counter-clockwise, with endpoints oriented around the cell.

```c
void draw_edges(const jcv_diagram* diagram)
{
    jcv_edge_iter iter;
    jcv_edge edge;
    jcv_diagram_get_edges(diagram, &iter);
    while( jcv_edge_next(&iter, &edge) )
        draw_line(edge.pos[0], edge.pos[1]);
}

void draw_cells(const jcv_diagram* diagram)
{
    const jcv_site* sites = jcv_diagram_get_sites(diagram);
    for( int i = 0; i < diagram->numsites; ++i )
    {
        const jcv_site* site = &sites[i];
        jcv_edge_iter iter;
        jcv_edge edge;
        jcv_site_get_edges(diagram, site, &iter);
        while( jcv_edge_next(&iter, &edge) )
            draw_triangle(site->p, edge.pos[0], edge.pos[1]);
    }
}
```

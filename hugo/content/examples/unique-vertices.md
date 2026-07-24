---
title: Unique vertices
weight: 3
---

Each edge endpoint has a contiguous vertex index, so clients can create an indexed list without comparing floating-point coordinates.

```c
jcv_point* vertices = malloc(
    (size_t)jcv_get_num_vertices(&diagram) * sizeof(*vertices));
jcv_diagram_get_vertices(&diagram, vertices);

jcv_edge_iter iter;
jcv_edge edge;
jcv_diagram_get_edges(&diagram, &iter);
while( jcv_edge_next(&iter, &edge) )
    add_indexed_edge(edge.vertices[0], edge.vertices[1]);
```

To map each vertex to its connected sites, walk every site's ordered loop:

```c
int num_vertices = jcv_get_num_vertices(&diagram);
allocate_vertex_site_storage(num_vertices);

const jcv_site* sites = jcv_diagram_get_sites(&diagram);
for( int i = 0; i < diagram.numsites; ++i )
{
    const jcv_site* site = &sites[i];
    jcv_edge_iter iter;
    jcv_edge edge;
    jcv_site_get_edges(&diagram, site, &iter);
    while( jcv_edge_next(&iter, &edge) )
        link_vertex_site(edge.vertices[0], site);
}
```

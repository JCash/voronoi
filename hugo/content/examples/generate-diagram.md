---
title: Generate a diagram
weight: 1
toc: false
---

{{< example-diagram points="0.08,0.1;0.92,0.1;0.5,0.9" >}}
Create a zero-initialized diagram, generate it from an array of points, iterate through its sites and edges, then release its storage.

```c
jcv_point points[] = {
    { 0, 0 },
    { 100, 0 },
    { 50, 100 }
};

jcv_diagram diagram = {0};
jcv_diagram_generate(3, points, NULL, NULL, &diagram);

const jcv_site* sites = jcv_diagram_get_sites(&diagram);
for( int i = 0; i < diagram.numsites; ++i )
{
    const jcv_site* site = &sites[i];
    jcv_edge_iter iter;
    jcv_edge edge;
    jcv_site_get_edges(&diagram, site, &iter);
    while( jcv_edge_next(&iter, &edge) )
    {
        // Use site and edge here.
    }
}

jcv_diagram_free(&diagram);
```
{{< /example-diagram >}}

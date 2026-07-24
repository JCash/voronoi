---
title: Sites in input order
weight: 2
aliases:
  - /examples/site-order/
---

Sites are sorted for the sweep algorithm. Each site's `index` points back to its original input point, so create a reverse lookup when input order matters.

```c
const jcv_site* sites = jcv_diagram_get_sites(&diagram);
const jcv_site** sites_by_input = calloc(
    (size_t)num_points, sizeof(*sites_by_input));

for( int i = 0; i < diagram.numsites; ++i )
    sites_by_input[sites[i].index] = &sites[i];

const jcv_site* site = sites_by_input[input_index];
```

Allocate the lookup for the original point count. Entries can remain `NULL` for points pruned as duplicates, outside the bounds, or by the clipper. Site pointers remain valid until `jcv_diagram_free`; free the lookup separately.

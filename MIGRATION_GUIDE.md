# Migration Guide

## 0.11.0

This release corrects the historical misspelling of **Delaunay** throughout the
C and JavaScript APIs. The misspelled names are removed rather than retained as
aliases, so callers must update them when upgrading.

### C API

Replace the public C names as follows:

| 0.10 name | 0.11 name |
|---|---|
| `jcv_delauney_edge` | `jcv_delaunay_edge` |
| `jcv_delauney_iter` | `jcv_delaunay_iter` |
| `JCV_OPTION_DELAUNEY_ONLY` | `JCV_OPTION_DELAUNAY_ONLY` |
| `jcv_delauney_generate()` | `jcv_delaunay_generate()` |
| `jcv_delauney_get_edge_count()` | `jcv_delaunay_get_edge_count()` |
| `jcv_delauney_begin()` | `jcv_delaunay_begin()` |
| `jcv_delauney_next()` | `jcv_delaunay_next()` |
| `jcv_delauney_iter.delauney_only` | `jcv_delaunay_iter.delaunay_only` |

Recompile every translation unit that includes `jc_voronoi.h` after making
these changes.

### JavaScript API

Replace the JavaScript and TypeScript members as follows:

| 0.10 name | 0.11 name |
|---|---|
| `voronoi.delauneyEdges()` | `voronoi.delaunayEdges()` |
| `diagram.numDelauneyEdges` | `diagram.numDelaunayEdges` |
| `diagram.renderDelauney()` | `diagram.renderDelaunay()` |

Serialized diagram objects now use `delaunayEdges` instead of
`delauneyEdges`. The interactive demo query parameter is likewise renamed from
`delauney` to `delaunay`.

## 0.10.0

This release introduces some breaking API changes.

### Edge iteration

Edges are now returned by value through `jcv_edge_iter`. Replace linked-list
iteration:

```c
const jcv_edge* edge = jcv_diagram_get_edges(&diagram);
while (edge) {
    use(edge);
    edge = jcv_diagram_get_next_edge(edge);
}
```

with:

```c
jcv_edge_iter iter;
jcv_edge edge;
jcv_diagram_get_edges(&diagram, &iter);
while (jcv_edge_next(&iter, &edge)) {
    use(&edge);
}
```

`jcv_diagram_get_next_edge` and `jcv_edge.next` were removed. The value written
by `jcv_edge_next` is caller-owned; reusing the output variable overwrites it.

### Site edges

`jcv_graphedge` and `jcv_site.edges` were removed. Use the same iterator API:

```c
jcv_edge_iter iter;
jcv_edge edge;
jcv_site_get_edges(&diagram, site, &iter);
while (jcv_edge_next(&iter, &edge)) {
    use(&edge);
}
```

Site edges remain counter-clockwise. `edge.sites[0]` is the requested site and
`edge.sites[1]` replaces `jcv_graphedge.neighbor`.

Custom clipper `fill_fn` implementations that accessed `jcv_site.edges` must be
updated; use `src/jc_voronoi_clip.h` as the reference implementation.

### Delaunay edges

`jcv_delauney_edge.edge` is now an embedded `jcv_edge`, not a pointer. Replace
`delauney_edge.edge->...` with `delauney_edge.edge....`; the edge belongs to
the caller-owned `jcv_delauney_edge` value.

### Site order

Do not assume `jcv_diagram_get_sites()` follows input order. Use
`site.index` to map a site back to its input point. Pruned input points have no
site.

### Struct layout

Public structs are no longer packed and several layouts changed. Rebuild all
code that uses the header; do not mix 0.9 and 0.10 objects across an ABI
boundary.

`JCV_DISABLE_STRUCT_PACKING` was removed because struct packing is no longer
used. Remove this define from build configurations; defining it has no effect
in 0.10.

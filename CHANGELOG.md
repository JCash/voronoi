# Changelog

All notable changes are collected from the project's GitHub releases.

## [0.10.0](https://github.com/JCash/voronoi/releases/tag/v0.10.0) - 2026-07-22

- Replaced the beachline scan with a RAVL tree, eliminating pathological scaling. The 100k pathological case improves from approximately 9.15 s to 19.43 ms.
- Replaced persistent graph-edge copies with shared edges and per-site iterators, substantially reducing retained memory.
- Added packed unique-vertex storage and the `jcv_get_num_vertices()` / `jcv_diagram_get_vertices()` APIs.
- Optimized edge sorting with an overflow-safe pseudo-angle.
- Skip gap filling for interior cells when using the default box clipper.
- Improved handling of nearly collinear sites.
- Added more regression tests, documentation, examples, and showcases.

### Breaking changes

- Edge traversal now uses `jcv_edge_iter` and caller-owned `jcv_edge` values.
- Removed `jcv_diagram_get_next_edge`, `jcv_edge.next`, `jcv_graphedge`, and `jcv_site.edges`.
- Site edges now use `jcv_site_get_edges()` and `jcv_edge_next()`.
- `jcv_delauney_edge.edge` is now embedded rather than a pointer.
- Site array order is not guaranteed to match input order; use `site.index`.
- Public structs are no longer packed and their layouts have changed.
- Removed `JCV_DISABLE_STRUCT_PACKING`; defining it now has no effect.
- Custom clipper `fill_fn` implementations using `jcv_site.edges` must be updated.

## [0.9.0](https://github.com/JCash/voronoi/releases/tag/v0.9.0) - 2023-01-22

Modified jcv_delauney_begin api

## [0.8.0](https://github.com/JCash/voronoi/releases/tag/v0.8.0) - 2022-12-25

- Added iterator for Delaunay triangles
- Fix for missing border edges
- Fix for inserting duplicate edges

## [0.7.0](https://github.com/JCash/voronoi/releases/tag/v0.7.0) - 2019-11-02

- Added support for clipping against convex polygons
- Added JCV_EDGE_INTERSECT_THRESHOLD for edge intersections
- Fixed issue where the bounds calculation wasn’t considering all points

## [0.6.0](https://github.com/JCash/voronoi/releases/tag/v0.6.0) - 2018-10-21

Removed JCV_FABS/JCV_CEIL/JCV_FLOOR in favor of internal implementations
Performance optimisations

## [0.5.0](https://github.com/JCash/voronoi/releases/tag/v0.5.0) - 2018-10-14

- Fixed issue where the graph edge had the wrong edge assigned (issue #28)
- Fixed issue where a point was falsely passing the jcv_is_valid() test (issue #22)
- Fixed jcv_diagram_get_edges() so it now returns _all_ edges (issue #28)
- Added jcv_diagram_get_next_edge() to skip zero length edges (issue #10)
- Added defines JCV_CEIL/JCV_FLOOR/JCV_FLT_MAX for easier configuration

## [0.4.0](https://github.com/JCash/voronoi/releases/tag/v0.4.0) - 2018-06-03

jc_voronoi v0.4.0

## [0.3.0](https://github.com/JCash/voronoi/releases/tag/v0.3.0) - 2017-04-20

- Added clipping box as input argument (Automatically calculated if needed)
- Input points are pruned based on bounding box

## [0.2.0](https://github.com/JCash/voronoi/releases/tag/v0.2.0) - 2017-04-20

jc_voronoi v0.2.0

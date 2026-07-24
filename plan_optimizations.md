# Voronoi Optimization Plan

The main optimization opportunity is the output representation rather than Fortune's sweep algorithm itself. The sweep already has the expected `O(n log n)` structure, using a heap for circle events and a relaxed AVL (RAVL) tree for the beachline.

Local baseline using random float sites and `clang++ -O3`:

| Sites | Median time | Allocated memory | Allocations |
|---:|---:|---:|---:|
| 300k | 191 ms | 145.5 MB | 8,295 |
| 1m | 659 ms | 473.1 MB | 26,922 |

## Starting point

Sections 2 and 3 were selected for evaluation:

1. Introduce a compact, array-based cell representation.
2. Delay graph-edge construction until after the sweep and build the arrays contiguously.

Section 3 is retained. The first section 2 prototype was reverted, but a narrower
version was later adopted: geometry remains in one shared `jcv_edge`, while a
compact CSR-style array stores only sorted edge pointers for each site.

## WebAssembly

The WebAssembly comparison needs to distinguish core diagram construction from
the amount of output each library materializes during that call. JCV eagerly
clips every Voronoi edge, creates the per-site topology, sorts cell edges, fills
boundary gaps, and finalizes its output arrays. In contrast, d3-delaunay defers
some clipped Voronoi work until its output is requested. This is why JCV can be
slower in the generation-only benchmark while being faster when all Voronoi
edges are retrieved.

The first optimization is now implemented. `jcv_delauney_generate()` and
`jcv_diagram_generate()` share a flag-based `jcv_diagram_generate_internal()`.
`JCV_OPTION_DELAUNEY_ONLY` skips clipping, graph-edge construction, boundary-gap
filling, per-site edge finalization, and the temporary per-site topology. In the
100k Wasm workflow, generation plus complete Delauney iteration dropped from
about 85 ms to 69 ms; the pathological case dropped from about 38 ms to 30 ms.
The Delauney-only circle-event path also skips Voronoi endpoint and unique-vertex
writes, reducing the 100k Wasm result further to about 68 ms.
`jcv_delauney_get_edge_count()` exposes the maintained edge counter so callers do
not need a counting traversal before allocating output.

The generic `qsort` has also been replaced by an allocation-free, inlinable
introsort with three-way partitioning, small-range insertion sort, heapsort
fallback, and sorted/reverse-sorted fast paths. The latest 100k Wasm results are
about 66 ms for the full Voronoi workflow and 48 ms for Delauney-only generation
plus iteration, down from approximately 85 ms and 68 ms respectively.

The priority queue is now specialized for `jcv_halfedge`. Typed heap storage and
direct inlined comparisons/position updates replace all three callbacks. This
reduces the latest 100k Wasm results to about 61 ms for full Voronoi generation
and 44 ms for Delauney-only generation plus iteration.

The remaining WebAssembly optimization priorities are:

1. Replace angle-sorted linked-list insertion for per-site graph edges with
   contiguous site ranges. Populate and sort those small ranges directly so the
   topology is not written once as linked nodes and again as final references.
2. Reduce fixed 16 KiB arena-block churn by sizing from the site count or growing
   blocks geometrically. This is a lower native CPU priority but may matter more
   when WebAssembly memory has to grow.
3. Optimize rectangle clipping only after the preceding work. Native sampling
   attributed much less time to clipping than to graph-edge construction,
   sorting, and sweep data structures.

A native 100k-site sampling profile attributed roughly 17% of samples to graph-
edge creation and sorted insertion, 10-12% to site sorting, and another sizable
share to beach-line and priority-queue maintenance. These percentages guide the
order above, but phase timers should be added to a profiling Wasm build before
micro-optimizing the queue, allocator, or clipping code.

The benchmark should retain generation-only and generation-plus-access tables,
but the performance charts should focus on complete public output workflows:
**Voronoi diagram** and **Delauney diagram**.

## 1. Add output-generation flags

Every completed Voronoi edge currently creates two `jcv_graphedge` objects, even if the caller only needs global or Delaunay edges.

On a 64-bit float build, the approximate packed sizes are:

- `jcv_edge`: 52 bytes
- `jcv_graphedge`: 44 bytes
- Two graph edges per Voronoi edge: 88 bytes
- Roughly three Voronoi edges per site for a typical planar diagram

At one million sites, graph-edge records therefore account for approximately 264 MB of the measured 473 MB.

Possible generation flags:

- `JCV_OUTPUT_EDGES`
- `JCV_OUTPUT_CELLS`
- `JCV_OUTPUT_CLOSED_CELLS`
- `JCV_OUTPUT_EDGE_ANGLES`

An edge-only mode could reduce memory by roughly 50-55% while skipping graph allocation, angular sorting, and gap filling.

This remains a possible future optimization for callers that do not need cells. It is separate from the retained section 3 work.

## 2. Add a compact, array-based cell representation

This is one of the preferred starting optimizations.

The current `jcv_graphedge` stores:

- A linked-list pointer
- An edge pointer
- A neighbor pointer
- Two endpoint positions copied from the global edge
- A sorting angle

The copied endpoints duplicate data already available in `jcv_edge`. The linked representation also scatters the edges belonging to a cell across the allocation arena.

### Compact representation

A minimally disruptive compact graph edge could use indices:

```c
typedef struct jcv_compact_graphedge_
{
    uint32_t edge;
    int32_t neighbor;
    uint8_t pos[2];
} jcv_compact_graphedge;
```

A more cache-friendly CSR-style representation would give each site a range in a shared graph-edge array:

```c
typedef struct jcv_compact_site_
{
    jcv_point p;
    int index;
    uint32_t first_edge;
    uint32_t num_edges;
} jcv_compact_site;
```

```text
sites[i].first_edge + sites[i].num_edges
                    |
                    v
compact_graphedges[] -> global edges[]
```

Expected benefits:

- 10 packed bytes instead of 44 bytes per graph edge.
- Consecutive edges for a cell are consecutive in memory.
- No linked-list pointer chasing during traversal.
- Faster drawing, triangulation, and Lloyd relaxation.
- Approximately 190 MB potential saving at one million sites.
- 32-bit indices reduce pointer storage while still allowing very large diagrams.

### First prototype decision: rejected; narrower design adopted

The compact representation was tested as an ABI-breaking replacement, together with an iterator-based `jcv_diagram_get_edges()` API. Keeping both old and new representations was deliberately avoided because it would retain most of the memory cost.

That prototype was reverted. The adopted design does not compact or duplicate
geometry. It keeps the global edge list used by the sweep, builds temporary
24-byte naturally aligned sorting incidences, then stores one pointer per final site incidence.
The public linked structures are replaced by an iterator that fills a
caller-owned, correctly oriented `jcv_edge`.

## 3. Delay graph-edge construction until after the sweep

This is the retained optimization. It works independently of section 2 and preserves the public API and record layout.

Graph edges are currently allocated and inserted into per-site linked lists whenever a geometric edge finishes during the sweep. As a result, edges belonging to one cell are distributed across many allocation blocks.

### Proposed phases

1. Run the Fortune sweep and retain the completed global edges.
2. Clip all completed edges.
3. Count the graph edges required by every site.
4. Prefix-sum the site counts to determine array ranges.
5. Allocate one exactly sized graph-edge array.
6. Populate each site's contiguous range.
7. Sort or chain the small range belonging to each site.
8. Fill boundary gaps into reserved or separately counted ranges.

This separates the sequential sweep from output construction:

```text
Input sites
    |
    v
Fortune sweep -- sequential, mutable beachline/events
    |
    v
Completed global edges
    |
    +--> clip and count per site
    |
    +--> prefix sum
    |
    v
Contiguous per-site graph-edge ranges
```

Expected benefits:

- Graph edges can be allocated exactly rather than through the general arena.
- Cell traversal becomes sequential in memory.
- Per-cell linked-list insertion is removed.
- Duplicate detection can operate on small contiguous ranges.
- Most post-processing becomes parallelizable by site or edge.
- Typed allocation separates long-lived output from transient sweep structures.
- It becomes easier to free or reuse transient half-edge storage independently.

### Boundary-gap handling

The current box clipper can introduce extra graph edges while closing cells. Exact allocation therefore requires either:

- A counting pass that predicts all required gap edges, or
- A first pass that creates compact gap descriptors, followed by exact allocation, or
- Conservative per-site capacity with a final compacting pass.

A counting pass is preferable if it can share the exact same topology decisions as the construction pass. Divergent count and fill logic would be a maintenance and correctness risk.

### Sorting cell edges

Possible strategies after ranges are contiguous:

1. Keep the existing angular ordering for the first version.
2. Use a quadrant-aware pseudo-angle instead of `atan2`.
3. Chain edges by matching the end of one edge to the beginning of the next.
4. Derive ordering directly from sweep topology where robust.

The first implementation should prioritize identical output and robustness. Angular calculation can be optimized separately after the representation is verified.

### Suggested first prototype

Build an internal, post-sweep contiguous representation while leaving the public API unchanged:

- Keep global `jcv_edge` behavior unchanged.
- Count per-site graph edges after the sweep.
- Allocate graph edges grouped by site.
- Link adjacent array entries through the existing `next` field.
- Preserve copied endpoints, neighbor pointers, and angles initially.

This prototype isolates the cache-coherency effect from API and record-size changes. The results justify retaining this version without proceeding to the rejected compact indexed form from section 2.

## 4. Replace `atan2` sorting with a cheaper angular key

Every graph edge currently computes an `atan2f` value for insertion into its site's sorted list.

A test using a quadrant-aware pseudo-angle based on:

```c
dy / (abs(dx) + abs(dy))
```

reduced the 300k-site median from 191 ms to 184 ms, approximately 3-4%.

This changes the numerical meaning of the public `angle` member. It should therefore be opt-in, internal to the compact representation, or used only when true angles were not requested.

Contiguous post-sweep construction may also permit topological chaining without any angle calculation.

## 5. Specialize the priority queue

Implemented. The internal heap now stores `jcv_halfedge*` directly and uses the
fixed event ordering and `pqpos` field without callbacks.

The specialization removed:

```c
pq->compare_fn(...)
pq->set_pos(...)
pq->get_pos(...)
```

Heap movement functions are inlinable, and the event buffer is typed as
`jcv_halfedge**` throughout. At 100k sites this reduced full Voronoi generation
from about 66 ms to 61 ms and Delauney-only generation plus iteration from about
48 ms to 44 ms in Wasm.

## 6. Replace generic `qsort`

Implemented. Input sorting now uses a specialized in-place introsort over
`jcv_site` records.

The implementation provides:

- An inlined strict `(y, x)` ordering that correctly treats identical points as
  equivalent; the old `qsort` comparator incorrectly returned `1` for equality.
- Three-way partitioning for duplicate-heavy inputs.
- Insertion sort for ranges of 20 sites or fewer.
- A heapsort fallback that preserves worst-case `O(n log n)` behavior.
- Linear detection of already sorted and reverse-sorted input.
- No allocations and bounded recursion by always recursing into the smaller
  partition first.

An explicit API for already-sorted input may still help callers that already
maintain spatial ordering, although the current sorted-input scan is already
linear.

## 7. Add reusable workspace support

Repeated generation currently releases and reallocates sites, the event heap, and arena blocks.

A reusable `jcv_workspace` could retain:

- Site storage
- Event heap capacity
- Half-edge pool
- Edge arena
- Graph-edge arena

This would reduce page allocation and page faults, and should make repeated Lloyd-relaxation runs more consistent.

## 8. Arena sizing

Increasing the arena block size from 16 KB to 256 KB reduced the 300k-site allocation count from 8,295 to 518, but did not improve runtime; median time remained approximately 192 ms.

Allocator call count is therefore not the main speed bottleneck. The block size should still be configurable for custom allocators and very large diagrams, but this is a lower-priority change.

## 9. Structure packing

Disabling structure packing increased memory by only about 0.9%, but produced no repeatable speed improvement at one million sites.

Structure packing has therefore been removed. All public and internal structures now use their platform's natural alignment. Typed contiguous arrays and compact indices should provide much larger cache benefits.

## 10. Other smaller memory opportunities

- The event queue reserves `2*n` pointers. Using 32-bit half-edge handles could save about 8 MB per million sites.
- One of `jcv_edge.a` or `jcv_edge.b` is always exactly `1`. A slope plus orientation bit could save roughly 4 bytes per edge, or about 12 MB per million sites.
- Separate typed arenas for global edges, graph edges, and transient half edges would improve locality even without changing the public structures.
- Reclaiming transient beachline and event memory independently would reduce retained memory after generation.

Avoid enabling `-ffast-math` as a general optimization. Near-collinear and degenerate geometry depends on predictable floating-point behavior, and relaxed semantics could compromise robustness.

## Recommended implementation order

1. Prototype delayed, per-site contiguous graph-edge construction while preserving the existing API and record layout.
2. Benchmark generation and, separately, full cell traversal and relaxation.
3. Treat the compact indexed/CSR representation as rejected unless a future use case changes the tradeoff.
4. Add output-generation flags, including an edge-only mode.
5. Specialize the event priority queue and input sort.
6. Evaluate pseudo-angle or topology-based cell ordering.
7. Add reusable workspace support.
8. Make arena sizing configurable.

## Validation requirements

Each representation change should be checked against:

- The complete existing test suite in float and double modes.
- Near-collinear and duplicate-point regression cases.
- Custom polygon clipping.
- Closed and consistently oriented cell polygons.
- Neighbor symmetry across shared edges.
- Delaunay iteration results.
- Memory usage, allocation count, generation time, and traversal time.
- Large random, regular-grid, skewed, and adversarial point sets.

Generation benchmarks alone are insufficient for sections 2 and 3. Their primary benefit is expected to appear in retained memory and downstream cell traversal, so drawing and Lloyd-relaxation benchmarks should be added as first-class measurements.

## Section 3 prototype results

The first delayed-construction prototype has been implemented while preserving the existing public structures and linked-list traversal API.

The implementation now:

- Runs the sweep without clipping edges or allocating graph edges.
- Reuses the inactive event-queue buffer for per-site counts and write cursors.
- Clips and counts completed global edges after the sweep.
- Allocates one exactly sized array for non-boundary graph edges.
- Assigns each site a contiguous range within that array.
- Builds the existing sorted linked lists over those ranges.
- Leaves boundary-gap edges on the existing allocation and insertion path.

An attempted follow-up that sorted each site's range as an array was rejected. It changed duplicate-edge selection in the `issue48` frontier pattern and left eight unique vertex IDs unreferenced. The compatibility-preserving linked insertion over contiguous storage passes that regression.

The following comparisons use the same `clang++ -O3 -DNDEBUG` performance harness. Times are medians. The `issue48` case contains 99,998 symmetric diagonal-pair sites. Lower values are better.

### Diagram generation

| Case | Dev | Current | Boost | Current/Boost |
|---|---:|---:|---:|---:|
| 10k random | 5.342 ms | 5.982 ms | 6.406 ms | 93.4% |
| 100k random | 61.741 ms | 59.463 ms | 78.870 ms | 75.4% |
| issue48 | 26.776 ms | 22.406 ms | 40.063 ms | 55.9% |

At 10k sites, the extra counting and population passes cost approximately 12%. At 100k, locality recovers that cost and improves generation by approximately 3.7%. The `issue48` pattern improves by approximately 16.3% relative to Dev.

### Unique-vertex gathering

This measures only copying unique vertices from an already generated diagram into preallocated output. Diagram generation and output allocation are excluded. Boost performs an actual copy from its native vertex container.

| Case | Dev | Current | Boost |
|---|---:|---:|---:|
| 10k random | 102.417 us | 51.062 us | 7.250 us |
| 100k random | 3.729 ms | 990.313 us | 71.437 us |
| issue48 | 502.875 us | 440.687 us | 19.250 us |

The largest benefit is downstream traversal. The 100k random case is approximately 3.6 times faster because graph edges belonging to a site are now close together in memory.

### Memory

Each entry reports the peak per-diagram allocation footprint measured by the harness, followed by the number of allocations.

| Case | Dev | Current | Boost | Current/Boost |
|---|---:|---:|---:|---:|
| 10k random | 5,685,121 B / 326 | 5,440,097 B / 124 | 4,501,168 B / 55,798 | 120.9% |
| 100k random | 56,438,721 B / 3,226 | 53,982,745 B / 1,187 | 43,116,296 B / 556,989 | 125.2% |
| issue48 | 56,176,577 B / 3,210 | 55,121,913 B / 2,332 | 59,392,232 B / 400,018 | 92.8% |

For the 100k random case, the prototype reduces reported memory by approximately 4.4% and allocation count by approximately 63%. The memory reduction comes mainly from storing packed graph-edge records directly next to each other rather than individually aligning every record in the arena.

### Additional large random checks

At 300k random sites:

- Generation improved from 194 ms to 190 ms median.
- Cell-area generation and traversal improved from 203 ms to 193 ms.
- Unique-vertex gathering improved from 14.002 ms to 3.040 ms.
- Memory decreased from 168,201,409 to 160,837,129 bytes.

At one million random sites:

- Generation improved from 679 ms to 667 ms median; Boost took 819 ms.
- Cell-area generation and traversal improved from 705 ms to 676 ms.
- Unique-vertex gathering improved from 44.398 ms to 10.018 ms.
- Memory decreased from 546,804,289 to 522,964,961 bytes; Boost used 900,618,784 bytes.
- Allocation count decreased from 31,178 to 11,455; Boost performed 5,372,247 allocations.

The accumulated cell area differs by a very small amount at one million grid-distributed random inputs. Changed insertion order among numerically tied graph edges is a likely explanation, but that has not been proven. Edge counts, cell counts, total unique vertices, and the regression suite remain consistent. This remains an explicit equivalence check for section 3.

## Current benchmark comparison

This is the authoritative comparison for the current shared-edge implementation
and supersedes the historical prototype measurements above.

The implementation has no persistent `jcv_graphedge`. Each geometric edge is
allocated once and remains on the global linked list. After clipping and
sorting, each site receives a range in one contiguous array of pointers to
those shared edges. `jcv_site_get_edges()` uses that range and `jcv_edge_next()`
fills a caller-owned edge oriented counter-clockwise for the site.
Construction-only sorting records are freed before generation returns. All
structures use natural platform alignment; structure packing is disabled.

The measurements use `clang++ -O3 -DNDEBUG`, 21 iterations, and report the
median. Dev was built from the `dev` branch, Current from this working tree, and
Boost from the same benchmark harness. The `issue48` case contains 99,998
symmetric diagonal-pair sites. Lower values are better.

### Diagram generation

| Case | Dev | Current | Boost | Current/Boost |
|---|---:|---:|---:|---:|
| 10k | 5.473 ms | 5.928 ms | 6.481 ms | 91.5% |
| 100k | 60.478 ms | 66.057 ms | 79.590 ms | 83.0% |
| issue48 | 26.400 ms | 24.054 ms | 38.285 ms | 62.8% |

Current is 8.3% slower than Dev at 10k, 9.2% slower at 100k, and 8.9% faster
for issue48. It remains faster than Boost in all three cases.

### Unique-vertex gathering

This excludes diagram generation and output allocation.

| Case | Dev | Current | Boost |
|---|---:|---:|---:|
| 10k | 103.708 us | 59.250 us | 6.834 us |
| 100k | 2.950 ms | 638.916 us | 73.167 us |
| issue48 | 482.625 us | 809.250 us | 17.875 us |

Current substantially improves unique-vertex gathering for random inputs, but
is slower than Dev for the adversarial `issue48` layout. Boost copies from its
already contiguous native vertex container and remains much faster here.

### Memory

Each entry is peak heap bytes followed by allocation count.

| Case | Dev | Current | Boost | Current/Boost |
|---|---:|---:|---:|---:|
| 10k | 5,685,121 B / 326 | 4,281,728 B / 126 | 4,501,168 B / 55,798 | 95.1% |
| 100k | 56,438,721 B / 3,226 | 42,428,636 B / 1,187 | 43,116,296 B / 556,989 | 98.4% |
| issue48 | 56,176,505 B / 3,210 | 46,771,484 B / 2,041 | 59,390,968 B / 400,010 | 78.8% |

At 100k random sites, Current peak heap is 24.8% lower than Dev. Retained heap
after generation is 27,305,724 bytes instead of Dev's 56,438,721 bytes, a
51.6% reduction. The difference between Current peak and retained heap is the
temporary sorting topology released before returning to the caller.

### Boundary-gap profiling

The box clipper now marks boundary sites during edge clipping and skips
`fill_fn` for interior sites. This makes gap filling negligible for ordinary
random data, but it does not help the `issue48` pathological layout because
almost every cell is unbounded and reaches the clipping rectangle.

Temporary `-O3` instrumentation produced the following operation counts. The
counts are deterministic for these inputs; the times are representative phase
medians from the current implementation.

| Case | Sites | Sites passed to `fill_fn` | Edge-loop iterations | Gap edges inserted | Gap-fill time |
|---|---:|---:|---:|---:|---:|
| 100k random | 100,000 | 1,134 | 6,084 | 1,137 | 0.33 ms |
| issue48 | 99,998 | 99,998 | 406,662 | 149,998 | 2.53 ms |

The `issue48` generator places points at `(+n, -n)` and `(-n, -n)`, forming two
diagonal rays. Its cells reach the clipping boundary, so the boundary flag is set for
all 99,998 sites. The outer site scan is not the material cost: gap filling
constructs roughly 1.5 additional boundary edges per site.

The work inside `jcv_boxshape_fillgaps()` consists primarily of:

- Traversing both original and newly inserted graph-edge nodes.
- Copying two complete `jcv_edge` values per loop iteration to obtain oriented
  endpoints and vertex IDs.
- Testing endpoint boundary flags and equality and selecting intervening box
  corners.
- Allocating and initializing one retained `jcv_edge_internal` and one
  temporary `jcv_graphedge` for every inserted boundary edge.
- Linking each new edge into the global edge list and the site's ordered list,
  then revisiting inserted nodes while advancing around multiple corners.

Possible follow-up optimizations, in increasing order of scope:

1. Do not calculate `gap->angle` in `jcv_insert_gap_after()`. Gap edges are
   inserted directly at the correct list position, and the angle is not read
   afterward.
2. Read oriented positions and vertex IDs directly from `jcv_graphedge` and
   its internal edge instead of materializing two full public `jcv_edge`
   values on every iteration.
3. Add a boundary-chain fast path that determines and emits all missing sides
   for a site in one pass, without traversing each newly inserted edge again.
4. If construction remains allocation-bound after the local changes, evaluate
   bulk reservation for retained gap edges and temporary graph edges. Merely
   increasing the general arena block size did not improve the broader random
   benchmark, so this should be measured specifically on `issue48`.

The first two changes are local and low risk. The boundary-chain fast path is
the most likely substantial improvement for `issue48`, but it must preserve
corner order, vertex IDs, and custom-clipper behavior. Explicit gap-edge
construction cannot be removed when closed cells are requested because these
edges are part of the returned diagram.

### Beachline tree comparison

The viable prototypes used release `-O3` builds and seed 4. Red-Black is kept
leftmost as the baseline; the remaining columns are sorted by ascending
arithmetic mean across the three cases. AVL through AA report medians from the
same interleaved 31-iteration session. RAVL and Zip report the median of five
drift-controlled 15-iteration batch medians with alternating executable order.

| Case | Red-Black | RAVL | WAVL | AVL | Treap | Splay | Zip | Weight-balanced | AA |
|---|---:|---:|---:|---:|---:|---:|---:|---:|---:|
| 10k random | 5.40 ms | 5.29 ms | 5.54 ms | 5.87 ms | 6.21 ms | 6.87 ms | 6.22 ms | 6.46 ms | 6.68 ms |
| 100k random | 58.14 ms | 57.29 ms | 59.50 ms | 62.07 ms | 65.07 ms | 74.59 ms | 67.02 ms | 68.84 ms | 73.16 ms |
| issue48 | 22.82 ms | 20.00 ms | 19.96 ms | 21.01 ms | 24.91 ms | 17.98 ms | 26.70 ms | 33.29 ms | 39.87 ms |

RAVL is the only alternative that improves both ordinary random workloads and
`issue48`. AVL, Splay, and WAVL improve `issue48` but regress random workloads.

#### AVL beachline prototype: rejected as a global replacement

An intrusive AVL prototype was tested as a like-for-like replacement for the
red-black beachline index. It retained the existing halfedge nodes, linked
neighbors, insertion positions, and `jcv_halfedge_rightof()` predicate. The
existing color byte was reused for the AVL height, so memory and node layout
were unchanged. Height propagation stopped as soon as a subtree height was
unchanged.

AVL reduced `jcv_halfedge_rightof()` calls from 963,708 to 930,588 for 100k
random sites, only 3.4%, while performing 387,787 rotations. For `issue48`, it
reduced comparisons from 2,772,846 to 1,951,168, approximately 29.6%, while
performing 409,555 rotations. The tighter height bound therefore helps the
large pathological beachline, but its rotation and height-maintenance cost is
larger for ordinary random diagrams.

Float and double regressions passed, as did explicit AVL height, balance,
parent, in-order, and linked-neighbor invariant checks. Exact semantic hashes
still changed in five of 4,000 comparison diagrams, all highly cocircular
cases. This is consistent with the earlier observation that changing only the
tree shape can affect tie-sensitive topology.

The AVL tree was reverted. It is not suitable as the default because it slows
the main random workloads and introduces a compatibility difference. A
selectable adversarial-only tree policy would add complexity and metadata for
a narrow benefit and is not currently recommended.

#### Splay beachline prototype: rejected as a global replacement

An intrusive bottom-up splay tree was tested with the same halfedge nodes,
linked neighbors, insertion positions, and comparison predicate. Site-event
searches splayed the located predecessor, insertion splayed its known
predecessor, and circle-event removal splayed the known node before joining its
left and right subtrees. No additional node storage was required.

At 100k random sites, splaying reduced search comparisons only from 963,708
to 943,381, about 2.1%, while performing approximately 5,049,224 rotations.
For `issue48`, comparisons fell much more substantially, from 2,772,846 to
623,821, while approximately 1,640,563 rotations were performed. This explains
both outcomes: temporal locality is weak for ordinary random site and circle
events, but strong enough in the pathological layout to shorten its unusually
deep searches.

Float and double regressions passed, including parent, in-order, and
linked-neighbor invariant checks. Exact semantic hashes changed in 55 of 4,000
comparison diagrams, all in the near-cocircular pattern: 28 at 100 sites and
27 at 1,000 sites. Four of those cases also changed the validator's closure
failure count. As with AVL, tree shape can therefore affect tie-sensitive
topology.

The splay tree was reverted. It is not suitable as the default because the
common random workloads regress by roughly 30% and exact compatibility is not
preserved. Its strong `issue48` comparison reduction confirms that adaptive
locality can help the pathological beachline, but a hybrid would need to avoid
splaying ordinary searches and would add policy complexity for a specialized
case.

#### Treap prototype: rejected

The intrusive treap used deterministic xorshift priorities so runs were
reproducible. It required a 32-bit priority per halfedge, which increased the
100k random peak and retained arena totals by 16,392 bytes in this allocator
configuration. It was slower than Red-Black on all three cases, including
`issue48`, so randomized balancing does not match the beachline access pattern.
It changed 75 of 4,000 exact hashes, all in the near-cocircular pattern.

#### WAVL prototype: promising

The WAVL prototype stores the two child rank differences in the existing color
byte, so halfedge size and memory totals are unchanged. The implementation was
adapted from LLVM libc's intrusive WAVL algorithm and preserves stable node
addresses during successor transplantation.

At 100k random sites it performed 946,281 search comparisons and 366,087
rotations, compared with Red-Black's 963,708 comparisons. On `issue48`, it
reduced comparisons from 2,772,846 to 1,959,201 with 409,562 rotations. It is
therefore much closer to Red-Black on ordinary input than AVL, while retaining
most of AVL's pathological-case improvement.

Float and double regressions pass, as do explicit rank-difference, parent,
in-order, and linked-neighbor checks after every mutation. Combined ASan and
UBSan runs pass. Exact hashes changed in four of 4,000 comparison diagrams,
all near-cocircular: three at 100 sites and one at 1,000 sites. This is one
fewer compatibility difference than AVL, but it still prevents claiming exact
topology compatibility.

WAVL was the first new candidate worth a production-quality follow-up, but the
later RAVL prototype superseded it on these timings. WAVL retains the advantage
of fewer cocircular compatibility differences.

#### RAVL beachline: adopted

The relaxed AVL implementation follows Sen, Tarjan, and Kim's explicit-rank
algorithm: insertions rebalance by promotion and at most two rotations, while
deletions perform no rebalancing and do not change ranks at tree positions. The
reference implementation at `irkingmaker/RAVL_implementation` was useful for
cross-checking insertion cases, but its payload-copying deletion cannot be used
for stable intrusive halfedges. The implementation instead transplants the successor
node while transferring the deleted tree position's rank.

RAVL needs an explicit rank because deletions can create arbitrarily large rank
differences. The implementation packs a 31-bit rank beside the existing one-bit
`direction` field, leaving `jcv_halfedge` size and measured memory unchanged.

At 100k random sites RAVL made 1,044,395 search comparisons, 200,474 rotations,
and 399,504 promotions. It does slightly more search work than Red-Black but
substantially less structural deletion work. On `issue48`, it made 1,955,617
comparisons, 335,064 rotations, and 511,867 promotions.

The 100k random improvement reproduced with seeds 1, 4, 7, and 13. Float and
double regressions pass, including positive-rank-difference, height-versus-rank,
parent, in-order, and linked-neighbor invariants. Combined ASan and UBSan runs
pass. Exact hashes changed in 17 of 4,000 comparison diagrams, all near-
cocircular: nine at 100 sites and eight at 1,000 sites. RAVL therefore has the
best measured performance, but more compatibility differences than WAVL's four.

RAVL has now replaced the Red-Black beachline implementation. A final
drift-controlled comparison used five alternating batches and took the median
of their medians:

| Case | Red-Black | RAVL |
|---|---:|---:|
| 10k random | 5.35 ms | 5.13 ms |
| 100k random | 57.97 ms | 55.98 ms |
| issue48 | 23.10 ms | 19.43 ms |

Float and double regressions and combined ASan/UBSan builds pass. The production
implementation also reproduces the tested RAVL prototype's hashes and closure
counts across the full 4,000-diagram validation matrix. The known near-
cocircular incidence problem remains deferred in `plan_degenerate_inputs.md`;
it also existed in the historical Red-Black implementation and is not part of
the tree replacement.

##### RAVL exact-output investigation

The initial interpretation of the closure results was incorrect. The aggregate
closure-failure count fell by four, but that does not mean four cases changed.
Of the 17 RAVL hash differences, 15 change the per-case closure count and only
two leave it unchanged. Across those 17 cases Red-Black reports 2,554 endpoint
discontinuities and RAVL reports 2,550. Across all 400 near-cocircular 100- and
1,000-site cases the corresponding totals are 59,700 and 59,696; both have the
same worst per-case count of 332. The counter therefore shows no aggregate RAVL
regression, but it also shows that these inputs are already not emitted as
closed CCW chains by the Red-Black baseline.

All 17 differences remain confined to the near-cocircular generator; random,
grid-like, and near-collinear cases match exactly. Every changed diagram has
finite endpoints inside the clipping rectangle. Fifteen retain the same global
edge and unique-vertex counts. Two select a different-size cocircular
decomposition: the 100-site seed 50 case loses one edge and vertex, while the
1,000-site seed 25 case gains one edge and vertex.

The 1,000-site seed 37 case was inspected edge by edge. Red-Black gives original
site 250 a four-edge boundary. RAVL gives it six edges by adding two very short
edges near the common center, including an edge dual to a long Delaunay chord.
That alternate cocircular decomposition can be legitimate, but the site's
returned edges do not form the documented CCW chain: one center edge has the
opposite incidence orientation, producing a 24.58-unit iterator discontinuity.
Replacing the pseudo-angle sort metric with `atan2` leaves this ordering and
discontinuity unchanged, so the recent angle approximation is not the cause.

The exact hash changes alone are therefore not evidence of invalid geometry,
but “multiple valid decompositions” is also not enough to dismiss them. The
public per-site edge-ordering contract is violated on these degenerate inputs
by both the current Red-Black implementation and the RAVL prototype, with tree
shape changing the locations and sizes of the failures. RAVL should be judged
against a degeneracy regression that distinguishes topology freedom from hard
requirements: finite and clipped endpoints, consistent two-site incidence,
and a closed CCW edge chain.

##### Degenerate-incidence fix: deferred

The common Red-Black/RAVL degeneracy work was prototyped, validated, measured,
and then reverted so it can be considered independently from the beach-line
tree choice. The focused regression remains disabled as a TODO. The complete
problem statement, prototype design, results, tradeoffs, and revisit checklist
are recorded in `plan_degenerate_inputs.md`.

#### Zip prototype: rejected

The Zip prototype uses geometric ranks stored in the existing color byte and
implements path unzipping and zipping rather than Treap-style rotations. The
known insertion neighbor is converted into the root-to-gap path using existing
parent links, so it needs neither key labels nor extra node storage.

Float and double tests pass, including heap-rank, tie-order, parent, in-order,
and linked-neighbor invariants. Nevertheless, Zip was slower than Red-Black on
all three cases and changed 69 of 4,000 near-cocircular hashes. Zip-zip was not
implemented because the agreed criterion was to continue only if ordinary Zip
was competitive; its additional tie-rank cannot address Zip's measured update
and search deficit.

A skip-list control was also deferred. It is not a BST, and an intrusive
implementation would require separately allocated variable-height towers or a
larger halfedge layout. With RAVL already improving every primary case, that
memory-model change is no longer needed to choose the next BST candidate.

#### AA and weight-balanced prototypes: rejected

The AA tree reused the existing color byte as its level. Its simplified
balancing did not translate into lower cost here: it was substantially slower
on random input and especially poor on `issue48`. It changed 16 of 4,000 exact
hashes, all near-cocircular.

The weight-balanced prototype used a subtree-size field and the conventional
delta-3, ratio-2 balancing policy. Size maintenance increased node storage and
its timings lost to Red-Black in every case, particularly `issue48`. It changed
seven of 4,000 exact hashes, all near-cocircular.

#### B-tree family and Tango assessment

A B-tree or B+ tree is not a drop-in replacement for the current intrusive
index. It needs separately allocated multi-key nodes plus a stable mapping from
each halfedge to a movable node slot. That changes the memory model and deletion
path enough that a quick prototype would not be a fair tree-only comparison.
A meaningful test should be a separate beachline-index redesign that removes
the now-unused binary-tree pointers from each halfedge and accounts for its
temporary node arena.

Tango trees are likewise not justified by this trace. Full splaying already
showed that useful temporal locality is concentrated in `issue48`, while the
ordinary random cases pay heavily for adaptive rotations. A Tango or more
complex splay variant would target the specialized case rather than improve the
default workload.

### Validation

- Float tests: 25 passed, 1,839 assertions.
- Double tests: 26 passed, 1,848 assertions.
- Combined AddressSanitizer and UndefinedBehaviorSanitizer runs pass for both.
- The C main program, simple C example, and custom polygon clipping compile and run.
- Dev and Current produce matching random and `issue48` edge, cell, area, and
  jc_voronoi vertex counts.

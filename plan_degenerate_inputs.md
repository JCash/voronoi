# Degenerate Voronoi inputs

## Status

Deferred. The production prototype described below was tested and then
reverted. The focused regression remains disabled in `test/test.cpp` as a TODO.

This work is not specific to RAVL. The historical Red-Black beach line has the
same underlying issue as the current RAVL implementation; changing tree shape
only changes which numerically tied event order and cocircular decomposition
exposes it. The RAVL replacement therefore does not attempt to resolve this
separate robustness issue.

## Problem

On cocircular or nearly cocircular inputs, several Fortune circle events may
represent the same conceptual Voronoi vertex but produce slightly different
floating-point coordinates and vertex IDs. The input sites do not need to be
close together: the focused failure places 1,000 sites around a circle of
radius approximately 3,000.

Observed consequences:

- consecutive edges of a site do not meet exactly;
- a site's returned edge chain is open or inconsistently oriented;
- changing the beach-line tree changes the failing cases and decomposition;
- a valid alternative cocircular decomposition is difficult to distinguish
  from an actual incidence-contract violation.

The intended public contract is that every non-empty site boundary is a closed,
counter-clockwise chain with consistent shared vertex identities. Exact
neighbor topology need not match between valid cocircular decompositions.

Integer input would make orientation and incircle predicates exactly decidable
with sufficiently wide arithmetic, but it would not remove exact collinear or
cocircular degeneracies. A deterministic tie and zero-length-edge policy would
still be required.

## Deferred prototype

The prototype combined four pieces:

1. Incidence orientation was stored in the low alignment bit of each retained
   site-edge pointer. Broken pseudo-angle lists were rebuilt by matching vertex
   IDs and reversed when their signed area was clockwise.
2. Nearly parallel bisectors were reconstructed from original sites using a
   software double-double type. Float and double builds used the same fallback.
3. Reuse of a nearby incident circle vertex required an additional three-site
   squared-radius predicate, rather than proximity alone.
4. Explicit tiny edges and tiny gaps between consecutive edges of an already
   repaired site cycle were collapsed with union-find. Scratch memory was
   released immediately after vertex compaction.

Topology tolerances were separated from storage precision through a proposed
`JCV_DEGENERACY_EPSILON`, defaulting to `FLT_EPSILON` for both `jcv_real`
modes. This gave float and double the same resolution policy, although exact
decompositions still differed because beach-line predicates and event ordering
elsewhere in the sweep continued to use `jcv_real`.

## Prototype results

- A 4,000-diagram matrix covering random, grid-like, near-cocircular, and
  near-collinear inputs reported zero closure and winding failures in both
  float and double builds.
- It fixed all 1,147 diagrams that had open cells in the Red-Black baseline and
  introduced no closure regression among previously closed diagrams.
- 1,247 exact diagram hashes changed: the 1,147 broken diagrams plus 100
  previously closed diagrams that selected a different valid decomposition.
- Float and double tests, C99 compilation, ASan, and UBSan passed.
- Five `-O3` batch medians changed from 5.528 to 5.847 ms at 10k random sites,
  60.731 to 64.189 ms at 100k random sites, and 24.289 to 24.637 ms on the 100k
  symmetric-diagonal case.
- At 100k random sites, peak memory changed from 42,428,636 to 45,609,484
  bytes. Retained memory was effectively unchanged: 27,305,724 versus
  27,304,252 bytes.

## Remaining design question

“Same behavior” can mean either:

- the same validity contract and degeneracy-resolution scale; or
- identical neighbor topology and event decomposition.

The prototype achieved the first, not the second. Identical topology for the
same representable input requires precision-independent robust predicates and
tie handling throughout the Fortune sweep, including beach-line decisions and
event ordering. Fixing only circle construction cannot guarantee it.

Boost's relevant model is exact input predicates, approximate output
coordinates, deterministic handling of exact ties, and removal of degenerate
edges after construction. A future implementation should decide whether to
adopt that broader predicate architecture before restoring the TODO test.

## Revisit checklist

- Decide whether the required contract is validity-equivalent or
  topology-identical between float and double.
- Apply the solution to the shared sweep implementation and compare its output
  with the archived Red-Black and RAVL validation baselines.
- Re-enable the focused near-cocircular regression.
- Run the 4,000-case closure/winding matrix in float and double.
- Compare exact hash changes only after separating valid cocircular alternatives
  from contract failures.
- Re-run performance and peak/retained-memory measurements.

---
title: "Voronoi app"
layout: hextra-home
---

{{< voronoi-app copy="true" intro="false" >}}

<div class="voronoi-app-api content">

## URL API

All coordinates use the normalized range `0`–`1`, so the same URL renders consistently at any canvas size.

| Property | Value | Description |
|---|---|---|
| `points` | `x,y;x,y;…` | Exact site coordinates. Supply between 2 and 250 points. |
| `count` | `2`–`250` | Number of random sites when `points` is omitted. Default: `42`. |
| `seed` | integer | Makes a random `count` reproducible. |
| `delauney` | `true`, `1`, `yes`, or `on` | Shows Delauney edges. |

For example:

[`/app/?points=0.15,0.2;0.8,0.25;0.5,0.85&delauney=true`](?points=0.15%2C0.2%3B0.8%2C0.25%3B0.5%2C0.85&delauney=true)

Or create a reproducible random diagram:

[`/app/?count=24&seed=1234`](?count=24&seed=1234)

## Copied JSON

**Copy JSON** writes a versioned object to the clipboard containing normalized sites, normalized Voronoi edges, the currently visible Delauney edges, and the canvas bounds used for the render.

## Open files

Use **Open**, or drag a file onto the diagram, to load previously copied diagram JSON or a plain-text point list. Point lists use one whitespace- or comma-separated `x y` pair per line, like the repository's `testdata_issue*.txt` files. Coordinates outside the normalized range are scaled to fit while preserving their aspect ratio.

</div>

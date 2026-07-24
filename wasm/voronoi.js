import createVoronoiModule from "./jc_voronoi.js";

export async function loadVoronoi(moduleOptions = {}) {
  const module = await createVoronoiModule(moduleOptions);

  function generateEdges(points, width, height, generate) {
    if (!Array.isArray(points) && !(points instanceof Float32Array)) {
      throw new TypeError("points must be an array or Float32Array");
    }
    const flat = points instanceof Float32Array
      ? points
      : new Float32Array(points.flatMap(({ x, y }) => [x, y]));
    if (flat.length % 2 !== 0) {
      throw new TypeError("the flat point array must contain x/y pairs");
    }

    let pointsPointer = 0;
    let countPointer = 0;
    let edgesPointer = 0;
    try {
      pointsPointer = module._malloc(flat.byteLength);
      module.HEAPF32.set(flat, pointsPointer / Float32Array.BYTES_PER_ELEMENT);
      countPointer = module._malloc(Int32Array.BYTES_PER_ELEMENT);
      edgesPointer = generate(
        pointsPointer, flat.length / 2, width, height, countPointer,
      );
      const edgeCount = module.HEAP32[countPointer / Int32Array.BYTES_PER_ELEMENT];
      if (edgeCount === -2) throw new Error("Voronoi output allocation failed");
      if (edgeCount < 0) throw new Error("invalid Voronoi input");
      if (edgeCount === 0) return new Float32Array();

      const outputLength = edgeCount * 4;
      const start = edgesPointer / Float32Array.BYTES_PER_ELEMENT;
      return module.HEAPF32.slice(start, start + outputLength);
    } finally {
      if (edgesPointer) module._free(edgesPointer);
      if (countPointer) module._free(countPointer);
      if (pointsPointer) module._free(pointsPointer);
    }
  }

  return {
    edges: (points, width, height) => generateEdges(
      points, width, height, module._jcv_voronoi_edges,
    ),
    delauneyEdges: (points, width, height) => generateEdges(
      points, width, height, module._jcv_delauney_edges,
    ),
  };
}

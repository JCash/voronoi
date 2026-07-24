import createVoronoiModule from "./jc_voronoi.js";

function flattenPoints(points) {
  if (!Array.isArray(points) && !(points instanceof Float32Array)) {
    throw new TypeError("points must be an array or Float32Array");
  }
  const flat = points instanceof Float32Array
    ? points
    : new Float32Array(points.flatMap(({ x, y }) => [x, y]));
  if (flat.length % 2 !== 0) {
    throw new TypeError("the flat point array must contain x/y pairs");
  }
  return flat;
}

function parseBounds(boundsOrWidth, height) {
  if (typeof boundsOrWidth === "number") {
    if (!(boundsOrWidth > 0) || !(height > 0)) {
      throw new RangeError("width and height must be positive");
    }
    return [0, 0, boundsOrWidth, height];
  }

  const options = boundsOrWidth ?? {};
  if (Array.isArray(options.bounds) && options.bounds.length === 4) {
    const [minX, minY, maxX, maxY] = options.bounds;
    if (![minX, minY, maxX, maxY].every(Number.isFinite) ||
        !(maxX > minX) || !(maxY > minY)) {
      throw new RangeError("bounds must be [minX, minY, maxX, maxY]");
    }
    return options.bounds;
  }
  if (Number.isFinite(options.width) && Number.isFinite(options.height)) {
    if (!(options.width > 0) || !(options.height > 0)) {
      throw new RangeError("width and height must be positive");
    }
    return [0, 0, options.width, options.height];
  }
  throw new TypeError("provide width and height, or { bounds: [minX, minY, maxX, maxY] }");
}

export async function loadVoronoi(moduleOptions = {}) {
  const module = await createVoronoiModule(moduleOptions);
  const finalizer = typeof FinalizationRegistry === "function"
    ? new FinalizationRegistry((pointer) => module._jcv_wasm_diagram_destroy(pointer))
    : null;

  class Point {
    constructor(diagram, pointer) {
      this._diagram = diagram;
      this._pointer = pointer;
    }

    get x() {
      this._diagram._assertAlive();
      return module._jcv_wasm_point_x(this._pointer);
    }

    get y() {
      this._diagram._assertAlive();
      return module._jcv_wasm_point_y(this._pointer);
    }

    *[Symbol.iterator]() {
      yield this.x;
      yield this.y;
    }

    toJSON() {
      return { x: this.x, y: this.y };
    }
  }

  class Site {
    constructor(diagram, pointer) {
      this._diagram = diagram;
      this._pointer = pointer;
      this._point = null;
    }

    get p() {
      this._diagram._assertAlive();
      if (!this._point) {
        this._point = new Point(this._diagram, module._jcv_wasm_site_point(this._pointer));
      }
      return this._point;
    }

    get index() {
      this._diagram._assertAlive();
      return module._jcv_wasm_site_index(this._pointer);
    }

    get boundary() {
      this._diagram._assertAlive();
      return Boolean(module._jcv_wasm_site_boundary(this._pointer));
    }

    get cell() {
      return this._diagram.cell(this.index);
    }
  }

  class Edge {
    constructor(diagram, pointer, flags = 0) {
      this._diagram = diagram;
      this._pointer = pointer;
      this._siteFlip = flags & 1;
      this._positionFlip = (flags >> 1) & 1;
      this._sites = null;
      this._positions = null;
      this._vertices = null;
    }

    get sites() {
      this._diagram._assertAlive();
      if (!this._sites) {
        this._sites = Object.freeze([0, 1].map((index) => {
          const pointer = module._jcv_wasm_edge_site(this._pointer, index ^ this._siteFlip);
          return pointer ? this._diagram._site(pointer) : null;
        }));
      }
      return this._sites;
    }

    get pos() {
      this._diagram._assertAlive();
      if (!this._positions) {
        this._positions = Object.freeze([0, 1].map((index) => new Point(
          this._diagram,
          module._jcv_wasm_edge_position(this._pointer, index ^ this._positionFlip),
        )));
      }
      return this._positions;
    }

    get vertices() {
      this._diagram._assertAlive();
      if (!this._vertices) {
        this._vertices = Object.freeze([
          module._jcv_wasm_edge_vertex(this._pointer, this._positionFlip),
          module._jcv_wasm_edge_vertex(this._pointer, 1 ^ this._positionFlip),
        ]);
      }
      return this._vertices;
    }
  }

  class Cell {
    constructor(diagram, inputIndex, site) {
      this._diagram = diagram;
      this._inputIndex = inputIndex;
      this.site = site;
      this._edges = null;
      this._neighbors = null;
      this._polygon = null;
    }

    get edges() {
      this._diagram._assertAlive();
      if (!this._edges) {
        const count = module._jcv_wasm_cell_edge_count(
          this._diagram._pointer,
          this._inputIndex,
        );
        this._edges = Object.freeze(Array.from({ length: count }, (_, index) => new Edge(
          this._diagram,
          module._jcv_wasm_cell_edge(this._diagram._pointer, this._inputIndex, index),
          module._jcv_wasm_cell_edge_flags(this._diagram._pointer, this._inputIndex, index),
        )));
      }
      return this._edges;
    }

    get neighbors() {
      this._diagram._assertAlive();
      if (!this._neighbors) {
        const seen = new Set();
        const neighbors = [];
        for (const edge of this.edges) {
          const neighbor = edge.sites[1];
          if (neighbor && !seen.has(neighbor._pointer)) {
            seen.add(neighbor._pointer);
            neighbors.push(neighbor);
          }
        }
        this._neighbors = Object.freeze(neighbors);
      }
      return this._neighbors;
    }

    get polygon() {
      this._diagram._assertAlive();
      if (!this._polygon) {
        const polygon = this.edges.map((edge) => edge.pos[0]);
        if (this.edges.length > 0) polygon.push(this.edges[this.edges.length - 1].pos[1]);
        this._polygon = Object.freeze(polygon);
      }
      return this._polygon;
    }
  }

  class Diagram {
    constructor(pointer, inputCount, bounds) {
      this._pointer = pointer;
      this._inputCount = inputCount;
      this._bounds = Object.freeze([...bounds]);
      this._siteCache = new Map();
      this._cellCache = new Map();
      this._sites = null;
      this._edges = null;
      finalizer?.register(this, pointer, this);
    }

    _assertAlive() {
      if (!this._pointer) throw new Error("Voronoi diagram has been disposed");
    }

    _site(pointer) {
      let site = this._siteCache.get(pointer);
      if (!site) {
        site = new Site(this, pointer);
        this._siteCache.set(pointer, site);
      }
      return site;
    }

    get bounds() {
      return this._bounds;
    }

    get inputCount() {
      return this._inputCount;
    }

    get numSites() {
      this._assertAlive();
      return module._jcv_wasm_diagram_site_count(this._pointer);
    }

    get numVertices() {
      this._assertAlive();
      return module._jcv_wasm_diagram_vertex_count(this._pointer);
    }

    get sites() {
      this._assertAlive();
      if (!this._sites) {
        this._sites = Object.freeze(Array.from({ length: this.numSites }, (_, index) => this._site(
          module._jcv_wasm_diagram_site_at(this._pointer, index),
        )));
      }
      return this._sites;
    }

    site(inputIndex) {
      this._assertInputIndex(inputIndex);
      this._assertAlive();
      const pointer = module._jcv_wasm_diagram_site(this._pointer, inputIndex);
      return pointer ? this._site(pointer) : null;
    }

    cell(inputIndex) {
      this._assertInputIndex(inputIndex);
      this._assertAlive();
      if (this._cellCache.has(inputIndex)) return this._cellCache.get(inputIndex);
      const site = this.site(inputIndex);
      const cell = site ? new Cell(this, inputIndex, site) : null;
      this._cellCache.set(inputIndex, cell);
      return cell;
    }

    neighbors(inputIndex) {
      const cell = this.cell(inputIndex);
      return cell ? cell.neighbors : Object.freeze([]);
    }

    get edges() {
      this._assertAlive();
      if (!this._edges) {
        const count = module._jcv_wasm_diagram_edge_count(this._pointer);
        this._edges = Object.freeze(Array.from({ length: count }, (_, index) => new Edge(
          this,
          module._jcv_wasm_diagram_edge(this._pointer, index),
        )));
      }
      return this._edges;
    }

    _assertInputIndex(inputIndex) {
      if (!Number.isInteger(inputIndex) || inputIndex < 0 || inputIndex >= this._inputCount) {
        throw new RangeError(`site index must be an integer from 0 to ${this._inputCount - 1}`);
      }
    }

    dispose() {
      if (!this._pointer) return;
      finalizer?.unregister(this);
      module._jcv_wasm_diagram_destroy(this._pointer);
      this._pointer = 0;
    }
  }

  function withPoints(points, callback) {
    const flat = flattenPoints(points);
    let pointsPointer = 0;
    try {
      if (flat.byteLength > 0) {
        pointsPointer = module._malloc(flat.byteLength);
        if (!pointsPointer) throw new Error("Voronoi input allocation failed");
        module.HEAPF32.set(flat, pointsPointer / Float32Array.BYTES_PER_ELEMENT);
      }
      return callback(flat, pointsPointer);
    } finally {
      if (pointsPointer) module._free(pointsPointer);
    }
  }

  function generate(points, boundsOrWidth, height) {
    const bounds = parseBounds(boundsOrWidth, height);
    return withPoints(points, (flat, pointsPointer) => {
      const pointer = module._jcv_wasm_diagram_create(
        pointsPointer,
        flat.length / 2,
        ...bounds,
      );
      if (!pointer) throw new Error("Voronoi diagram allocation failed");
      return new Diagram(pointer, flat.length / 2, bounds);
    });
  }

  function generateEdges(points, width, height, generate) {
    return withPoints(points, (flat, pointsPointer) => {
      let countPointer = 0;
      let edgesPointer = 0;
      try {
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
      }
    });
  }

  return {
    generate,
    edges: (points, width, height) => generateEdges(
      points, width, height, module._jcv_voronoi_edges,
    ),
    delauneyEdges: (points, width, height) => generateEdges(
      points, width, height, module._jcv_delauney_edges,
    ),
  };
}

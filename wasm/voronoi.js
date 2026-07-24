import createVoronoiModule from "./jc_voronoi.js";

const PACK_MAGIC = 0x4a435631;
const PACK_VERSION = 1;
const HEADER = Object.freeze({
  magic: 0,
  version: 1,
  byteLength: 2,
  inputCount: 3,
  siteCount: 4,
  vertexCount: 5,
  edgeCount: 6,
  cellEdgeCount: 7,
  inputMapOffset: 8,
  sitesOffset: 9,
  verticesOffset: 10,
  edgesOffset: 11,
  cellOffsetsOffset: 12,
  cellRefsOffset: 13,
});
const SITE_WORDS = 4;
const EDGE_WORDS = 4;
const CELL_SITE_FLIP = 1;
const CELL_POSITION_FLIP = 2;
const CELL_EDGE_SHIFT = 2;
const DECODE_ONLY = Symbol("decodeOnly");

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
  const decodeOnly = moduleOptions[DECODE_ONLY] === true;
  const module = decodeOnly ? null : await createVoronoiModule(moduleOptions);

  class Point {
    constructor(diagram, kind, index) {
      this._diagram = diagram;
      this._kind = kind;
      this._index = index;
    }

    get x() {
      this._diagram._assertAlive();
      return this._kind === "site"
        ? this._diagram._siteFloats[this._index * SITE_WORDS]
        : this._diagram._vertices[this._index * 2];
    }

    get y() {
      this._diagram._assertAlive();
      return this._kind === "site"
        ? this._diagram._siteFloats[this._index * SITE_WORDS + 1]
        : this._diagram._vertices[this._index * 2 + 1];
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
    constructor(diagram, recordIndex) {
      this._diagram = diagram;
      this._recordIndex = recordIndex;
      this._point = null;
    }

    get p() {
      this._diagram._assertAlive();
      if (!this._point) this._point = new Point(this._diagram, "site", this._recordIndex);
      return this._point;
    }

    get index() {
      this._diagram._assertAlive();
      return this._diagram._siteWords[this._recordIndex * SITE_WORDS + 2];
    }

    get boundary() {
      this._diagram._assertAlive();
      return Boolean(this._diagram._siteWords[this._recordIndex * SITE_WORDS + 3]);
    }

    get cell() {
      return this._diagram.cell(this.index);
    }
  }

  class Edge {
    constructor(diagram, edgeIndex, flags = 0) {
      this._diagram = diagram;
      this._edgeIndex = edgeIndex;
      this._siteFlip = flags & CELL_SITE_FLIP;
      this._positionFlip = (flags & CELL_POSITION_FLIP) >> 1;
      this._sites = null;
      this._positions = null;
      this._vertexIndices = null;
    }

    get sites() {
      this._diagram._assertAlive();
      if (!this._sites) {
        const offset = this._edgeIndex * EDGE_WORDS;
        this._sites = Object.freeze([0, 1].map((endpoint) => {
          const inputIndex = this._diagram._edgeWords[offset + (endpoint ^ this._siteFlip)];
          return inputIndex < 0 ? null : this._diagram._siteForInput(inputIndex);
        }));
      }
      return this._sites;
    }

    get pos() {
      this._diagram._assertAlive();
      if (!this._positions) {
        const offset = this._edgeIndex * EDGE_WORDS + 2;
        this._positions = Object.freeze([0, 1].map((endpoint) => new Point(
          this._diagram,
          "vertex",
          this._diagram._edgeWords[offset + (endpoint ^ this._positionFlip)],
        )));
      }
      return this._positions;
    }

    get vertices() {
      this._diagram._assertAlive();
      if (!this._vertexIndices) {
        const offset = this._edgeIndex * EDGE_WORDS + 2;
        this._vertexIndices = Object.freeze([
          this._diagram._edgeWords[offset + this._positionFlip],
          this._diagram._edgeWords[offset + (1 ^ this._positionFlip)],
        ]);
      }
      return this._vertexIndices;
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
        const begin = this._diagram._cellOffsets[this._inputIndex];
        const end = this._diagram._cellOffsets[this._inputIndex + 1];
        this._edges = Object.freeze(Array.from({ length: end - begin }, (_, index) => {
          const reference = this._diagram._cellRefs[begin + index];
          return new Edge(
            this._diagram,
            reference >>> CELL_EDGE_SHIFT,
            reference & ((1 << CELL_EDGE_SHIFT) - 1),
          );
        }));
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
          if (neighbor && !seen.has(neighbor._recordIndex)) {
            seen.add(neighbor._recordIndex);
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
    constructor(buffer, bounds) {
      const header = new Uint32Array(buffer, 0, HEADER.cellRefsOffset + 1);
      if (header[HEADER.magic] !== PACK_MAGIC || header[HEADER.version] !== PACK_VERSION ||
          header[HEADER.byteLength] !== buffer.byteLength) {
        throw new Error("invalid packed Voronoi diagram");
      }
      this._buffer = buffer;
      this._bounds = Object.freeze([...bounds]);
      this._inputCount = header[HEADER.inputCount];
      this._siteCount = header[HEADER.siteCount];
      this._vertexCount = header[HEADER.vertexCount];
      this._edgeCount = header[HEADER.edgeCount];
      this._delauneyEdgeCount = null;
      this._inputToSite = new Int32Array(buffer, header[HEADER.inputMapOffset], this._inputCount);
      this._siteWords = new Uint32Array(buffer, header[HEADER.sitesOffset], this._siteCount * SITE_WORDS);
      this._siteFloats = new Float32Array(buffer, header[HEADER.sitesOffset], this._siteCount * SITE_WORDS);
      this._vertices = new Float32Array(buffer, header[HEADER.verticesOffset], this._vertexCount * 2);
      this._edgeWords = new Int32Array(buffer, header[HEADER.edgesOffset], this._edgeCount * EDGE_WORDS);
      this._cellOffsets = new Uint32Array(buffer, header[HEADER.cellOffsetsOffset], this._inputCount + 1);
      this._cellRefs = new Uint32Array(buffer, header[HEADER.cellRefsOffset], header[HEADER.cellEdgeCount]);
      this._siteCache = Array(this._siteCount);
      this._cellCache = new Map();
      this._sites = null;
      this._edges = null;
    }

    _assertAlive() {
      if (!this._buffer) throw new Error("Voronoi diagram has been disposed");
    }

    _site(recordIndex) {
      let site = this._siteCache[recordIndex];
      if (!site) {
        site = new Site(this, recordIndex);
        this._siteCache[recordIndex] = site;
      }
      return site;
    }

    _siteForInput(inputIndex) {
      const recordIndex = this._inputToSite[inputIndex];
      return recordIndex < 0 ? null : this._site(recordIndex);
    }

    get bounds() {
      return this._bounds;
    }

    get inputCount() {
      return this._inputCount;
    }

    get byteLength() {
      this._assertAlive();
      return this._buffer.byteLength;
    }

    get numSites() {
      this._assertAlive();
      return this._siteCount;
    }

    get numVertices() {
      this._assertAlive();
      return this._vertexCount;
    }

    get numEdges() {
      this._assertAlive();
      return this._edgeCount;
    }

    get numDelauneyEdges() {
      this._assertAlive();
      if (this._delauneyEdgeCount === null) {
        let count = 0;
        for (let index = 0; index < this._edgeCount; ++index) {
          const offset = index * EDGE_WORDS;
          if (this._edgeWords[offset] >= 0 && this._edgeWords[offset + 1] >= 0) ++count;
        }
        this._delauneyEdgeCount = count;
      }
      return this._delauneyEdgeCount;
    }

    get sites() {
      this._assertAlive();
      if (!this._sites) {
        this._sites = Object.freeze(Array.from({ length: this._siteCount }, (_, index) => this._site(index)));
      }
      return this._sites;
    }

    site(inputIndex) {
      this._assertInputIndex(inputIndex);
      this._assertAlive();
      return this._siteForInput(inputIndex);
    }

    cell(inputIndex) {
      this._assertInputIndex(inputIndex);
      this._assertAlive();
      if (this._cellCache.has(inputIndex)) return this._cellCache.get(inputIndex);
      const site = this._siteForInput(inputIndex);
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
        this._edges = Object.freeze(Array.from({ length: this._edgeCount }, (_, index) => new Edge(this, index)));
      }
      return this._edges;
    }

    render(context) {
      this._assertAlive();
      for (let index = 0; index < this._edgeCount; ++index) {
        const offset = index * EDGE_WORDS + 2;
        const vertex0 = this._edgeWords[offset] * 2;
        const vertex1 = this._edgeWords[offset + 1] * 2;
        context.moveTo(this._vertices[vertex0], this._vertices[vertex0 + 1]);
        context.lineTo(this._vertices[vertex1], this._vertices[vertex1 + 1]);
      }
      return context;
    }

    renderDelauney(context) {
      this._assertAlive();
      for (let index = 0; index < this._edgeCount; ++index) {
        const offset = index * EDGE_WORDS;
        const input0 = this._edgeWords[offset];
        const input1 = this._edgeWords[offset + 1];
        if (input0 < 0 || input1 < 0) continue;
        const site0 = this._inputToSite[input0] * SITE_WORDS;
        const site1 = this._inputToSite[input1] * SITE_WORDS;
        context.moveTo(this._siteFloats[site0], this._siteFloats[site0 + 1]);
        context.lineTo(this._siteFloats[site1], this._siteFloats[site1 + 1]);
      }
      return context;
    }

    _assertInputIndex(inputIndex) {
      if (!Number.isInteger(inputIndex) || inputIndex < 0 || inputIndex >= this._inputCount) {
        throw new RangeError(`site index must be an integer from 0 to ${this._inputCount - 1}`);
      }
    }

    dispose() {
      if (!this._buffer) return;
      this._buffer = null;
      this._inputToSite = null;
      this._siteWords = null;
      this._siteFloats = null;
      this._vertices = null;
      this._edgeWords = null;
      this._cellOffsets = null;
      this._cellRefs = null;
      this._siteCache = null;
      this._cellCache = null;
      this._sites = null;
      this._edges = null;
    }

    _takeBuffer() {
      this._assertAlive();
      const buffer = this._buffer;
      this.dispose();
      return buffer;
    }
  }

  if (decodeOnly) {
    return {
      _diagramFromBuffer: (buffer, bounds) => new Diagram(buffer, bounds),
    };
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
      let pointer = 0;
      try {
        pointer = module._jcv_wasm_generate_packed(
          pointsPointer,
          flat.length / 2,
          ...bounds,
        );
        if (!pointer) throw new Error("Voronoi diagram allocation failed");
        const byteLength = module.HEAP32[(pointer >> 2) + HEADER.byteLength];
        const buffer = module.HEAPU8.slice(pointer, pointer + byteLength).buffer;
        return new Diagram(buffer, bounds);
      } finally {
        if (pointer) module._free(pointer);
      }
    });
  }

  function generateEdges(points, width, height, generateEdgesNative) {
    return withPoints(points, (flat, pointsPointer) => {
      let countPointer = 0;
      let edgesPointer = 0;
      try {
        countPointer = module._malloc(Int32Array.BYTES_PER_ELEMENT);
        edgesPointer = generateEdgesNative(
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
    _wasmMemoryBytes: () => module.HEAPU8.buffer.byteLength,
  };
}

function workerRequest(worker, message, transfer) {
  return new Promise((resolve, reject) => {
    const succeed = (value) => {
      if (value.error) {
        reject(new Error(value.error));
      } else {
        resolve(value);
      }
    };
    if (typeof worker.addEventListener === "function") {
      worker.addEventListener("message", (event) => succeed(event.data), { once: true });
      worker.addEventListener("error", reject, { once: true });
    } else {
      worker.once("message", succeed);
      worker.once("error", reject);
    }
    worker.postMessage(message, transfer);
  });
}

async function createWorker(url) {
  if (typeof Worker !== "undefined") return new Worker(url, { type: "module" });
  const { Worker: NodeWorker } = await import("node:worker_threads");
  return new NodeWorker(url, { type: "module" });
}

async function terminateWorker(worker) {
  const result = worker.terminate();
  if (result && typeof result.then === "function") await result;
}

export async function loadVoronoiWorker(options = {}) {
  const decoder = await loadVoronoi({ [DECODE_ONLY]: true });
  const workerUrl = options.workerUrl ?? new URL("./voronoi.worker.js", import.meta.url);

  return {
    async generate(points, boundsOrWidth, height) {
      const bounds = parseBounds(boundsOrWidth, height);
      const flat = flattenPoints(points);
      const workerPoints = points instanceof Float32Array ? flat.slice() : flat;
      const worker = await createWorker(workerUrl);
      try {
        const result = await workerRequest(worker, {
          points: workerPoints.buffer,
          bounds,
        }, [workerPoints.buffer]);
        const diagram = decoder._diagramFromBuffer(result.buffer, bounds);
        Object.defineProperty(diagram, "_generationMemory", {
          value: Object.freeze({
            wasmHeapBytes: result.wasmHeapBytes,
            inputBytes: result.inputBytes,
            packedBytes: result.buffer.byteLength,
          }),
        });
        return diagram;
      } finally {
        await terminateWorker(worker);
      }
    },
  };
}

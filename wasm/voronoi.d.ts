export interface Point {
  readonly x: number;
  readonly y: number;
  [Symbol.iterator](): Iterator<number>;
  toJSON(): { x: number; y: number };
}

export type PointInput = readonly {
  readonly x: number;
  readonly y: number;
}[] | Float32Array;

export interface Site {
  readonly p: Point;
  readonly index: number;
  readonly boundary: boolean;
  readonly cell: Cell | null;
}

export interface Edge {
  readonly sites: readonly [Site | null, Site | null];
  readonly pos: readonly [Point, Point];
  readonly vertices: readonly [number, number];
}

export interface Cell {
  readonly site: Site;
  readonly edges: readonly Edge[];
  readonly neighbors: readonly Site[];
  readonly polygon: readonly Point[];
}

export interface PathContext {
  moveTo(x: number, y: number): void;
  lineTo(x: number, y: number): void;
}

export interface Diagram {
  readonly bounds: readonly [number, number, number, number];
  readonly inputCount: number;
  readonly byteLength: number;
  readonly numSites: number;
  readonly numVertices: number;
  readonly numEdges: number;
  readonly numDelauneyEdges: number;
  readonly sites: readonly Site[];
  readonly edges: readonly Edge[];
  site(inputIndex: number): Site | null;
  cell(inputIndex: number): Cell | null;
  neighbors(inputIndex: number): readonly Site[];
  render(context: PathContext): PathContext;
  renderDelauney(context: PathContext): PathContext;
  /** Eagerly releases the JavaScript-owned result buffer. Optional. */
  dispose(): void;
}

export interface GenerateOptions {
  bounds?: readonly [number, number, number, number];
  width?: number;
  height?: number;
}

export interface VoronoiModuleOptions {
  locateFile?: (path: string, scriptDirectory: string) => string;
  wasmBinary?: Uint8Array;
  print?: (...args: unknown[]) => void;
  printErr?: (...args: unknown[]) => void;
  [option: string]: unknown;
}

export interface Voronoi {
  /** Creates an ergonomic diagram backed by one JavaScript-owned ArrayBuffer. */
  generate(points: PointInput, options: GenerateOptions): Diagram;
  generate(points: PointInput, width: number, height: number): Diagram;

  /** Returns flat x0, y0, x1, y1 coordinates for every Voronoi edge. */
  edges(points: PointInput, width: number, height: number): Float32Array;

  /** Returns flat x0, y0, x1, y1 coordinates for every Delaunay edge. */
  delauneyEdges(points: PointInput, options: GenerateOptions): Float32Array;
  delauneyEdges(
    points: PointInput,
    width: number,
    height: number,
  ): Float32Array;
}

export interface VoronoiWorkerOptions {
  /** Override the URL of the one-shot module worker. */
  workerUrl?: string | URL;
}

export interface VoronoiWorker {
  /** Generates in a one-shot worker and resolves after its WASM runtime is terminated. */
  generate(points: PointInput, options: GenerateOptions): Promise<Diagram>;
  generate(points: PointInput, width: number, height: number): Promise<Diagram>;
}

export function loadVoronoi(
  moduleOptions?: VoronoiModuleOptions,
): Promise<Voronoi>;

export function loadVoronoiWorker(
  options?: VoronoiWorkerOptions,
): Promise<VoronoiWorker>;

export interface Point {
  readonly x: number;
  readonly y: number;
  [Symbol.iterator](): Iterator<number>;
}

export type PointInput = readonly Point[] | Float32Array;

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

export interface Diagram {
  readonly bounds: readonly [number, number, number, number];
  readonly inputCount: number;
  readonly numSites: number;
  readonly numVertices: number;
  readonly sites: readonly Site[];
  readonly edges: readonly Edge[];
  site(inputIndex: number): Site | null;
  cell(inputIndex: number): Cell | null;
  neighbors(inputIndex: number): readonly Site[];
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
  /** Creates a persistent, zero-copy view of the complete diagram. */
  generate(points: PointInput, options: GenerateOptions): Diagram;
  generate(points: PointInput, width: number, height: number): Diagram;

  /** Returns flat x0, y0, x1, y1 coordinates for every Voronoi edge. */
  edges(points: PointInput, width: number, height: number): Float32Array;

  /** Returns flat x0, y0, x1, y1 coordinates for every Delaunay edge. */
  delauneyEdges(
    points: PointInput,
    width: number,
    height: number,
  ): Float32Array;
}

export function loadVoronoi(
  moduleOptions?: VoronoiModuleOptions,
): Promise<Voronoi>;

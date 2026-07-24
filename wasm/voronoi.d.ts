export interface Point {
  readonly x: number;
  readonly y: number;
}

export type PointInput = readonly Point[] | Float32Array;

export interface VoronoiModuleOptions {
  locateFile?: (path: string, scriptDirectory: string) => string;
  wasmBinary?: Uint8Array;
  print?: (...args: unknown[]) => void;
  printErr?: (...args: unknown[]) => void;
  [option: string]: unknown;
}

export interface Voronoi {
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

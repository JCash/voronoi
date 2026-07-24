import { Delaunay } from "d3-delaunay";
import { voronoi as createD3Voronoi } from "d3-voronoi";
import Voronoi from "voronoi";

function d3Delaunay(points, bounds) {
  const generateDelaunay = () => new Delaunay(points);
  const generate = () => generateDelaunay().voronoi(bounds);

  return {
    generate,
    sites() {
      const diagram = generate();
      return diagram.delaunay.points.length / 2;
    },
    edges() {
      const diagram = generate();
      return diagram.render().length;
    },
    delauney() {
      const delaunay = generateDelaunay();
      return delaunay.triangles.length / 3;
    },
  };
}

function d3Voronoi(points, [minX, minY, maxX, maxY]) {
  const generateDiagram = createD3Voronoi().extent([[minX, minY], [maxX, maxY]]);
  const generate = () => generateDiagram(points);

  return {
    generate,
    sites() {
      return generate().cells.length;
    },
    edges() {
      return generate().edges.length;
    },
    delauney() {
      return generate().triangles().length;
    },
  };
}

function raymondHill(points, [minX, minY, maxX, maxY]) {
  const bounds = { xl: minX, xr: maxX, yt: minY, yb: maxY };
  const generate = () => new Voronoi().compute(points, bounds);

  return {
    generate,
    sites() {
      return generate().cells.length;
    },
    edges() {
      return generate().edges.length;
    },
    delauney: null,
  };
}

export function prepareComparisons(flatPoints, bounds) {
  const doublePoints = new Float64Array(flatPoints);
  const arrayPoints = new Array(flatPoints.length / 2);
  const objectPoints = new Array(flatPoints.length / 2);
  for (let i = 0, point = 0; i < flatPoints.length; i += 2, ++point) {
    arrayPoints[point] = [flatPoints[i], flatPoints[i + 1]];
    objectPoints[point] = { x: flatPoints[i], y: flatPoints[i + 1] };
  }

  return {
    "d3-delaunay": d3Delaunay(doublePoints, bounds),
    "d3-voronoi": d3Voronoi(arrayPoints, bounds),
    "gorhill/voronoi": raymondHill(objectPoints, bounds),
  };
}

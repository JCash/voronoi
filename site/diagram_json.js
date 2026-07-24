function pointToJSON(point) {
  return { x: point.x, y: point.y };
}

function edgeToJSON(edge) {
  return {
    sites: edge.sites.map((site) => site?.index ?? null),
    pos: edge.pos.map(pointToJSON),
    vertices: [...edge.vertices],
  };
}

export function diagramToJSON(diagram) {
  const edges = diagram.edges.map(edgeToJSON);
  const vertices = Array(diagram.numVertices).fill(null);
  for (const edge of diagram.edges) {
    for (let endpoint = 0; endpoint < 2; ++endpoint) {
      const vertex = edge.vertices[endpoint];
      if (vertex >= 0) vertices[vertex] = pointToJSON(edge.pos[endpoint]);
    }
  }

  return {
    bounds: [...diagram.bounds],
    inputCount: diagram.inputCount,
    numSites: diagram.numSites,
    numVertices: diagram.numVertices,
    sites: diagram.sites.map((site) => ({
      index: site.index,
      p: pointToJSON(site.p),
      boundary: site.boundary,
    })),
    vertices,
    edges,
    cells: Array.from({ length: diagram.inputCount }, (_, inputIndex) => {
      const cell = diagram.cell(inputIndex);
      return cell ? {
        site: cell.site.index,
        edges: cell.edges.map(edgeToJSON),
        neighbors: cell.neighbors.map((site) => site.index),
        polygon: cell.polygon.map(pointToJSON),
      } : null;
    }),
    delauneyEdges: diagram.edges
      .filter((edge) => edge.sites[0] && edge.sites[1])
      .map((edge) => edge.sites.map((site) => site.index)),
  };
}

export function importPointList(rawPoints, normalized = false) {
  if (!Array.isArray(rawPoints) || rawPoints.length < 1 || rawPoints.length > 250) {
    throw new Error("files must contain between 1 and 250 points");
  }
  const imported = rawPoints.map((point) => {
    const x = Number(Array.isArray(point) ? point[0] : point?.x ?? point?.p?.x);
    const y = Number(Array.isArray(point) ? point[1] : point?.y ?? point?.p?.y);
    if (!Number.isFinite(x) || !Number.isFinite(y)) throw new Error("each point must contain numeric x and y values");
    return { x, y };
  });
  const alreadyNormalized = imported.every(({ x, y }) => x >= 0 && x <= 1 && y >= 0 && y <= 1);
  if (normalized || alreadyNormalized) {
    if (!alreadyNormalized) throw new Error("normalized JSON coordinates must be between 0 and 1");
    return imported;
  }
  const xs = imported.map(({ x }) => x);
  const ys = imported.map(({ y }) => y);
  const minimumX = Math.min(...xs);
  const maximumX = Math.max(...xs);
  const minimumY = Math.min(...ys);
  const maximumY = Math.max(...ys);
  const rangeX = maximumX - minimumX;
  const rangeY = maximumY - minimumY;
  const range = Math.max(rangeX, rangeY);
  if (range === 0) return imported.map(() => ({ x: 0.5, y: 0.5 }));
  const scale = 0.9 / range;
  const offsetX = (1 - rangeX * scale) / 2;
  const offsetY = (1 - rangeY * scale) / 2;
  return imported.map(({ x, y }) => ({
    x: offsetX + (x - minimumX) * scale,
    y: offsetY + (y - minimumY) * scale,
  }));
}

export function readPointFile(text) {
  const trimmed = text.trim();
  if (!trimmed) throw new Error("the selected file is empty");
  if (trimmed.startsWith("{") || trimmed.startsWith("[")) {
    let document;
    try {
      document = JSON.parse(trimmed);
    } catch {
      throw new Error("the selected JSON is invalid");
    }
    const source = Array.isArray(document) ? document : document.sites || document.points;
    if (!source) throw new Error("JSON must contain a sites or points array");
    return importPointList(source, document.coordinateSpace === "normalized");
  }
  const numberPattern = /[-+]?(?:\d+\.?\d*|\.\d+)(?:[eE][-+]?\d+)?/g;
  const imported = [];
  for (const originalLine of trimmed.split(/\r?\n/)) {
    const line = originalLine.replace(/#.*/, "").replace(/\/\/.*/, "").trim();
    if (!line) continue;
    const values = line.match(numberPattern);
    if (!values || values.length !== 2) throw new Error("point-list lines must contain one x y pair");
    imported.push({ x: Number(values[0]), y: Number(values[1]) });
  }
  return importPointList(imported);
}

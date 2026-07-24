import { loadVoronoi } from "./voronoi.js";

let receive;
let send;

if (typeof globalThis.postMessage === "function") {
  receive = (callback) => globalThis.addEventListener("message", (event) => callback(event.data));
  send = (message, transfer) => globalThis.postMessage(message, transfer);
} else {
  const { parentPort } = await import("node:worker_threads");
  receive = (callback) => parentPort.once("message", callback);
  send = (message, transfer) => parentPort.postMessage(message, transfer);
}

receive(async ({ points, bounds }) => {
  try {
    const voronoi = await loadVoronoi();
    const diagram = voronoi.generate(new Float32Array(points), { bounds });
    const buffer = diagram._takeBuffer();
    send({
      buffer,
      inputBytes: points.byteLength,
      wasmHeapBytes: voronoi._wasmMemoryBytes(),
    }, [buffer]);
  } catch (error) {
    send({ error: error instanceof Error ? error.message : String(error) });
  }
});

# Documentation site

The GitHub Pages site is generated with Hugo and the Hextra theme. Its source lives in this directory; the interactive app and offline benchmark remain in `../site` and are mounted into the generated site.

## Preview locally

Install Hugo Extended, Go, and the Emscripten SDK, then run:

```sh
./hugo/serve_local.sh
```

Open <http://127.0.0.1:1313/voronoi/>. The helper rebuilds the WebAssembly package before starting Hugo.

To include the offline comparison bundle, install its dependencies and build it before starting the server:

```sh
npm ci --prefix benchmark
npm run build --prefix benchmark
```

The Pages workflow performs all three builds—WebAssembly, benchmark bundle, and Hugo—and deploys `public/` as its artifact.

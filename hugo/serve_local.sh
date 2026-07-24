#!/usr/bin/env bash
set -euo pipefail

cd "$(dirname "$0")/.."

./scripts/build_wasm.sh site/vendor

hugo server \
    --source hugo \
    --buildDrafts \
    --disableFastRender \
    --renderToMemory \
    --bind 127.0.0.1 \
    --port 1313

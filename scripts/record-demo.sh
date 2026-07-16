#!/usr/bin/env bash
# Record a mock product-demo video of Aegis Secure Chat.
set -euo pipefail
ROOT="$(cd "$(dirname "$0")/.." && pwd)"
REC="$ROOT/scripts/demo-recorder"

if ! curl -sf "${DEMO_BASE:-http://localhost:8080}/health" >/dev/null 2>&1; then
  echo "==> Starting server in background…"
  export PORT="${PORT:-8080}"
  export STATIC_DIR="$ROOT/dist"
  export LOG_SECRET="${LOG_SECRET:-$(openssl rand -hex 32)}"
  export TURN_SECRET="${TURN_SECRET:-$(openssl rand -hex 32)}"
  export CORS_ORIGIN="*"
  (cd "$ROOT/server" && node keyserver.js) &
  sleep 2
fi

cd "$REC"
if [[ ! -d node_modules/playwright ]]; then
  npm install
  npx playwright install chromium
fi

node record-demo.mjs --view "${1:-split}"

# record-demo.mjs runs compose-intro automatically; fallback if demo-only mp4 exists:
if [[ -f demo-output/aegis-demo-main-only.mp4 ]] && [[ ! -f demo-output/aegis-product-demo-full.mp4 ]]; then
  node compose-intro.mjs demo-output/aegis-demo-main-only.mp4
fi

echo "==> Final video: $REC/demo-output/aegis-product-demo.mp4"

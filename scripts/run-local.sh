#!/usr/bin/env bash
# Full local stack: build WASM + serve dist with keyserver (API + TURN config + key transparency)
set -euo pipefail
ROOT="$(cd "$(dirname "$0")/.." && pwd)"

"$ROOT/scripts/build.sh"

cd "$ROOT/server"
if [[ ! -d node_modules ]]; then
  npm install --omit=dev
fi

export PORT="${PORT:-8080}"
export STATIC_DIR="$ROOT/dist"
export TURN_SECRET="${TURN_SECRET:-local-dev-turn-secret-change-me}"
export LOG_SECRET="${LOG_SECRET:-$(openssl rand -hex 32)}"
export CORS_ORIGIN="*"

echo "==> Starting Aegis at http://localhost:${PORT}"
echo "    (includes /api/ice-config and /api/v1/keys)"
exec node keyserver.js

#!/usr/bin/env bash
# Automated E2E test: Host + Guest in Playwright — invite, signaling, chat, files.
#
# Usage:
#   ./scripts/run-e2e-test.sh
#   E2E_BASE=https://your-url.trycloudflare.com ./scripts/run-e2e-test.sh
#   ./scripts/run-e2e-test.sh --headed
#
set -euo pipefail

ROOT="$(cd "$(dirname "$0")/.." && pwd)"
E2E_DIR="$ROOT/scripts/e2e-test"
BASE="${E2E_BASE:-http://localhost:8080}"
HEADED=""
SERVER_PID=""

for arg in "$@"; do
  case "$arg" in
    --headed) HEADED="--headed" ;;
    --base)
      echo "Use E2E_BASE=https://... or: node e2e-test.mjs --base URL"
      exit 1
      ;;
  esac
done

cleanup() {
  if [[ -n "$SERVER_PID" ]] && kill -0 "$SERVER_PID" 2>/dev/null; then
    kill "$SERVER_PID" 2>/dev/null || true
    wait "$SERVER_PID" 2>/dev/null || true
  fi
}
trap cleanup EXIT

if ! curl -sf "${BASE}/health" >/dev/null 2>&1; then
  if [[ "$BASE" != http://localhost* && "$BASE" != http://127.0.0.1* ]]; then
    echo "ERROR: Server not reachable at ${BASE}/health"
    echo "       Start the server or set E2E_BASE to a running instance."
    exit 1
  fi
  echo "==> Starting local keyserver on :8080…"
  export PORT="${PORT:-8080}"
  export STATIC_DIR="$ROOT/dist"
  export LOG_SECRET="${LOG_SECRET:-$(openssl rand -hex 32)}"
  export TURN_SECRET="${TURN_SECRET:-$(openssl rand -hex 32)}"
  export CORS_ORIGIN="*"
  if [[ ! -d "$ROOT/server/node_modules" ]]; then
    (cd "$ROOT/server" && npm install --omit=dev)
  fi
  (cd "$ROOT/server" && node keyserver.js) &
  SERVER_PID=$!
  for _ in $(seq 1 30); do
    curl -sf "${BASE}/health" >/dev/null 2>&1 && break
    sleep 1
  done
  if ! curl -sf "${BASE}/health" >/dev/null 2>&1; then
    echo "ERROR: Failed to start keyserver"
    exit 1
  fi
  echo "==> Keyserver ready"
fi

if [[ ! -f "$ROOT/dist/cyberknot.js" ]]; then
  echo "ERROR: dist/ missing. Run: ./scripts/build.sh"
  exit 1
fi

cd "$E2E_DIR"
if [[ ! -d node_modules/playwright ]]; then
  echo "==> Installing E2E dependencies…"
  npm install
  npx playwright install chromium
fi

echo "==> Running E2E test against ${BASE}"
E2E_BASE="$BASE" node e2e-test.mjs $HEADED

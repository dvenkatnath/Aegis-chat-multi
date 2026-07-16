#!/usr/bin/env bash
# Aegis Secure Chat — local launcher
#
# Usage:
#   ./run.sh              Build if needed, start server
#   ./run.sh --no-build   Start server only (requires dist/)
#   ./run.sh --build      Force rebuild WASM, then start
#   ./run.sh --open       Start and open http://localhost:8080 in browser
#   ./run.sh --native     Build and run native C test (MainWF-SRC)
#
set -euo pipefail

ROOT="$(cd "$(dirname "$0")" && pwd)"
PORT="${PORT:-8080}"
URL="http://localhost:${PORT}"

DO_BUILD=auto
DO_OPEN=false
NATIVE=false

for arg in "$@"; do
  case "$arg" in
    --no-build) DO_BUILD=false ;;
    --build)    DO_BUILD=true ;;
    --open)     DO_OPEN=true ;;
    --native)   NATIVE=true ;;
    -h|--help)
      sed -n '2,12p' "$0" | sed 's/^# \?//'
      exit 0
      ;;
    *)
      echo "Unknown option: $arg (try --help)" >&2
      exit 1
      ;;
  esac
done

need_cmd() {
  if ! command -v "$1" >/dev/null 2>&1; then
    echo "ERROR: '$1' not found. Install it and try again." >&2
    exit 1
  fi
}

start_server() {
  if [[ ! -f "$ROOT/dist/index.html" ]] || [[ ! -f "$ROOT/dist/cyberknot.js" ]]; then
    echo "ERROR: dist/ is missing. Run: ./run.sh --build" >&2
    echo "       (requires Emscripten: emcc)" >&2
    exit 1
  fi

  cd "$ROOT/server"
  if [[ ! -d node_modules ]]; then
    echo "==> Installing server dependencies…"
    npm install --omit=dev
  fi

  export PORT
  export STATIC_DIR="$ROOT/dist"
  export TURN_SECRET="${TURN_SECRET:-$(openssl rand -hex 32)}"
  export LOG_SECRET="${LOG_SECRET:-$(openssl rand -hex 32)}"
  export CORS_ORIGIN="${CORS_ORIGIN:-*}"

  if curl -sf "$URL/health" >/dev/null 2>&1; then
    echo "==> Server already running at $URL"
  else
    echo "==> Starting Aegis Secure Chat at $URL"
    echo "    API: /api/ice-config  /api/v1/keys"
    echo "    Press Ctrl+C to stop"
    echo ""
    if $DO_OPEN; then
      (sleep 1 && open "$URL" 2>/dev/null || xdg-open "$URL" 2>/dev/null || true) &
    fi
    exec node keyserver.js
  fi

  if $DO_OPEN; then
    open "$URL" 2>/dev/null || xdg-open "$URL" 2>/dev/null || true
  fi
}

run_native_test() {
  need_cmd gcc
  echo "==> Building native test program…"
  cd "$ROOT/src"
  make -f Makefile-Main-Workflow
  echo "==> Running MainWF-SRC (press Enter when prompted to continue loops)…"
  exec ./MainWF-SRC
}

maybe_build() {
  local want_build=$1
  if [[ "$want_build" == false ]]; then
    return
  fi
  if [[ "$want_build" == auto ]] && [[ -f "$ROOT/dist/cyberknot.js" ]]; then
    echo "==> Using existing dist/ (pass --build to recompile WASM)"
    return
  fi
  need_cmd emcc
  need_cmd openssl
  echo "==> Building WASM and static assets…"
  "$ROOT/scripts/build.sh"
}

# ── main ──
if $NATIVE; then
  run_native_test
fi

need_cmd node
need_cmd npm
need_cmd openssl
need_cmd curl

case "$DO_BUILD" in
  true)  maybe_build true ;;
  false) ;;
  auto)  maybe_build auto ;;
esac

start_server

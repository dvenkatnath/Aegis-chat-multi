#!/usr/bin/env bash
# Headless Mac mini server: keyserver + Cloudflare quick tunnel.
# Used by launchd (scripts/mac-server-install.sh). Do not run interactively
# unless debugging — use ./run.sh for local dev.
set -euo pipefail

# launchd cannot execute scripts on external volumes (/Volumes/*).
# Install copies this file to ~/Library/Application Support/Aegis/ and sets AEGIS_ROOT.
ROOT="${AEGIS_ROOT:-$(cd "$(dirname "$0")/.." && pwd)}"
PORT="${PORT:-8080}"
LOG_DIR="${HOME}/Library/Logs/Aegis"
ENV_FILE="${ROOT}/deploy/macos/aegis-server.env"
URL_FILE="${ROOT}/deploy/macos/public-url.txt"
PID_FILE="${LOG_DIR}/keyserver.pid"

mkdir -p "$LOG_DIR" "${ROOT}/deploy/macos" "${ROOT}/server/data"

log() {
  echo "[$(date -u +%Y-%m-%dT%H:%M:%SZ)] $*" | tee -a "${LOG_DIR}/daemon.log"
}

# Load or create persistent secrets (stable across reboots)
if [[ -f "$ENV_FILE" ]]; then
  # shellcheck disable=SC1090
  source "$ENV_FILE"
fi
if [[ -z "${LOG_SECRET:-}" ]]; then
  LOG_SECRET="$(openssl rand -hex 32)"
  echo "LOG_SECRET=${LOG_SECRET}" >> "$ENV_FILE"
fi
if [[ -z "${TURN_SECRET:-}" ]]; then
  TURN_SECRET="$(openssl rand -hex 32)"
  echo "TURN_SECRET=${TURN_SECRET}" >> "$ENV_FILE"
fi

export PORT STATIC_DIR="${ROOT}/dist" LOG_SECRET TURN_SECRET
export CORS_ORIGIN="${CORS_ORIGIN:-*}"
export DATA_DIR="${ROOT}/server/data"

need_cmd() {
  command -v "$1" >/dev/null 2>&1 || { log "ERROR: missing command: $1"; exit 1; }
}

need_cmd node
need_cmd openssl
need_cmd cloudflared

if [[ ! -f "${ROOT}/dist/cyberknot.js" ]]; then
  log "ERROR: dist/ missing. Run ./scripts/build.sh once on the Mac mini."
  exit 1
fi

start_keyserver() {
  if curl -sf "http://127.0.0.1:${PORT}/health" >/dev/null 2>&1; then
    log "Keyserver already running on :${PORT}"
    return 0
  fi

  cd "${ROOT}/server"
  if [[ ! -d node_modules ]]; then
    log "Installing server npm dependencies…"
    npm install --omit=dev >> "${LOG_DIR}/npm-install.log" 2>&1
  fi

  log "Starting keyserver on :${PORT}"
  node keyserver.js >> "${LOG_DIR}/keyserver.log" 2>&1 &
  echo $! > "$PID_FILE"

  for _ in $(seq 1 30); do
    if curl -sf "http://127.0.0.1:${PORT}/health" >/dev/null 2>&1; then
      log "Keyserver healthy"
      return 0
    fi
    sleep 1
  done
  log "ERROR: keyserver failed to start — see ${LOG_DIR}/keyserver.log"
  exit 1
}

capture_tunnel_url() {
  local line="$1"
  if [[ "$line" =~ https://[a-z0-9-]+\.trycloudflare\.com ]]; then
    local url
    url="$(echo "$line" | grep -oE 'https://[a-z0-9-]+\.trycloudflare\.com' | head -1)"
    if [[ -n "$url" && "$url" != "$(cat "$URL_FILE" 2>/dev/null || true)" ]]; then
      echo "$url" > "$URL_FILE"
      log "Public URL: $url"
      log "Colleagues open this URL — one laptop Host, one laptop Guest."
    fi
  fi
}

write_public_url() {
  local url="$1"
  echo "$url" > "$URL_FILE"
  log "Public URL: $url"
}

start_keyserver

CF_CONFIG="${ROOT}/deploy/macos/cloudflared.yml"
log "Logs: ${LOG_DIR}/"

if [[ -f "$CF_CONFIG" ]]; then
  if [[ -z "${PUBLIC_URL:-}" ]]; then
    log "ERROR: cloudflared.yml exists but PUBLIC_URL is not set in ${ENV_FILE}"
    log "       Add: PUBLIC_URL=https://aegis.yourdomain.com"
    log "       See docs/CUSTOM-URL.md"
    exit 1
  fi
  write_public_url "$PUBLIC_URL"
  log "Starting named Cloudflare tunnel (${PUBLIC_URL})"
  cloudflared tunnel --config "$CF_CONFIG" run 2>&1 | while IFS= read -r line; do
    echo "$line" >> "${LOG_DIR}/cloudflared.log"
    echo "$line"
  done
else
  log "Starting Cloudflare quick tunnel → http://127.0.0.1:${PORT}"
  log "Tip: use a custom URL — see docs/CUSTOM-URL.md"
  cloudflared tunnel --url "http://127.0.0.1:${PORT}" 2>&1 | while IFS= read -r line; do
    echo "$line" >> "${LOG_DIR}/cloudflared.log"
    capture_tunnel_url "$line"
    echo "$line"
  done
fi

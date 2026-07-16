#!/usr/bin/env bash
set -euo pipefail

ROOT="$(cd "$(dirname "$0")/.." && pwd)"
RUNTIME_ROOT="${HOME}/AegisServer"
if [[ -d "${RUNTIME_ROOT}/deploy/macos" ]]; then
  URL_FILE="${RUNTIME_ROOT}/deploy/macos/public-url.txt"
else
  URL_FILE="${ROOT}/deploy/macos/public-url.txt"
fi
LOG_DIR="${HOME}/Library/Logs/Aegis"

echo "=== Aegis Mac Server Status ==="
echo ""

if launchctl print "gui/${UID}/com.aegis.server" &>/dev/null; then
  echo "LaunchAgent: running (com.aegis.server)"
else
  echo "LaunchAgent: NOT loaded — run ./scripts/mac-server-install.sh"
fi

echo ""
if curl -sf "http://127.0.0.1:8080/health" >/dev/null 2>&1; then
  echo "Keyserver:   OK (http://127.0.0.1:8080)"
  curl -s "http://127.0.0.1:8080/health"
  echo ""
else
  echo "Keyserver:   DOWN"
fi

echo ""
if [[ -f "$URL_FILE" ]]; then
  echo "Public URL (share with colleagues):"
  echo "  $(cat "$URL_FILE")"
else
  echo "Public URL:  not ready yet — tunnel still starting"
  echo "  Check: tail -f ${LOG_DIR}/cloudflared.log"
  grep -oE 'https://[a-z0-9-]+\.trycloudflare\.com' "${LOG_DIR}/cloudflared.log" 2>/dev/null | tail -1 || true
fi

echo ""
echo "Logs: ${LOG_DIR}/"
echo ""
echo "Colleague test:"
echo "  1. Both open the Public URL above"
echo "  2. Host laptop: Create Invite → Connect Peer → send offer JSON to Guest"
echo "  3. Guest laptop: open guest link → paste offer → send answer back"
echo "  4. Chat when security panel is green"

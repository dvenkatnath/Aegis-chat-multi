#!/usr/bin/env bash
set -euo pipefail

PLIST_DST="${HOME}/Library/LaunchAgents/com.aegis.server.plist"
LAUNCHER="${HOME}/Library/Application Support/Aegis/mac-server-daemon.sh"
LOG_DIR="${HOME}/Library/Logs/Aegis"
PID_FILE="${LOG_DIR}/keyserver.pid"

launchctl bootout "gui/${UID}/com.aegis.server" 2>/dev/null || true
rm -f "$PLIST_DST"

if [[ -f "$PID_FILE" ]]; then
  kill "$(cat "$PID_FILE")" 2>/dev/null || true
  rm -f "$PID_FILE"
fi

pkill -f "cloudflared tunnel --url http://127.0.0.1:8080" 2>/dev/null || true
rm -f "$LAUNCHER"

echo "Aegis launch agent removed. Keyserver/tunnel processes stopped."

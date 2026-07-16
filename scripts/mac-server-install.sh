#!/usr/bin/env bash
# One-time setup: auto-start Aegis on Mac mini login/boot (launchd).
set -euo pipefail

ROOT="$(cd "$(dirname "$0")/.." && pwd)"
PLIST_DST="${HOME}/Library/LaunchAgents/com.aegis.server.plist"
LAUNCHER_DIR="${HOME}/Library/Application Support/Aegis"
LAUNCHER="${LAUNCHER_DIR}/mac-server-daemon.sh"
RUNTIME_ROOT="${HOME}/AegisServer"
LOG_DIR="${HOME}/Library/Logs/Aegis"

echo "==> Aegis Mac mini headless server install"
echo "    Project: ${ROOT}"

if [[ ! -f "${ROOT}/dist/cyberknot.js" ]]; then
  echo "ERROR: Build first: ./scripts/build.sh"
  exit 1
fi

if ! command -v cloudflared >/dev/null 2>&1; then
  echo "ERROR: Install cloudflared: brew install cloudflared"
  exit 1
fi

chmod +x "${ROOT}/scripts/mac-server-daemon.sh"
chmod +x "${ROOT}/scripts/mac-server-status.sh"
chmod +x "${ROOT}/scripts/mac-server-uninstall.sh"
mkdir -p "${LOG_DIR}" "${ROOT}/deploy/macos" "$LAUNCHER_DIR"

# macOS launchd blocks executing scripts/binaries on external drives (/Volumes/*).
# Mirror the runtime bundle to the internal disk for reliable auto-start.
if [[ "$ROOT" == /Volumes/* ]]; then
  echo "==> Project on external volume — syncing runtime to ${RUNTIME_ROOT}"
  mkdir -p "${RUNTIME_ROOT}"
  rsync -a --delete \
    --exclude 'node_modules' \
    --exclude 'data' \
    "${ROOT}/dist/" "${RUNTIME_ROOT}/dist/"
  rsync -a --delete \
    --exclude 'node_modules' \
    --exclude 'data' \
    "${ROOT}/server/" "${RUNTIME_ROOT}/server/"
  mkdir -p "${RUNTIME_ROOT}/deploy/macos"
  if [[ -f "${ROOT}/deploy/macos/aegis-server.env" ]]; then
    cp "${ROOT}/deploy/macos/aegis-server.env" "${RUNTIME_ROOT}/deploy/macos/"
  else
    touch "${RUNTIME_ROOT}/deploy/macos/aegis-server.env"
    chmod 600 "${RUNTIME_ROOT}/deploy/macos/aegis-server.env"
  fi
  if [[ -f "${ROOT}/deploy/macos/cloudflared.yml" ]]; then
    cp "${ROOT}/deploy/macos/cloudflared.yml" "${RUNTIME_ROOT}/deploy/macos/"
  fi
  AEGIS_ROOT="${RUNTIME_ROOT}"
  echo "    (Re-run install after rebuilding dist/ to refresh the mirror)"
else
  AEGIS_ROOT="${ROOT}"
fi

cp "${ROOT}/scripts/mac-server-daemon.sh" "$LAUNCHER"
chmod +x "$LAUNCHER"

# Generate plist — launcher lives on internal disk
cat > "$PLIST_DST" <<EOF
<?xml version="1.0" encoding="UTF-8"?>
<!DOCTYPE plist PUBLIC "-//Apple//DTD PLIST 1.0//EN" "http://www.apple.com/DTDs/PropertyList-1.0.dtd">
<plist version="1.0">
<dict>
  <key>Label</key>
  <string>com.aegis.server</string>
  <key>ProgramArguments</key>
  <array>
    <string>/bin/bash</string>
    <string>${LAUNCHER}</string>
  </array>
  <key>RunAtLoad</key>
  <true/>
  <key>KeepAlive</key>
  <true/>
  <key>StandardOutPath</key>
  <string>${LOG_DIR}/launchd-stdout.log</string>
  <key>StandardErrorPath</key>
  <string>${LOG_DIR}/launchd-stderr.log</string>
  <key>EnvironmentVariables</key>
  <dict>
    <key>PATH</key>
    <string>/opt/homebrew/bin:/usr/local/bin:/usr/bin:/bin</string>
    <key>AEGIS_ROOT</key>
    <string>${AEGIS_ROOT}</string>
  </dict>
</dict>
</plist>
EOF

launchctl bootout "gui/${UID}/com.aegis.server" 2>/dev/null || true
launchctl bootstrap "gui/${UID}" "$PLIST_DST"
launchctl enable "gui/${UID}/com.aegis.server"
launchctl kickstart -k "gui/${UID}/com.aegis.server"

echo ""
echo "==> Installed and started."
echo "    Runtime root: ${AEGIS_ROOT}"
echo "    Wait ~20 seconds, then run:"
echo "      ./scripts/mac-server-status.sh"
echo ""
echo "==> Mac mini tips (do once):"
echo "    • System Settings → Energy → prevent sleep when plugged in"
echo "    • Keep Mac mini on power and network"
echo ""
echo "==> Share public URL with colleagues (from status script)."

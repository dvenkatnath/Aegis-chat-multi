#!/usr/bin/env bash
# Print (and optionally create) the public Railway URL for Aegis-chat-multi.
set -euo pipefail

ROOT="$(cd "$(dirname "$0")/.." && pwd)"
PROJECT_NAME="${RAILWAY_PROJECT:-Aegis-chat-multi}"
SERVICE_NAME="${RAILWAY_SERVICE:-}"

cd "$ROOT"

if ! command -v railway >/dev/null 2>&1; then
  echo "ERROR: Railway CLI not installed." >&2
  exit 1
fi

if ! railway whoami >/dev/null 2>&1; then
  echo "ERROR: Run: railway login" >&2
  exit 1
fi

railway link --project "$PROJECT_NAME" >/dev/null 2>&1 || true

echo "==> Railway project: ${PROJECT_NAME}"
railway status 2>/dev/null || true
echo ""

DOMAINS="$(railway domain list --json 2>/dev/null || echo '[]')"
if [ "$DOMAINS" = "[]" ] || [ -z "$DOMAINS" ] || [ "$DOMAINS" = "null" ]; then
  echo "==> No public domain yet. Generating one now..."
  if [ -n "$SERVICE_NAME" ]; then
    railway domain --service "$SERVICE_NAME" --json
  else
    railway domain --json
  fi
else
  echo "==> Existing domains:"
  railway domain list
fi

echo ""
echo "Open the URL shown above (https://....up.railway.app)"
echo "Then set this variable in Railway → Variables:"
echo '  CORS_ORIGIN=https://${{RAILWAY_PUBLIC_DOMAIN}}'
echo '  NODE_ENV=production'

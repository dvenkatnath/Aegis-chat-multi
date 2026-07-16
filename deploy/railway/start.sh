#!/bin/sh
set -eu

export PORT="${PORT:-8080}"
export STATIC_DIR="${STATIC_DIR:-/app/dist}"
export DATA_DIR="${DATA_DIR:-/app/data}"
export NODE_ENV="${NODE_ENV:-production}"

mkdir -p "$DATA_DIR"

if [ -z "${CORS_ORIGIN:-}" ]; then
  if [ -n "${RAILWAY_PUBLIC_DOMAIN:-}" ]; then
    export CORS_ORIGIN="https://${RAILWAY_PUBLIC_DOMAIN}"
  elif [ -n "${PUBLIC_URL:-}" ]; then
    export CORS_ORIGIN="${PUBLIC_URL}"
  else
    # Bootstrap: Railway does not set RAILWAY_PUBLIC_DOMAIN until a domain is
    # generated. Run in dev mode so the service stays healthy for the first deploy.
    export NODE_ENV=development
    echo "WARN: No public domain yet. Generate one in Railway → Networking, then set CORS_ORIGIN=https://\${{RAILWAY_PUBLIC_DOMAIN}}"
  fi
fi

if [ -z "${LOG_SECRET:-}" ] || [ "${#LOG_SECRET}" -lt 32 ]; then
  export LOG_SECRET="$(openssl rand -hex 32)"
fi

# Railway has no coturn sidecar — use Metered Open Relay (free tier) when TURN is unset.
# Override with your own TURN_SECRET + TURN_HOST for self-hosted coturn.
if [ -z "${TURN_SECRET:-}" ] && [ -z "${TURN_USERNAME:-}" ]; then
  export TURN_SECRET="openrelayprojectsecret"
  export TURN_HOST="staticauth.openrelay.metered.ca"
  export TURN_PORT="80"
  echo "INFO: Using Metered Open Relay TURN (staticauth.openrelay.metered.ca). Set TURN_SECRET/TURN_HOST to override."
fi

exec node keyserver.js

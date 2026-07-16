#!/bin/sh
set -eu

export PORT="${PORT:-8080}"
export STATIC_DIR="${STATIC_DIR:-/app/dist}"
export DATA_DIR="${DATA_DIR:-/app/data}"
export NODE_ENV="${NODE_ENV:-production}"

mkdir -p "$DATA_DIR"

if [ -z "${CORS_ORIGIN:-}" ] && [ -n "${RAILWAY_PUBLIC_DOMAIN:-}" ]; then
  export CORS_ORIGIN="https://${RAILWAY_PUBLIC_DOMAIN}"
fi

if [ -z "${LOG_SECRET:-}" ] || [ "${#LOG_SECRET}" -lt 32 ]; then
  export LOG_SECRET="$(openssl rand -hex 32)"
fi

exec node keyserver.js

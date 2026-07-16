# Railway deployment — Aegis-chat-multi

Single-service deploy: Node keyserver serves the static app (`dist/`) plus `/api/*` and `/ws`.

## Prerequisites

- Railway project: **Aegis-chat-multi**
- GitHub repo: `dvenkatnath/Aegis-chat-multi`
- Authenticated CLI: `railway login`

## First deploy

```bash
cd /path/to/Aegis-chat-multi
railway link --project Aegis-chat-multi
railway up --detach -m "Deploy Aegis chat multi-guest build"
```

## Recommended variables

| Variable | Value |
|----------|-------|
| `NODE_ENV` | `production` |
| `CORS_ORIGIN` | `https://${{RAILWAY_PUBLIC_DOMAIN}}` (or your custom domain) |
| `LOG_SECRET` | `openssl rand -hex 32` |
| `TURN_SECRET` | `openssl rand -hex 32` (optional; STUN-only if unset) |
| `TURN_PUBLIC_HOST` | Your public hostname (optional) |

`start.sh` auto-sets `CORS_ORIGIN` from `RAILWAY_PUBLIC_DOMAIN` when omitted.

## Persistent key transparency data

Attach a Railway volume at `/app/data` so `keys.json` survives redeploys.

## Notes

- TURN/coturn from `deploy/docker-compose.yml` is **not** included — Railway does not support host-network coturn. Use STUN via `/api/ice-config`, or add an external TURN service later.
- Health check: `GET /health`

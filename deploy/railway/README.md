# Railway deployment — Aegis-chat-multi

Single-service deploy: Node keyserver serves the static app (`dist/`) plus `/api/*` and `/ws`.

## Prerequisites

- Railway project: **Aegis-chat-multi**
- GitHub repo: `dvenkatnath/Aegis-chat-multi`
- Authenticated CLI: `railway login`

## Get your public link

Railway does **not** assign a URL automatically. After the first successful deploy:

### Option A — Dashboard (easiest)

1. Open https://railway.com/dashboard → **Aegis-chat-multi**
2. Click your **service** (the one connected to GitHub)
3. Go to **Settings** → **Networking** → **Public Networking**
4. Click **Generate Domain**
5. Copy the `https://something.up.railway.app` URL

### Option B — CLI

```bash
cd /path/to/Aegis-chat-multi
railway login
./scripts/railway-get-link.sh
```

### After you have the domain

In Railway → **Variables**, set:

| Variable | Value |
|----------|-------|
| `CORS_ORIGIN` | `https://${{RAILWAY_PUBLIC_DOMAIN}}` |
| `NODE_ENV` | `production` |

Redeploy (or wait for auto-redeploy). Then open your `https://....up.railway.app` link.

Verify:

```bash
curl -s https://YOUR-DOMAIN.up.railway.app/health
```


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

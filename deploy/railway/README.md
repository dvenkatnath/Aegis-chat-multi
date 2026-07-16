# Railway deployment — Aegis-chat-multi

Single-service deploy: Node keyserver serves the static app (`dist/`) plus `/api/*` and `/ws`.

## Prerequisites

- Railway project: **Aegis-chat-multi**
- GitHub repo: `dvenkatnath/Aegis-chat-multi`
- Authenticated CLI: `railway login`

## Generate Domain is disabled?

Railway only enables **Generate Domain** when the service is **deployed and listening on HTTP**. Check these in order:

### 1. Confirm the deployment is healthy
- Open your **service** (not just the project) → **Deployments**
- Latest deploy must be **Success** / green
- Open **Logs** — you should see: `Aegis keyserver listening on 0.0.0.0:XXXX`
- If logs show `FATAL: CORS_ORIGIN` or crash loops, redeploy after the latest Git push

### 2. Remove TCP Proxy (most common UI blocker)
If a **TCP Proxy** exists under **Settings → Networking**, **Generate Domain** is hidden.
- Click the **trash icon** next to the TCP Proxy to remove it
- Then **Generate Domain** should appear

### 3. Do not set PORT manually (unless required)
- In **Variables**, **delete** any `PORT` variable you added yourself
- Let Railway inject `PORT` automatically
- Our app reads `process.env.PORT` at runtime

### 4. Try the canvas prompt instead
Sometimes the button in Settings is disabled, but the **service tile** on the project canvas shows **"Generate Domain"** after a healthy deploy. Click the service box on the canvas and look for that prompt.

### 5. Use CLI as fallback
```bash
railway login
cd /path/to/Aegis-chat-multi
railway link --project Aegis-chat-multi
railway domain --json
```

### 6. India ISP note
Some Indian ISPs block `*.up.railway.app`. If the domain generates but won't load, add a **Custom Domain** via Cloudflare instead.


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
| `TURN_SECRET` | coturn `static-auth-secret`, or `openrelayprojectsecret` for [Open Relay](https://www.metered.ca/tools/openrelay/) |
| `TURN_HOST` | e.g. `staticauth.openrelay.metered.ca` (Railway default via `start.sh`) |
| `TURN_PORT` | `80` or `3478` depending on provider |
| `TURN_USERNAME` / `TURN_CREDENTIAL` | Optional static creds instead of HMAC (Metered dashboard) |
| `TURN_PUBLIC_HOST` | Hostname for static creds when different from `TURN_HOST` |

`start.sh` auto-sets `CORS_ORIGIN` from `RAILWAY_PUBLIC_DOMAIN` when omitted.

## Persistent key transparency data

Attach a Railway volume at `/app/data` so `keys.json` survives redeploys.

## Notes

- TURN/coturn from `deploy/docker-compose.yml` is **not** included on Railway. **`start.sh` defaults to Metered Open Relay** (`staticauth.openrelay.metered.ca`) so ICE can traverse strict NAT/firewalls. Override with your own TURN env vars for production.
- Health check: `GET /health`

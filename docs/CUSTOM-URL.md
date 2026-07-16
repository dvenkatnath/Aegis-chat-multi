# Custom public URL for Aegis

The random `https://something.trycloudflare.com` URLs **cannot be renamed**. They change on every restart and Cloudflare assigns the name.

To get a **fixed, custom URL** like `https://aegis.yourdomain.com`, use a **named Cloudflare Tunnel** with a domain you control.

---

## Requirements

| Item | Cost |
|------|------|
| Domain name (e.g. `yourdomain.com`) | ~$10/year |
| Cloudflare account | Free |
| Domain DNS on Cloudflare | Free |

---

## Setup (one-time, ~15 minutes)

### 1. Add domain to Cloudflare

1. Sign up at [cloudflare.com](https://cloudflare.com)
2. Add your domain → switch nameservers at your registrar to Cloudflare's

### 2. Create a named tunnel (on the Mac mini)

```bash
cloudflared tunnel login
cloudflared tunnel create aegis-chat
```

Note the tunnel UUID printed (e.g. `a1b2c3d4-...`).

### 3. Route DNS to the tunnel

Pick a hostname, e.g. `aegis.yourdomain.com`:

```bash
cloudflared tunnel route dns aegis-chat aegis.yourdomain.com
```

### 4. Create config file

```bash
cp deploy/macos/cloudflared.yml.example deploy/macos/cloudflared.yml
```

Edit `deploy/macos/cloudflared.yml`:

```yaml
tunnel: aegis-chat
credentials-file: /Users/YOUR_USERNAME/.cloudflared/TUNNEL_UUID.json

ingress:
  - hostname: aegis.yourdomain.com
    service: http://127.0.0.1:8080
  - service: http_status:404
```

Replace `YOUR_USERNAME` and `TUNNEL_UUID` with your values.

### 5. Set the public URL in env

Edit `deploy/macos/aegis-server.env` (create if missing):

```bash
PUBLIC_URL=https://aegis.yourdomain.com
```

### 6. Re-install the headless server

```bash
./scripts/mac-server-install.sh
./scripts/mac-server-status.sh
```

Status should show your custom URL.

---

## Verify

```bash
curl -s https://aegis.yourdomain.com/health
```

Share `https://aegis.yourdomain.com` with colleagues — it stays the same across reboots.

---

## Without a domain?

| Option | Custom name? |
|--------|----------------|
| `trycloudflare.com` (current) | No — random only |
| Cloudflare named tunnel + domain | **Yes** |
| Railway `*.up.railway.app` | Partially — Railway assigns subdomain |
| ngrok free | Random |
| ngrok paid | Custom subdomain |

---

## Revert to quick tunnel

```bash
rm deploy/macos/cloudflared.yml
# Remove PUBLIC_URL from deploy/macos/aegis-server.env
./scripts/mac-server-install.sh
```

Random `trycloudflare.com` URL will return.

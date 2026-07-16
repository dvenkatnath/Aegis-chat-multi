# Colleague test guide (remote laptops)

The Mac mini runs the server only. **Nobody needs to use the Mac mini** for testing.

## What you need from the admin

One HTTPS URL, for example:

```
https://something.trycloudflare.com
```

The admin runs `./scripts/mac-server-status.sh` on the Mac mini to get this URL.

> **Note:** If the Mac mini reboots, the URL may change. Ask the admin for the new URL.

---

## Roles

| Person | Role | Laptop |
|--------|------|--------|
| Colleague A | **Host** | Creates room, sends offer |
| Colleague B | **Guest** | Joins via invite link |

Use **Chrome** or **Firefox**. Avoid Slack/Teams in-app browsers.

---

## Step-by-step

### 1 — Host (Colleague A)

1. Open the **public URL** from the admin.
2. Click **Create Invite**.
3. Click **Copy Guest Link** → send to Guest via **Signal** or **WhatsApp** (not Slack/email).
4. Click **Connect Peer**.
5. Copy the **Offer JSON** from the signal box → send to Guest (separate message from the invite link).

### 2 — Guest (Colleague B)

1. Open the **guest link** from Host (in a new browser tab).
2. Paste the **Offer JSON** into the signal area.
3. Click **Process Signal**.
4. Copy the **Answer JSON** → send back to Host.

### 3 — Host (Colleague A)

1. Paste the **Answer JSON**.
2. Click **Process Signal**.
3. Wait until the **security panel** shows green checks on both sides.

### 4 — Chat

Send messages both ways. Optionally try a small file.

---

## Troubleshooting

| Problem | Fix |
|---------|-----|
| Page won't load | Ask admin: `./scripts/mac-server-status.sh` — server may be down |
| "Signal authentication failed" | Wrong invite link or mixed-up offer/answer — click **Reset** and redo |
| Stuck on ICE / no data channel | Try without VPN; both on same or different networks should work with STUN |
| Guest link uses wrong host | Host must open the **public URL**, not localhost, before creating invite |

---

## What the Mac mini does

- Serves the app (HTML + WASM)
- Key transparency API
- ICE config (STUN)

It does **not** see chat messages (P2P encrypted between laptops).

# Aegis Secure Chat

End-to-end encrypted peer-to-peer chat with CyberKnot (AES-256), WebRTC, invite-only links, key transparency, and optional TURN relay.

## What's new in this build — automated waiting room + up to 5 simultaneous guests

The manual copy-paste of Offer/Answer JSON is now automated through a
blind relay (`/ws` on the keyserver). The relay only forwards opaque
blobs that a human was already copying by hand — it never sees a
decryption key and cannot read message content.

**Topology:** this build uses a host-hub star, not a full mesh. Every
guest connects directly and independently to the host — its own ECDH
handshake, its own safety-number check, its own encrypted channel. When
a guest sends a message, the host (a legitimate participant, not the
relay) re-encrypts it once per other connected guest and forwards it.
The untrusted `keyserver.js` relay is never part of that — it only ever
sees encrypted signaling blobs during connection setup.

**What changed:** `server/keyserver.js`'s relay now admits up to 5
simultaneously active guests instead of exactly one. `index.html`'s
entire connection-management layer — fingerprint verification,
security status display, encryption/decryption, message sending — was
restructured from a single shared connection to an explicit per-peer
state object, so the host can hold several independent encrypted
connections without one guest's asynchronous events touching another's
state.

**What did NOT change, verified by checksum and function-level diff:**
`cyberknot.wasm`, `cyberknot.js`, every C source under `src/`, and the
ECDH P-256 + HKDF key derivation functions in `index.html` are
byte-for-byte identical to before this change. See `CHANGES.md` for the
exact hashes and the method to re-verify them yourself.

**Testing boundary — read this before trusting this build with anything
sensitive.** The relay/signaling protocol (waiting room, admit/deny,
opaque signal routing for up to 5 simultaneous guests) is verified by
two automated test suites that start the real server and drive it with
real WebSocket connections — 36 assertions, all passing, and you can
rerun them yourself. The browser-side WebRTC and Web Crypto layer
(the actual encrypted connections, the safety-number UI, the host-hub
message fan-out) was written and statically reviewed with the same
care, but this environment has no browser to execute it in. It has
**not** been exercised end-to-end in a real browser. Test it yourself
with real browser tabs before relying on it for anything sensitive.

## Quick start (local test)

**Requirements:** Emscripten (`emcc`), Node.js 18+, OpenSSL

```bash
chmod +x scripts/build.sh scripts/run-local.sh
./scripts/run-local.sh
```

Open **http://localhost:8080** in several tabs (or several browsers/devices):

1. **Host:** Click Create Invite → Copy Guest Link → send it to each guest
2. **Guest:** open the guest link → enter a name → wait in the waiting room
3. **Host:** sees each guest appear → click Admit (repeat for up to 5 guests)
4. **Each pair (host + guest):** compare the 6-digit safety code shown on
   screen → click Confirm on each side
5. Chat — messages you send go to every connected guest; a message from
   any guest is relayed by the host to every other connected guest too

## Testing this build

Two automated tests exercise the relay directly (not the crypto, which
is unchanged and covered by the existing workflow):

```bash
cd server && npm install --omit=dev && cd ..
node scripts/e2e-test/test-relay.cjs              # basic protocol, 12 assertions
node scripts/e2e-test/test-relay-multiguest.cjs   # up to 5 simultaneous guests, 24 assertions
```

The multi-guest test starts a real server instance and asserts: five
guests can be admitted simultaneously, a sixth is correctly rejected at
the cap, signal data addressed to one guest never reaches another, a
guest leaving doesn't disturb the others' connections, and a new guest
can join once a slot frees up.

## Production build

```bash
./scripts/build.sh
```

Output goes to `dist/` with WASM compiled and **Subresource Integrity** hash on `cyberknot.js`.

## Cloud deploy (Docker)

**Requirements:** A cloud VM with ports **80**, **443**, **3478** (UDP/TCP), and **49152–49252** (UDP for TURN relay) open.

```bash
# 1. Build static assets on your machine or CI
./scripts/build.sh

# 2. Configure environment
cd deploy
cp .env.example .env
# Edit .env: DOMAIN, ACME_EMAIL, TURN_SECRET, LOG_SECRET, TURN_PUBLIC_HOST

# 3. Point DNS A record: chat.yourdomain.com → server IP

# 4. Start stack
docker compose up -d --build
```

### Stack components

| Service | Role |
|---------|------|
| **Caddy** | HTTPS (Let's Encrypt), serves `dist/`, proxies `/api/*` |
| **keyserver** | Key transparency log, ephemeral TURN credentials |
| **coturn** | TURN relay for strict NAT / cross-network |

### Generate secrets

```bash
openssl rand -hex 32   # TURN_SECRET and LOG_SECRET
```

Use the **same** `TURN_SECRET` in `.env` and coturn config (handled automatically by compose).

## Security features

| Feature | Description |
|---------|-------------|
| Invite-only | 256-bit room secret in URL `#fragment` (never sent to HTTP server) |
| HMAC signaling | Tampered offer/answer rejected |
| ECDH P-256 + HKDF | Unique session key per chat |
| CyberKnot AES-256 | Application-layer encryption (CTR mode + HMAC) |
| Auto DTLS fingerprint | Live cert verified against SDP |
| Key transparency | Host/guest ECDH keys registered and verified via `/api/v1/keys` |
| SRI | Built `cyberknot.js` integrity-checked in production `dist/` |
| TURN + STUN | `/api/ice-config` returns time-limited TURN credentials |
| Waiting room relay | `/ws` forwards signal data only between the host and up to 5 admitted guests; holds no keys, persists nothing to disk |
| Safety number | 6-digit code from both ECDH public keys per host/guest pair, confirmed by both humans before that pair unlocks — catches a relay that substituted a key |

### Cryptography note

- **AES-256** is used for message encryption (there is no AES-512 standard).  
- HKDF derives 512 **bits** of key material, encoded as a hex password for CyberKnot — not “AES-512”.
- The waiting-room relay automates transport of data a human previously copy-pasted by hand (SDP, ECDH public keys). It does not have access to anything that wasn't already flowing through an even less trusted channel (clipboard, WhatsApp, email) in the manual flow, and it never receives the derived session key.

## API (keyserver)

| Endpoint | Method | Purpose |
|----------|--------|---------|
| `/health` | GET | Health check |
| `/api/ice-config` | GET | STUN + ephemeral TURN credentials |
| `/api/v1/keys` | POST | Register host/guest ECDH public key |
| `/api/v1/keys/:inviteId/:role` | GET | Fetch registered key |
| `/api/v1/transparency` | GET | Append-only transparency log |
| `/ws` | WebSocket | Waiting room presence, admit/deny, opaque signal relay |

## Project layout

```
AegisChat/
  index.html          # App UI (dev)
  ice-config.js       # Fallback STUN for dev without keyserver
  src/                # CyberKnot C sources + bridge.c
  scripts/build.sh    # Compile WASM → dist/
  scripts/run-local.sh # Local full stack
  scripts/e2e-test/test-relay.cjs # Automated test — basic waiting-room protocol
  scripts/e2e-test/test-relay-multiguest.cjs # Automated test — up to 5 simultaneous guests
  server/             # Keyserver + static host (production) + /ws relay
  deploy/             # Docker Compose + Caddy + coturn
  dist/               # Build output (gitignored)
```

## Firewall checklist (cloud VM)

- [ ] TCP 80, 443 (HTTPS)  
- [ ] UDP/TCP 3478 (TURN)  
- [ ] UDP 49152–49252 (TURN relay ports — match `deploy/coturn/turnserver.conf`)  
- [ ] DNS A record for `DOMAIN`  

## What this does not include

- **Formal third-party security audit** — recommended before high-stakes production use.

# Changes in this build — automated waiting room + handshake

## Summary

Replaces manual copy-paste of Offer/Answer JSON with an automated
handshake through a blind relay, adds a waiting room with host
admit/deny, and adds a Short Authentication String ("safety number")
that both participants confirm before chat unlocks. This closes the
verification gap that manual copy-paste accidentally provided, while
removing the tedious part.

## What changed

- `server/keyserver.js` — added a WebSocket relay at `/ws`: presence
  notifications, admit/deny, and opaque signal forwarding between an
  admitted host/guest pair. In-memory only, nothing persisted to disk.
- `server/package.json` — added the `ws` dependency (v8.18.0).
- `index.html` — added a guest name-entry modal, a waiting-room panel
  with Admit/Deny, a safety-number display and confirm step, and the
  WebSocket client that drives all of it. The manual Offer/Answer
  textarea remains, collapsed under "Advanced / manual signaling".
- `scripts/e2e-test/test-relay.cjs` — new automated test for the relay
  protocol (see below).

## What did NOT change — verified, not assumed

Every cryptographic artifact and function is byte-for-byte or
function-for-function identical to the version in this backup before
these changes:

| Artifact | MD5 |
|---|---|
| `cyberknot.wasm` | `e782ab299f833b969333f1c009f043db` |
| `cyberknot.js` | `0efbb2b2356d91a3ec988666ba4af06e` |
| `src/aes.c` | `5798ad75c7e5709f674939382cb3c9c5` |
| `src/Hmac.c` | `a0f003f1a9016016022650cab8114db2` |
| `src/HmacSha256.c` | `57429a1847f3bf61500079dec0df59ab` |
| `src/SHA256.c` | `89983c55ecfa43e23a47d4215e85102d` |
| `src/EncryptionToolsApi.c` | `39e7dd69a5da6b43aa6bfadbc2392992` |
| `src/CryptoRng.c` | `cc74d55df9480ef89665df1f58419aba` |
| `src/bridge.c` | `743b305d3082e85df9ad1376f33044b3` |

Re-verify yourself: `md5sum cyberknot.wasm cyberknot.js src/*.c` and
compare against the table above.

The following JavaScript functions in `index.html` were extracted and
diffed character-for-character against the pre-change version —
`generateEcdhKeyPair`, `exportPublicKeyB64`, `importPublicKeyB64`,
`deriveSessionPassword`, `hmacSign`, `hmacVerify` — all identical.
The `Module._bridge_encrypt_b64` / `_bridge_decrypt_b64` /
`_bridge_set_session_key` / `_bridge_clear_session_key` call sites are
also unchanged.

## Why this is safe to automate

The relay only transports data that a human was already copying and
pasting by hand: SDP offer/answer (which contains network paths and a
DTLS fingerprint, not a secret) and exported ECDH public keys (public
by definition). None of this is the room secret and none of it is the
derived session key — those never touch the server, exactly as
before. The existing DTLS fingerprint cross-check (already present in
the app) plus the new safety-number check together catch a relay that
tried to substitute a key during the automated exchange — a stronger
position than the manual flow, which had no equivalent verification
step at all beyond trusting the copy-paste itself.

## Testing performed

`node scripts/e2e-test/test-relay.cjs` starts a real instance of the
modified server and asserts, against real WebSocket connections:

1. Host registration is acknowledged.
2. A guest's join produces a presence notification to the host with
   the correct name and connection id.
3. A second guest also appears in presence.
4. Admitting the first guest sends that guest an `admitted` event.
5. The second, unadmitted guest is denied and its connection closed.
6. Signal data sent host → guest reaches only the admitted guest,
   payload byte-identical to what was sent.
7. The denied second guest never receives that signal data.
8. The guest's answer reaches the host tagged with the correct
   connection id, payload byte-identical.
9. A malformed invite id is rejected with an error rather than
   silently accepted.
10. Flooding the relay with messages triggers the rate limiter.
11. Closing the host's connection tears down the room; a late joiner
    gets a clear "not found or expired" error rather than reaching an
    orphaned room.

All 12 assertions pass. This is a protocol-level test against the real
server code, not a mock.

## What was not built in this pass

Multi-party sessions (3 or more participants) are out of scope for
this change. The current codebase — before and after this change —
supports exactly one host and one guest. A group session mechanism
(the MLS/OpenMLS approach discussed separately) is materially new
engineering and was not attempted here.

## Update — multi-guest support (up to 5 simultaneous guests)

### What changed

- `server/keyserver.js` — the relay now tracks `admittedConnIds` as a
  set (was a single value), capped at `MAX_ACTIVE_GUESTS = 5`. Admitting
  one guest no longer auto-denies others; each can be admitted
  independently up to the cap. Signal routing requires an explicit
  target `connId` on every message rather than falling back to a single
  slot.
- `index.html` — the entire connection-management layer was
  restructured from module-global singleton state (`peerConnection`,
  `dataChannel`, `ecdhKeyPair`, `sessionKeyReady`, `fingerprintVerified`,
  and related variables) to an explicit per-peer state object stored in
  a `Map<connId, peerState>`. This was necessary, not cosmetic: the old
  globals were closed over by asynchronous WebRTC event callbacks (ICE
  candidates, data channel open/message events), and swapping a shared
  global's contents between guests would have let one guest's
  in-flight callback read or corrupt another guest's state under real
  concurrency.
- The host's ECDH keypair is generated once and reused across every
  guest connection — this is cryptographically sound because the
  shared secret is still unique per guest (it depends on the guest's
  own distinct public key too), and it means no change was needed to
  the existing key-transparency endpoint or its slot scheme.
- CyberKnot's WASM module holds exactly one active session key at a
  time (see `src/bridge.c`). The correct peer's key is now installed
  immediately before each encrypt/decrypt call, synchronously with no
  `await` in between — safe because JS is single-threaded and each
  encrypt call generates its own fresh random IV internally rather than
  relying on a shared counter, so switching keys between peers
  introduces no IV-reuse risk.
- Topology is a host-hub star: guests connect only to the host, not to
  each other. When the host receives a message from one guest, it
  re-encrypts it once per other connected guest and forwards it. This
  is an act of a legitimate call participant relaying to other
  legitimate participants — it does not touch or weaken the
  opaque-relay property of `keyserver.js`, which still never holds a
  key or sees plaintext.
- The security/safety-number panel changed from a single fixed display
  to a dynamically rendered list, one card per connected peer.
- Added `scripts/e2e-test/test-relay-multiguest.cjs`.

### What did NOT change — re-verified after this update

The same checksum table from the section above still applies —
`cyberknot.wasm`, `cyberknot.js`, and every C source file are
byte-identical, re-verified after this round of changes. The six
key-derivation JavaScript functions were re-diffed function-by-function
against the original upload and remain identical.

### Testing performed for this update

`node scripts/e2e-test/test-relay-multiguest.cjs` — 24 assertions, all
passing: five guests admitted simultaneously, a sixth correctly
rejected at the cap, signal data addressed to one guest never reaching
another, a guest disconnecting without disturbing the others, and a new
guest able to join once a slot frees up.

`node scripts/e2e-test/test-relay.cjs` — updated to reflect the new
intended behavior (a second guest is no longer auto-denied when the
first is admitted) and re-run — 12 assertions, all passing.

### The honest boundary of what was verified

The relay/signaling protocol was tested against the real running
server with real WebSocket clients, exactly as described above. The
browser-side WebRTC and Web Crypto layer — the actual per-peer
encrypted connections, the per-peer safety-number computation and UI,
and the host-hub message fan-out logic — was written and statically
reviewed with the same care (every call site checked for consistent
argument passing, every old global reference searched for and
confirmed removed, every function this update touched confirmed to
exist), but this sandbox has no browser and cannot execute
`RTCPeerConnection` or the WASM module. **It has not been exercised
end-to-end with real browser tabs.** Test it with real browsers before
relying on it for anything sensitive, particularly the multi-guest fan
out and the per-peer WASM re-keying under real concurrent traffic.

## How to test it yourself, end to end

```bash
cd server && npm install --omit=dev && cd ..
./scripts/run-local.sh
```

Open several browser tabs at `http://localhost:8080` — one for the
host, one for each guest (up to 5). In the host tab, click Create
Invite and copy the guest link. In each guest tab, open that link,
enter a name, and wait. Back in the host tab, each name appears in the
waiting room — click Admit for each one you want to bring in. Every
admitted host/guest pair will automatically complete its own handshake
and display its own 6-digit safety code. Confirm on both sides of each
pair that the codes match, click Confirm, and that pair's channel
unlocks. Send a message from any tab — the host relays it to everyone
else, so all connected guests see it.

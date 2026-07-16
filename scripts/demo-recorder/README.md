# Aegis Product Demo Video Recorder

Automates the full host/guest WebRTC flow and saves a **WebM screen recording** you can share as a mock product demo.

## Quick start

**Terminal 1 — start the app:**

```bash
cd /path/to/aegischat-hardened
LOG_SECRET="$(openssl rand -hex 32)" ./scripts/run-local.sh
```

**Terminal 2 — record the demo:**

```bash
cd scripts/demo-recorder
npm install
npx playwright install chromium
npm run record
```

Video is saved to `scripts/demo-recorder/demo-output/`:

| File | Description |
|------|-------------|
| `aegis-product-demo.mp4` | **Side-by-side Host + Guest** (1920×900, share-ready) |
| `aegis-demo-*-split.webm` | Same layout, WebM source |
| `aegis-demo-*-host.webm` / `-guest.webm` | Individual panes |

Default mode is **split** (side-by-side). Requires `ffmpeg` for composition.

### On-screen callouts

The recorder injects:
- **HOST / GUEST** badges (top-right)
- **Step callouts** (bottom panel) explaining each phase
- **Orange highlight rings** on buttons and panels being used

### Intro slide + voiceover

Every full recording prepends a **branded intro slide** (~32 s) with **macOS text-to-speech** narration covering the security stack, then transitions into the side-by-side demo.

| File | Contents |
|------|----------|
| `aegis-product-demo.mp4` | **Intro + demo** (share this) |
| `aegis-product-demo-full.mp4` | Same as above |
| `aegis-demo-main-only.mp4` | Demo without intro |

Add intro to an existing demo only:

```bash
npm run intro
# or
node compose-intro.mjs demo-output/aegis-demo-main-only.mp4
```

Customize voice (Microsoft neural TTS — female American, natural):

```bash
# Default: en-US-JennyNeural (warm, conversational)
npm run intro

# Alternatives: en-US-AriaNeural, en-US-MichelleNeural
DEMO_VOICE=en-US-AriaNeural DEMO_VOICE_RATE=-10% npm run intro
```

Requires network for neural TTS. Falls back to macOS **Samantha** if offline.

Edit narration in `compose-intro.mjs` (`NARRATION` constant).

## Recording modes

| Command | What it records |
|---------|-----------------|
| `npm run record` | Host browser window (recommended for product demo) |
| `npm run record:host-only` | Same as above |
| `npm run record:split` | Host + guest windows (two files) |

Custom server URL:

```bash
node record-demo.mjs --base http://localhost:8080 --view host
```

## What the video shows

1. Host opens Aegis Secure Chat (WASM loads)
2. **Create Invite** → guest link generated
3. Guest opens invite link (second browser)
4. Host **Connect Peer** → Offer JSON
5. Guest **Process Signal** → Answer JSON
6. Host **Process Signal** → security panel turns green
7. Encrypted chat messages exchanged both ways
8. Hold on success screen (~6 s) for outro

Pacing is slowed (`slowMo` + pauses) so viewers can follow each step.

## Polish for sharing

Convert WebM → MP4 (better compatibility on LinkedIn / Slack):

```bash
ffmpeg -i demo-output/aegis-demo-*-host.webm \
  -c:v libx264 -preset slow -crf 20 -pix_fmt yuv420p \
  aegis-product-demo.mp4
```

Add a title card (optional):

```bash
ffmpeg -f lavfi -i color=c=0x020817:s=1440x900:d=3 \
  -vf "drawtext=text='Aegis Secure Chat':fontsize=48:fontcolor=white:x=(w-text_w)/2:y=(h-text_h)/2" \
  -c:v libx264 title.mp4

ffmpeg -i title.mp4 -i aegis-product-demo.mp4 -filter_complex "[0:v][1:v]concat=n=2:v=1:a=0" final-demo.mp4
```

## Manual recording (alternative)

If you prefer a hand-guided screencast:

1. Start `./scripts/run-local.sh`
2. Use **QuickTime → New Screen Recording** (macOS) or OBS
3. Follow the flow in the main [README](../../README.md):
   - Tab 1 (Host): Create Invite → Connect Peer → copy Offer
   - Tab 2 (Guest): open guest link → paste Offer → Process Signal → copy Answer
   - Tab 1: paste Answer → Process Signal → chat

## Troubleshooting

| Issue | Fix |
|-------|-----|
| `Server not healthy` | Start `run-local.sh` with a 32+ char `LOG_SECRET` |
| WASM timeout | Hard-refresh; ensure `dist/cyberknot.js` exists (`./scripts/build.sh`) |
| P2P channel timeout | Both tabs must stay open; retry on same machine (STUN direct) |
| Clipboard errors | Ignored — script copies via DOM, not clipboard |

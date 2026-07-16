#!/usr/bin/env node
/**
 * Slow-paced Aegis demo with on-screen callouts.
 * Signaling steps run quickly (WebRTC is timing-sensitive);
 * callouts explain before/after each phase.
 */

import { chromium } from 'playwright';
import { execSync } from 'child_process';
import fs from 'fs';
import path from 'path';
import { fileURLToPath } from 'url';

const __dirname = path.dirname(fileURLToPath(import.meta.url));
const OUT_DIR = path.join(__dirname, 'demo-output');

const args = process.argv.slice(2);
const viewMode = args.includes('--view')
  ? args[args.indexOf('--view') + 1] || 'split'
  : 'split';
const BASE = args.includes('--base')
  ? args[args.indexOf('--base') + 1]
  : process.env.DEMO_BASE || 'http://localhost:8080';

const SLOW_MO = 450;
const PAUSE = 1600;
const CALLOUT = 5500;
const HOLD = 4000;

const PANE_W = 960;
const PANE_H = 900;

function stamp() {
  return new Date().toISOString().replace(/[:.]/g, '-').slice(0, 19);
}

async function sleep(ms) {
  return new Promise((r) => setTimeout(r, ms));
}

async function injectDemoChrome(page, role) {
  await page.evaluate((roleLabel) => {
    if (document.getElementById('demo-chrome-styles')) return;
    const style = document.createElement('style');
    style.id = 'demo-chrome-styles';
    style.textContent = `
      #demo-role-badge {
        position: fixed; top: 12px; right: 12px; z-index: 99998;
        padding: 8px 16px; border-radius: 8px; font-weight: 700; font-size: 14px;
        letter-spacing: 1px; font-family: 'Segoe UI', system-ui, sans-serif;
        box-shadow: 0 4px 20px rgba(0,0,0,0.5);
      }
      #demo-role-badge.host { background: #1D9E75; color: #fff; }
      #demo-role-badge.guest { background: #EF9F27; color: #020817; }
      #demo-callout {
        position: fixed; bottom: 20px; left: 14px; right: 14px; z-index: 99999;
        background: rgba(2, 8, 23, 0.95); border: 2px solid #1D9E75;
        border-radius: 12px; padding: 14px 18px; color: #e2e8f0;
        font-family: 'Segoe UI', system-ui, sans-serif;
        box-shadow: 0 8px 32px rgba(0,0,0,0.6);
        animation: demo-in 0.35s ease;
        pointer-events: none;
      }
      #demo-callout.guest-border { border-color: #EF9F27; }
      #demo-callout .step {
        display: inline-block; background: #1D9E75; color: #fff;
        font-size: 11px; font-weight: 700; padding: 3px 10px;
        border-radius: 999px; margin-bottom: 8px;
      }
      #demo-callout.guest-border .step { background: #EF9F27; color: #020817; }
      #demo-callout h3 { margin: 0 0 6px; font-size: 16px; color: #fff; }
      #demo-callout p { margin: 0; font-size: 13px; line-height: 1.55; color: #94a3b8; }
      #demo-highlight {
        position: fixed; z-index: 99997; pointer-events: none;
        border: 3px solid #EF9F27; border-radius: 10px;
        box-shadow: 0 0 0 4px rgba(239,159,39,0.3);
        animation: demo-pulse 1.2s ease infinite; display: none;
      }
      @keyframes demo-in { from { opacity:0; transform:translateY(10px); } to { opacity:1; transform:none; } }
      @keyframes demo-pulse { 0%,100%{ box-shadow:0 0 0 4px rgba(239,159,39,0.3);} 50%{ box-shadow:0 0 0 10px rgba(239,159,39,0.08);} }
    `;
    document.head.appendChild(style);
    if (!document.getElementById('demo-role-badge')) {
      const badge = document.createElement('div');
      badge.id = 'demo-role-badge';
      badge.className = roleLabel.toLowerCase();
      badge.textContent = roleLabel;
      document.body.appendChild(badge);
    }
    if (!document.getElementById('demo-callout')) {
      document.body.appendChild(Object.assign(document.createElement('div'), { id: 'demo-callout' }));
    }
    if (!document.getElementById('demo-highlight')) {
      document.body.appendChild(Object.assign(document.createElement('div'), { id: 'demo-highlight' }));
    }
  }, role);
}

async function showCallout(page, { step, title, body, isGuest = false, duration = CALLOUT }) {
  await page.evaluate(({ step, title, body, isGuest }) => {
    const el = document.getElementById('demo-callout');
    if (!el) return;
    el.className = isGuest ? 'guest-border' : '';
    el.innerHTML = `<span class="step">${step}</span><h3>${title}</h3><p>${body}</p>`;
    el.style.display = 'block';
  }, { step, title, body, isGuest });
  await sleep(duration);
}

async function hideCallout(page) {
  await page.evaluate(() => {
    const el = document.getElementById('demo-callout');
    if (el) el.style.display = 'none';
    const hl = document.getElementById('demo-highlight');
    if (hl) hl.style.display = 'none';
  });
}

async function highlightElement(page, selector, duration = 1400) {
  await page.evaluate((sel) => {
    const target = document.querySelector(sel);
    const hl = document.getElementById('demo-highlight');
    if (!target || !hl) return;
    const r = target.getBoundingClientRect();
    hl.style.cssText += `display:block;top:${r.top - 4}px;left:${r.left - 4}px;width:${r.width + 8}px;height:${r.height + 8}px;`;
  }, selector);
  await sleep(duration);
}

async function waitForWasm(page) {
  await page.waitForFunction(
    () => document.getElementById('sec-wasm')?.innerHTML?.includes('✓'),
    { timeout: 90_000 }
  );
}

async function waitForSignalJson(page) {
  await page.waitForFunction(
    () => {
      const v = document.getElementById('signal-area')?.value || '';
      return v.trim().startsWith('{') && v.includes('"sdp"');
    },
    { timeout: 60_000 }
  );
}

async function waitForChatReady(page) {
  await page.waitForFunction(
    () => !document.getElementById('msg-input')?.disabled,
    { timeout: 120_000 }
  );
}

async function waitForChatMessage(page, text) {
  await page.waitForFunction(
    (t) => document.getElementById('chat-box')?.innerText?.includes(t),
    text,
    { timeout: 60_000 }
  );
}

async function healthCheck(base) {
  const res = await fetch(`${base}/health`);
  if (!res.ok) throw new Error(`Server not healthy at ${base}`);
}

function hasFfmpeg() {
  try { execSync('ffmpeg -version', { stdio: 'ignore' }); return true; } catch { return false; }
}

function composeSideBySide(hostWebm, guestWebm, outWebm, outMp4) {
  const filter = [
    `[0:v]scale=${PANE_W}:${PANE_H}:force_original_aspect_ratio=decrease,pad=${PANE_W}:${PANE_H}:(ow-iw)/2:(oh-ih)/2,setsar=1[v0]`,
    `[1:v]scale=${PANE_W}:${PANE_H}:force_original_aspect_ratio=decrease,pad=${PANE_W}:${PANE_H}:(ow-iw)/2:(oh-ih)/2,setsar=1[v1]`,
    '[v0][v1]hstack=inputs=2[vout]'
  ].join(';');
  execSync(
    `ffmpeg -y -i "${hostWebm}" -i "${guestWebm}" -filter_complex "${filter}" -map "[vout]" -an -shortest "${outWebm}"`,
    { stdio: 'inherit' }
  );
  if (outMp4) {
    execSync(
      `ffmpeg -y -i "${outWebm}" -c:v libx264 -preset medium -crf 22 -pix_fmt yuv420p -movflags +faststart "${outMp4}"`,
      { stdio: 'inherit' }
    );
  }
}

async function runDemo() {
  fs.mkdirSync(OUT_DIR, { recursive: true });
  const tag = stamp();

  await healthCheck(BASE);
  const browser = await chromium.launch({
    headless: false,
    slowMo: SLOW_MO,
    args: ['--use-fake-ui-for-media-stream']
  });

  const paneSize = { width: PANE_W, height: PANE_H };
  const hostContext = await browser.newContext({ recordVideo: { dir: OUT_DIR, size: paneSize }, viewport: paneSize, colorScheme: 'dark' });
  const guestContext = await browser.newContext({ recordVideo: { dir: OUT_DIR, size: paneSize }, viewport: paneSize, colorScheme: 'dark' });
  const host = await hostContext.newPage();
  const guest = await guestContext.newPage();
  host.setDefaultTimeout(120_000);
  guest.setDefaultTimeout(120_000);

  let outHost = '';
  let outGuest = '';

  try {
    /* ── Intro (host only until guest joins) ── */
    await host.goto(BASE);
    await injectDemoChrome(host, 'HOST');
    await waitForWasm(host);

    await showCallout(host, {
      step: 'INTRO',
      title: 'Aegis Secure Chat',
      body: 'End-to-end encrypted peer-to-peer chat. Left side = Host. Right side = Guest. CyberKnot AES-256 (WASM) + WebRTC + ECDH session keys.'
    });

    /* ── Step 1: Invite ── */
    await showCallout(host, {
      step: 'STEP 1',
      title: 'Create an invite-only room',
      body: 'The host clicks Create Invite. A 256-bit room secret is generated and embedded in the URL fragment — it never reaches the HTTP server.'
    });
    await highlightElement(host, '#create-invite-btn');
    await host.click('#create-invite-btn');
    await sleep(PAUSE);

    await showCallout(host, {
      step: 'STEP 2',
      title: 'Share the guest link privately',
      body: 'Copy the guest invite link and send it via a secure channel (Signal, WhatsApp, in person). Avoid Slack/email — URL previews can leak secrets.'
    });
    await highlightElement(host, '#guest-invite-link', 2000);
    const guestLink = await host.inputValue('#guest-invite-link');
    if (!guestLink) throw new Error('No guest link');

    /* ── Guest joins ── */
    await guest.goto(guestLink);
    await injectDemoChrome(guest, 'GUEST');
    await waitForWasm(guest);

    await showCallout(guest, {
      step: 'STEP 3',
      title: 'Guest opens the invite link',
      body: 'Room secret and invite ID load automatically. No password typing — security comes from the link itself.',
      isGuest: true
    });
    await showCallout(host, {
      step: 'STEP 3',
      title: 'Guest has joined the room',
      body: 'Both sides now share the same invite ID and room secret. Next: establish a WebRTC peer connection with signed signaling.'
    });

    /* ── Signaling (fast — WebRTC timing) ── */
    await hideCallout(host);
    await hideCallout(guest);

    await showCallout(host, {
      step: 'STEP 4',
      title: 'Starting WebRTC…',
      body: 'Connecting now — host generates Offer JSON with HMAC signature, ECDH public key, and DTLS fingerprint.',
      duration: 2800
    });

    await host.click('#connect-btn');
    await waitForSignalJson(host);
    const offerJson = await host.inputValue('#signal-area');

    await guest.fill('#signal-area', offerJson);
    await guest.click('#signal-btn');
    await waitForSignalJson(guest);
    const answerJson = await guest.inputValue('#signal-area');

    await host.fill('#signal-area', answerJson);
    await host.click('#signal-btn');

    await Promise.all([waitForChatReady(host), waitForChatReady(guest)]);

    /* ── Explain signaling after success ── */
    await showCallout(host, {
      step: 'STEP 5',
      title: 'Signaling complete',
      body: 'Offer and Answer were HMAC-verified. ECDH P-256 + HKDF derived a unique session key. DTLS fingerprint matched SDP. Key transparency keys registered.'
    });
    await showCallout(guest, {
      step: 'STEP 5',
      title: 'Encrypted P2P channel open',
      body: 'WebRTC data channel is live. Messages will be AES-256 encrypted in WASM before they enter the transport layer.',
      isGuest: true
    });

    /* ── Security panel ── */
    await showCallout(host, {
      step: 'STEP 6',
      title: 'Security panel — all checks green',
      body: 'WASM ready · Room secret · Invite bound · HMAC signaling · ECDH session key · DTLS verified · P2P open · Key transparency OK. Chat enabled only when every check passes.'
    });
    await highlightElement(host, '#security-panel', 2500);
    await showCallout(guest, {
      step: 'STEP 6',
      title: 'Defense in depth',
      body: 'Layer 1: DTLS (WebRTC transport). Layer 2: CyberKnot AES-256 + HMAC-SHA256 per message. Layer 3: Invite-only access with room secret in URL fragment.',
      isGuest: true
    });
    await highlightElement(guest, '#security-panel', 2500);
    await sleep(HOLD);

    /* ── Encrypted chat ── */
    const hostMsg = 'Hello from Host — encrypted with CyberKnot AES-256.';
    const guestMsg = 'Guest received — end-to-end encryption verified.';

    await showCallout(host, {
      step: 'STEP 7',
      title: 'Host sends an encrypted message',
      body: 'Plaintext never leaves the browser unencrypted. WASM encrypts → WebRTC sends ciphertext → guest WASM decrypts locally.',
      duration: 3500
    });
    await hideCallout(host);
    await host.fill('#msg-input', hostMsg);
    await highlightElement(host, '#send-btn');
    await host.click('#send-btn');
    await sleep(PAUSE);

    if ((await host.locator('#chat-box').innerText()).includes('Send failed')) {
      throw new Error('Host send failed');
    }
    await waitForChatMessage(guest, hostMsg);

    await showCallout(guest, {
      step: 'STEP 8',
      title: 'Guest replies — bidirectional E2E confirmed',
      body: 'The guest message is also encrypted in WASM. The keyserver and TURN relay never see plaintext — only encrypted blobs.',
      isGuest: true,
      duration: 3500
    });
    await hideCallout(guest);
    await guest.fill('#msg-input', guestMsg);
    await highlightElement(guest, '#send-btn');
    await guest.click('#send-btn');
    await sleep(PAUSE);

    if ((await guest.locator('#chat-box').innerText()).includes('Send failed')) {
      throw new Error('Guest send failed');
    }
    await waitForChatMessage(host, guestMsg);

    await showCallout(host, {
      step: 'COMPLETE',
      title: 'Demo complete',
      body: 'Aegis Secure Chat: invite-only · layered encryption · automated verification. Run locally: ./scripts/run-local.sh · Deploy: Docker Compose + Caddy + coturn.'
    });
    await showCallout(guest, {
      step: 'COMPLETE',
      title: 'Thank you for watching',
      body: 'See README.md for build, deploy, and security details.',
      isGuest: true
    });
    await sleep(HOLD * 2);

  } finally {
    await host.close();
    await guest.close();
    const hostVideoPath = host.video() ? await host.video().path() : null;
    const guestVideoPath = guest.video() ? await guest.video().path() : null;
    await hostContext.close();
    await guestContext.close();
    await browser.close();

    if (hostVideoPath) {
      outHost = path.join(OUT_DIR, `aegis-demo-${tag}-host.webm`);
      fs.renameSync(hostVideoPath, outHost);
    }
    if (guestVideoPath) {
      outGuest = path.join(OUT_DIR, `aegis-demo-${tag}-guest.webm`);
      fs.renameSync(guestVideoPath, outGuest);
    }
  }

  if (outHost && outGuest && hasFfmpeg()) {
    const outSplitWebm = path.join(OUT_DIR, `aegis-demo-${tag}-split.webm`);
    const outMp4 = path.join(OUT_DIR, 'aegis-product-demo.mp4');
    console.log('==> Composing side-by-side video…');
    composeSideBySide(outHost, outGuest, outSplitWebm, outMp4);
    console.log(`    MP4: ${outMp4}`);

    console.log('==> Adding intro slide + voiceover…');
    execSync(`node "${path.join(__dirname, 'compose-intro.mjs')}" "${outMp4}"`, { stdio: 'inherit' });
  }
}

runDemo().catch((err) => {
  console.error('Demo recording failed:', err.message);
  process.exit(1);
});

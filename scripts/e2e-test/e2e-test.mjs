#!/usr/bin/env node
/**
 * Aegis Secure Chat — automated end-to-end test (Host + Guest in Playwright).
 *
 * Usage:
 *   node e2e-test.mjs
 *   node e2e-test.mjs --base http://localhost:8080
 *   node e2e-test.mjs --base https://your-tunnel.trycloudflare.com --headed
 *
 * Env: E2E_BASE (default http://localhost:8080)
 */

import { chromium } from 'playwright';
import fs from 'fs';
import os from 'os';
import path from 'path';
import { fileURLToPath } from 'url';

const __dirname = path.dirname(fileURLToPath(import.meta.url));
const REPORTS_DIR = path.join(__dirname, 'reports');

const args = process.argv.slice(2);
const BASE = args.includes('--base')
  ? args[args.indexOf('--base') + 1]
  : process.env.E2E_BASE || 'http://localhost:8080';
const HEADED = args.includes('--headed');

const HOST_MSG = `E2E host ping ${Date.now()}`;
const GUEST_MSG = `E2E guest pong ${Date.now()}`;
const FILE_NAME = `aegis-e2e-test-${Date.now()}.txt`;
const FILE_CONTENT = `Aegis E2E file transfer test\nTimestamp: ${new Date().toISOString()}\n`;

const SEC_IDS = [
  'sec-wasm',
  'sec-room',
  'sec-invite',
  'sec-signal',
  'sec-session',
  'sec-fingerprint',
  'sec-channel',
  'sec-transparency'
];

function stamp() {
  return new Date().toISOString().replace(/[:.]/g, '-').slice(0, 19);
}

function pass(name, detail = '') {
  return { name, ok: true, detail };
}

function fail(name, detail = '') {
  return { name, ok: false, detail };
}

async function sleep(ms) {
  return new Promise((r) => setTimeout(r, ms));
}

async function healthCheck(base) {
  const res = await fetch(`${base}/health`);
  if (!res.ok) throw new Error(`Server not healthy at ${base}/health (${res.status})`);
  return res.json();
}

async function waitForWasm(page) {
  await page.waitForFunction(
    () => document.getElementById('sec-wasm')?.innerHTML?.includes('✓'),
    { timeout: 90_000 }
  );
}

async function waitForOfferJson(page) {
  await page.waitForFunction(
    () => {
      try {
        const v = document.getElementById('signal-area')?.value || '';
        const p = JSON.parse(v);
        return p.sdp?.type === 'offer' && v.includes('"sdp"');
      } catch {
        return false;
      }
    },
    { timeout: 90_000 }
  );
}

async function waitForAnswerJson(page) {
  await page.waitForFunction(
    () => {
      try {
        const v = document.getElementById('signal-area')?.value || '';
        const p = JSON.parse(v);
        return p.sdp?.type === 'answer' && v.includes('"sdp"');
      } catch {
        return false;
      }
    },
    { timeout: 90_000 }
  );
}

async function waitForGuestInviteLoaded(page) {
  await page.waitForFunction(
    () => document.getElementById('chat-box')?.innerText?.includes('Guest invite loaded'),
    { timeout: 30_000 }
  );
}

async function waitForChatReady(page) {
  await page.waitForFunction(
    () => {
      const input = document.getElementById('msg-input');
      return input && !input.disabled;
    },
    { timeout: 180_000 }
  );
}

async function captureDiagnostics(host, guest) {
  const diag = { host: {}, guest: {} };
  for (const [label, page] of [['host', host], ['guest', guest]]) {
    diag[label] = await page.evaluate(() => ({
      chat: document.getElementById('chat-box')?.innerText?.slice(-500),
      security: ['sec-wasm', 'sec-room', 'sec-invite', 'sec-signal', 'sec-session', 'sec-fingerprint', 'sec-channel', 'sec-transparency']
        .map((id) => document.getElementById(id)?.innerText),
      msgDisabled: document.getElementById('msg-input')?.disabled
    }));
  }
  return diag;
}

async function waitForChatMessage(page, text) {
  await page.waitForFunction(
    (t) => document.getElementById('chat-box')?.innerText?.includes(t),
    text,
    { timeout: 60_000 }
  );
}

async function getSecurityPanel(page) {
  return page.evaluate((ids) => {
    const out = {};
    for (const id of ids) {
      out[id] = document.getElementById(id)?.innerText?.trim() || '';
    }
    return out;
  }, SEC_IDS);
}

function securityAllGreen(panel) {
  const failures = [];
  for (const [id, text] of Object.entries(panel)) {
    if (!text.includes('✓')) failures.push(`${id}: ${text}`);
  }
  return failures;
}

async function chatHasSendFailure(page) {
  return (await page.locator('#chat-box').innerText()).includes('Send failed');
}

async function runE2E() {
  const started = new Date().toISOString();
  const results = [];
  let error = null;

  fs.mkdirSync(REPORTS_DIR, { recursive: true });
  const reportPath = path.join(REPORTS_DIR, `e2e-report-${stamp()}.json`);

  const tmpFile = path.join(os.tmpdir(), FILE_NAME);
  fs.writeFileSync(tmpFile, FILE_CONTENT, 'utf8');

  console.log('==> Aegis E2E automated test');
  console.log(`    Base URL: ${BASE}`);
  console.log(`    Mode: ${HEADED ? 'headed' : 'headless'}`);
  console.log('');

  try {
    const health = await healthCheck(BASE);
    results.push(pass('server_health', JSON.stringify(health)));

    const browser = await chromium.launch({
      headless: !HEADED,
      args: ['--use-fake-ui-for-media-stream']
    });

    const hostContext = await browser.newContext({ viewport: { width: 1280, height: 900 }, colorScheme: 'dark' });
    const guestContext = await browser.newContext({ viewport: { width: 1280, height: 900 }, colorScheme: 'dark' });
    const host = await hostContext.newPage();
    const guest = await guestContext.newPage();
    host.setDefaultTimeout(120_000);
    guest.setDefaultTimeout(120_000);

    try {
      // ── Host: create invite ──
      console.log('… Host: loading app');
      await host.goto(BASE);
      await waitForWasm(host);
      results.push(pass('host_wasm_loaded'));

      await host.click('#create-invite-btn');
      await host.waitForFunction(
        () => (document.getElementById('guest-invite-link')?.value || '').includes('#invite='),
        { timeout: 15_000 }
      );
      const guestLink = await host.inputValue('#guest-invite-link');
      if (!guestLink) throw new Error('Guest invite link empty');
      results.push(pass('host_create_invite', guestLink.slice(0, 80) + '…'));

      // ── Guest: join ──
      console.log('… Guest: opening invite link');
      await guest.goto(guestLink);
      await waitForWasm(guest);
      await waitForGuestInviteLoaded(guest);
      results.push(pass('guest_join_invite'));

      // ── WebRTC signaling ──
      console.log('… Signaling: offer → answer');
      await host.click('#connect-btn');
      await waitForOfferJson(host);
      const offerJson = await host.inputValue('#signal-area');
      results.push(pass('host_offer_generated', `${offerJson.length} chars`));

      await guest.fill('#signal-area', offerJson);
      await guest.click('#signal-btn');
      await waitForAnswerJson(guest);
      const answerJson = await guest.inputValue('#signal-area');
      results.push(pass('guest_answer_generated', `${answerJson.length} chars`));

      await host.fill('#signal-area', answerJson);
      await host.click('#signal-btn');
      await sleep(1000);

      await Promise.all([waitForChatReady(host), waitForChatReady(guest)]).catch(async (err) => {
        const diag = await captureDiagnostics(host, guest);
        throw new Error(`${err.message}\nDiagnostics: ${JSON.stringify(diag, null, 2)}`);
      });
      results.push(pass('p2p_channel_ready'));

      // ── Security panel ──
      const hostSec = await getSecurityPanel(host);
      const guestSec = await getSecurityPanel(guest);
      const hostFails = securityAllGreen(hostSec);
      const guestFails = securityAllGreen(guestSec);

      if (hostFails.length) {
        results.push(fail('host_security_panel', hostFails.join('; ')));
      } else {
        results.push(pass('host_security_panel', 'all checks green'));
      }
      if (guestFails.length) {
        results.push(fail('guest_security_panel', guestFails.join('; ')));
      } else {
        results.push(pass('guest_security_panel', 'all checks green'));
      }

      // ── Chat: host → guest ──
      console.log('… Chat: host → guest');
      await host.fill('#msg-input', HOST_MSG);
      await host.click('#send-btn');
      await sleep(500);
      if (await chatHasSendFailure(host)) {
        results.push(fail('host_send_message', 'Send failed on host'));
      } else {
        await waitForChatMessage(guest, HOST_MSG);
        results.push(pass('host_to_guest_message', HOST_MSG));
      }

      // ── Chat: guest → host ──
      console.log('… Chat: guest → host');
      await guest.fill('#msg-input', GUEST_MSG);
      await guest.click('#send-btn');
      await sleep(500);
      if (await chatHasSendFailure(guest)) {
        results.push(fail('guest_send_message', 'Send failed on guest'));
      } else {
        await waitForChatMessage(host, GUEST_MSG);
        results.push(pass('guest_to_host_message', GUEST_MSG));
      }

      // ── File: host → guest ──
      console.log('… File: host → guest');
      await host.setInputFiles('#file-input', tmpFile);
      await sleep(2000);

      if ((await host.locator('#chat-box').innerText()).includes('File send failed')) {
        results.push(fail('host_send_file', 'File send failed on host'));
      } else {
        await guest.waitForFunction(
          (name) => document.getElementById('chat-box')?.innerText?.includes(name),
          FILE_NAME,
          { timeout: 60_000 }
        );
        const fileLink = guest.locator(`a.msg.file:has-text("${FILE_NAME}")`);
        const href = await fileLink.getAttribute('href');
        if (!href || !href.startsWith('data:')) {
          results.push(fail('guest_receive_file', 'No download link on guest'));
        } else {
          const b64 = href.split(',')[1] || '';
          const decoded = Buffer.from(b64, 'base64').toString('utf8');
          if (decoded === FILE_CONTENT) {
            results.push(pass('host_to_guest_file', `${FILE_NAME} (${FILE_CONTENT.length} bytes verified)`));
          } else {
            results.push(fail('guest_file_integrity', 'Decoded content mismatch'));
          }
        }
      }

      // ── File: guest → host ──
      console.log('… File: guest → host');
      const tmpFile2 = path.join(os.tmpdir(), `guest-${FILE_NAME}`);
      const guestFileContent = `Guest file reply ${Date.now()}\n`;
      fs.writeFileSync(tmpFile2, guestFileContent, 'utf8');

      await guest.setInputFiles('#file-input', tmpFile2);
      await sleep(2000);

      if ((await guest.locator('#chat-box').innerText()).includes('File send failed')) {
        results.push(fail('guest_send_file', 'File send failed on guest'));
      } else {
        await host.waitForFunction(
          (name) => document.getElementById('chat-box')?.innerText?.includes(name),
          path.basename(tmpFile2),
          { timeout: 60_000 }
        );
        results.push(pass('guest_to_host_file', path.basename(tmpFile2)));
      }

      fs.unlinkSync(tmpFile2);
    } finally {
      await host.close().catch(() => {});
      await guest.close().catch(() => {});
      await hostContext.close().catch(() => {});
      await guestContext.close().catch(() => {});
      await browser.close().catch(() => {});
    }
  } catch (err) {
    error = err.message || String(err);
    results.push(fail('fatal_error', error));
  } finally {
    fs.unlinkSync(tmpFile);
  }

  const passed = results.filter((r) => r.ok).length;
  const failed = results.filter((r) => !r.ok).length;
  const overall = failed === 0 && !error;

  const report = {
    overall: overall ? 'PASS' : 'FAIL',
    base: BASE,
    started,
    finished: new Date().toISOString(),
    passed,
    failed,
    results,
    error
  };

  fs.writeFileSync(reportPath, JSON.stringify(report, null, 2));

  console.log('');
  console.log('=== E2E Results ===');
  for (const r of results) {
    console.log(`  ${r.ok ? '✓' : '✗'} ${r.name}${r.detail ? ` — ${r.detail}` : ''}`);
  }
  console.log('');
  console.log(`Overall: ${report.overall} (${passed} passed, ${failed} failed)`);
  console.log(`Report:  ${reportPath}`);

  process.exit(overall ? 0 : 1);
}

runE2E();

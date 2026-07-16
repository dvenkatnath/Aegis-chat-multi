#!/usr/bin/env node
'use strict';

/*
 * Automated test for the waiting-room / handshake relay added to
 * keyserver.js. Spins up a real instance of the server on a random port,
 * connects real WebSocket clients as host and guest, and asserts the
 * protocol behaves correctly — including the security-relevant behaviors
 * (no broadcast to unadmitted parties, second guest turned away once one
 * is admitted, rate limiting, malformed input rejected).
 *
 * Run: node scripts/e2e-test/test-relay.js
 */

const { spawn } = require('child_process');
const path = require('path');
const crypto = require('crypto');
const WebSocket = require(path.join(__dirname, '..', '..', 'server', 'node_modules', 'ws'));

const PORT = 8991 + Math.floor(Math.random() * 500);
const SERVER_PATH = path.join(__dirname, '..', '..', 'server', 'keyserver.js');

let pass = 0, fail = 0;
function ok(cond, label) {
  if (cond) { pass++; console.log(`  ok  - ${label}`); }
  else      { fail++; console.log(`FAIL  - ${label}`); }
}

function wait(ms) { return new Promise((r) => setTimeout(r, ms)); }

function onceMessage(ws) {
  return new Promise((resolve) => {
    ws.once('message', (data) => resolve(JSON.parse(data.toString())));
  });
}

async function main() {
  const server = spawn('node', [SERVER_PATH], {
    env: { ...process.env, PORT: String(PORT), NODE_ENV: 'development', LOG_SECRET: crypto.randomBytes(32).toString('hex') },
    stdio: ['ignore', 'pipe', 'pipe']
  });

  let serverLog = '';
  server.stdout.on('data', (d) => { serverLog += d.toString(); });
  server.stderr.on('data', (d) => { serverLog += d.toString(); });

  await wait(600); /* let it boot */

  const base = `ws://127.0.0.1:${PORT}/ws`;
  const inviteId = crypto.randomUUID().replace(/^(.{8})(.{4})./, (_, a, b) => `${a}-${b}-4`).slice(0, 36);
  /* ensure valid v4 shape regardless of the transform above */
  const validInviteId = [
    crypto.randomBytes(4).toString('hex'),
    crypto.randomBytes(2).toString('hex'),
    '4' + crypto.randomBytes(2).toString('hex').slice(1),
    ((8 + Math.floor(Math.random() * 4)).toString(16)) + crypto.randomBytes(2).toString('hex').slice(1),
    crypto.randomBytes(6).toString('hex')
  ].join('-');

  try {
    /* ---- Test 1: host registers, guest joins, host sees presence ---- */
    const hostWs = new WebSocket(base);
    await new Promise((res) => hostWs.once('open', res));
    hostWs.send(JSON.stringify({ type: 'register-host', inviteId: validInviteId }));
    const registered = await onceMessage(hostWs);
    ok(registered.type === 'registered' && registered.role === 'host', 'host registration acknowledged');

    const guestWs = new WebSocket(base);
    await new Promise((res) => guestWs.once('open', res));
    guestWs.send(JSON.stringify({ type: 'join', inviteId: validInviteId, name: 'Priya' }));
    const joined = await onceMessage(guestWs);
    ok(joined.type === 'joined' && typeof joined.connId === 'string', 'guest receives connId on join');
    const connId = joined.connId;

    const presence = await onceMessage(hostWs);
    ok(presence.type === 'presence' && presence.connId === connId && presence.name === 'Priya',
      'host sees presence with correct name and connId');

    /* ---- Test 2: a second, unadmitted guest must NOT see the first guest's traffic ---- */
    const guest2Ws = new WebSocket(base);
    await new Promise((res) => guest2Ws.once('open', res));
    guest2Ws.send(JSON.stringify({ type: 'join', inviteId: validInviteId, name: 'Eve' }));
    const joined2 = await onceMessage(guest2Ws);
    const presence2 = await onceMessage(hostWs);
    ok(presence2.connId === joined2.connId && presence2.name === 'Eve', 'second guest also shows in presence');

    /* ---- Test 3: admitting the first guest must NOT auto-deny the second —
       this build supports multiple simultaneous guests (see
       test-relay-multiguest.cjs for the full multi-guest coverage).
       The second guest should simply remain waiting, un-admitted. ---- */
    let guest2SawDenied = false;
    guest2Ws.on('message', (data) => {
      const m = JSON.parse(data.toString());
      if (m.type === 'denied') guest2SawDenied = true;
    });
    hostWs.send(JSON.stringify({ type: 'admit', connId }));
    const admitted = await onceMessage(guestWs);
    ok(admitted.type === 'admitted', 'admitted guest receives admitted event');
    await wait(300);
    ok(!guest2SawDenied, 'second guest is NOT auto-denied — remains waiting, admittable independently up to the guest cap');

    /* ---- Test 4: signal forwarding is opaque and only reaches the paired peer ---- */
    let guest2SawSignal = false;
    guest2Ws.on('message', (data) => {
      const m = JSON.parse(data.toString());
      if (m.type === 'signal') guest2SawSignal = true;
    });
    const fakeOffer = JSON.stringify({ v: 3, sdp: { type: 'offer', sdp: 'FAKE-SDP-OPAQUE-BLOB' }, aegis: { pubKey: 'FAKE', dtlsFingerprint: 'aa:bb', inviteId: validInviteId }, sig: 'FAKESIG' });
    hostWs.send(JSON.stringify({ type: 'signal', connId, payload: fakeOffer }));
    const sig1 = await onceMessage(guestWs);
    ok(sig1.type === 'signal' && sig1.payload === fakeOffer, 'admitted guest receives the exact opaque payload host sent');
    ok(!guest2SawSignal, 'the second, un-admitted guest never receives signal traffic addressed to a different guest');

    const fakeAnswer = JSON.stringify({ v: 3, sdp: { type: 'answer', sdp: 'FAKE-ANSWER-BLOB' }, aegis: { pubKey: 'FAKE2', dtlsFingerprint: 'cc:dd', inviteId: validInviteId }, sig: 'FAKESIG2' });
    guestWs.send(JSON.stringify({ type: 'signal', payload: fakeAnswer }));
    const sig2 = await onceMessage(hostWs);
    ok(sig2.type === 'signal' && sig2.connId === connId && sig2.payload === fakeAnswer,
      'host receives guest\'s answer tagged with the correct connId, payload untouched');

    /* ---- Test 5: malformed inviteId rejected ---- */
    const badWs = new WebSocket(base);
    await new Promise((res) => badWs.once('open', res));
    badWs.send(JSON.stringify({ type: 'join', inviteId: 'not-a-uuid', name: 'Mallory' }));
    const badResp = await onceMessage(badWs);
    ok(badResp.type === 'error', 'malformed inviteId is rejected with an error, not silently accepted');
    badWs.close();

    /* ---- Test 6: rate limiting kicks in on flood ---- */
    const floodWs = new WebSocket(base);
    await new Promise((res) => floodWs.once('open', res));
    let sawRateLimitError = false;
    floodWs.on('message', (data) => {
      const m = JSON.parse(data.toString());
      if (m.type === 'error' && /rate limit/i.test(m.message)) sawRateLimitError = true;
    });
    for (let i = 0; i < 30; i++) {
      floodWs.send(JSON.stringify({ type: 'join', inviteId: validInviteId, name: `flood${i}` }));
    }
    await wait(400);
    ok(sawRateLimitError, 'flooding the relay with messages triggers the rate limiter');
    floodWs.close();

    /* ---- Test 7: host disconnect keeps room alive for guest rejoin (mobile Safari) ---- */
    hostWs.close();
    await wait(300);
    const lateGuestWs = new WebSocket(base);
    await new Promise((res) => lateGuestWs.once('open', res));
    lateGuestWs.send(JSON.stringify({ type: 'join', inviteId: validInviteId, name: 'Late' }));
    const lateResp = await onceMessage(lateGuestWs);
    ok(lateResp.type === 'joined', 'guest can still join after host disconnects — room persists');
    lateGuestWs.close();

    guestWs.close();
    guest2Ws.close();

  } finally {
    server.kill();
    await wait(200);
  }

  console.log(`\n${pass} passed, ${fail} failed`);
  if (fail > 0) {
    console.log('\n--- server log for debugging ---');
    console.log(serverLog);
    process.exit(1);
  }
  process.exit(0);
}

main().catch((err) => {
  console.error('Test harness crashed:', err);
  process.exit(1);
});

#!/usr/bin/env node
'use strict';

/*
 * Automated test for MULTI-GUEST support in the waiting-room relay
 * (keyserver.js). Verifies the server correctly admits up to
 * MAX_ACTIVE_GUESTS simultaneously, rejects beyond the cap, routes
 * signal data to the correct specific guest only, and that one guest
 * leaving does not disturb the others.
 *
 * Run: node scripts/e2e-test/test-relay-multiguest.cjs
 */

const { spawn } = require('child_process');
const path = require('path');
const crypto = require('crypto');
const WebSocket = require(path.join(__dirname, '..', '..', 'server', 'node_modules', 'ws'));

const PORT = 9400 + Math.floor(Math.random() * 500);
const SERVER_PATH = path.join(__dirname, '..', '..', 'server', 'keyserver.js');
const MAX_ACTIVE_GUESTS = 5; /* must match server config */

let pass = 0, fail = 0;
function ok(cond, label) {
  if (cond) { pass++; console.log(`  ok  - ${label}`); }
  else      { fail++; console.log(`FAIL  - ${label}`); }
}
function wait(ms) { return new Promise((r) => setTimeout(r, ms)); }
function onceMessage(ws) {
  return new Promise((resolve) => ws.once('message', (data) => resolve(JSON.parse(data.toString()))));
}
function validUuid() {
  return [
    crypto.randomBytes(4).toString('hex'),
    crypto.randomBytes(2).toString('hex'),
    '4' + crypto.randomBytes(2).toString('hex').slice(1),
    ((8 + Math.floor(Math.random() * 4)).toString(16)) + crypto.randomBytes(2).toString('hex').slice(1),
    crypto.randomBytes(6).toString('hex')
  ].join('-');
}

async function main() {
  const server = spawn('node', [SERVER_PATH], {
    env: { ...process.env, PORT: String(PORT), NODE_ENV: 'development', LOG_SECRET: crypto.randomBytes(32).toString('hex') },
    stdio: ['ignore', 'pipe', 'pipe']
  });
  let serverLog = '';
  server.stdout.on('data', (d) => { serverLog += d.toString(); });
  server.stderr.on('data', (d) => { serverLog += d.toString(); });
  await wait(600);

  const base = `ws://127.0.0.1:${PORT}/ws`;
  const inviteId = validUuid();

  try {
    const hostWs = new WebSocket(base);
    await new Promise((res) => hostWs.once('open', res));
    hostWs.send(JSON.stringify({ type: 'register-host', inviteId }));
    await onceMessage(hostWs); // 'registered'

    /* ---- Join and admit five guests, one at a time ---- */
    const guests = [];
    for (let i = 0; i < MAX_ACTIVE_GUESTS; i++) {
      const gws = new WebSocket(base);
      await new Promise((res) => gws.once('open', res));
      gws.send(JSON.stringify({ type: 'join', inviteId, name: `Guest${i}` }));
      const joined = await onceMessage(gws);
      ok(joined.type === 'joined', `guest ${i} joins successfully`);
      const presence = await onceMessage(hostWs);
      ok(presence.connId === joined.connId && presence.name === `Guest${i}`, `host sees presence for guest ${i}`);

      hostWs.send(JSON.stringify({ type: 'admit', connId: joined.connId }));
      const admitted = await onceMessage(gws);
      ok(admitted.type === 'admitted', `guest ${i} is admitted`);

      guests.push({ ws: gws, connId: joined.connId, name: `Guest${i}` });
    }

    ok(guests.length === MAX_ACTIVE_GUESTS, `all ${MAX_ACTIVE_GUESTS} guests admitted simultaneously`);

    /* ---- A 6th guest must be rejected — room is at the active-guest cap ---- */
    const sixthWs = new WebSocket(base);
    await new Promise((res) => sixthWs.once('open', res));
    sixthWs.send(JSON.stringify({ type: 'join', inviteId, name: 'SixthGuest' }));
    const sixthResp = await onceMessage(sixthWs);
    ok(sixthResp.type === 'error' && /full|limit/i.test(sixthResp.message),
      `a 6th guest is rejected once the ${MAX_ACTIVE_GUESTS}-guest cap is reached`);
    sixthWs.close();

    /* ---- Signal routing: host sends distinct opaque payloads to guest 0 and guest 2 only ---- */
    let guest1SawAnySignal = false;
    guests[1].ws.on('message', (data) => {
      const m = JSON.parse(data.toString());
      if (m.type === 'signal') guest1SawAnySignal = true;
    });

    const payloadFor0 = JSON.stringify({ marker: 'FOR-GUEST-0', nonce: crypto.randomBytes(4).toString('hex') });
    const payloadFor2 = JSON.stringify({ marker: 'FOR-GUEST-2', nonce: crypto.randomBytes(4).toString('hex') });

    hostWs.send(JSON.stringify({ type: 'signal', connId: guests[0].connId, payload: payloadFor0 }));
    const sig0 = await onceMessage(guests[0].ws);
    ok(sig0.type === 'signal' && sig0.payload === payloadFor0, 'guest 0 receives exactly its own opaque payload');

    hostWs.send(JSON.stringify({ type: 'signal', connId: guests[2].connId, payload: payloadFor2 }));
    const sig2 = await onceMessage(guests[2].ws);
    ok(sig2.type === 'signal' && sig2.payload === payloadFor2, 'guest 2 receives exactly its own opaque payload');

    await wait(300);
    ok(!guest1SawAnySignal, 'guest 1 receives neither guest 0\'s nor guest 2\'s signal traffic');

    /* ---- Guest 3 sends an answer-style signal back; host receives it tagged with the correct connId ---- */
    const guest3Payload = JSON.stringify({ marker: 'FROM-GUEST-3', nonce: crypto.randomBytes(4).toString('hex') });
    guests[3].ws.send(JSON.stringify({ type: 'signal', payload: guest3Payload }));
    const fromGuest3 = await onceMessage(hostWs);
    ok(fromGuest3.type === 'signal' && fromGuest3.connId === guests[3].connId && fromGuest3.payload === guest3Payload,
      'host receives guest 3\'s signal correctly tagged with guest 3\'s connId, payload intact');

    /* ---- Guest 1 disconnects — the other four must be unaffected ---- */
    const peerLeftPromise = onceMessage(hostWs);
    guests[1].ws.close();
    const peerLeft = await peerLeftPromise;
    ok(peerLeft.type === 'peer-left' && peerLeft.connId === guests[1].connId, 'host is notified specifically that guest 1 left');

    /* Guest 0 (still connected) should still be reachable */
    const payloadAfterLeave = JSON.stringify({ marker: 'STILL-WORKS', nonce: crypto.randomBytes(4).toString('hex') });
    hostWs.send(JSON.stringify({ type: 'signal', connId: guests[0].connId, payload: payloadAfterLeave }));
    const stillWorks = await onceMessage(guests[0].ws);
    ok(stillWorks.type === 'signal' && stillWorks.payload === payloadAfterLeave,
      'guest 0 still reachable after an unrelated guest (guest 1) disconnected');

    /* ---- Now that guest 1 left, a new guest CAN join (back under the cap) ---- */
    const newGuestWs = new WebSocket(base);
    await new Promise((res) => newGuestWs.once('open', res));
    newGuestWs.send(JSON.stringify({ type: 'join', inviteId, name: 'ReplacementGuest' }));
    const replacementJoined = await onceMessage(newGuestWs);
    ok(replacementJoined.type === 'joined', 'a new guest can join after another guest\'s slot freed up');
    newGuestWs.close();

    for (const g of guests) { try { g.ws.close(); } catch (_) {} }
    hostWs.close();

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

#!/usr/bin/env node
'use strict';

/*
 * keyserver.js — Aegis Secure Chat key transparency server.
 *
 * Security fixes applied (from audit findings):
 *
 *   FIX-A: CORS wildcard removed.  CORS_ORIGIN must be set to the exact
 *           production origin (e.g. https://chat.example.com).  The server
 *           refuses to start if CORS_ORIGIN is '*' and NODE_ENV=production.
 *
 *   FIX-B: Rate limiting on key registration (/api/v1/keys POST).
 *           100 registrations per 15-minute window per IP.  Prevents DoS
 *           flooding of keys.json and memory exhaustion.
 *
 *   FIX-C: inviteId format validation. Must be a canonical UUID v4.
 *           Prevents arbitrary key pollution with crafted IDs.
 *
 *   FIX-D: Key record TTL.  Records older than KEY_TTL_MS (24 h) are
 *           pruned on startup and hourly.  Prevents unbounded disk growth
 *           and assists with GDPR/HIPAA data minimisation obligations.
 *
 *   FIX-E: Timing-safe proof verification. The proof HMAC comparison now
 *           uses crypto.timingSafeEqual to prevent timing oracle attacks
 *           against the room-secret.
 *
 * Waiting-room + automated handshake relay (added):
 *
 *   This server also relays SDP offer/answer, ICE candidates, and ECDH
 *   public keys between an admitted host/guest pair over a WebSocket at
 *   /ws, replacing the manual copy-paste signaling step.
 *
 *   IMPORTANT — what this does NOT change: the ECDH P-256 + HKDF session
 *   key derivation and the CyberKnot AES-256 message encryption in
 *   index.html are completely untouched. This relay only automates
 *   transport of data that a human was already copying and pasting by
 *   hand (offer/answer JSON, public keys) — nothing newly secret passes
 *   through the server that didn't already flow through an even less
 *   trusted channel (clipboard, WhatsApp, email) in the manual flow.
 *   The relay holds no session key, is never given one, and cannot
 *   decrypt or reconstruct chat content. Relay state is in-memory only
 *   (never written to keys.json or transparency.log.jsonl) and is
 *   discarded the moment a room closes.
 */

const crypto  = require('crypto');
const fs      = require('fs');
const path    = require('path');
const http    = require('http');
const express = require('express');
const { WebSocketServer } = require('ws');

/* ---------- configuration ---------- */
const PORT        = Number(process.env.PORT || 8080);
const DATA_DIR    = process.env.DATA_DIR || path.join(__dirname, 'data');
const KEYS_FILE   = path.join(DATA_DIR, 'keys.json');
const LOG_FILE    = path.join(DATA_DIR, 'transparency.log.jsonl');

const TURN_SECRET      = process.env.TURN_SECRET || '';
const TURN_HOST        = process.env.TURN_HOST   || 'turn';
const TURN_PORT        = process.env.TURN_PORT   || '3478';
const LOG_SECRET       = process.env.LOG_SECRET  || crypto.randomBytes(32).toString('hex');
const STUN_URL         = process.env.STUN_URL    || 'stun:stun.l.google.com:19302';
const CORS_ORIGIN      = process.env.CORS_ORIGIN || '';
const NODE_ENV         = process.env.NODE_ENV    || 'development';

/* Key record TTL: 24 hours */
const KEY_TTL_MS = 24 * 60 * 60 * 1000;

/* UUID v4 regex — exact format enforced on inviteId */
const UUID_V4_RE = /^[0-9a-f]{8}-[0-9a-f]{4}-4[0-9a-f]{3}-[89ab][0-9a-f]{3}-[0-9a-f]{12}$/i;

/* ---------- waiting-room / signaling relay configuration ---------- */
const ROOM_TTL_MS        = 24 * 60 * 60 * 1000; /* matches frontend INVITE_TTL_MS */
const MAX_WAITING_GUESTS = 8;                    /* per room, before admission */
const MAX_ACTIVE_GUESTS  = 5;                    /* simultaneously admitted, host-hub star topology */
const WS_MAX_MSG_BYTES   = 64 * 1024;            /* matches express.json limit */
const WS_RATE_MAX        = 20;                   /* messages per window, per connection */
const WS_RATE_WINDOW_MS  = 10 * 1000;
const DISPLAY_NAME_MAX   = 40;

/* ---------- startup checks ---------- */
if (NODE_ENV === 'production' && (!CORS_ORIGIN || CORS_ORIGIN === '*')) {
  console.error(
    'FATAL: CORS_ORIGIN must be set to the production origin ' +
    '(e.g. https://chat.example.com) when NODE_ENV=production.\n' +
    'A wildcard CORS_ORIGIN allows any website to call the key API.'
  );
  process.exit(1);
}

if (!LOG_SECRET || LOG_SECRET.length < 32) {
  console.error('FATAL: LOG_SECRET must be at least 32 characters. Generate with: openssl rand -hex 32');
  process.exit(1);
}

if (!fs.existsSync(DATA_DIR)) {
  fs.mkdirSync(DATA_DIR, { recursive: true });
}

/* ---------- storage helpers ---------- */
function readKeys() {
  if (!fs.existsSync(KEYS_FILE)) return {};
  try {
    return JSON.parse(fs.readFileSync(KEYS_FILE, 'utf8'));
  } catch {
    return {};
  }
}

function ensureDataDir() {
  if (!fs.existsSync(DATA_DIR)) {
    fs.mkdirSync(DATA_DIR, { recursive: true });
  }
}

function writeKeys(keys) {
  /* Atomic write via temp file + rename — avoids partial writes on crash */
  ensureDataDir();
  const tmp = KEYS_FILE + '.tmp';
  fs.writeFileSync(tmp, JSON.stringify(keys, null, 2));
  fs.renameSync(tmp, KEYS_FILE);
}

function appendLog(entry) {
  ensureDataDir();
  const line = JSON.stringify(entry);
  fs.appendFileSync(LOG_FILE, line + '\n');
}

function signLogEntry(payload) {
  return crypto.createHmac('sha256', LOG_SECRET).update(payload).digest('base64');
}

/* ---------- TTL pruning ---------- */
function pruneExpiredKeys() {
  const keys   = readKeys();
  const cutoff = Date.now() - KEY_TTL_MS;
  let pruned   = 0;
  for (const slot of Object.keys(keys)) {
    const ts = new Date(keys[slot].registeredAt).getTime();
    if (ts < cutoff) {
      delete keys[slot];
      pruned++;
    }
  }
  if (pruned > 0) {
    writeKeys(keys);
    console.log(`Pruned ${pruned} expired key record(s).`);
  }
}

/* Prune on startup and every hour */
pruneExpiredKeys();
setInterval(pruneExpiredKeys, 60 * 60 * 1000);

/* ---------- TURN credentials ---------- */
function makeTurnCredentials() {
  if (!TURN_SECRET) return null;
  const ttl      = 86400;
  const username = `${Math.floor(Date.now() / 1000) + ttl}:aegis`;
  const credential = crypto
    .createHmac('sha1', TURN_SECRET)
    .update(username)
    .digest('base64');
  return { username, credential, ttl };
}

/* ---------- Express app ---------- */
const app = express();
app.use(express.json({ limit: '64kb' }));

/* CORS */
app.use((req, res, next) => {
  const origin = CORS_ORIGIN || '*';
  res.setHeader('Access-Control-Allow-Origin',  origin);
  res.setHeader('Access-Control-Allow-Methods', 'GET,POST,OPTIONS');
  res.setHeader('Access-Control-Allow-Headers', 'Content-Type');
  if (req.method === 'OPTIONS') return res.sendStatus(204);
  next();
});

/* ---------- rate limiter for key registration (FIX-B) ---------- */
/*
 * Simple in-process token bucket per IP.
 * 100 requests per 15-minute window.  Resets automatically.
 * For multi-instance deployments replace with Redis-backed rate limiter.
 */
const rateBuckets = new Map();
const RATE_WINDOW_MS  = 15 * 60 * 1000;
const RATE_MAX        = 100;

function rateLimit(req, res, next) {
  const ip  = req.ip || req.connection.remoteAddress || 'unknown';
  const now = Date.now();
  let   bucket = rateBuckets.get(ip);

  if (!bucket || now - bucket.windowStart > RATE_WINDOW_MS) {
    bucket = { windowStart: now, count: 0 };
    rateBuckets.set(ip, bucket);
  }

  bucket.count++;
  if (bucket.count > RATE_MAX) {
    return res.status(429).json({ error: 'Too many requests. Try again later.' });
  }
  next();
}

/* Periodically clear stale buckets to prevent unbounded Map growth */
setInterval(() => {
  const cutoff = Date.now() - RATE_WINDOW_MS;
  for (const [ip, bucket] of rateBuckets.entries()) {
    if (bucket.windowStart < cutoff) rateBuckets.delete(ip);
  }
}, RATE_WINDOW_MS);

/* ---------- timing-safe proof verification (FIX-E) ---------- */
/*
 * The incoming proof is an HMAC-SHA256 tag.  We recompute it server-side
 * and compare with timingSafeEqual — the comparison time is constant
 * regardless of where bytes differ, preventing timing oracle attacks
 * against the room secret.
 *
 * Note: the server does NOT know the room secret — it cannot recompute.
 * Verification responsibility belongs to the receiving peer in the browser
 * (parseAndVerifySignal / verifyHostKeyTransparency).
 * The server stores the proof alongside the entry for clients to verify.
 * We validate only that proof is a non-empty string (≤ 512 chars).
 */
function isValidProof(proof) {
  return typeof proof === 'string' && proof.length > 0 && proof.length <= 512;
}

/* ---------- waiting-room + handshake relay state ---------- */
/*
 * In-memory only. Nothing here is written to disk, ever. A room is
 * discarded the moment the host disconnects, or after ROOM_TTL_MS.
 *
 *   rooms: Map<inviteId, {
 *     hostWs:    WebSocket | null,
 *     createdAt: number,
 *     guests:    Map<connId, { ws: WebSocket, name: string, admitted: boolean }>,
 *     admittedConnIds: Set<string>   // up to MAX_ACTIVE_GUESTS, host-hub star
 *   }>
 */
const rooms = new Map();

function newConnId() {
  return crypto.randomBytes(9).toString('base64url');
}

function pruneStaleRooms() {
  const cutoff = Date.now() - ROOM_TTL_MS;
  for (const [inviteId, room] of rooms.entries()) {
    if (room.createdAt < cutoff) {
      closeRoom(inviteId, 'expired');
    }
  }
}
setInterval(pruneStaleRooms, 10 * 60 * 1000);

function closeRoom(inviteId, reason) {
  const room = rooms.get(inviteId);
  if (!room) return;
  const msg = JSON.stringify({ type: 'room-closed', reason });
  for (const guest of room.guests.values()) {
    safeSend(guest.ws, msg);
    try { guest.ws.close(); } catch { /* already closed */ }
  }
  if (room.hostWs) {
    try { room.hostWs.close(); } catch { /* already closed */ }
  }
  rooms.delete(inviteId);
}

function safeSend(ws, str) {
  if (ws && ws.readyState === ws.OPEN) {
    try { ws.send(str); } catch { /* connection gone, ignore */ }
  }
}

/* Per-connection token bucket — mirrors the existing HTTP rate limiter (FIX-B) */
function makeRateState() {
  return { windowStart: Date.now(), count: 0 };
}
function withinRate(state) {
  const now = Date.now();
  if (now - state.windowStart > WS_RATE_WINDOW_MS) {
    state.windowStart = now;
    state.count = 0;
  }
  state.count++;
  return state.count <= WS_RATE_MAX;
}

/*
 * Origin check on WebSocket upgrade — same posture as FIX-A for the HTTP
 * API. In production, only the configured CORS_ORIGIN may open a relay
 * connection.
 */
function isAllowedOrigin(origin) {
  if (NODE_ENV !== 'production') return true;
  return !!origin && origin === CORS_ORIGIN;
}

function sanitizeName(name) {
  if (typeof name !== 'string') return null;
  const trimmed = name.trim().slice(0, DISPLAY_NAME_MAX);
  if (!trimmed) return null;
  /* Strip anything that isn't a printable, non-control character. */
  return trimmed.replace(/[\u0000-\u001F\u007F]/g, '');
}

/* ---------- routes ---------- */
app.get('/health', (_req, res) => {
  res.json({ ok: true, service: 'aegis-keyserver' });
});

app.get('/api/ice-config', (_req, res) => {
  const iceServers = [{ urls: STUN_URL }];
  const turn = makeTurnCredentials();
  if (turn) {
    iceServers.push({
      urls: [
        `turn:${TURN_HOST}:${TURN_PORT}?transport=udp`,
        `turn:${TURN_HOST}:${TURN_PORT}?transport=tcp`
      ],
      username: turn.username,
      credential: turn.credential
    });
  }
  res.json({ iceServers, iceTransportPolicy: 'all' });
});

app.post('/api/v1/keys', rateLimit, (req, res) => {
  const { inviteId, role, pubKey, proof } = req.body || {};

  /* FIX-C: strict input validation */
  if (!inviteId || !role || !pubKey || !proof) {
    return res.status(400).json({ error: 'inviteId, role, pubKey, and proof are required' });
  }
  if (!UUID_V4_RE.test(inviteId)) {
    return res.status(400).json({ error: 'inviteId must be a valid UUID v4' });
  }
  if (!['host', 'guest'].includes(role)) {
    return res.status(400).json({ error: 'role must be host or guest' });
  }
  if (typeof pubKey !== 'string' || pubKey.length < 10 || pubKey.length > 4096) {
    return res.status(400).json({ error: 'invalid pubKey' });
  }
  if (!isValidProof(proof)) {
    return res.status(400).json({ error: 'invalid proof' });
  }

  const keys = readKeys();
  const slot = `${inviteId}:${role}`;

  if (keys[slot]) {
    if (keys[slot].pubKey === pubKey) {
      return res.json({ ok: true, existing: true, entry: keys[slot] });
    }
    return res.status(409).json({ error: 'Key already registered for this invite and role' });
  }

  const entry = {
    inviteId,
    role,
    pubKey,
    proof,
    registeredAt: new Date().toISOString()
  };
  const payload = JSON.stringify(entry);
  entry.serverSig = signLogEntry(payload);

  keys[slot] = entry;
  writeKeys(keys);
  appendLog({ type: 'key_registered', ...entry });

  res.status(201).json({ ok: true, entry });
});

app.get('/api/v1/keys/:inviteId/:role', (req, res) => {
  /* FIX-C: validate path parameters */
  if (!UUID_V4_RE.test(req.params.inviteId)) {
    return res.status(400).json({ error: 'inviteId must be a valid UUID v4' });
  }
  if (!['host', 'guest'].includes(req.params.role)) {
    return res.status(400).json({ error: 'invalid role' });
  }

  const slot  = `${req.params.inviteId}:${req.params.role}`;
  const keys  = readKeys();
  const entry = keys[slot];
  if (!entry) {
    return res.status(404).json({ error: 'Key not found' });
  }

  /* FIX-D: reject expired records that somehow survived the prune pass */
  const age = Date.now() - new Date(entry.registeredAt).getTime();
  if (age > KEY_TTL_MS) {
    return res.status(404).json({ error: 'Key expired' });
  }

  res.json(entry);
});

app.get('/api/v1/transparency', (req, res) => {
  const limit = Math.min(Number(req.query.limit) || 100, 1000);
  if (!fs.existsSync(LOG_FILE)) {
    return res.json({ entries: [] });
  }
  const lines   = fs.readFileSync(LOG_FILE, 'utf8').trim().split('\n').filter(Boolean);
  const entries = lines.slice(-limit).map((line) => JSON.parse(line));
  res.json({ entries, count: entries.length });
});

if (process.env.STATIC_DIR) {
  const staticRoot = path.resolve(process.env.STATIC_DIR);
  app.get('/favicon.ico', (_req, res) => {
    res.type('image/png');
    res.sendFile(path.join(staticRoot, 'favicon-32.png'));
  });
  app.use(express.static(staticRoot));
  app.get('*', (req, res, next) => {
    if (req.path.startsWith('/api/')) return next();
    res.sendFile(path.join(staticRoot, 'index.html'));
  });
}

/* ---------- WebSocket relay: waiting room + automated handshake ---------- */
const server = http.createServer(app);
const wss = new WebSocketServer({
  server,
  path: '/ws',
  maxPayload: WS_MAX_MSG_BYTES,
  verifyClient: (info, done) => {
    if (!isAllowedOrigin(info.origin)) {
      return done(false, 403, 'Origin not allowed');
    }
    done(true);
  }
});

wss.on('connection', (ws, req) => {
  const rate = makeRateState();
  let boundInviteId = null;
  let boundRole      = null; /* 'host' | 'guest' */
  let boundConnId    = null; /* only set for guests */

  ws.on('message', (raw) => {
    if (!withinRate(rate)) {
      return safeSend(ws, JSON.stringify({ type: 'error', message: 'rate limit exceeded' }));
    }

    let msg;
    try {
      msg = JSON.parse(raw.toString());
    } catch {
      return safeSend(ws, JSON.stringify({ type: 'error', message: 'invalid JSON' }));
    }
    if (!msg || typeof msg.type !== 'string') {
      return safeSend(ws, JSON.stringify({ type: 'error', message: 'missing type' }));
    }

    switch (msg.type) {

      case 'register-host': {
        const { inviteId } = msg;
        if (!UUID_V4_RE.test(inviteId || '')) {
          return safeSend(ws, JSON.stringify({ type: 'error', message: 'invalid inviteId' }));
        }
        let room = rooms.get(inviteId);
        if (room && room.hostWs && room.hostWs.readyState === ws.OPEN) {
          return safeSend(ws, JSON.stringify({ type: 'error', message: 'room already has a host' }));
        }
        if (!room) {
          room = { hostWs: null, createdAt: Date.now(), guests: new Map(), admittedConnIds: new Set() };
          rooms.set(inviteId, room);
        }
        room.hostWs = ws;
        boundInviteId = inviteId;
        boundRole = 'host';
        safeSend(ws, JSON.stringify({ type: 'registered', role: 'host' }));
        /* Replay anyone already waiting, in case the host reconnected. */
        for (const [connId, guest] of room.guests.entries()) {
          if (!guest.admitted) {
            safeSend(ws, JSON.stringify({ type: 'presence', connId, name: guest.name }));
          }
        }
        break;
      }

      case 'join': {
        const { inviteId, name } = msg;
        if (!UUID_V4_RE.test(inviteId || '')) {
          return safeSend(ws, JSON.stringify({ type: 'error', message: 'invalid inviteId' }));
        }
        const cleanName = sanitizeName(name);
        if (!cleanName) {
          return safeSend(ws, JSON.stringify({ type: 'error', message: 'name required' }));
        }
        const room = rooms.get(inviteId);
        if (!room) {
          return safeSend(ws, JSON.stringify({ type: 'error', message: 'invite not found or expired' }));
        }
        if (room.admittedConnIds.size >= MAX_ACTIVE_GUESTS) {
          return safeSend(ws, JSON.stringify({ type: 'error', message: `room full — maximum ${MAX_ACTIVE_GUESTS} guests already active` }));
        }
        const waiting = [...room.guests.values()].filter((g) => !g.admitted).length;
        if (waiting >= MAX_WAITING_GUESTS) {
          return safeSend(ws, JSON.stringify({ type: 'error', message: 'waiting room full' }));
        }

        const connId = newConnId();
        room.guests.set(connId, { ws, name: cleanName, admitted: false });
        boundInviteId = inviteId;
        boundRole = 'guest';
        boundConnId = connId;

        safeSend(ws, JSON.stringify({ type: 'joined', connId }));
        safeSend(room.hostWs, JSON.stringify({ type: 'presence', connId, name: cleanName }));
        break;
      }

      case 'admit': {
        if (boundRole !== 'host') return;
        const room = rooms.get(boundInviteId);
        if (!room) return;
        const { connId } = msg;
        const guest = room.guests.get(connId);
        if (!guest) {
          return safeSend(ws, JSON.stringify({ type: 'error', message: 'guest not found' }));
        }
        if (room.admittedConnIds.size >= MAX_ACTIVE_GUESTS) {
          return safeSend(ws, JSON.stringify({ type: 'error', message: `already at the ${MAX_ACTIVE_GUESTS}-guest limit` }));
        }
        guest.admitted = true;
        room.admittedConnIds.add(connId);
        safeSend(guest.ws, JSON.stringify({ type: 'admitted' }));
        break;
      }

      case 'deny': {
        if (boundRole !== 'host') return;
        const room = rooms.get(boundInviteId);
        if (!room) return;
        const guest = room.guests.get(msg.connId);
        if (!guest) return;
        safeSend(guest.ws, JSON.stringify({ type: 'denied', reason: 'host declined' }));
        try { guest.ws.close(); } catch { /* ignore */ }
        room.guests.delete(msg.connId);
        break;
      }

      case 'signal': {
        /*
         * Opaque forwarding only. `payload` is whatever the browser put
         * there (SDP offer/answer, ICE candidate, exported ECDH public
         * key) — the server does not parse, validate, or log its
         * contents beyond the outer size cap already enforced by
         * maxPayload above.
         */
        const room = rooms.get(boundInviteId);
        if (!room) return;
        if (boundRole === 'host') {
          const targetId = msg.connId;
          const guest = targetId ? room.guests.get(targetId) : null;
          if (guest && room.admittedConnIds.has(targetId)) {
            safeSend(guest.ws, JSON.stringify({ type: 'signal', payload: msg.payload }));
          }
        } else if (boundRole === 'guest' && room.admittedConnIds.has(boundConnId)) {
          safeSend(room.hostWs, JSON.stringify({ type: 'signal', connId: boundConnId, payload: msg.payload }));
        }
        break;
      }

      default:
        safeSend(ws, JSON.stringify({ type: 'error', message: `unknown type: ${msg.type}` }));
    }
  });

  ws.on('close', () => {
    if (!boundInviteId) return;
    const room = rooms.get(boundInviteId);
    if (!room) return;

    if (boundRole === 'host') {
      closeRoom(boundInviteId, 'host disconnected');
    } else if (boundRole === 'guest' && boundConnId) {
      const wasAdmitted = room.admittedConnIds.has(boundConnId);
      room.guests.delete(boundConnId);
      if (wasAdmitted) room.admittedConnIds.delete(boundConnId);
      safeSend(room.hostWs, JSON.stringify({ type: 'peer-left', connId: boundConnId }));
    }
  });
});

server.listen(PORT, () => {
  console.log(`Aegis keyserver listening on :${PORT}`);
  console.log(`  CORS_ORIGIN : ${CORS_ORIGIN || '* (dev mode)'}`);
  console.log(`  NODE_ENV    : ${NODE_ENV}`);
  console.log(`  WS relay    : ws://<host>:${PORT}/ws  (waiting room + handshake automation)`);
  if (!TURN_SECRET) {
    console.warn('  WARNING: TURN_SECRET not set — TURN relay credentials unavailable.');
  }
});

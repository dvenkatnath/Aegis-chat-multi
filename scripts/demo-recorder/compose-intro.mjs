#!/usr/bin/env node
/**
 * Prepends a branded intro slide + voiceover to aegis-product-demo.mp4
 *
 * Voice: Microsoft Edge neural TTS (en-US-JennyNeural) — female, American, natural.
 * Fallback: macOS Samantha if offline / TTS unavailable.
 *
 * Usage: node compose-intro.mjs [demo.mp4]
 * Env:   DEMO_VOICE=en-US-AriaNeural  DEMO_VOICE_RATE=-8%
 */

import { chromium } from 'playwright';
import { EdgeTTS } from 'node-edge-tts';
import { execSync } from 'child_process';
import fs from 'fs';
import path from 'path';
import { fileURLToPath } from 'url';

const __dirname = path.dirname(fileURLToPath(import.meta.url));
const ROOT = path.resolve(__dirname, '../..');
const OUT_DIR = path.join(__dirname, 'demo-output');
const INTRO_HTML = path.join(__dirname, 'intro-slide.html');
const LOGO_PATH = path.join(ROOT, 'assets', 'aegis-logo.png');
const W = 1920;
const H = 900;

/* Pauses via punctuation — neural voices read these naturally */
const NARRATION = `
Welcome to Aegis Secure Chat.
An invite-only, end-to-end encrypted messaging platform, built for zero-trust communication.

Every session combines WebRTC with DTLS transport, ECDH key agreement,
and CyberKnot AES-256 encryption, running in WebAssembly.

Signaling is HMAC-authenticated.
DTLS fingerprints are verified automatically.
And public keys are registered in a transparency log.

Let's walk through a live host and guest session.
`.replace(/\n+/g, '\n').trim();

/* Female American neural voices (Microsoft Edge TTS):
 *   en-US-JennyNeural  — warm, conversational (default)
 *   en-US-AriaNeural   — clear, professional
 *   en-US-MichelleNeural — calm narrator
 */
const NEURAL_VOICE = process.env.DEMO_VOICE || 'en-US-JennyNeural';
const NEURAL_RATE = process.env.DEMO_VOICE_RATE || '-8%';
const MACOS_FALLBACK = process.env.DEMO_MACOS_VOICE || 'Samantha';
const MACOS_RATE = process.env.DEMO_MACOS_RATE || '155';

function run(cmd) {
  execSync(cmd, { stdio: 'inherit', shell: true });
}

function runCapture(cmd) {
  return execSync(cmd, { encoding: 'utf8' }).trim();
}

function hasFfmpeg() {
  try { execSync('ffmpeg -version', { stdio: 'ignore' }); return true; } catch { return false; }
}

function hasSay() {
  try { execSync('which say', { stdio: 'ignore' }); return true; } catch { return false; }
}

function probeDuration(file) {
  return parseFloat(
    runCapture(`ffprobe -v error -show_entries format=duration -of csv=p=0 "${file}"`)
  );
}

async function captureIntroSlide(pngPath) {
  if (!fs.existsSync(LOGO_PATH)) {
    throw new Error(`Product logo not found: ${LOGO_PATH}`);
  }
  const browser = await chromium.launch();
  const page = await browser.newPage({ viewport: { width: W, height: H } });
  await page.goto(`file://${INTRO_HTML}`);
  await page.locator('.app-logo').waitFor({ state: 'visible' });
  await page.evaluate((logoPath) => {
    const img = document.querySelector('.app-logo');
    if (img) img.src = `file://${logoPath}`;
  }, LOGO_PATH);
  await page.waitForFunction(() => {
    const img = document.querySelector('.app-logo');
    return img && img.complete && img.naturalWidth > 0;
  });
  await page.waitForTimeout(300);
  await page.screenshot({ path: pngPath, type: 'png' });
  await browser.close();
}

async function generateVoiceoverNeural(mp3Path, m4aPath) {
  const textFile = path.join(OUT_DIR, 'intro-narration.txt');
  fs.writeFileSync(textFile, NARRATION, 'utf8');

  console.log(`==> Generating voiceover (neural: ${NEURAL_VOICE}, rate: ${NEURAL_RATE})…`);
  const tts = new EdgeTTS({
    voice: NEURAL_VOICE,
    lang: 'en-US',
    rate: NEURAL_RATE,
    pitch: '+0Hz',
    outputFormat: 'audio-24khz-96kbitrate-mono-mp3',
    timeout: 30_000
  });
  await tts.ttsPromise(NARRATION, mp3Path);
  run(`ffmpeg -y -i "${mp3Path}" -c:a aac -b:a 192k "${m4aPath}" -loglevel error`);
}

function generateVoiceoverMacOS(aiffPath, m4aPath) {
  const textFile = path.join(OUT_DIR, 'intro-narration.txt');
  fs.writeFileSync(textFile, NARRATION, 'utf8');
  console.log(`==> Generating voiceover (macOS fallback: ${MACOS_FALLBACK})…`);
  run(`say -v "${MACOS_FALLBACK}" -r ${MACOS_RATE} -o "${aiffPath}" -f "${textFile}"`);
  run(`ffmpeg -y -i "${aiffPath}" -c:a aac -b:a 192k "${m4aPath}" -loglevel error`);
}

async function generateVoiceover(mp3Path, aiffPath, m4aPath) {
  try {
    await generateVoiceoverNeural(mp3Path, m4aPath);
    return 'neural';
  } catch (err) {
    console.warn(`==> Neural TTS failed (${err.message}) — trying macOS Samantha…`);
    if (!hasSay()) {
      throw new Error('No TTS available. Install node-edge-tts (npm install) or use macOS say.');
    }
    generateVoiceoverMacOS(aiffPath, m4aPath);
    return 'macos';
  }
}

function buildIntroVideo(pngPath, audioPath, outPath) {
  const audioDur = probeDuration(audioPath);
  const totalDur = (audioDur + 1.2).toFixed(2);
  console.log(`==> Intro slide video (${totalDur}s)…`);
  run(
    `ffmpeg -y -loop 1 -framerate 25 -i "${pngPath}" -i "${audioPath}" ` +
    `-vf "scale=${W}:${H},fade=t=in:st=0:d=0.6,fade=t=out:st=${(audioDur + 0.4).toFixed(2)}:d=0.8" ` +
    `-c:v libx264 -pix_fmt yuv420p -t ${totalDur} -c:a aac -b:a 192k ` +
    `-movflags +faststart -shortest "${outPath}" -loglevel error`
  );
}

function addSilentAudioToDemo(demoIn, demoOut) {
  run(
    `ffmpeg -y -i "${demoIn}" -f lavfi -i anullsrc=channel_layout=stereo:sample_rate=44100 ` +
    `-c:v copy -c:a aac -shortest "${demoOut}" -loglevel error`
  );
}

function concatIntroAndDemo(introPath, demoPath, outPath) {
  const listFile = path.join(OUT_DIR, 'concat-list.txt');
  fs.writeFileSync(
    listFile,
    `file '${introPath.replace(/'/g, "'\\''")}'\nfile '${demoPath.replace(/'/g, "'\\''")}'\n`
  );
  console.log('==> Merging intro + demo…');
  run(
    `ffmpeg -y -f concat -safe 0 -i "${listFile}" ` +
    `-c:v libx264 -preset medium -crf 22 -pix_fmt yuv420p -c:a aac -b:a 192k ` +
    `-movflags +faststart "${outPath}" -loglevel warning`
  );
}

async function main() {
  if (!hasFfmpeg()) throw new Error('ffmpeg required');

  const demoArg = process.argv[2];
  const demoIn = demoArg
    ? path.resolve(demoArg)
    : path.join(OUT_DIR, 'aegis-demo-main-only.mp4');

  if (!fs.existsSync(demoIn)) {
    throw new Error(`Demo not found: ${demoIn}\nRun: node record-demo.mjs --view split`);
  }

  fs.mkdirSync(OUT_DIR, { recursive: true });

  const png = path.join(OUT_DIR, 'intro-slide.png');
  const mp3 = path.join(OUT_DIR, 'intro-voice.mp3');
  const aiff = path.join(OUT_DIR, 'intro-voice.aiff');
  const voice = path.join(OUT_DIR, 'intro-voice.m4a');
  const introVid = path.join(OUT_DIR, 'intro-segment.mp4');
  const demoAudio = path.join(OUT_DIR, 'demo-with-silent-audio.mp4');
  const fullOut = path.join(OUT_DIR, 'aegis-product-demo-full.mp4');
  const mainOut = path.join(OUT_DIR, 'aegis-product-demo.mp4');

  console.log('==> Capturing intro slide…');
  await captureIntroSlide(png);

  const engine = await generateVoiceover(mp3, aiff, voice);
  console.log(`    Voice engine: ${engine}`);
  buildIntroVideo(png, voice, introVid);
  addSilentAudioToDemo(demoIn, demoAudio);
  concatIntroAndDemo(introVid, demoAudio, fullOut);

  const demoOnly = path.join(OUT_DIR, 'aegis-demo-main-only.mp4');
  fs.copyFileSync(demoIn, demoOnly);
  fs.copyFileSync(fullOut, mainOut);

  const dur = probeDuration(fullOut).toFixed(1);
  console.log('');
  console.log(`==> Done (${dur}s total)`);
  console.log(`    ${mainOut}`);
}

main().catch((err) => {
  console.error('compose-intro failed:', err.message);
  process.exit(1);
});

#!/usr/bin/env bash
set -euo pipefail

ROOT="$(cd "$(dirname "$0")/.." && pwd)"
DIST="$ROOT/dist"
SRC="$ROOT/src"

echo "==> Building Aegis Secure Chat → $DIST"
rm -rf "$DIST"
mkdir -p "$DIST"

if ! command -v emcc >/dev/null 2>&1; then
  echo "ERROR: emcc not found. Install Emscripten first."
  exit 1
fi

echo "==> Compiling CyberKnot WASM..."
emcc -I "$SRC/include" \
  "$SRC/bridge.c" "$SRC/api.c" "$SRC/aes.c" "$SRC/Hmac.c" "$SRC/Base64.c" \
  "$SRC/SHA256.c" "$SRC/HighLevED.c" "$SRC/UberHighLevED.c" "$SRC/Functions3.c" \
  "$SRC/Global2.c" "$SRC/ProcMSG.c" "$SRC/Pstr.c" "$SRC/EncryptionToolsApi.c" \
  "$SRC/CryptoRng.c" \
  "$SRC/HmacSha256.c" \
  -o "$DIST/cyberknot.js" \
  -s EXPORTED_FUNCTIONS='["_bridge_encrypt","_bridge_decrypt","_bridge_encrypt_b64","_bridge_decrypt_b64","_bridge_set_session_key","_bridge_has_session_key","_bridge_clear_session_key","_malloc","_free"]' \
  -s EXPORTED_RUNTIME_METHODS='["ccall","cwrap","allocateUTF8","UTF8ToString","lengthBytesUTF8"]' \
  -s WASM=1 -s SINGLE_FILE=1 \
  -O2 \
  -D_FORTIFY_SOURCE=2 \
  -fstack-protector-strong

echo "==> Generating icons..."
ICON_SRC="$ROOT/assets/favicon.png"
ICON_DIR="$ROOT/assets/icons"
mkdir -p "$ICON_DIR"
sips --cropToHeightWidth 404 404 "$ICON_SRC" --out "$ICON_DIR/square.png" >/dev/null 2>&1
for size in 16 32 48 180 192; do
  sips -z "$size" "$size" "$ICON_DIR/square.png" --out "$ICON_DIR/favicon-${size}.png" >/dev/null 2>&1
done

echo "==> Copying static assets..."
cp "$ROOT/index.html" "$DIST/"
cp "$ROOT/ice-config.js" "$DIST/"
cp "$ROOT/assets/favicon.svg" "$DIST/favicon.svg"
cp "$ICON_DIR/favicon-16.png" "$DIST/favicon-16.png"
cp "$ICON_DIR/favicon-32.png" "$DIST/favicon-32.png"
cp "$ICON_DIR/favicon-32.png" "$DIST/favicon.ico"
cp "$ICON_DIR/favicon-48.png" "$DIST/logo.png"
cp "$ICON_DIR/favicon-180.png" "$DIST/apple-touch-icon.png"
cp "$ICON_DIR/favicon-192.png" "$DIST/favicon-192.png"

echo "==> Generating Subresource Integrity hashes..."
CYBER_SRI="sha384-$(openssl dgst -sha384 -binary "$DIST/cyberknot.js" | openssl base64 -A)"

if [[ "$OSTYPE" == darwin* ]]; then
  sed -i '' "s|<script src=\"cyberknot.js?v=dev\"></script>|<script src=\"cyberknot.js\" integrity=\"${CYBER_SRI}\" crossorigin=\"anonymous\"></script>|" "$DIST/index.html"
else
  sed -i "s|<script src=\"cyberknot.js?v=dev\"></script>|<script src=\"cyberknot.js\" integrity=\"${CYBER_SRI}\" crossorigin=\"anonymous\"></script>|" "$DIST/index.html"
fi

echo "==> Build complete."
echo "    dist/cyberknot.js  SRI: ${CYBER_SRI}"
echo ""
echo "Local test (recommended):  ./scripts/run-local.sh"
echo "                           then open http://localhost:8080"
echo "Production:  cd deploy && cp .env.example .env && edit .env && docker compose up -d --build"

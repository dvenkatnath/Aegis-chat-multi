/*
 * CryptoRng.h — Secure random byte source for WASM builds.
 *
 * In the Emscripten/WASM environment there is no /dev/urandom and ftime()
 * gives only millisecond resolution (predictable, low-entropy salt).
 *
 * Instead we call back into JavaScript's crypto.getRandomValues() via an
 * EM_JS function and fill the caller's buffer with cryptographically
 * strong random bytes.
 *
 * On non-WASM builds (native Linux test builds) we fall back to
 * getrandom(2) / /dev/urandom so the same source file compiles everywhere.
 */

#ifndef CRYPTO_RNG_H
#define CRYPTO_RNG_H

#include "Hmac.h"  /* for byte typedef */

/*
 * Fill 'len' bytes of 'buf' with cryptographically strong random bytes.
 * Returns 0 on success, -1 on failure.
 * Callers must treat a non-zero return as a fatal error.
 */
int CryptoRng_fill(byte* buf, int len);

#endif /* CRYPTO_RNG_H */

/*
 * CryptoRng.c — Cryptographically strong random bytes.
 *
 * WASM path: uses an EM_JS shim that calls crypto.getRandomValues() in the
 *            browser.  This is the only correct source of entropy in a
 *            sandboxed WASM module — ftime(), rand(), and time() are all
 *            predictable and MUST NOT be used for IV/salt generation.
 *
 * Native path: uses getrandom(2) (Linux 3.17+) with a /dev/urandom fallback
 *              so the file compiles and works in native test builds.
 */

#include <stdlib.h>
#include <string.h>
#include <stdio.h>

#include "CryptoRng.h"

#ifdef __EMSCRIPTEN__

#include <emscripten.h>

/*
 * EM_JS defines a C-callable function whose body runs as JavaScript.
 * crypto.getRandomValues() fills a Uint8Array view over the WASM heap at
 * the pointer 'ptr' for 'len' bytes — no copy needed.
 * Returns 0 on success, -1 if crypto is unavailable (should never happen
 * in any modern browser or Node.js environment).
 */
EM_JS(int, js_crypto_random_fill, (byte* ptr, int len), {
    if (typeof crypto === 'undefined' || typeof crypto.getRandomValues !== 'function') {
        return -1;
    }
    crypto.getRandomValues(HEAPU8.subarray(ptr, ptr + len));
    return 0;
});

int CryptoRng_fill(byte* buf, int len) {
    if (buf == NULL || len <= 0) return -1;
    return js_crypto_random_fill(buf, len);
}

#else /* native build */

#ifdef __linux__
#include <sys/random.h>

int CryptoRng_fill(byte* buf, int len) {
    if (buf == NULL || len <= 0) return -1;
    ssize_t got = getrandom(buf, (size_t)len, 0);
    return (got == (ssize_t)len) ? 0 : -1;
}

#else /* fallback: /dev/urandom */

#include <stdio.h>

int CryptoRng_fill(byte* buf, int len) {
    if (buf == NULL || len <= 0) return -1;
    FILE* f = fopen("/dev/urandom", "rb");
    if (f == NULL) return -1;
    size_t got = fread(buf, 1, (size_t)len, f);
    fclose(f);
    return (got == (size_t)len) ? 0 : -1;
}

#endif /* __linux__ */
#endif /* __EMSCRIPTEN__ */

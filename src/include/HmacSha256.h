/*
 * HmacSha256.h — HMAC-SHA256 for CyberKnot message authentication.
 *
 * Replaces the HMAC-SHA1 used in EncryptionToolsApi for per-message MAC,
 * as recommended in the CyberKnot Technical Analysis Report (Finding 1).
 *
 * Output: 32 bytes (HMAC_SHA256_LEN).
 * The legacy HMAC_LEN (20-byte SHA-1) constant is preserved for the
 * getEncryptedPswd export pathway only; it is NOT used for message auth.
 */

#ifndef HMAC_SHA256_H
#define HMAC_SHA256_H

#include <stdint.h>
#include "Hmac.h"   /* for byte typedef */

/* HMAC-SHA256 output is 32 bytes */
#define HMAC_SHA256_LEN  32

/*
 * Compute HMAC-SHA256.
 *   key     - key bytes
 *   keyLen  - length of key in bytes
 *   data    - message bytes
 *   dataLen - length of message in bytes
 *   out     - buffer of HMAC_SHA256_LEN bytes, filled upon return
 */
void HmacSha256_compute(const byte* key,  int keyLen,
                        const byte* data, int dataLen,
                        byte out[HMAC_SHA256_LEN]);

/*
 * Constant-time comparison of two HMAC-SHA256 tags.
 * Returns 1 if equal, 0 if not.  Never short-circuits.
 */
int HmacSha256_verify(const byte expected[HMAC_SHA256_LEN],
                      const byte actual[HMAC_SHA256_LEN]);

#endif /* HMAC_SHA256_H */

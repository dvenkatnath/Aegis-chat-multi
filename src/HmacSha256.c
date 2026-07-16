/*
 * HmacSha256.c — HMAC-SHA256 implementation for CyberKnot.
 *
 * Built on the sha256_transform() / state machinery already present in
 * SHA256.c, but using raw binary output instead of the hex-string wrapper.
 *
 * Follows RFC 2104.  Block size B = 64 bytes.  Output L = 32 bytes.
 *
 * Key handling:
 *   - If keyLen > 64: key = SHA-256(key), then zero-pad to 64 bytes.
 *   - If keyLen <= 64: zero-pad to 64 bytes.
 */

#include <stdint.h>
#include <string.h>
#include <stdlib.h>

#include "HmacSha256.h"

/* ------------------------------------------------------------------ */
/* Minimal self-contained SHA-256 raw-output helper.                  */
/* We cannot call the existing sha256() because it returns a malloc'd  */
/* hex string. We inline the same state logic with binary output.      */
/* ------------------------------------------------------------------ */

#define SHA256_BLOCK  64
#define SHA256_DIGEST 32

/* Round constants — same as SHA256.c */
static const uint32_t K256[64] = {
    0x428a2f98, 0x71374491, 0xb5c0fbcf, 0xe9b5dba5,
    0x3956c25b, 0x59f111f1, 0x923f82a4, 0xab1c5ed5,
    0xd807aa98, 0x12835b01, 0x243185be, 0x550c7dc3,
    0x72be5d74, 0x80deb1fe, 0x9bdc06a7, 0xc19bf174,
    0xe49b69c1, 0xefbe4786, 0x0fc19dc6, 0x240ca1cc,
    0x2de92c6f, 0x4a7484aa, 0x5cb0a9dc, 0x76f988da,
    0x983e5152, 0xa831c66d, 0xb00327c8, 0xbf597fc7,
    0xc6e00bf3, 0xd5a79147, 0x06ca6351, 0x14292967,
    0x27b70a85, 0x2e1b2138, 0x4d2c6dfc, 0x53380d13,
    0x650a7354, 0x766a0abb, 0x81c2c92e, 0x92722c85,
    0xa2bfe8a1, 0xa81a664b, 0xc24b8b70, 0xc76c51a3,
    0xd192e819, 0xd6990624, 0xf40e3585, 0x106aa070,
    0x19a4c116, 0x1e376c08, 0x2748774c, 0x34b0bcb5,
    0x391c0cb3, 0x4ed8aa4a, 0x5b9cca4f, 0x682e6ff3,
    0x748f82ee, 0x78a5636f, 0x84c87814, 0x8cc70208,
    0x90befffa, 0xa4506ceb, 0xbef9a3f7, 0xc67178f2
};

/* Initial hash values */
static const uint32_t H0[8] = {
    0x6a09e667, 0xbb67ae85, 0x3c6ef372, 0xa54ff53a,
    0x510e527f, 0x9b05688c, 0x1f83d9ab, 0x5be0cd19
};

#define ROTR32(x, n) (((x) >> (n)) | ((x) << (32 - (n))))
#define SIG0(x) (ROTR32(x,7)  ^ ROTR32(x,18) ^ ((x)>>3))
#define SIG1(x) (ROTR32(x,17) ^ ROTR32(x,19) ^ ((x)>>10))
#define EP0(x)  (ROTR32(x,2)  ^ ROTR32(x,13) ^ ROTR32(x,22))
#define EP1(x)  (ROTR32(x,6)  ^ ROTR32(x,11) ^ ROTR32(x,25))
#define CH(x,y,z)  (((x)&(y))^(~(x)&(z)))
#define MAJ(x,y,z) (((x)&(y))^((x)&(z))^((y)&(z)))

typedef struct {
    uint32_t state[8];
    uint64_t bitlen;
    uint8_t  buf[SHA256_BLOCK];
    uint32_t buflen;
} Sha256Ctx;

static void sha256_compress(uint32_t state[8], const uint8_t blk[SHA256_BLOCK]) {
    uint32_t w[64], a, b, c, d, e, f, g, h1, t1, t2;
    int i;
    for (i = 0; i < 16; i++)
        w[i] = ((uint32_t)blk[i*4]<<24)|((uint32_t)blk[i*4+1]<<16)|
               ((uint32_t)blk[i*4+2]<<8)|(uint32_t)blk[i*4+3];
    for (; i < 64; i++)
        w[i] = SIG1(w[i-2]) + w[i-7] + SIG0(w[i-15]) + w[i-16];
    a=state[0]; b=state[1]; c=state[2]; d=state[3];
    e=state[4]; f=state[5]; g=state[6]; h1=state[7];
    for (i = 0; i < 64; i++) {
        t1 = h1 + EP1(e) + CH(e,f,g) + K256[i] + w[i];
        t2 = EP0(a) + MAJ(a,b,c);
        h1=g; g=f; f=e; e=d+t1; d=c; c=b; b=a; a=t1+t2;
    }
    state[0]+=a; state[1]+=b; state[2]+=c; state[3]+=d;
    state[4]+=e; state[5]+=f; state[6]+=g; state[7]+=h1;
}

static void sha256_init(Sha256Ctx* ctx) {
    memcpy(ctx->state, H0, sizeof(H0));
    ctx->bitlen = 0;
    ctx->buflen = 0;
}

static void sha256_update(Sha256Ctx* ctx, const uint8_t* data, size_t len) {
    size_t i;
    for (i = 0; i < len; i++) {
        ctx->buf[ctx->buflen++] = data[i];
        if (ctx->buflen == SHA256_BLOCK) {
            sha256_compress(ctx->state, ctx->buf);
            ctx->bitlen += 512;
            ctx->buflen = 0;
        }
    }
}

static void sha256_final(Sha256Ctx* ctx, uint8_t digest[SHA256_DIGEST]) {
    uint32_t i = ctx->buflen;
    ctx->buf[i++] = 0x80;
    if (i > 56) {
        while (i < 64) ctx->buf[i++] = 0;
        sha256_compress(ctx->state, ctx->buf);
        i = 0;
    }
    while (i < 56) ctx->buf[i++] = 0;
    ctx->bitlen += ctx->buflen * 8;
    ctx->buf[63] = (uint8_t)(ctx->bitlen);
    ctx->buf[62] = (uint8_t)(ctx->bitlen >> 8);
    ctx->buf[61] = (uint8_t)(ctx->bitlen >> 16);
    ctx->buf[60] = (uint8_t)(ctx->bitlen >> 24);
    ctx->buf[59] = (uint8_t)(ctx->bitlen >> 32);
    ctx->buf[58] = (uint8_t)(ctx->bitlen >> 40);
    ctx->buf[57] = (uint8_t)(ctx->bitlen >> 48);
    ctx->buf[56] = (uint8_t)(ctx->bitlen >> 56);
    sha256_compress(ctx->state, ctx->buf);
    for (i = 0; i < 8; i++) {
        digest[i*4  ] = (uint8_t)(ctx->state[i] >> 24);
        digest[i*4+1] = (uint8_t)(ctx->state[i] >> 16);
        digest[i*4+2] = (uint8_t)(ctx->state[i] >> 8);
        digest[i*4+3] = (uint8_t)(ctx->state[i]);
    }
}

/* ------------------------------------------------------------------ */
/* HMAC-SHA256 (RFC 2104)                                              */
/* ------------------------------------------------------------------ */

void HmacSha256_compute(const byte* key,  int keyLen,
                        const byte* data, int dataLen,
                        byte out[HMAC_SHA256_LEN])
{
    uint8_t k_pad[SHA256_BLOCK];
    uint8_t inner[SHA256_DIGEST];
    Sha256Ctx ctx;
    int i;

    /* Step 1: derive block-sized key */
    memset(k_pad, 0, SHA256_BLOCK);
    if (keyLen > SHA256_BLOCK) {
        /* hash the key down */
        sha256_init(&ctx);
        sha256_update(&ctx, (const uint8_t*)key, (size_t)keyLen);
        sha256_final(&ctx, k_pad);
    } else {
        memcpy(k_pad, key, (size_t)keyLen);
    }

    /* Step 2: inner hash = SHA-256(k_pad XOR ipad || data) */
    uint8_t ipad_key[SHA256_BLOCK];
    for (i = 0; i < SHA256_BLOCK; i++) ipad_key[i] = k_pad[i] ^ 0x36;
    sha256_init(&ctx);
    sha256_update(&ctx, ipad_key, SHA256_BLOCK);
    sha256_update(&ctx, (const uint8_t*)data, (size_t)dataLen);
    sha256_final(&ctx, inner);

    /* Step 3: outer hash = SHA-256(k_pad XOR opad || inner) */
    uint8_t opad_key[SHA256_BLOCK];
    for (i = 0; i < SHA256_BLOCK; i++) opad_key[i] = k_pad[i] ^ 0x5c;
    sha256_init(&ctx);
    sha256_update(&ctx, opad_key, SHA256_BLOCK);
    sha256_update(&ctx, inner, SHA256_DIGEST);
    sha256_final(&ctx, (uint8_t*)out);

    /* Wipe temporaries from stack */
    memset(k_pad,    0, sizeof(k_pad));
    memset(ipad_key, 0, sizeof(ipad_key));
    memset(opad_key, 0, sizeof(opad_key));
    memset(inner,    0, sizeof(inner));
}

/*
 * Constant-time HMAC comparison — prevents timing side-channel.
 * Returns 1 if equal, 0 otherwise.  Never short-circuits on mismatch.
 */
int HmacSha256_verify(const byte expected[HMAC_SHA256_LEN],
                      const byte actual[HMAC_SHA256_LEN])
{
    volatile uint8_t diff = 0;
    int i;
    for (i = 0; i < HMAC_SHA256_LEN; i++)
        diff |= expected[i] ^ actual[i];
    return (diff == 0) ? 1 : 0;
}

#include <stdio.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>

// SHA-256 constants
#define ROTRIGHT(a, b) (((a) >> (b)) | ((a) << (32 - (b))))
#define CH(x, y, z) (((x) & (y)) ^ (~(x) & (z)))
#define MAJ(x, y, z) (((x) & (y)) ^ ((x) & (z)) ^ ((y) & (z)))
#define EP0(x) (ROTRIGHT(x, 2) ^ ROTRIGHT(x, 13) ^ ROTRIGHT(x, 22))
#define EP1(x) (ROTRIGHT(x, 6) ^ ROTRIGHT(x, 11) ^ ROTRIGHT(x, 25))
#define SIG0(x) (ROTRIGHT(x, 7) ^ ROTRIGHT(x, 18) ^ ((x) >> 3))
#define SIG1(x) (ROTRIGHT(x, 17) ^ ROTRIGHT(x, 19) ^ ((x) >> 10))

// Initial hash values
uint32_t h[8] = {
    0x6a09e667, 0xbb67ae85, 0x3c6ef372, 0xa54ff53a,
    0x510e527f, 0x9b05688c, 0x1f83d9ab, 0x5be0cd19
};

// SHA-256 round constants
uint32_t k[64] = {
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

void sha256_transform(uint32_t state[8], const uint8_t data[]) {
    uint32_t a, b, c, d, e, f, g, h1, t1, t2, w[64];
    int i, j;

    for (i = 0, j = 0; i < 16; ++i, j += 4)
        w[i] = (data[j] << 24) | (data[j + 1] << 16) | (data[j + 2] << 8) | (data[j + 3]);

    for (; i < 64; ++i)
        w[i] = SIG1(w[i - 2]) + w[i - 7] + SIG0(w[i - 15]) + w[i - 16];

    a = state[0];
    b = state[1];
    c = state[2];
    d = state[3];
    e = state[4];
    f = state[5];
    g = state[6];
    h1 = state[7];

    for (i = 0; i < 64; ++i) {
        t1 = h1 + EP1(e) + CH(e, f, g) + k[i] + w[i];
        t2 = EP0(a) + MAJ(a, b, c);
        h1 = g;
        g = f;
        f = e;
        e = d + t1;
        d = c;
        c = b;
        b = a;
        a = t1 + t2;
    }

    state[0] += a;
    state[1] += b;
    state[2] += c;
    state[3] += d;
    state[4] += e;
    state[5] += f;
    state[6] += g;
    state[7] += h1;
}
char* sha256_to_string(uint8_t hash[32]) {
    char* hash_string = (char*)malloc(65); // 64 characters + null terminator
    if (!hash_string) {
        return NULL;
    }

    for (int i = 0; i < 32; ++i) {
        //sprintf_s(hash_string + (i * 2), 65 - (i * 2), "%02x", hash[i]);
        snprintf(hash_string + (i * 2), 65 - (i * 2), "%02x", hash[i]);
    }

    hash_string[64] = '\0'; // Null-terminate the string
    return hash_string;
}


char* sha256(const uint8_t data[], size_t len) {
    uint32_t state[8];
    memcpy(state, h, sizeof(h));

    size_t chunk_count = (len + 9) / 64 + 1;
    uint8_t chunk[64] = { 0 };

    for (size_t i = 0; i < chunk_count; ++i) {
        size_t offset = i * 64;
        size_t remaining = len - offset;
        if (remaining >= 64) {
            memcpy(chunk, data + offset, 64);
        }
        else {
            memcpy(chunk, data + offset, remaining);
            chunk[remaining] = 0x80;
            if (remaining <= 55) {
                uint64_t bit_len = len * 8;
                for (int j = 0; j < 8; j++)
                    chunk[63 - j] = (bit_len >> (8 * j)) & 0xff;
            }
        }
        sha256_transform(state, chunk);
    }

    uint8_t hash[32];
    for (int i = 0; i < 8; ++i) {
        hash[i * 4] = (state[i] >> 24) & 0xff;
        hash[i * 4 + 1] = (state[i] >> 16) & 0xff;
        hash[i * 4 + 2] = (state[i] >> 8) & 0xff;
        hash[i * 4 + 3] = state[i] & 0xff;
    }

    return sha256_to_string(hash);
}

char* SHAit( char* msg ) {

    char hash[257];
	char* hastptr = hash;
    hastptr = sha256((uint8_t*)msg, strlen(msg));
    //printf("SHA-256 hash of \"%s\": %s\n", msg, hastptr);
    return hastptr;
}

char* testSHA( char* msg, char* hash_string ) {
    //const char* msg = "hello world";
    //char* hash_string = sha256((uint8_t*)msg, strlen(msg));

    //const char* msg = "hello world";
    hash_string = sha256((uint8_t*)msg, strlen(msg));
    if (hash_string) {
        printf("SHA-256 hash of \"%s\": %s\n", msg, hash_string);
        //free(hash_string);
    }
    else {
        printf("Memory allocation failed\n");
    }
    return hash_string;

    
}

/*
 * sha256_buf() -- stack-based SHA-256, no malloc, no memory leak.
 *
 * out must point to a buffer of at least 65 bytes.
 * The hex digest (64 chars + NUL) is written there.
 * Returns out.
 *
 * Replaces the malloc-based sha256()/SHAit() used in SHAPassedDeux and
 * SHAPassed. Those functions called sha256() which malloc'd 65 bytes per
 * call, leaked the allocation on the double-SHA path in SHAPassedDeux,
 * and crashed after ~thousands of iterations once the heap was exhausted.
 */
char* sha256_buf(const uint8_t* data, size_t len, char out[65])
{
    uint32_t state[8];
    memcpy(state, h, sizeof(h));

    /* Number of 64-byte chunks needed (message + padding + length field) */
    size_t chunk_count = (len + 9) / 64 + 1;
    uint8_t chunk[64];

    for (size_t i = 0; i < chunk_count; ++i) {
        size_t offset    = i * 64;
        size_t remaining = (offset < len) ? (len - offset) : 0;

        memset(chunk, 0, 64);

        if (remaining >= 64) {
            memcpy(chunk, data + offset, 64);
        } else {
            if (remaining > 0) memcpy(chunk, data + offset, remaining);
            chunk[remaining] = 0x80;
            if (remaining <= 55) {
                uint64_t bit_len = (uint64_t)len * 8;
                for (int j = 0; j < 8; j++)
                    chunk[63 - j] = (uint8_t)(bit_len >> (8 * j));
            }
        }
        sha256_transform(state, chunk);
    }

    uint8_t hash[32];
    for (int i = 0; i < 8; ++i) {
        hash[i*4  ] = (state[i] >> 24) & 0xff;
        hash[i*4+1] = (state[i] >> 16) & 0xff;
        hash[i*4+2] = (state[i] >>  8) & 0xff;
        hash[i*4+3] =  state[i]        & 0xff;
    }

    for (int i = 0; i < 32; ++i)
        snprintf(out + i*2, 3, "%02x", hash[i]);
    out[64] = '\0';
    return out;
}

/* Stack-based SHAit replacement — no malloc */
char* SHAit_buf(char* msg, char out[65])
{
    return sha256_buf((const uint8_t*)msg, strlen(msg), out);
}

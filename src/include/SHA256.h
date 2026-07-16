#pragma once
#ifndef SHA256_H
#define SHA256_H

#include <stdint.h>
#include <stdlib.h>
#include <string.h>
#include <stdio.h>

// SHA-256 macros
#define ROTRIGHT(a, b) (((a) >> (b)) | ((a) << (32 - (b))))
#define CH(x, y, z) (((x) & (y)) ^ (~(x) & (z)))
#define MAJ(x, y, z) (((x) & (y)) ^ ((x) & (z)) ^ ((y) & (z)))
#define EP0(x) (ROTRIGHT(x, 2) ^ ROTRIGHT(x, 13) ^ ROTRIGHT(x, 22))
#define EP1(x) (ROTRIGHT(x, 6) ^ ROTRIGHT(x, 11) ^ ROTRIGHT(x, 25))
#define SIG0(x) (ROTRIGHT(x, 7) ^ ROTRIGHT(x, 18) ^ ((x) >> 3))
#define SIG1(x) (ROTRIGHT(x, 17) ^ ROTRIGHT(x, 19) ^ ((x) >> 10))

// External declaration of initial hash values
extern uint32_t h[8];

// External declaration of SHA-256 round constants
extern uint32_t k[64];

// Function prototypes
void sha256_transform(uint32_t state[8], const uint8_t data[]);
char* sha256_to_string(uint8_t hash[32]);
char* sha256(const uint8_t data[], size_t len);
char* testSHA(char* msg, char* hash_string);
char* SHAit( char* msg );

#endif // SHA256_H

/*
 * sha256_buf() -- stack-based SHA-256, no malloc.
 * out must be at least 65 bytes. Returns out.
 */
char* sha256_buf(const uint8_t* data, size_t len, char out[65]);

/*
 * SHAit_buf() -- stack-based wrapper matching SHAit() API.
 * out must be at least 65 bytes. Returns out.
 * Use this instead of SHAit() to avoid heap leaks.
 */
char* SHAit_buf(char* msg, char out[65]);

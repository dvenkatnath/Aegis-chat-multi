#include <emscripten.h>
#include <string.h>
#include <stdlib.h>
#include "api.h"
#include "common.h"
#include "HighLevelED.h"
#include "EncryptionToolsApi.h"
#include "HmacSha256.h"
#include "Base64.h"

/*
 * BRIDGE_MAX_PLAIN: maximum plaintext length accepted by bridge_encrypt*.
 * The CyberKnot packet payload limit is BBUFSZ (953) minus the per-message
 * overhead of SALT_LEN (8) + HMAC_SHA256_LEN (32) = 40 bytes → 913.
 * We use 880 as a conservative safe limit with margin.
 */
#define BRIDGE_MAX_PLAIN 880
#define BRIDGE_B64_OUT   8192
#define BRIDGE_MIN_KEY   32
#define BRIDGE_MAX_KEY   255

static byte enc_bin_buf[BBUFSZ];
static char enc_b64_buf[BRIDGE_B64_OUT];
static byte dec_bin_buf[eBBUFSZ];
static char dec_plain_buf[BBUFSZ + 1];
static char session_pswd[BRIDGE_MAX_KEY + 1];
static int session_pswd_active = 0;

EMSCRIPTEN_KEEPALIVE
int bridge_set_session_key(const char* key, int len) {
    if (key == NULL || len < BRIDGE_MIN_KEY || len > BRIDGE_MAX_KEY) {
        return -1;
    }
    memcpy(session_pswd, key, len);
    session_pswd[len] = '\0';
    session_pswd_active = 1;
    return 0;
}

EMSCRIPTEN_KEEPALIVE
int bridge_has_session_key(void) {
    return session_pswd_active;
}

EMSCRIPTEN_KEEPALIVE
void bridge_clear_session_key(void) {
    memset(session_pswd, 0, sizeof(session_pswd));
    session_pswd_active = 0;
}

static int init_ed(void) {
    if (!session_pswd_active) {
        return -1;
    }
    encryptionToolsApi_init();
    if (encryptionToolsApi_setPswd(session_pswd) < 0) {
        encryptionToolsApi_destructor();
        return -1;
    }
    return 0;
}

static void deinit_ed(void) {
    encryptionToolsApi_destructor();
}

static int decrypt_binary_packet(const byte* packet, int packetLen, char* outPlain) {
    if (packetLen < 4 || packetLen > eBBUFSZ) {
        return -1;
    }

    char PktLen[5];
    memcpy(PktLen, packet, 4);
    PktLen[4] = '\0';
    int payloadLen = atoi(PktLen);
    if (payloadLen <= 0 || 4 + payloadLen > packetLen) {
        return -1;
    }

    if (init_ed() != 0) {
        return -1;
    }
    int retVal = DecryptIt((byte*)(packet + 4), payloadLen, outPlain);
    deinit_ed();
    return retVal;
}

static int encrypt_plain_to_b64(const char* plaintext, int len) {
    if (len <= 0 || len > BRIDGE_MAX_PLAIN) {
        enc_b64_buf[0] = '\0';
        return -1;
    }

    if (init_ed() != 0) {
        enc_b64_buf[0] = '\0';
        return -1;
    }
    int encLen = EncryptIt((char*)plaintext, enc_bin_buf, len);
    deinit_ed();
    if (encLen <= 0 || encLen > BBUFSZ) {
        enc_b64_buf[0] = '\0';
        return -1;
    }

    int b64Len = B64_GetSize(encLen, B64_TRUE);
    if (b64Len + 1 >= BRIDGE_B64_OUT) {
        enc_b64_buf[0] = '\0';
        return -1;
    }

    B64_Encode(enc_bin_buf, encLen, enc_b64_buf);
    enc_b64_buf[b64Len] = '\0';
    return b64Len;
}

EMSCRIPTEN_KEEPALIVE
int bridge_encrypt(const char* plaintext, int len, char* out_buffer) {
    if (encrypt_plain_to_b64(plaintext, len) < 0) {
        return 0;
    }
    int binLen = B64_GetSize((int)strlen(enc_b64_buf), B64_FALSE);
    if (binLen <= 0 || binLen > BBUFSZ) {
        return 0;
    }
    B64_Decode(enc_b64_buf, (int)strlen(enc_b64_buf), enc_bin_buf);
    memcpy(out_buffer, enc_bin_buf, binLen);
    return binLen;
}

EMSCRIPTEN_KEEPALIVE
int bridge_decrypt(const char* ciphertext, int len, char* out_buffer) {
    if (len <= 0 || len > eBBUFSZ) {
        return 0;
    }
    if (decrypt_binary_packet((const byte*)ciphertext, len, dec_plain_buf) != 0) {
        return 0;
    }
    int dlen = (int)strlen(dec_plain_buf);
    memcpy(out_buffer, dec_plain_buf, dlen);
    return dlen;
}

EMSCRIPTEN_KEEPALIVE
const char* bridge_encrypt_b64(const char* plaintext, int len) {
    encrypt_plain_to_b64(plaintext, len);
    return enc_b64_buf;
}

EMSCRIPTEN_KEEPALIVE
const char* bridge_decrypt_b64(const char* b64_ciphertext) {
    int b64Len = (int)strlen(b64_ciphertext);
    int binLen = B64_GetSize(b64Len, B64_FALSE);
    if (binLen <= 0 || binLen > (int)sizeof(dec_bin_buf)) {
        dec_plain_buf[0] = '\0';
        return dec_plain_buf;
    }

    B64_Decode(b64_ciphertext, b64Len, dec_bin_buf);

    if (decrypt_binary_packet(dec_bin_buf, binLen, dec_plain_buf) != 0) {
        dec_plain_buf[0] = '\0';
        return dec_plain_buf;
    }

    return dec_plain_buf;
}

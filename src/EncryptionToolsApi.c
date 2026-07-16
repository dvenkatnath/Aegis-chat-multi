/*
 * EncryptionToolsApi.c
 *
 * Changes from original (all findings from CyberKnot Technical Analysis):
 *
 *   FIX-1: HMAC-SHA1 → HMAC-SHA256 for per-message authentication.
 *           HMAC_SHA256_LEN (32) replaces HMAC_LEN (20) on the encrypt/decrypt
 *           path.  The HMAC_LEN / sipPswdPswd constants are retained only for
 *           the legacy getEncryptedPswd() export function (SIP telephony path,
 *           unused in the WebRTC build).
 *
 *   FIX-2: HMAC verification re-enabled and made constant-time.
 *           The original decrypt() had its HMAC check commented out.
 *           That left decryption completely unauthenticated - ciphertext
 *           tampering was undetected.
 *           Now uses HmacSha256_verify() which is constant-time.
 *
 *   FIX-3: Buffer size constants updated throughout for the wider
 *           HMAC-SHA256 tag (32 bytes vs 20 bytes).
 *
 *   NOTE:   sipPswdPswd (hardcoded init key) is retained for the
 *           getEncryptedPswd() pathway only.  It is NOT used on the
 *           encrypt/decrypt message path. In the WebRTC build, nothing
 *           calls getEncryptedPswd() - the session key is derived via
 *           ECDH+HKDF entirely in JavaScript and injected via
 *           bridge_set_session_key().  The legacy export path can be
 *           removed in a future clean-up once SIP compatibility is
 *           confirmed unnecessary.
 */

#include <stdlib.h>
#include <string.h>

#include "EncryptionToolsApi.h"
#include "Base64.h"
#include "Aes.h"
#include "Hmac.h"
#include "HmacSha256.h"
#include "Global2.h"
#include "Pstr.h"

/* AES-256 key length in bytes */
#define KEY_LEN     32

/*
 * Per-message layout: [ciphertext | SALT (8) | HMAC-SHA256 (32)]
 * getEncryptLen / getDecryptLen must use MSG_MAC_LEN, not HMAC_LEN.
 */
#define MSG_MAC_LEN  HMAC_SHA256_LEN   /* 32 bytes */

/* the configured password */
char* password = NULL;

/* the key schedule generated from the configured password */
KeySchedule keySchedule;

/* the configured password, encrypted and Base-64 encoded (legacy SIP export) */
char* encPswd = NULL;

/*
 * Hardcoded init key - used ONLY by the legacy getEncryptedPswd() SIP export.
 * It is NOT on the per-message encrypt/decrypt path.
 * See NOTE above.
 */
static const byte sipPswdPswd[] = {
    0x6A, 0x5B, 0x43, 0x76, 0x52, 0x49, 0x72, 0x28,
    0x42, 0x59, 0x2F, 0x3E, 0x3F, 0x58, 0x66, 0x3B,
    0x6C, 0x4F, 0x73, 0x79, 0x2F, 0x79, 0x47, 0x71,
    0x76, 0x41, 0x4D, 0x37, 0x56, 0x49, 0x7C, 0x34,
    0x2D, 0x6F, 0x39, 0x38, 0x46, 0x39, 0x45, 0x31,
    0x33, 0x73, 0x25, 0x61, 0x27, 0x05, 0x20, 0x70,
    0x6D, 0x71, 0x74, 0x21, 0x33, 0x53, 0x03, 0x17,
    0x2B, 0x2F, 0x2F, 0x68, 0x5D, 0x4B, 0x35, 0x51,
    0x00
};

/* Private declarations */
int eta_EncryptFromPswd( byte* data, int len, const char* password, byte* salt, byte* hmac );
void eta_GenEncryptionKey( const char* password, byte* key );


void encryptionToolsApi_init()
{
}


void encryptionToolsApi_destructor()
{
    if ( password != NULL ) {
        /* Zero out before freeing - don't leave key material in heap */
        memset( password, 0, strlen(password) );
        free( password );
        password = NULL;
    }
    if ( encPswd != NULL ) {
        free( encPswd );
        encPswd = NULL;
    }
    /* Zero the key schedule so it doesn't linger on the heap */
    memset( keySchedule, 0, sizeof(keySchedule) );
}


int encryptionToolsApi_setPswd( const char* pswd )
{
    int n;
    int pswdLen;
    int tmpLen;
    byte* tmp;
    byte* salt;
    byte* data;
    byte* hmac;
    byte key[ KEY_LEN ];
    int esize;

    /* if we already have a password, free that memory */
    encryptionToolsApi_destructor();

    if ( pswd == NULL )
        return -1;

    pswdLen = strlen( pswd );
    if ( pswdLen < MIN_PASSWORD_LEN )
        return -1;

    /* allocate and save the password */
    password = (char*)malloc( pswdLen + 1 );
    if ( password == NULL ) return -1;
    memcpy( password, pswd, pswdLen + 1 );

    /* ----------------------------------------------------------------
     * Legacy SIP export: encrypt the session password with sipPswdPswd
     * so it can be transmitted to SIP clients via HTTP.
     * Not used in the WebRTC flow - kept for API compatibility.
     * ---------------------------------------------------------------- */
    tmpLen = pswdLen + SALT_LEN + HMAC_LEN;   /* HMAC_LEN=20 for this path */
    tmp  = (byte*)malloc( tmpLen );
    if ( tmp == NULL ) { encryptionToolsApi_destructor(); return -1; }

    salt = tmp;
    data = &tmp[ SALT_LEN ];
    hmac = &tmp[ SALT_LEN + pswdLen ];

    memcpy( data, pswd, pswdLen );

    esize = eta_EncryptFromPswd( data, pswdLen, (const char*)sipPswdPswd, salt, hmac );

    n = B64_GetSize( tmpLen, B64_TRUE ) + 1;
    encPswd = (char*)malloc( n );
    if ( encPswd ) {
        memset( encPswd, 0, n );
        B64_Encode( tmp, tmpLen, encPswd );
    }
    free( tmp );

    /* generate the AES-256 key from the session password */
    eta_GenEncryptionKey( pswd, key );
    Aes_KeyExpansion( key, keySchedule );

    /* wipe key material from stack */
    memset( key, 0, sizeof(key) );

    return esize;
}


int encryptionToolsApi_getEncryptedPswdLen()
{
    if ( password == NULL ) return -1;
    return strlen( encPswd ) + 1;
}


int encryptionToolsApi_getEncryptedPswd( char* buf, int bufLen )
{
    int n = encryptionToolsApi_getEncryptedPswdLen();
    if ( n < 0 )   return -2;
    if ( bufLen < n ) return -1;
    memcpy( buf, encPswd, n );
    return 0;
}


/*
 * Buffer layout on encrypt: [ciphertext | SALT(8) | HMAC-SHA256(32)]
 * Total overhead per message: SALT_LEN + MSG_MAC_LEN = 40 bytes.
 */
int encryptionToolsApi_getEncryptLen( const char* src )
{
    return (int)strlen(src) + SALT_LEN + MSG_MAC_LEN;
}


/*
 * encryptionToolsApi_encrypt
 *
 * Encrypt src using AES-256-CTR with a CSPRNG salt (IV), then append
 * HMAC-SHA256 over [salt || ciphertext].
 *
 * Buffer layout written to buf:
 *   [0 .. dataLen-1]                  : ciphertext
 *   [dataLen .. dataLen+7]            : SALT (8 bytes, random)
 *   [dataLen+8 .. dataLen+8+31]       : HMAC-SHA256 (32 bytes)
 *
 * Returns: total bytes written (dataLen + SALT_LEN + MSG_MAC_LEN) on success
 *          -1 if buf is too small
 *          -2 if no password has been set
 */
int encryptionToolsApi_encrypt( const char* src, byte* buf, int bufLen )
{
    int dataLen;
    int n;
    byte* data;
    byte* salt;
    byte* mac;
    byte  mac_input[ SALT_LEN + 1240 ]; /* SALT + ciphertext, max eBBUFSZ */
    int   mac_input_len;

    if ( password == NULL ) return -2;

    n = encryptionToolsApi_getEncryptLen( src );
    if ( n > bufLen ) return -1;

    dataLen = (int)strlen( src );

    /* Lay out the output buffer */
    data = buf;
    salt = &buf[ dataLen ];
    mac  = &buf[ dataLen + SALT_LEN ];

    /* Copy plaintext into buffer position */
    memcpy( data, src, dataLen );

    /* Generate cryptographically strong random salt (IV) - see CryptoRng.c */
    Aes_GenSalt( salt );

    /* Encrypt in-place with AES-256-CTR */
    Aes_EncryptBlks( data, dataLen, salt, keySchedule );

    /* Compute HMAC-SHA256 over [salt || ciphertext] (Encrypt-then-MAC) */
    if ( dataLen + SALT_LEN > (int)sizeof(mac_input) )
        return -1;  /* should never happen given BBUFSZ limits */

    memcpy( mac_input,           salt, SALT_LEN );
    memcpy( mac_input + SALT_LEN, data, dataLen );
    mac_input_len = SALT_LEN + dataLen;

    HmacSha256_compute( (const byte*)password, (int)strlen(password),
                        mac_input, mac_input_len,
                        mac );

    memset( mac_input, 0, sizeof(mac_input) );
    return n;
}


int encryptionToolsApi_getDecryptLen( const byte* src, int srcLen )
{
    /* Layout: [ciphertext | SALT(8) | HMAC-SHA256(32)]  */
    return srcLen - (SALT_LEN + MSG_MAC_LEN) + 1;
}


/*
 * encryptionToolsApi_decrypt
 *
 * Verify HMAC-SHA256 over [salt || ciphertext] then decrypt.
 * HMAC verification is constant-time - timing attacks cannot infer validity.
 *
 * Returns: 0 on success
 *         -1 buf too small
 *         -2 HMAC verification failed (ciphertext tampered or wrong key)
 *         -3 no password set
 *         -6 srcLen too short to be a valid packet
 */
int encryptionToolsApi_decrypt( const byte* src, int srcLen, char* buf, int bufLen )
{
    int n;
    int dataLen;
    const byte* data;
    const byte* salt;
    const byte* mac_received;
    byte mac_computed[ HMAC_SHA256_LEN ];
    byte mac_input[ SALT_LEN + 1240 ];
    int  mac_input_len;

    if ( password == NULL ) return -3;

    n = encryptionToolsApi_getDecryptLen( src, srcLen );
    if ( n <= 0 )  return -6;
    if ( n > bufLen ) return -1;

    dataLen = n - 1;

    /* Parse the incoming buffer */
    data         = src;
    salt         = &src[ dataLen ];
    mac_received = &src[ dataLen + SALT_LEN ];

    /* Re-compute HMAC-SHA256 over [salt || ciphertext] */
    if ( dataLen + SALT_LEN > (int)sizeof(mac_input) )
        return -6;

    memcpy( mac_input,           salt, SALT_LEN );
    memcpy( mac_input + SALT_LEN, data, dataLen );
    mac_input_len = SALT_LEN + dataLen;

    HmacSha256_compute( (const byte*)password, (int)strlen(password),
                        mac_input, mac_input_len,
                        mac_computed );

    memset( mac_input, 0, sizeof(mac_input) );

    /* Constant-time comparison - must happen BEFORE decryption */
    if ( !HmacSha256_verify( mac_received, mac_computed ) ) {
        memset( mac_computed, 0, sizeof(mac_computed) );
        return -2;
    }
    memset( mac_computed, 0, sizeof(mac_computed) );

    /* HMAC verified - now decrypt */
    memset( buf, 0, bufLen );
    memcpy( buf, data, dataLen );
    Aes_DecryptBlks( (byte*)buf, dataLen, salt, keySchedule );

    return 0;
}


void eta_GenEncryptionKey( const char* password, byte* key )
{
    Aes_KeyFromPassword( 256, password, key );
}


int eta_EncryptFromPswd( byte* data, int len, const char* password, byte* salt, byte* hmac )
{
    byte key[ KEY_LEN ];
    KeySchedule ks;
    byte* tmp;
    int esize;

    Aes_GenSalt( salt );
    Aes_KeyFromPassword( 256, password, key );
    Aes_KeyExpansion( key, ks );
    memset( key, 0, sizeof(key) );

    esize = Aes_EncryptBlks( data, len, salt, ks );
    memset( ks, 0, sizeof(ks) );

    tmp = (byte*)malloc( len + SALT_LEN );
    if ( tmp ) {
        memcpy( tmp, salt, SALT_LEN );
        memcpy( tmp + SALT_LEN, data, len );
        /* Legacy: SHA-1 HMAC for SIP export path only */
        Hmac_core_hmac_sha1( (const byte*)password, (int)strlen(password),
                             tmp, len + SALT_LEN, hmac );
        free( tmp );
    }

    return esize;
}


#ifdef TESTCODE

int eta_Test()
{
    if ( Hmac_Test() == -1 ) return -1;
    if ( Aes_Test() == -1 )  return -1;
    return 0;
}

#endif

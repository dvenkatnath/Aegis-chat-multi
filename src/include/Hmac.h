/*
 * HMAC code obtained from
 * http://pajhome.org.uk/crypt/md5/sha1src.html
 *    - // http://tools.ietf.org/html/rfc4648
 *
 * A JavaScript implementation of the Secure Hash Algorithm, SHA-1, as defined
 * in FIPS PUB 180-1
 * Version 2.1a Copyright Paul Johnston 2000 - 2002.
 * Other contributors: Greg Holt, Andrew Kepert, Ydnar, Lostinet
 * Distributed under the BSD License
 * See http://pajhome.org.uk/crypt/md5 for details.
 */

#ifndef HMAC_H
#define HMAC_H

typedef unsigned char byte;
typedef unsigned int uint;

// an HMAC is always exactly 20 bytes
#define HMAC_LEN		20

// an HMAC is always exactly 5 words
#define HMAC_LEN_WORDS	5


// generate the HMAC for the given data, using the given password bytes
// hmac - buffer of HMAC_LEN, filled upon return
void core_hmac_sha1( const byte* pswd, int pswdLen, const byte* data, int dataLen, byte* hmac );

#ifdef TESTCODE

// do an internal test
int Hmac_Test();

#endif // TESTCODE

// generate the HMAC for the given data, using the given password bytes
// hmac - buffer of HMAC_LEN, filled upon return
void Hmac_core_hmac_sha1( const byte* pswd, int pswdLen, const byte* data, int dataLen, byte* hmac );

// Calculate the SHA-1 of an array of big-endian words, and a bit length.
// xLen - number of words in x
// Len - the number of bits in the data, excluding padding bytes.
// out - array of length HMAC_LEN_WORDS, filled upon return
void Hmac_core_sha1( uint* xOrig, int xLen, int len, uint* out );

// Perform the appropriate triplet combination function for the current
// iteration
uint Hmac_sha1_ft( uint t, uint b, uint c, uint d );

// Determine the appropriate additive constant for the current iteration
uint Hmac_sha1_kt( uint t );

// Add integers, wrapping at 2^32. This uses 16-bit operations internally
// to work around bugs in some JS interpreters.
uint Hmac_safe_add( uint x, uint y );

// Bitwise rotate a 32-bit number to the left.
uint Hmac_rol( uint num, int cnt);

// convert an array of bytes to a packed array of words,
// padding with zeros
// out - array of length Ceiling(len/4), filled upon return
void Hmac_BytesToWords( const byte* bytes, int len, uint* out );

// return the Ceiling of a/b
int Hmac_Ceiling( int a, int b );

#endif // HMAC_H

#include <stdlib.h>
#include <string.h>

#include "Hmac.h"


// generate the HMAC for the given data, using the given password bytes
// hmac - buffer of HMAC_LEN, filled upon return
void Hmac_core_hmac_sha1( const byte* pswd, int pswdLen, const byte* data, int dataLen, byte* hmac )
{
	int uiPswdLen;
	int uiDataLen;
	int i;
	int len;
	int j;
	uint* uiPswd;
	uint* uiData;
	uint* tmp;
	uint ipad[16];
	uint opad[16];
	uint hash[ HMAC_LEN_WORDS ];
	uint ihmac[ HMAC_LEN_WORDS ];

	uiPswdLen = Hmac_Ceiling( pswdLen, 4 );
	uiPswd = (uint*)malloc( sizeof(uint) * uiPswdLen );
	Hmac_BytesToWords( pswd, pswdLen, uiPswd );

	uiDataLen = Hmac_Ceiling( dataLen, 4 );
	uiData = (uint*)malloc( sizeof(uint) * uiDataLen );
	Hmac_BytesToWords( data, dataLen, uiData );

	// ensure we have exactly 16 pswd words 
	if( uiPswdLen < 16 ) {
		tmp = (uint*)malloc( sizeof(uint) * 16 );
		memcpy( tmp, uiPswd, uiPswdLen * sizeof(uint) );
		for( i = uiPswdLen; i != 16; ++i )
			tmp[i] = 0;
		free( uiPswd );
		uiPswd = tmp;
		uiPswdLen = 16;
	}

	for( i = 0; i < 16; i++ ) {
		ipad[i] = uiPswd[i] ^ 0x36363636;
		opad[i] = uiPswd[i] ^ 0x5C5C5C5C;
	}

	len = 16 + uiDataLen;
	tmp = (uint*)malloc( sizeof(uint) * len );
	memcpy( tmp, ipad, 16 * sizeof(uint) );
	memcpy( tmp + 16, uiData, uiDataLen * sizeof(uint) );
	Hmac_core_sha1( tmp, len, 512 + ( dataLen * 8 ), hash );
	free( tmp );

	len = 16 + HMAC_LEN_WORDS;
	tmp = (uint*)malloc( sizeof(uint) * len );
	memcpy( tmp, opad, 16 * sizeof(uint) );
	memcpy( tmp + 16, hash, HMAC_LEN_WORDS * sizeof(uint) );
	Hmac_core_sha1( tmp, len, 512 + (HMAC_LEN_WORDS * 32), ihmac );
	free( tmp );

	// convert the array of HMAC_LEN_WORDS 32-bit words to HMAC_LEN bytes
	for( i = 0; i != HMAC_LEN_WORDS; i++ ) {
		j = 4 * (i + 1);
		hmac[ j - 1 ] = (byte)( ihmac[i] & 0x0ff );
		ihmac[i] >>= 8;
		hmac[ j - 2 ] = (byte)( ihmac[i] & 0x0ff );
		ihmac[i] >>= 8;
		hmac[ j - 3 ] = (byte)( ihmac[i] & 0x0ff );
		ihmac[i] >>= 8;
		hmac[ j - 4 ] = (byte)( ihmac[i] & 0x0ff );
	}

	free( uiPswd );
	free( uiData );
}


// Calculate the SHA-1 of an array of big-endian words, and a bit length.
// xLen - number of words in x
// Len - the number of bits in the data, excluding padding bytes.
// out - array of length HMAC_LEN_WORDS, filled upon return
void Hmac_core_sha1( uint* xOrig, int xLen, int len, uint* out )
{
	uint* x;
	uint* newX;
	int newLen;
	int i;
	uint w[80];
	uint a;
	uint b;
	uint c;
	uint d;
	uint e;
	uint olda;
	uint oldb;
	uint oldc;
	uint oldd;
	uint olde;
	uint j;
	uint t;

	x = xOrig;
	newX = NULL;

	// increase the length of x.
	// new array elements have the value of 0.
	newLen = ( ( ( len + 64 ) >> 9 ) << 4 ) + 15 + 1;
	if( xLen < newLen ) {
		newX = (uint*)malloc( sizeof(uint) * newLen );
		memcpy( newX, x, xLen * sizeof(uint) );
		memset( newX + xLen, 0, ( newLen - xLen ) * sizeof(uint) );
		x = newX;
		xLen = newLen;
	}

	x[len >> 5] |= ( (uint)0x80 ) << ( 24 - len % 32 );
	x[(((len + 64) >> 9) << 4) + 15] = (uint)len;

	a = 0x67452301;	// 1732584193;
	b = 0xEFCDAB89;	// -271733879;
	c = 0x98BADCFE;	// -1732584194;
	d = 0x10325476;	// 271733878;
	e = 0xC3D2E1F0;	// -1009589776;

	for( i = 0; i < xLen; i += 16 ) {
		olda = a;
		oldb = b;
		oldc = c;
		oldd = d;
		olde = e;

		for( j = 0; j < 80; j++ ) {
			if( j < 16 ) 
				w[j] = x[i + j];
			else 
				w[j] = Hmac_rol( w[j-3] ^ w[j-8] ^ w[j-14] ^ w[j-16], 1 );

			t = Hmac_safe_add( 
						Hmac_safe_add( Hmac_rol(a, 5), Hmac_sha1_ft(j, b, c, d) ),
						Hmac_safe_add( Hmac_safe_add(e, w[j]), Hmac_sha1_kt(j) )
					);
			e = d;
			d = c;
			c = Hmac_rol(b, 30);
			b = a;
			a = t;
		}

		a = Hmac_safe_add( a, olda );
		b = Hmac_safe_add( b, oldb );
		c = Hmac_safe_add( c, oldc );
		d = Hmac_safe_add( d, oldd );
		e = Hmac_safe_add( e, olde );
	}

	if( newX != NULL )
		free( newX );

	out[0] = a;
	out[1] = b;
	out[2] = c;
	out[3] = d;
	out[4] = e;
}


// Perform the appropriate triplet combination function for the current
// iteration
uint Hmac_sha1_ft( uint t, uint b, uint c, uint d )
{
	if(t < 20) return (b & c) | ((~b) & d);
	if(t < 40) return b ^ c ^ d;
	if(t < 60) return (b & c) | (b & d) | (c & d);
	return b ^ c ^ d;
}


// Determine the appropriate additive constant for the current iteration
uint Hmac_sha1_kt( uint t )
{
	return	( t < 20 ) ? 1518500249 : ( t < 40 ) ? 1859775393 :
			( t < 60 ) ? 0x8F1BBCDC : 0xCA62C1D6;
}


// Add integers, wrapping at 2^32. This uses 16-bit operations internally
// to work around bugs in some JS interpreters.
uint Hmac_safe_add( uint x, uint y )
{
	return ( x + y ) & 0x0FFFFFFFF;
}


// Bitwise rotate a 32-bit number to the left.
uint Hmac_rol( uint num, int cnt)
{
	return (num << cnt) | (num >> (32 - cnt));
}


// convert an array of bytes to a packed array of words,
// padding with zeros
// words - array of length Ceiling(len/4), filled upon return
void Hmac_BytesToWords( const byte* bytes, int len, uint* words )
{
	int fullWordCnt;
	int srcNdx;
	int i;

	// get the number of full words we can pack
	fullWordCnt = len / 4;

	// pack the full words
	srcNdx = 0;
	for( i = 0; i != fullWordCnt; ++i ) {
		words[i] = (uint)(
				   ( bytes[srcNdx] << 24 )
				 | ( bytes[srcNdx + 1] << 16 )
				 | ( bytes[srcNdx + 2] << 8 )
				 | bytes[srcNdx + 3] );
		srcNdx += 4;
	}

	// pack the remaining bytes in the last word
	if( srcNdx != len )
		words[fullWordCnt] = (uint)( bytes[srcNdx++] << 24 );
	if( srcNdx != len )
		words[fullWordCnt] |= (uint)( bytes[srcNdx++] << 16 );
	if( srcNdx != len )
		words[fullWordCnt] |= (uint)( bytes[srcNdx++] << 8 );
}


// return the Ceiling of a/b
int Hmac_Ceiling( int a, int b )
{
	int r;

	r = a / b;
	if( r * b < a )
		++r;
	return r;
}

#ifdef TESTCODE

// do an internal test
int Hmac_Test()
{
	const char* str;
	int strLen;
	int numWords;
	int i;
	int pswdLen;
	int dataLen;
	uint* words;
	uint sha[ HMAC_LEN_WORDS ];
	uint expectedSha[] = { 0xa9993e36, 0x4706816a, 0xba3e2571, 0x7850c26c, 0x9cd0d89d };
	const char* pswd = "12345678901234567890123456789012";
	const char* data = "this is my original string";
	byte hmac[ HMAC_LEN ];
	byte expectedHmac[] = {	0x9b, 0x00, 0x7e, 0x40, 0x3c, 0xb2, 0xd1, 0xa6, 0x0c, 0x51, 
							0x36, 0x42, 0xee, 0xf0, 0xc1, 0x96, 0x1e, 0x92, 0xe1, 0x7e };

	str = "abc";
	strLen = (int)strlen( str );
	numWords = Hmac_Ceiling( strLen, 4 );
	words = (uint*)malloc( sizeof(uint) * numWords );
	Hmac_BytesToWords( (byte*)str, strLen, words );
	Hmac_core_sha1( words, numWords, strLen * 8, sha );
	free( words );

	for( i = 0; i != HMAC_LEN_WORDS; i++ )
		if( sha[i] != expectedSha[i] )
			return -1;

	pswdLen = (int)strlen( pswd );
	dataLen = (int)strlen( data );
	Hmac_core_hmac_sha1( (byte*)pswd, pswdLen, (byte*)data, dataLen, hmac );

	for( i = 0; i != HMAC_LEN; i++ )
		if( hmac[i] != expectedHmac[i] )
			return -1;

	return 0;
}

#endif // TESTCODE

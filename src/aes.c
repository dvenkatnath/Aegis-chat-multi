#include <stdlib.h>
#include <stdio.h>
#include <string.h>

#include "Aes.h"
#include "EncryptionToolsApi.h"
#include "Hmac.h"
#include "CryptoRng.h"

// AES data table
const byte Aes_Sbox[256] = {
	0x63,0x7c,0x77,0x7b,0xf2,0x6b,0x6f,0xc5,0x30,0x01,0x67,0x2b,0xfe,0xd7,0xab,0x76,
	0xca,0x82,0xc9,0x7d,0xfa,0x59,0x47,0xf0,0xad,0xd4,0xa2,0xaf,0x9c,0xa4,0x72,0xc0,
	0xb7,0xfd,0x93,0x26,0x36,0x3f,0xf7,0xcc,0x34,0xa5,0xe5,0xf1,0x71,0xd8,0x31,0x15,
	0x04,0xc7,0x23,0xc3,0x18,0x96,0x05,0x9a,0x07,0x12,0x80,0xe2,0xeb,0x27,0xb2,0x75,
	0x09,0x83,0x2c,0x1a,0x1b,0x6e,0x5a,0xa0,0x52,0x3b,0xd6,0xb3,0x29,0xe3,0x2f,0x84,
	0x53,0xd1,0x00,0xed,0x20,0xfc,0xb1,0x5b,0x6a,0xcb,0xbe,0x39,0x4a,0x4c,0x58,0xcf,
	0xd0,0xef,0xaa,0xfb,0x43,0x4d,0x33,0x85,0x45,0xf9,0x02,0x7f,0x50,0x3c,0x9f,0xa8,
	0x51,0xa3,0x40,0x8f,0x92,0x9d,0x38,0xf5,0xbc,0xb6,0xda,0x21,0x10,0xff,0xf3,0xd2,
	0xcd,0x0c,0x13,0xec,0x5f,0x97,0x44,0x17,0xc4,0xa7,0x7e,0x3d,0x64,0x5d,0x19,0x73,
	0x60,0x81,0x4f,0xdc,0x22,0x2a,0x90,0x88,0x46,0xee,0xb8,0x14,0xde,0x5e,0x0b,0xdb,
	0xe0,0x32,0x3a,0x0a,0x49,0x06,0x24,0x5c,0xc2,0xd3,0xac,0x62,0x91,0x95,0xe4,0x79,
	0xe7,0xc8,0x37,0x6d,0x8d,0xd5,0x4e,0xa9,0x6c,0x56,0xf4,0xea,0x65,0x7a,0xae,0x08,
	0xba,0x78,0x25,0x2e,0x1c,0xa6,0xb4,0xc6,0xe8,0xdd,0x74,0x1f,0x4b,0xbd,0x8b,0x8a,
	0x70,0x3e,0xb5,0x66,0x48,0x03,0xf6,0x0e,0x61,0x35,0x57,0xb9,0x86,0xc1,0x1d,0x9e,
	0xe1,0xf8,0x98,0x11,0x69,0xd9,0x8e,0x94,0x9b,0x1e,0x87,0xe9,0xce,0x55,0x28,0xdf,
	0x8c,0xa1,0x89,0x0d,0xbf,0xe6,0x42,0x68,0x41,0x99,0x2d,0x0f,0xb0,0x54,0xbb,0x16
};

// AES data table
const byte Aes_Rcon[11][4] = {
	{ 0x00, 0x00, 0x00, 0x00 },
	{ 0x01, 0x00, 0x00, 0x00 },
	{ 0x02, 0x00, 0x00, 0x00 },
	{ 0x04, 0x00, 0x00, 0x00 },
	{ 0x08, 0x00, 0x00, 0x00 },
	{ 0x10, 0x00, 0x00, 0x00 },
	{ 0x20, 0x00, 0x00, 0x00 },
	{ 0x40, 0x00, 0x00, 0x00 },
	{ 0x80, 0x00, 0x00, 0x00 },
	{ 0x1b, 0x00, 0x00, 0x00 },
	{ 0x36, 0x00, 0x00, 0x00 }
}; 


// declarations for private methods
void Aes_ReverseBytes( byte* b, int bLen );


// generate a "key" of the specified number of bits, 
// using the given password string
// numBits - number of bits - 128, 192 or 256
// key - buffer of (numBits/8) bytes, filled upon return 
void Aes_KeyFromPassword( int numBits, const char* password, byte* key )
{
	int numBytes;
	byte* pwBytes;
	int pswdLen;
	byte hmac[ HMAC_LEN ];
	int i;
	byte key16[ AES_BLOCKSIZE ];
	KeySchedule ks;

	numBytes = numBits / 8;
	pwBytes = (byte*)malloc( numBytes );

	// the cipher key must be 32 bytes.
	// In order to allow longer password strings,
	// we first get the HMAC of the password string, resulting in 20 bytes,
	// and suffix that with the last 12 bytes of the original password string.
	// The minimum length of the password string is 32 characters.
	pswdLen = (int)strlen( password );
	Hmac_core_hmac_sha1( (const byte*)password, pswdLen, (const byte*)password, pswdLen, hmac );
	//memcpy( pwBytes, hmac, HMAC_LEN );
    if (pwBytes != NULL && hmac != NULL) {
        memcpy(pwBytes, hmac, HMAC_LEN);
    } else {
        // Handle error: either pwBytes or hmac is NULL
        fprintf(stderr, "Error: pwBytes or hmac is NULL\n");
        exit(EXIT_FAILURE);
    }
	for( i = HMAC_LEN; i < numBytes; i++ )
		pwBytes[i] = password[( pswdLen - 1 ) - ( i - HMAC_LEN )];

	// use AES itself to encrypt the password bytes to get the cipher key.
	// gives us a 16-byte key
	Aes_KeyExpansion( pwBytes, ks );
	Aes_Cipher( pwBytes, ks, key16 );
	memcpy( key, key16, AES_BLOCKSIZE );

	// now reverse the bytes in the pwBytes
	Aes_ReverseBytes( pwBytes, numBytes );

	// use AES to encrypt these reversed bytes to get another 16 bytes,
	// for the rest of the key
	Aes_KeyExpansion( pwBytes, ks );
	Aes_Cipher( pwBytes, ks, key16 );
	memcpy( key + AES_BLOCKSIZE, key16, AES_BLOCKSIZE );

	free( pwBytes );
}


// reverse the bytes in the given array
//	b    - buffer whose bytes are to be reversed
//  bLen - number of bytes in "b"
void Aes_ReverseBytes( byte* b, int bLen )
{
	byte tmp;
	int i1;
	int i2;

	i1 = 0;
	i2 = bLen - 1;
	while( i1 < i2 ) {
		tmp = b[i1];
		b[i1] = b[i2];
		b[i2] = tmp;
		++i1;
		--i2;
	}
}


// generate a cryptographically strong random salt array.
// Uses crypto.getRandomValues() in WASM builds (via CryptoRng.c EM_JS shim)
// and getrandom(2) / /dev/urandom in native builds.
// The previous ftime()-based implementation produced only ~32 bits of
// effective entropy with the upper half mirrored — both issues are now fixed.
// salt - buffer of SALT_LEN bytes, filled upon return.
// Aborts on entropy failure (callers must not proceed with a zero salt).
void Aes_GenSalt( byte* salt )
{
    if ( CryptoRng_fill( salt, SALT_LEN ) != 0 ) {
        /* Entropy source failure is fatal — a predictable IV breaks the
           CTR-mode encryption. Abort rather than silently use bad data.   */
        fprintf( stderr, "Aes_GenSalt: entropy source failed — aborting\n" );
        abort();
    }
}


// generate Key Schedule (byte-array Nr+1 x Nb) from Key [ß5.2]
void Aes_KeyExpansion( const byte* key, KeySchedule w )
{
	byte temp[4];
	int i;
	int t;

	for( i = 0; i < AES_Nk; i++ ) {
		w[i][0] = key[4 * i];
		w[i][1] = key[(4 * i) + 1];
		w[i][2] = key[(4 * i) + 2];
		w[i][3] = key[(4 * i) + 3];
	}

	for( i = AES_Nk; i < ( AES_Nb * ( AES_Nr + 1 ) ); i++ ) {
		for( t = 0; t < 4; t++ ) 
			temp[t] = w[ i - 1 ][t];
		if( i % AES_Nk == 0 ) {
			Aes_RotWord( temp );
			Aes_SubWord( temp );
			for( t = 0; t < 4; t++ )
				temp[t] ^= Aes_Rcon[ i / AES_Nk ][ t ];
		} 
		else if( ( AES_Nk > 6 ) && ( i % AES_Nk == 4 ) ) {
			Aes_SubWord( temp );
		}
		for( t = 0; t < 4; t++ )
			w[i][t] = (byte)( w[ i - AES_Nk ][t] ^ temp[t] );
	}
}


// encrypt the given data array, using the given salt and key schedule.
// salt - SaltLength bytes
// return the encrpyted length - change!
int Aes_EncryptBlks( byte* data, int len, const byte* salt, KeySchedule ks )
{
	byte counterBlock[ AES_BLOCKSIZE ];
	long blockCount;
	int lastBlkLen;
	long dataNdx;
	long b;
	long tmp;
	int i;
	byte cipherCntr[ AES_BLOCKSIZE ];
	int blockLength;

	// initialise counter block (NIST SP800-38A ßB.2)
	memcpy( counterBlock, salt, SALT_LEN );
    
	// if the data length is not an integral number of block size,
	// then the last block will be a partial
	blockCount = len / AES_BLOCKSIZE;
	lastBlkLen = AES_BLOCKSIZE;
	if( blockCount * AES_BLOCKSIZE != len ) {
		++blockCount;
		lastBlkLen = len % AES_BLOCKSIZE;
	}
    
	dataNdx = 0;
  
	for( b = 0; b < blockCount; b++ ) {

		// set counter (block #) in last 8 bytes of counter block
		tmp = b;
		for( i = 0; i != 8; i++ ) {
			counterBlock[( AES_BLOCKSIZE - 1 ) - i] = (byte)( tmp & 0x0ff );
			tmp >>= 8;
		}

		// -- encrypt counter block --
		Aes_Cipher( counterBlock, ks, cipherCntr );
    
		// block size is reduced on final block
		blockLength = ( b != blockCount - 1 ) ? AES_BLOCKSIZE : lastBlkLen;

		// -- xor plaintext with ciphered counter char-by-char --
		for( i = 0; i != blockLength; i++ ) {
			data[dataNdx] = (byte)( cipherCntr[i] ^ data[dataNdx] );
			++dataNdx;
		}
	}
	return dataNdx;
}


// decrypte the given data array, using the given salt and key schedule
// salt - SaltLength bytes
void Aes_DecryptBlks( byte* data, int len, const byte* salt, KeySchedule ks )
{
	// the decryption algorithm is the same as encryption
	Aes_EncryptBlks( data, len, salt, ks );
}

// AES Cipher function: encrypt 'input' with Rijndael algorithm.
// Applies Nr rounds (10/12/14) using key schedule w for 'add round key' stage.
// Main Cipher function [ß5.1]
// input - one block (16 bytes) to be encrypted
// w - a Key Schedule
// output - buffer of 16 bytes, filled upon return
void Aes_Cipher( const byte* input, KeySchedule w, byte* output )
{
	byte state[4][4];
	int i;
	int round;

	// initialise 4xNb byte-array 'state' with input [ß3.4]
	for( i = 0; i < 4*AES_Nb; i++ )
		state[ i%4 ][ i/4 ] = input[i];

	Aes_AddRoundKey( state, w, 0, AES_Nb );

	for( round = 1; round < AES_Nr; round++ ) {
		Aes_SubBytes( state, AES_Nb );
		Aes_ShiftRows( state, AES_Nb );
		Aes_MixColumns( state, AES_Nb );
		Aes_AddRoundKey( state, w, round, AES_Nb );
	}

	Aes_SubBytes( state, AES_Nb );
	Aes_ShiftRows( state, AES_Nb );
	Aes_AddRoundKey( state, w, AES_Nr, AES_Nb );

	// convert state to 1-d array before returning [ß3.4]
	for( i = 0; i < 4 * AES_Nb; i++ )
		output[i] = state[ i%4 ][ i/4 ];
}


// apply SBox to state S [ß5.1.1]
void Aes_SubBytes( byte s[4][4], int Nb )
{
	int r;
	int c;

	for( r = 0; r < 4; r++ ) {
		for( c = 0; c < Nb; c++ )
			s[r][c] = Aes_Sbox[ s[r][c] ];
	}
}


// shift row r of state S left by r bytes [ß5.1.2].
// Note that this will work for Nb=4,5,6, but not 7,8 (always 4 for AES):
// see fp.gladman.plus.com/cryptography_technology/rijndael/aes.spec.311.pdf 
void Aes_ShiftRows( byte s[4][4], int Nb )
{
	byte t[4];
	int r;
	int c;

	for( r = 1; r < 4; r++ ) {

		// shift into temp copy
		for( c = 0; c < 4; c++ )
			t[c] = s[r][ (c+r) % Nb ];

		// and copy back
		for( c = 0; c < 4; c++ )
			s[r][c] = t[c];
	}
}


// combine bytes of each col of state S [ß5.1.3]
void Aes_MixColumns( byte s[4][4], int Nb )
{
	int c;
	byte a[4];
	byte b[4];
	int i;
	byte x;

	for( c = 0; c < 4; c++ ) {

		// 'a' is a copy of the current column from 's'
		// 'b' is aï{02} in GF(2^8)

		for( i = 0; i < 4; i++ ) {
			a[i] = s[i][c];
			x = (byte)( s[i][c] & 0x80 );
			b[i] = (byte)( ( x != 0 ) ? ( s[i][c] << 1 ^ 0x011b ) : ( s[i][c] << 1 ) );
		}

		// a[n] ^ b[n] is aï{03} in GF(2^8)
		s[0][c] = (byte)( b[0] ^ a[1] ^ b[1] ^ a[2] ^ a[3] ); // 2*a0 + 3*a1 + a2 + a3
		s[1][c] = (byte)( a[0] ^ b[1] ^ a[2] ^ b[2] ^ a[3] ); // a0 * 2*a1 + 3*a2 + a3
		s[2][c] = (byte)( a[0] ^ a[1] ^ b[2] ^ a[3] ^ b[3] ); // a0 + a1 + 2*a2 + 3*a3
		s[3][c] = (byte)( a[0] ^ b[0] ^ a[1] ^ a[2] ^ b[3] ); // 3*a0 + a1 + a2 + 2*a3
	}
}


// xor Round Key into state S [ß5.1.4]
void Aes_AddRoundKey( byte state[4][4], KeySchedule w, int rnd, int Nb )
{
	int r;
	int c;

	for( r = 0; r < 4; r++ ) {
		for( c = 0; c < Nb; c++ )
			state[r][c] ^= w[ (rnd*4) + c ][r];
	}
}


// apply SBox to 4-byte word w
void Aes_SubWord( byte* w )
{
	int i;

	for( i = 0; i < 4; i++ )
		w[i] = Aes_Sbox[ w[i] ];
}


// rotate 4-byte word w left by one byte
void Aes_RotWord( byte* w )
{
	byte tmp;
	int i;

	tmp = w[0];
	for( i = 0; i < 3; i++ )
		w[i] = w[ i + 1 ];
	w[3] = tmp;
}


// return the Ceiling of a/b
int Aes_Ceiling( int a, int b )
{
	int r;

	r = a / b;
	if( r * b < a )
		++r;
	return r;
}

#ifdef TESTCODE

// dump a key schedule to the console
void Aes_DumpKeySchedule( KeySchedule ks )
{
	FILE* f;
	int i;
	int j;

	f = NULL;
	if( fopen_s( &f, "dumpks.txt", "w" ) != 0 )
		return;

	for( i = 0; i != AES_KS_ROWS; ++i ) {
		fprintf( f, "ks[%d] = ", i );
		for( j = 0; j != AES_KS_COLS; ++j ) {
			if( j != 0 )
				fprintf( f, ", " );
			fprintf( f, "%x", ks[i][j] );
		}
		fprintf( f, "\n");
	}

	fflush( f );
	fclose( f );
}


// do an internal test
int Aes_Test()
{
	byte input[] = { 0x00, 0x11, 0x22, 0x33, 0x44, 0x55, 0x66, 0x77, 0x88, 0x99, 0xaa, 0xbb, 0xcc, 0xdd, 0xee, 0xff };
	byte key[]   = { 0x00,0x01,0x02,0x03,0x04,0x05,0x06,0x07,0x08,0x09,0x0a,0x0b,0x0c,0x0d,0x0e,0x0f, 
					0x10,0x11,0x12,0x13,0x14,0x15,0x16,0x17,0x18,0x19,0x1a,0x1b,0x1c,0x1d,0x1e,0x1f };
	byte expected[] = { 0x8e, 0xa2, 0xb7, 0xca, 0x51, 0x67, 0x45, 0xbf, 0xea, 0xfc, 0x49, 0x90, 0x4b, 0x49, 0x60, 0x89 };
	byte encBytes[ AES_BLOCKSIZE ];
	KeySchedule ks;
	int i;

	Aes_KeyExpansion( key, ks );
	//Aes_DumpKeySchedule( ks );
	Aes_Cipher( input, ks, encBytes );

	for( i = 0; i != AES_BLOCKSIZE; i++ )
		if( encBytes[i] != expected[i] )
			return -1;

	return 0;
}

#endif // TESTCODE

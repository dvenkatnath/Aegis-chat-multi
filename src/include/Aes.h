/*
 * implement AES encryption.  This code was translated from Javascript code
 * obtained from:
 *		AES implementation in JavaScript (c) Chris Veness 2005-2008 
 *		http://www.movable-type.co.uk/scripts/aes.html
 *			Encrypt a text using AES encryption in Counter mode of operation
 *			- see http://csrc.nist.gov/publications/nistpubs/800-38a/sp800-38a.pdf
 */

#ifndef AES_H
#define AES_H

#include "Hmac.h"

typedef unsigned char byte;

// the size of a cipher block
#define AES_BLOCKSIZE	16

// block size (in words): no of columns in state (fixed at 4 for AES)
#define AES_Nb			4

// to avoid having to dynamically allocate data for a KeySchedule,
// and since the KeyLength we will use is fixed (at ENCTOOLS_KEY_LEN=32),
// when we can pre-compute some values --

// key length in words (32/4)
#define AES_Nk			8

// no of rounds (based on a 256-bit key)
#define AES_Nr			14

// number of rows in a KeySchedule
#define	AES_KS_ROWS		( AES_Nb * ( AES_Nr + 1 ) )

// number of columns in a KeySchedule
#define	AES_KS_COLS		4

// thus a KeySchedule can be defined as
typedef byte	KeySchedule[AES_KS_ROWS][AES_KS_COLS];


// generate a "key" of the specified number of bits, 
// using the given password string
// numBits - number of bits - 128, 192 or 256
// key - buffer of (numBits/8) bytes, filled upon return 
void Aes_KeyFromPassword( int numBits, const char* password, byte* key );

// generate a random salt array
// salt - buffer of SaltLength bytes, filled upon return
void Aes_GenSalt( byte* salt );

// generate Key Schedule (byte-array Nr+1 x Nb) from Key [�5.2].
void Aes_KeyExpansion( const byte* key, KeySchedule ks );

// encrypt the given data array, using the given salt and key schedule.
// salt - SaltLength bytes
// keyLen - length of the key used to generate the keySchedule
// returns size of the encrypted string
int Aes_EncryptBlks( byte* data, int len, const byte* salt, KeySchedule ks );

// decrypte the given data array, using the given salt and key schedule
// salt - SaltLength bytes
// keyLen - length of the key used to generate the keySchedule
void Aes_DecryptBlks( byte* data, int len, const byte* salt, KeySchedule ks );

#ifdef TESTCODE

// do an internal test
int Aes_Test();

// dump a key schedule to the console
void Aes_DumpKeySchedule( KeySchedule ks );

#endif // TESTCODE

// AES Cipher function: encrypt 'input' with Rijndael algorithm.
// Applies Nr rounds (10/12/14) using key schedule w for 'add round key' stage.
// Main Cipher function [�5.1]
// input - one block (16 bytes) to be encrypted
// w - a Key Schedule
// output - buffer of 16 bytes, filled upon return
void Aes_Cipher( const byte* input, KeySchedule w, byte* output );

// apply SBox to state S [�5.1.1]
void Aes_SubBytes( byte s[4][4], int Nb );

// shift row r of state S left by r bytes [�5.1.2].
// Note that this will work for Nb=4,5,6, but not 7,8 (always 4 for AES):
// see fp.gladman.plus.com/cryptography_technology/rijndael/aes.spec.311.pdf 
void Aes_ShiftRows( byte s[4][4], int Nb );

// combine bytes of each col of state S [�5.1.3]
void Aes_MixColumns( byte s[4][4], int Nb );

// xor Round Key into state S [�5.1.4]
void Aes_AddRoundKey( byte state[4][4], KeySchedule w, int rnd, int Nb );

// apply SBox to 4-byte word w
void Aes_SubWord( byte* w );

// rotate 4-byte word w left by one byte
void Aes_RotWord( byte* w );

// return the Ceiling of a/b
int Aes_Ceiling( int a, int b );

#endif // AES_H

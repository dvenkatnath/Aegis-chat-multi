/*
 * this class provides methods to encode and decode base-64.
 * See - http://authors.aspalliance.com/mamanze/articles/?p=Base64
 */

#ifndef BASE64_H
#define BASE64_H

#define B64_TRUE	1
#define B64_FALSE	0

typedef unsigned char byte;

// get the size of the result buffer required for Base-64 
// encoding/decoding.
// sz - size of original buffer to be encoded/decoded
// isEncoded - true (1) when encoding the original buffer;
//				false (0) when decoding the original buffer.
int B64_GetSize( int sz, int isEncode );

// Base-64 encode the given byte array
// outChars - buffer of length returned by GetSize(), filled upon return
void B64_Encode( const byte* srcBytes, int srcLen, char* outChars );

// Base-64 decode the given string 
// srcChars - characters to be decoded 
// outBytes - buffer of length returned by GetSize(), filled upon return
void B64_Decode( const char* srcChars, int srcLen, byte* outBytes );

// return the Base-64 encoded char for the given source byte
char B64_EncodeByte( byte b );

// return the Base-64 decoded byte for the given source char
// <returns></returns>
byte B64_DecodeByte( byte b );

#endif // BASE64_H

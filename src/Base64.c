#include <stdlib.h>
#include "Base64.h"

// get the size of the result buffer required for Base-64 
// encoding/decoding.
// sz - size of original buffer to be encoded/decoded
// isEncoded - true (1) when encoding the original buffer;
//				false (0) when decoding the original buffer.
int B64_GetSize( int sz, int isEncode )
{
	int n = 0;

	if( isEncode ) {
		n = ( sz / 3 ) * 4;
		switch( sz % 3 ) {
		case 0: break;
		case 1: n += 2; break;
		case 2: n += 3; break;
		}
	}
	else {
		n = ( sz / 4 ) * 3;
		switch( sz % 4 ) {
		case 0: break;
		case 1: break;
		case 2: n += 1; break;
		case 3: n += 2; break;
		}
	}
	return n;
}


// Base-64 encode the given byte array
// outChars - buffer of length returned by GetSize(), filled upon return
void B64_Encode( const byte* srcBytes, int srcLen, char* outChars )
{
	byte b1, b2, b3;
	byte* destBytes = (byte*)outChars;

	// walk through the source, taking 3 bytes at a time
	int srcNdx = 0;
	int destNdx = 0;
	int remaining = srcLen;
	for( ; remaining > 2; remaining -= 3 ) {
		b1 = srcBytes[ srcNdx++ ];
		b2 = srcBytes[ srcNdx++ ];
		b3 = srcBytes[ srcNdx++ ];
		destBytes[destNdx++] = B64_EncodeByte( (byte)( b1 >> 2 ) );
		destBytes[destNdx++] = B64_EncodeByte( (byte)( ( b1 << 4 ) | ( b2 >> 4 ) ) );
		destBytes[destNdx++] = B64_EncodeByte( (byte)( ( b2 << 2 ) | ( b3 >> 6 ) ) );
		destBytes[destNdx++] = B64_EncodeByte( (byte)b3 );
	}

	// process the remaining bytes
	b2 = 0;
	if( remaining > 0 ) {
		b1 = srcBytes[srcNdx++];
		if( remaining == 2 )
			b2 = srcBytes[srcNdx++];

		destBytes[destNdx++] = B64_EncodeByte( (byte)( b1 >> 2 ) );
		destBytes[destNdx++] = B64_EncodeByte( (byte)( ( b1 << 4 ) | ( b2 >> 4 ) ) );
		if( remaining == 2 )
			destBytes[destNdx++] = B64_EncodeByte( (byte)( b2 << 2 ) );
	}
}


// Base-64 decode the given string 
// srcChars - characters to be decoded
// outBytes - buffer of length returned by GetSize(), filled upon return
void B64_Decode( const char* srcChars, int srcLen, byte* outBytes )
{
	byte b1, b2, b3, b4;
	const byte* srcBytes = (byte*)srcChars;
	byte* destBytes = outBytes;

	// walk through the source, taking 4 bytes at a time
	int srcNdx = 0;
	int destNdx = 0;
	int remaining = srcLen;
	for( ; remaining > 3; remaining -= 4 ) {
		b1 = B64_DecodeByte( srcBytes[srcNdx++] );
		b2 = B64_DecodeByte( srcBytes[srcNdx++] );
		b3 = B64_DecodeByte( srcBytes[srcNdx++] );
		b4 = B64_DecodeByte( srcBytes[srcNdx++] );

		destBytes[destNdx++] = (byte)( ( b1 << 2 ) | ( b2 >> 4 ) );
		destBytes[destNdx++] = (byte)( ( b2 << 4 ) | ( b3 >> 2 ) );
		destBytes[destNdx++] = (byte)( ( b3 << 6 ) | b4 );
	}

	// process the remaining bytes
	b2 = b3 = 0;
	if( remaining > 0 ) {
		b1 = B64_DecodeByte( srcBytes[srcNdx++] );
		if( remaining > 1 )
			b2 = B64_DecodeByte( srcBytes[srcNdx++] );
		if( remaining == 3 )
			b3 = B64_DecodeByte( srcBytes[srcNdx++] );

		destBytes[destNdx++] = (byte)( ( b1 << 2 ) | ( b2 >> 4 ) );
		if( remaining == 3 )
			destBytes[destNdx++] = (byte)( ( b2 << 4 ) | ( b3 >> 2 ) );
	}
}


// return the Base-64 encoded char for the given source byte
char B64_EncodeByte( byte b )
{
	b &= 0x3f;
	if( b <= 25 )
		return (byte)( b +'A' );
	if( b <= 51 )
		return (byte)( b - 26 + 'a' );
	if( b <= 61 )
		return (byte)( b - 52 + '0' );
	if( b == 62 )
		return (byte)'+';
	//if( b == 63 )
	return (byte)'/';
}


// return the Base-64 decoded byte for the given source char
// <returns></returns>
byte B64_DecodeByte( byte b )
{
	if( b == '+' )
		return 62;
	if( b == '/' )
		return 63;
	if( b <= '9' )
		return (byte)( b - '0' + 52 );
	if( b <= 'Z' )
		return (byte)( b - 'A' );
	return (byte)( b - 'a' + 26 );
}


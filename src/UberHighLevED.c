#include <stdio.h>
#include <stdbool.h>
#include <string.h>
#include "common.h"
#include "api.h"
#include "EncryptionToolsApi.h"
#include "Pstr.h"
#include "Global2.h"
#include "HighLevelED.h"
#include "UberHighLevED.h"

// Prototype declarations
int UDecrypt(char* bbuf, char* rcvdPacket);
//int KUEncrypt(char* input, char* output);

static unsigned char staticBuffer[BBUFSZ];  // not in .h, not intended to be used outside of this file

////////////////// modifyBinaryBufferStatic ///////////////
// Desc: pass an unsigned char buffer to his function to operate
//on it internally, not from the caller buffer.
// Params:  const unsigned char *buffer - the buffer (not pointer) passed
//          size_t size- the size of the buffer
// Returns: staticBuffer        
unsigned char* modifyBinaryBufferStatic(const unsigned char *buffer, size_t size) {
    unsigned char mylen[5];

    if (size > sizeof(staticBuffer)) //Given CT the code, this should never happen
        return NULL;

    for (size_t i = 0; i < size; i++) {
        staticBuffer[i] = buffer[i];
    }
    return staticBuffer;
}

////////////////// UUEncrypt ///////////////
// Dec: take a string returning an encrypted value based on the PW at the time of execution
// Passed: char* input
// returns: encrpyted value
unsigned char* UUEncrypt(unsigned char* input, size_t size) {
//    PingData_ptr = modifyBinaryBufferStatic(input, size);
    int i = KUEncrypt(input, PingData_ptr, size);   // place encrypted value into a global variable
                // Not All global variables are working, so using an old one whih does
    return PingData_ptr;  // return encrypted value in global variable
}

// Use this one when passing encrpyted buffer size, the version for use in a
int STR_UDecrypt(unsigned char* bbuf, unsigned char* rcvdPacket, size_t size) {
    register int i;
    int retVal;

    InitforED();

    (void)size;
    char PktLen[5];
    PktLen[0] = bbuf[0];
    PktLen[1] = bbuf[1];
    PktLen[2] = bbuf[2];
    PktLen[3] = bbuf[3];
    PktLen[4] = '\0';
    int incomingLen = atoi(PktLen);

    char* buf = (char*)(bbuf + 4);

    i = encryptionToolsApi_getDecryptLen((const byte*)buf, incomingLen);
    retVal = DecryptIt((byte*)buf, incomingLen, (char*)rcvdPacket);

    DeInitforED();

    return retVal;
}	

BinaryData binData = {0}; // short hand clear

////////////////// PUEncrypt ///////////////
// Dec: take a string returning an encrypted value based on the PW at the time of execution
// Passed: char* input
// returns: encrpyted value
BinaryData PUEncrypt(unsigned char* input, size_t size) {
    BinaryData rcvData;
    rcvData = initBinaryData();

    /* Write encrypted output directly into binData.data (eBBUFSZ=1240 bytes).
     * Previously passed PingData_ptr (GENBUFSZ=65 bytes) which was too small
     * for any plaintext longer than ~24 bytes, causing buffer overrun / crash. */
    int i = KUEncrypt(input, binData.data, size);

    binData.Len[ELL]   = (i > 0) ? i : 0;
    binData.Len[SCSID] = rcvData.Len[SCSID];
    binData.Len[2]     = rcvData.Len[2];
    binData.Len[3]     = rcvData.Len[3];
    memcpy(binData.SCS_ID, rcvData.SCS_ID, sizeof(binData.SCS_ID));
    binData.statusindex = rcvData.statusindex;

    return binData;
}
/* NOT USED ANY LONGER
////////////////// P2UEncrypt ///////////////
// Dec: take a string returning an encrypted value based on the PW at the time of execution
// Passed: char* input
// returns: encrpyted value
BinaryData P2UEncrypt(unsigned char* input, size_t size) {
    BinaryData* binDataptr = &binData;
    int i = KUEncrypt(input, PingData_ptr, size);   // place encrypted value into a global variable
                // Not All global variables are working, so using an old one whih does
// clear binData data object
/* PWR UPDATE
    register int w=0;
	for( w=0; w<BBUFSZ; w++ ) {    // clear out the BinaryData data buffer
		binData.data[w] = '\0';
		binData.status[w] = '\0';
		binData.SCS_ID[w] = '\0';
    }
    binData.L = 0;   binData.statL = 0;   binData.SCSID_L = 0;

    binData.L = i;  // Store the length
    memcpy(binData.data, PingData_ptr, i); 
    return binData;
}
*/
////////////////// P3UEncrypt ///////////////
// Dec: take a string returning an encrypted value based on the PW at the time of execution
// Passed: char* input
// returns: binData
BinaryData P3UEncrypt(unsigned char* input, size_t size) {
    int i = KUEncrypt(input, PingData_ptr, size);   // place encrypted value into a global variable
    // clear binData data object
    BinaryData rcvData;
    rcvData = initBinaryData(); // MAKES EVERYTHIMG 0 OR nul IN RECEIVING OBJECT
    binData.Len[ELL] =  rcvData.Len[ELL];
    memcpy(binData.data, rcvData.data, sizeof(binData.data)); // empty data
    binData.Len[SCSID] =  rcvData.Len[SCSID];
    binData.Len[2] =  rcvData.Len[2];
    binData.Len[3] =  rcvData.Len[3];
    memcpy(binData.SCS_ID, rcvData.SCS_ID, sizeof(binData.SCS_ID)); // empty data
    binData.statusindex = rcvData.statusindex;
    // binData ready to be used
    
    binData.Len[ELL] = i;  // Store the length
    memcpy(binData.data, PingData_ptr, i); 

    return binData;
}


////////////////// PUDecrypt ///////////////
// Desc: Decrypt passed value to decrypt based on current PW
// Passed:  char* bbuf; - the encrypted string
// Returns: size - the size of the encryted buffer

unsigned char* PUDecrypt( BinaryData DaData ) {
    // bring the passed data into local data

    size_t L = DaData.Len[ELL];
    /* decryptOutBuf: sized for the largest possible plaintext (BBUFSZ=953) + NUL.
     * Previously PongData_ptr (GENBUFSZ=65 bytes) was used here, which overflowed
     * for any plaintext longer than ~24 bytes. */
    static char decryptOutBuf[BBUFSZ + 1];
    char estr[eBBUFSZ];

    if (L > eBBUFSZ) {
        return NULL;
    }
    memcpy(estr, DaData.data, L);

    int i = STR_UDecrypt((unsigned char*)estr, (unsigned char*)decryptOutBuf, L);
    if (i != 0) {
        return NULL;
    }

    return decryptOutBuf;
}

////////////////// UUDecrypt ///////////////
// Desc: Decrypt passed value to decrypt based on current PW
// Passed:  char* bbuf; - the encrypted string
// Returns: char* Decptr the decrypted string

unsigned char* UUDecrypt(char* ebuf) {
    //PingData_ptr = modifyBinaryBufferStatic(ebuf, size);
    // PingData_ptr now contains the encrpyted buffer
    int i = UDecrypt(ebuf, PongData_ptr);  // Decrypt into a global string
                    // Not using a glbal variables are working, using an old one which does
	return PongData_ptr;
}

// Encrypts the given string
// Parameters:
//     char* S1: pointer to the buffer containing the string to be encrypted
//     byte* bbuf: pointer to the buffer containing the encrypted buffer after encryption
//     int bbufLen: the length of that buffer
// Returns:
//     int retVal: 0 if successful, -1 if not
/// @brief //////////////////// KUEncrypt ///////////////////////////////////////////
/// @param S1 
/// @param bbuf 
/// @param size 
/// @return retVal
/////////////////////
int KUEncrypt(unsigned char* S1, unsigned char* bbuf, size_t size) {
    int ebufsz = 0;
    int esize;

    esize = InitforED();    /* initialise crypto library with current session key */

    /*
     * Pass eBBUFSZ as the output buffer capacity, not the input size.
     * The encrypt function needs room for: ciphertext + SALT(8) + HMAC-SHA256(32).
     * Previously 'size' (the plaintext length) was passed here, which caused
     * encryptionToolsApi_encrypt to return -1 (buffer too small) on any message
     * longer than ~24 bytes, and corrupted the stack on messages <= 24 bytes.
     * The caller (PUEncrypt) already provides binData.data which is eBBUFSZ bytes.
     */
    ebufsz = EncryptIt((char *)S1, bbuf, eBBUFSZ);

    DeInitforED();   /* was "void DeInitforED();" — a declaration, not a call */

    return ebufsz;
}
	
//	Decrypt the given string
//  Description:
//		1. get the length of the buffer to be decrypted
//		2. Decrypt it
//	Parameters:
//       char* bbuf: pointer to the buffer containing the encrypted packet
//	     byte* rcvdPacket: pointer to the buffer containing the buffer after encryption			
//	Returns: 0 = SUCCESS
//           < 0 = FAILURE
int UDecrypt(char* bbuf, char* rcvdPacket) {
    register int i;
    int retVal;
    int bbufLen;

    // Initialize for ED
    InitforED();
         
    char PktLen[5];
    char* PktLenptr = PktLen;

// IF THIS CREATES A SEGMENT VIOLATION when code is paced into a library
//use versions of decrypt that passes the lenth.
    PktLenptr = Preturn_first_4_chars(bbuf);
    int incomingLen = atoi(PktLenptr);
    int zed = 0;  // BP

    // Strip out the prefix
    byte* incomingptr = bbuf + 4;

    //bbufLen = GetEncryptedLength( bbuf );   PROBLEM FUNCTION
    // move the bbuf point forward 4 bytes to do the decrpytion
    char* buf = bbuf + 4;

    // get the buffer size required to hold the result of decrypting the specified source data with the decrypt() method.
    i = encryptionToolsApi_getDecryptLen(buf, incomingLen);

    // To A Function
    retVal = DecryptIt(buf, incomingLen, rcvdPacket);

        // destruct the library
    DeInitforED();

    //char* NAKde1 = Pstrcpy(Decrptr, rcvdPacket);
    //NAKde1 = Pstrcpy(LastDecrptr, rcvdPacket);

    // old return rcvdPacket;
    return retVal;

}	

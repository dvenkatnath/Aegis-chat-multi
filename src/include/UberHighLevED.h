// UberHighLevel.h

#ifndef UBERHIGHLEVEL_H
#endif
#define UBERHIGHLEVEL_H

#include <stdio.h>
#include <stdbool.h>
#include <string.h>
#include "common.h"
#include "api.h"
#include "EncryptionToolsApi.h"
#include "Pstr.h"
#include "Global2.h"
#include "HighLevelED.h"

// global variables
extern BinaryData binData; // Ensure all values start at zero
extern BinaryData* binDataptr;


// Function prototypes
void DoSecondaryVals(char* origtext, char* encrval);
void ClearSecondaryVals();

void PRTAuditVars();		//Print the audit Encryption variables

// Use this one when passing encrpyted buffer size, the version for use in a
int STR_UDecrypt(unsigned char* bbuf, unsigned char* rcvdPacket, size_t size);

// High leve; encrypt and decrypt functions
////////////////// UUEncrypt ///////////////
// Dec: take a string returning an encrypted value based on the PW at the time of execution
// Passed: char* input
// returns: encrpyted value
unsigned char* UUEncrypt(unsigned char* input, size_t size);

BinaryData PUEncrypt(unsigned char* input, size_t size);
//BinaryData P2UEncrypt(unsigned char* input, size_t size);
//BinaryData P3UEncrypt(unsigned char* input, size_t size);

//unsigned char* PUDecrypt(char* ebuf, size_t size);
unsigned char* PUDecrypt( BinaryData DaData );

////////////////// UUDecrypt ///////////////
// Desc: Decrypt passed value to decrypt based on current PW
// Passed:  char* bbuf; - the encrypted string
// Returns: char* Decptr the decrypted string
///////unsigned char* UUDecrypt(char* ebuf);

////////////////// modifyBinaryBufferStatic ///////////////
// Desc: pass an unsigned char buffer to his function to operate on ir
// Params:  const unsigned char *buffer - the buffer (not pointer) passed
//          size_t size- the size of the buffer
// Returns: staticBuffer  
unsigned char* modifyBinaryBufferStatic(const unsigned char *buffer, size_t size);

/**
 * Encrypts the given string.
 *
 * Parameters:
 *   - char* S1: Pointer to the buffer containing the string to be encrypted.
 *   - byte* bbuf: Pointer to the buffer that will hold the encrypted data after encryption.
 *
 * Returns:
 *   - int: 0 if successful, -1 if not.
 */
int KUEncrypt(unsigned char* S1, unsigned char* bbuf, size_t size);

/**
 * Decrypt the given string.
 *
 * Parameters:
 *   - char* bbuf: Pointer to the buffer containing the string to be decrypted.
 *   - char* rcvdPacket: Pointer to the buffer that will hold the decrypted data after decryption.
 *
 * Returns:
 *   - int: 0 if successful, -1 if not.
 */
int UDecrypt(char* bbuf, char* rcvdPacket);


/**
 * Encrypts the given string.
 *
 * Parameters:
 *
 * Returns:
 *   - int: 0 if successful, -1 if not.
 */
 bool IsDecryptedOK();

// UBERHIGHLEVEL_H

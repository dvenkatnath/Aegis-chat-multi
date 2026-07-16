#pragma once
// HighLevelED.h
#ifndef HIGHLEVELED_H
#define HIGHLEVELED_H

#include <stdio.h>

// Include any other necessary headers
//#include "encryptionToolsApi.h"  // Ensure this header exists or replace it with the correct one

// Function declarations

// Initializes the library for AES & MUST be called before any ED actions
// returns size if encrypted buffer
int InitforED();

// Cleans up resources after all ED actions are completed
void DeInitforED();

// Encrypts the given string
// Parameters:
//     char* S1: pointer to the buffer containing the string to be encrypted
//     byte* bbuf: pointer to the buffer containing the encrypted buffer after encryption
//     int bbufLen: the length of that buffer
// Returns:
//     int retVal: 0 if successful, -1 if not
//int EncryptIt(char* S1, byte* bbuf, int bbufLen);
#include "EncryptionToolsApi.h"
int EncryptIt(char* S1, byte* bbuf, int bbufLen);
// Decrypts the encrypted packet
// Parameters:
//     byte* bbuf: pointer to the buffer containing the encrypted packet
//     int bbufLen: the length of that buffer
//     char* rcvdPacket: buffer for the received packet in clear text
// Returns:
//     int retVal: 0 if successful, -1 if not
int DecryptIt(byte* bbuf, int bbufLen, char* rcvdPacket);

//int GetEncryptedLength(char* bbuf);


#endif // HIGHLEVELED_H

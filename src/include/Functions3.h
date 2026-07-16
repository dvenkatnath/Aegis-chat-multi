// Functions3.h

#ifndef FUNCTIONS3_H
#define FUNCTIONS3_H

#include <stdio.h>
#include <stdbool.h>
#include <string.h>
#include <stdlib.h>
#include <time.h>

#include "common.h"

#include "SHA256.h"
#include "Global2.h"
#include "Pstr.h"
#include "HighLevelED.h"

// returns for SIM_Verified
#define V_SUCCESS 1
#define V_FAIL -1

char* GetUniqueNOShaStr();

// Global Vars
////////////////// SYNC Global Vars //////////////////////////
extern char ESYNC[GENBUFSZ];        // The encrpyted Sync String
extern char* ESYNCptr;

// Temp GVars for SM set up
extern char T_PingData[STDSZ]; // The ping string sent out by the initiator of a ping-pong exchange
extern char T_PongData[STDSZ]; // The pong string sent out by the responder of a ping-pong exchange
extern char T2_PingData[STDSZ]; // The ping string sent out by the initiator of a ping-pong exchange
extern char T2_PongData[STDSZ]; // The pong string sent out by the responder of a ping-pong exchange
extern char T2_RevPongdata[STDSZ]; // The reverse pong string is the actual pong string reversed order that is actional
extern char T2_DaKey[160]; // The concatenated ping and and revpong strings after a SHA256 is applied to it
extern char T2_F_DaKey[STDSZ]; // The Private Encrypt Key is THE key for secure communication session
extern char T_RevPongdata[STDSZ]; // The reverse pong string is the actual pong string reversed order that is actional
extern char T_DaKey[160]; // The concatenated ping and and revpong strings after a SHA256 is applied to it
extern char T_F_DaKey[STDSZ]; // The Private Encrypt Key is THE key for secure communication session
extern char SCS_ID[STDSZ];	// SCS ID

extern char* T_PingData_ptr;
extern char* T_PongData_ptr;
extern char* T_RevPongdata_ptr;
extern char* T_DaKey_ptr;
extern char* T_F_DaKey_ptr;
extern char* T2_PingData_ptr;
extern char* T2_PongData_ptr;
extern char* T2_RevPongdata_ptr;
extern char* T2_DaKey_ptr;
extern char* T2_F_DaKey_ptr;

extern char* SCS_ID_ptr;


//////////////////////////// Function prototypes

size_t getbdsize();

void ND_TSMP();
void DoRevPong(char* base, char* revbase);

//char* SO_Message(char* msg);
void CreateTDa_Key(int side);

char* CreateFinalPK(char* DK_ptr);

int SIM_Verified();

/////////////////////////// SHAPassedDeux //////////////////////////////
char* SHAPassedDeux(char* str);

/////////////////////////// CheckSMVars //////////////////////////////
// name: CheckSMVars
// @param: void
// @return: bool:   True if the SM variables are the same, false otherwise
//
bool CheckSMVars();

/////////////////////////// GetUniqueNO //////////////////////////////
// name: GetUniqueNO
// @param: void
// @return: uint64_t GetUniqueNO()   a unique number
//
uint64_t GetUniqueNO();

/*
 * Is it a prime number?
 * Parameters:
 *   - int num
 * Returns:
 *   - false: It is NOT a prime number
 *   - true: It IS a prime number
 */
bool is_prime(int num);

/*
 * get the nexgt prime number
 * Parameters:
 *   - int num
 * Returns:
 *   - nextprime
 */
int next_prime(int num);


/**
 * Encrypts a SHA-256 hash of the given string.
 *
 * Parameters:
 *   - char* str: Pointer to the string to hash and encrypt.
 *
 * Returns:
 *   - char*: Pointer to the hashed and encrypted string.
 */
char* SHAPassed(char* str);


/**
 * Adds a type prefix to a message.
 *
 * Parameters:
 *   - char* str: Pointer to the original message.
 *   - char* type: Pointer to the type string to add as a prefix.
 *
 * Returns:
 *   - char*: Pointer to the modified message with type prefix.
 */
char* Addtype2msg(char* str, char* type);

/**
 * Generates a private key based on type.
 *
 * Parameters:
 *   - int type: Type of key to generate (1 for ACK, 2 for NAK).
 *
 * Returns:
 *   - char*: Pointer to the generated key.
 */
char* GenPrivateKey(int type);

/**
 * Generates an ACK message.
 *
 * Returns:
 *   - char*: Pointer to the generated ACK message.
 *
char* GenAnACK();
*/

/**
 * Processes a received Pong message.
 *
 * Parameters:
 *   - char* rcvd: Pointer to the received Pong message.
 *
 * Returns:
 *   - char*: Pointer to the processed Pong message.
 */
char* ProcessRvdPong(char* rcvd);

/**
 * Encrypts a string using the library's encryption function.
 *
 * Parameters:
 *   - char* str: Pointer to the string to encrypt.
 */
void EString(char* str);

/**
 * Processes the first received message.
 *
 * Parameters:
 *   - char* rawstring: Pointer to the raw received string.
 *
 * Returns:
 *   - int: 0 if processing was successful, -1 otherwise.
 */
int ProcessRcvd1stNAK(char* rawstring);

/**
 * Creates a unique session key.
 */
void CreateDaKey(int type);

/**
 * Generate a NAK
 * returns: the encrypted NAK message to send back to the NI.
 */
BinaryData GenNACK();

/**
 * Is the string 100% ASCII?
 */
int isstr_ascii(const char* str);

#endif // FUNCTIONS3_H

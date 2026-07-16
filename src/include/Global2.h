// global2.h
#pragma once
#ifndef GLOBAL2_H
#define GLOBAL2_H

// Disable warnings for unsafe functions in Visual Studio
#define _CRT_SECURE_NO_WARNINGS

//#define DEEP_DEBUG  // allow printing of many debugging print statements

// SerNo fuilure
#define SERNO_FAILURE -3

//Message structure
// 4 digit number of size of ENCRYPTED | message 3 digit Type Code | Message
// 0000|000|Message
//     ^ ENCRYPTED ^ 
// IMPLICATIONS - The encrypted message will be 3 digits longer than prviously
// 
// MSGSize - 4 digit number of size of ENCRYPTED
// MSGType - 3 digit MSGTYPE
// MSG - Message - variable up to 1420 bytes - BBUFSZ

#include <stdbool.h>
#include "common.h"

//#define DOTSO	// Execute the version calling function in the .so

// License info
extern long int SerNo;	// Serial number for the Kobi Library
extern char OrgName[GENBUFSZ];	// Organization Name
extern char LicDate[GENBUFSZ];
extern char C_SerNo[GENBUFSZ];
extern char* C_SerNoPtr;
extern char C_grpSerNo[GENBUFSZ];
extern char* C_grpSerNoPtr;

// Global variables
extern BinaryData GL_Data;	// Global data for functions returning BinaryData structure
extern char C_closedloopbrdgeNo[GENBUFSZ];
extern char* C_closedloopbrdgeNoPtr;


extern int Sim_complete;
extern bool Sender_Side;	// IF true side1 is sender
extern bool Rcvr_Side;	// IF true side2 is receiver
extern bool IsEncrypted; // status: true = encrypted, false (default @ startup) not encrypted
extern bool S2_Sender_Side;	// IF true side1 is sender
extern bool S2_Rcvr_Side;	// IF true side2 is receiver
extern bool S2_IsEncrypted; // status: true = encrypted, false (default @ startup) not encrypted
extern unsigned char PingData[GENBUFSZ]; // The ping string sent out by the initiator of a ping-pong exchange
extern unsigned char PongData[GENBUFSZ]; // The pong string sent out by the responder of a ping-pong exchange
extern char RevPongdata[GENBUFSZ]; // The pong string sent out by the responder of a ping-pong exchange reversed order
extern char DaKey[258]; // The concatenated ping and SetRevretardation strings after a SHA256 is applied to it
extern char F_DaKey[GENBUFSZ]; // Encrypt DaKey with the init key creating the key for communication session
extern unsigned char S2_PingData[GENBUFSZ]; // The ping string saved by Side 2 sent out 
extern char S2_PongData[GENBUFSZ]; // The pong string saved by Side
extern unsigned char S2_RevPongdata[GENBUFSZ]; // The pong string sent out by the responder of a ping-pong exchange reversed order
extern char S2_DaKey[258]; // The Side 2 concatenated ping and and revpong strings after a SHA256 is applied to it

extern unsigned char rawping[128];
extern char Rawpong[GENBUFSZ];
extern unsigned char* rawpingptr;
extern char* Rawpongptr;
/*
extern char Encr[BBUFSZ];
extern char* Encrptr;
extern char Decr[BBUFSZ];
extern char* Decrptr;
*/
// Global msg state to msgtype
extern char LastMSGPassed[MaxPktSz];
extern char* LastMSGPassedptr;
extern char LastMSGReturned[MaxPktSz];			// Last msg returned from msgtype
extern char* LastMSGReturnedptr;

extern char GenNACKret[NOMINALSZ];	// Return value from GenNACK
extern char* GenNACKretptr;

// Pointer variables
extern unsigned char* PingData_ptr;
extern unsigned char* PongData_ptr;
extern char* RevPongdata_ptr;
extern char* DaKey_ptr;
extern char* F_DaKey_ptr;
extern unsigned char* S2_PingData_ptr;
extern char* S2_PongData_ptr;
extern unsigned char* S2_RevPongdata_ptr;
extern char* S2_DaKey_ptr;

extern char S2_clrNAK[GENBUFSZ];
extern char* S2_clrNAKptr;	// the NAK on S2 clear
extern char S2_F_DaKey[GENBUFSZ]; // The Private Encrypt Key is THE key for secure communication session
extern char* S2_F_DaKey_ptr;
extern char BU_S2_F_DaKey[GENBUFSZ]; 	// The S2 Private Encrypt Key BACKUP
extern char* BU_S2_F_DaKeyPTR;			// The S2 Private Encrypt Key BACKUP  PTR

extern char BU_S2_RevPongdata[GENBUFSZ]; // The pong string sent out of a ping-pong exchange reversed order
extern char* BU_S2_RevPongdataPTR;
extern char BU_S2_DaKey[258]; // The Side 2 concatenated ping and and revpong strings after a SHA256 is applied to it
extern char* BU_S2_DaKeyPTR;

// Add a state
extern bool Me_sender;		// This side is sender
extern bool Me_receiver;	// This side is receiver

extern long getSN();
extern void CLRSteMach();		//Clear the global variables
extern void PRTSteMach();		//Print the global variables
extern void ND_PRTSteMach();	//Print the global variables w/o _DEBUG

#endif // GLOBAL2_H
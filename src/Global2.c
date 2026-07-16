#define _CRT_SECURE_NO_WARNINGS

#include <stdio.h>
#include <stdbool.h>
#include <string.h>
#include <stdlib.h>
#include <stdbool.h>
#include "common.h"
#include "Functions3.h"
#include "HighLevelED.h"
#include "Global2.h"

// License info
// The serial number can be an 10 digit number.  It is stored as an long int and in a char array.
// Design: Both sides need to have the same searial number to communicate,.
// In GenAnACK, the SerNo 10 digit string iinserted between the type of ACK and the ACK variable and sent to site 2
// In msgtype, in the ACK processing, this variable needs to be extracted and compared to the Side 2 SerNo
// If NOT the same, the processing of a sequre sommuncation channel is to stop with no explanation
// For debugging, a statement will print indicating such.
// FUTURE FEATURE - If Side 2 SerNo == closedloopbrdgeNo, then the communication is allowed, if
// Side 1 closedloopbrdgeNo = Side 2 SerNo
// In the FUTURE, the closedloopbrdgeNo 10 digit number follows  the Side 1 SerNo

long int SerNo = 1001;	// Serial number for the Library. (0 - 4,294,967,295 )
char C_SerNo[GENBUFSZ];
char* C_SerNoPtr = C_SerNo;

// For testing, check agaist a Side 2 variable
long int SerNo2 = 9999;	// Serial number for the Library. (0 - 4,294,967,295 )
char C_SerNo2[GENBUFSZ];
char* C_SerNoPtr2 = C_SerNo2;

double BigSerBo = 10000234500000002; //Max: 2.2250738585072014 � 10 to tje power of 308 -  Use this if we think sales will be larger than 4,294,967,295

long int closedloopbrdgeNo = 9999;	// Serial number for the closed loop. (0 - 4,294,967,295 )
char C_closedloopbrdgeNo[GENBUFSZ];
char* C_closedloopbrdgeNoPtr = C_closedloopbrdgeNo;

BinaryData GL_Data;	// Global data for functions returning BinaryData structure

char OrgName[GENBUFSZ] = "USG";	// Organization Name, up to 513 chars long
char LicDate[GENBUFSZ] = "251210";  // YYMMDD

// Global variables
int Sim_complete = 0;	// Is the simulation complete?
bool Sender_Side;	// IF true side1 is sender
bool Rcvr_Side;	// IF true side2 is receiver
bool IsEncrypted = false; // status: true = encrypted, false (default @ startup) not encrypted
bool S2_Sender_Side;	// IF true side1 is sender
bool S2_Rcvr_Side;	// IF true side2 is receiver
bool S2_IsEncrypted = false; // status: true = encrypted, false (default @ startup) not encrypted
unsigned char PingData[GENBUFSZ]; // The ping string sent out by the initiator of a ping-pong exchange
unsigned char S2_PingData[GENBUFSZ]; // The ping string saved by Side 2 sent out 
unsigned char PongData[GENBUFSZ]; // The pong string sent out by the responder of a ping-pong exchange
char S2_PongData[GENBUFSZ]; // The pong string saved by Side
char RevPongdata[GENBUFSZ]; // The reverse pong string is the actual pong string reversed order that is actional
unsigned char S2_RevPongdata[GENBUFSZ]; // The pong string sent out by the responder of a ping-pong exchange reversed order
char DaKey[258]; // The concatenated ping and and revpong strings after a SHA256 is applied to it
char S2_DaKey[258]; // The Side 2 concatenated ping and and revpong strings after a SHA256 is applied to it
char F_DaKey[GENBUFSZ]; // The Private Encrypt Key is THE key for secure communication session
/* OLD
char Encr[BBUFSZ];
char* Encrptr = Encr;       // 2ndsary storage of the original text encrypted
char Decr[BBUFSZ];
char* Decrptr = Decr;
*/
unsigned char* PingData_ptr = PingData;
unsigned char* S2_PingData_ptr = S2_PingData;
unsigned char* PongData_ptr = PongData;
char* S2_PongData_ptr = S2_PongData;
char* RevPongdata_ptr = RevPongdata;
unsigned char* S2_RevPongdata_ptr = S2_RevPongdata;
char* DaKey_ptr = DaKey;
char* S2_DaKey_ptr = S2_DaKey;
char* F_DaKey_ptr = F_DaKey;

unsigned char rawping[128];				// ping data to be sent (128: GenPrivateKey output = type+SerNo+SHA256 = up to 79 bytes)
char Rawpong[GENBUFSZ];		// pong data to be sent
unsigned char* rawpingptr = rawping;	
char* Rawpongptr = Rawpong;

// Global msg state to msgtype
char LastMSGPassed[MaxPktSz];			// Last msg passed to msgtype
char* LastMSGPassedptr = LastMSGPassed;
char LastMSGReturned[MaxPktSz];			// Last msg returned from msgtype
char* LastMSGReturnedptr = LastMSGReturned;

char GenNACKret[NOMINALSZ];	// Return value from GenNACK
char* GenNACKretptr = GenNACKret;

char S2_clrNAK[GENBUFSZ];
char* S2_clrNAKptr = S2_clrNAK;	// the NAK on S2 clear

char S2_F_DaKey[GENBUFSZ]; 					// The Private Encrypt Key is THE key for secure communication session
char* S2_F_DaKey_ptr = S2_F_DaKey;

char BU_S2_F_DaKey[GENBUFSZ]; 				// The S2 Private Encrypt Key BACKUP
char* BU_S2_F_DaKeyPTR = BU_S2_F_DaKey;		// The S2 Private Encrypt Key BACKUP  PTR

char BU_S2_RevPongdata[GENBUFSZ]; // The pong string sent out of a ping-pong exchange reversed order
char* BU_S2_RevPongdataPTR = BU_S2_RevPongdata;
char BU_S2_DaKey[258]; // The Side 2 con
char* BU_S2_DaKeyPTR = BU_S2_DaKey;

//////////////// getSN /////////////////////////////////////////////////////
/// Descrition:	get the SerNo of this account 
/// Returns 	int
///////////////////////////////////////
long getSN() {
	return(SerNo);
/*	snprintf(C_SerNoPtr, 20, "%010ld", SerNo);  
	printf("SerNo: \t\t\t%s\n",C_SerNoPtr);
	snprintf(C_closedloopbrdgeNoPtr, 20, "%010ld", closedloopbrdgeNo);  
	printf("closedloopbrdgeNo:\t%s\n\n",C_closedloopbrdgeNoPtr);
*/

}

void CLRSteMach() {		//Clear the global vaiables
	//PingData[0] = '\0';
	//PongData[0] = '\0';
	//RevPongdata[0] = '\0';
	//DaKey[0] = '\0';
	//F_DaKey[0] = '\0';
	IsEncrypted = false;
	Sender_Side = false;
	Rcvr_Side = false;
	S2_Sender_Side = false;
	S2_Rcvr_Side = false;
	S2_IsEncrypted = false;
	S2_Sender_Side = false;
	S2_Rcvr_Side = false;
	Sim_complete = 0;
	//S2_PingData[0] = '\0';
	//S2_PongData[0] = '\0';
	//S2_clrNAK[0] = '\0';
	//S2_RevPongdata[0] = '\0';
	//S2_DaKey[0] = '\0';
	//S2_F_DaKey[0] = '\0';
	BU_S2_F_DaKey[0] = '\0';
	rawping[0] = '\0';
	Rawpong[0] = '\0';
	T_PingData[0] = '\0';
	T_PongData[0] = '\0';
	T_RevPongdata[0] = '\0';
	T_DaKey[0] = '\0';
	T_F_DaKey[0] = '\0';
	T2_PingData[0] = '\0';
	T2_PongData[0] = '\0';
	T2_RevPongdata[0] = '\0';
	T2_DaKey[0] = '\0';
	T2_F_DaKey[0] = '\0';
}
void ND_PRTSteMach() {		//Print the global vaiables
	printf("\n//////////////////////\nOrgName = %s\tDate: %s\n", OrgName, LicDate);
/*
	printf("PingData_ptr = \t\t%s - %d\n", PingData_ptr, strlen(PingData_ptr));
	printf("S2_PingData_ptr =\t%s - %d\n", S2_PingData_ptr, strlen(S2_PingData_ptr));
    printf("PongData_ptr =\t\t%s - %d\n", PongData_ptr, strlen(PongData_ptr));
	printf("S2_PongData_ptr =\t%s - %d\n", S2_PongData_ptr, strlen(S2_PongData_ptr));
	printf("RevPongdata_ptr =\t%s = %d\n", RevPongdata_ptr, strlen(RevPongdata_ptr));
	printf("S2_RevPongdata_ptr =\t%s - %d\n", S2_RevPongdata_ptr, strlen(S2_RevPongdata_ptr));
	printf("S2_DaKey_ptr =\t\t%s - %d\n", S2_DaKey_ptr, strlen(S2_DaKey_ptr));
	printf("DaKey_ptr =\t\t%s - %d\n", DaKey_ptr, strlen(DaKey_ptr));
	printf("F_DaKey_ptr =\t\t%s - %d\n", F_DaKey_ptr, strlen(F_DaKey_ptr));
	printf("S2_F_DaKey_ptr =\t%s - %d\n", S2_F_DaKey_ptr, strlen(S2_F_DaKey_ptr));
	printf("BU_S2_F_DaKeyPTR =\t%s - %d\n", BU_S2_F_DaKeyPTR, strlen(BU_S2_F_DaKeyPTR));
	*/

	snprintf(C_SerNoPtr, 20, "%010ld", SerNo);  
	//printf("SerNo: \t\t\t%s\n",C_SerNoPtr);
	snprintf(C_closedloopbrdgeNoPtr, 20, "%010ld", closedloopbrdgeNo);  
	//printf("closedloopbrdgeNo:\t%s\n\n",C_closedloopbrdgeNoPtr);
	
	//printf("IsEncrypted =\t\t%s\n", IsEncrypted ? "true" : "false");
	//printf("S2_IsEncrypted =\t%s\n", S2_IsEncrypted ? "true" : "false");
	//printf("Sender_Side =\t\t%s\n", Sender_Side ? "true" : "false");
	//printf("S2_Sender_nSide =\t%s\n", S2_Sender_Side ? "true" : "false");
	//printf("Rcvr_Side =\t\t%s\n", Rcvr_Side ? "true" : "false");
	//printf("S2_Rcvr_Side =\t\t%s\n//////////////////////\n", S2_Rcvr_Side ? "true" : "false");
	//printf("Sim_complete = %d\n", Sim_complete);
	ND_TSMP();	// Print TSM vaiables
	//PRTAuditVars();
}


void PRTSteMach() {		//Print the global vaiables
#ifdef _DEBUG
	ND_PRTSteMach();
#endif
}


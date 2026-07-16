#pragma once
#ifndef API_H
#define API_H

#include <stdio.h>
#include <stdbool.h>
#include <string.h>
#include "common.h"
#include "Functions3.h"
#include "UberHighLevED.h"

extern char *retGenAnACK;   // 0
extern char *retfromNACK;   // 1
extern char *SCScomplete;   // 2
extern char *DECMsg;        // 3
extern char *NoMsgtypeProc; // 4
extern char *NULLProbmsgtype;  //5
extern char *SCSNOT;       // 6
extern char *buffer2sml;      // 7
extern char *msgsNotPrcs;      // 8
extern char *RcvdEncTxt;      // 9

// globals, if needed
extern char Stellung[10][128];

// //Function prototypes

// Generates an ACK message to be sent to the network interface
BinaryData initBinaryData();
size_t getbdsize();


char* getSCS_ID ();

BinaryData GenAnACK();

BinaryData MessageHandler( BinaryData );

BinaryData EncryptNSend(char* input, unsigned char* scsid);

#endif // API_H

// ProcMSG.h

#ifndef PROCMSG_H
#define PROCMSG_H

#include <stdio.h>
#include <stdbool.h>
#include <string.h>
#include "EncryptionToolsApi.h"
#include "common.h"
#include "Pstr.h"
#include "Global2.h"
#include "HighLevelED.h"
#include "UberHighLevED.h"
#include "Functions3.h"

#define ACK 1	// - ACK , to be processed internally in the .so, eg,
//#define SND 2		// - S, to be sent to the other side
#define NAK 3		// - N (NAK), when received by the side that did the ACK, 
                        // process it to setup the final private key
#define DEC 4		// - D, DEENCRYPT the string and return it to the net infc

// Function prototypes
BinaryData GenNACK();

BinaryData msgtype(char* msg);

#endif // PROCMSG_H

//#include <stdio.h>
//#include <stdbool.h>
//#include <string.h>
//#include "EncryptionToolsApi.h"
#include "common.h"
#include "Pstr.h"
#include "Global2.h"
#include "HighLevelED.h"
#include "Functions3.h"
#include "api.h"
#include "UberHighLevED.h"
#include "ProcMSG.h"

//#define DEEP_DEBUG

BinaryData msgtype(char* msg) {
    char TYPE[5];
	char* Typeptr = TYPE;

    BinaryData myData;
    BinaryData rcvData;

    // clear rcvData data object
    rcvData = initBinaryData(); // MAKES EVERYTHIMG 0 OR nul IN RECEIVING OBJECT
    // copy received structure to local structure myData
    myData.Len[ELL] =  rcvData.Len[ELL];
    memcpy(myData.data, rcvData.data, sizeof(myData.data)); // empty data
    myData.Len[SCSID] =  rcvData.Len[SCSID];
    myData.Len[2] =  rcvData.Len[2];
    myData.Len[3] =  rcvData.Len[3];
    memcpy(myData.SCS_ID, rcvData.SCS_ID, sizeof(myData.SCS_ID)); // empty data
    myData.statusindex = rcvData.statusindex;
    // myData ready to be populated
#ifdef DEEP_DEBUG
    printf("In msgtype\t%s", msg);
#endif
    Typeptr = Preturn_next_3_chars(msg);
#ifdef DEEP_DEBUG	
	printf("MESSAGE TYPE: %s\n", Typeptr);
#endif

#define strswitch(s)    char *__strswitch = s;
#define strcase(x)      if (strcmp(__strswitch, x) == 0)
#define strcaseelse(x)  else if (strcmp(__strswitch, x) == 0)
#define strdefault      else

    strswitch(Typeptr) {
/////////////// ACK ///////////////////////
        strcase("ACK") { // It's an ACK on Side 2, the receiver side
#ifdef DEEP_DEBUG
            printf("ACK Received!\n%s\n", msg);
#endif
            // Get the SerNo from msg
            char SerNoPassed[GENBUFSZ];
            char* SerNoPassedPtr = SerNoPassed;
            SerNoPassedPtr = Preturn_next_10_chars(msg + 3);
            // Compare here between the SerNo on both sides
            long sn = getSN();
            char SN[25];
            char* SNptr = SN;
            snprintf(SNptr, 20, "%010ld", sn);  
            if(Pstrcmp(SerNoPassedPtr, SNptr) != 0){
                // Serial numbers between Side 1 & 2 DO NOT MARCH
                // Current version .a - exit(-3);
                exit(SERNO_FAILURE);
            }
            Pstrcpy(T2_PingData_ptr, msg + 13);  // Save the ACK to the T2 State Machine
            // Processing a NAK Side2 next
#ifdef DEEP_DEBUG
            printf("**Side 2: NOW Geneating a NAK***\n");
#endif
            rcvData = GenNACK();        // Process a NACK

            // Clear each buffer according to its correct size
            myData = initBinaryData(); // MAKES EVERYTHIMG 0 OR null
            // copy received structure to local structure myData
            myData.Len[ELL] =  rcvData.Len[ELL];
            memcpy(myData.data, rcvData.data, myData.Len[ELL]);
            myData.Len[SCSID] =  rcvData.Len[SCSID];
            myData.Len[2] =  rcvData.Len[2];
            myData.Len[3] =  rcvData.Len[3];
            memcpy(myData.SCS_ID, rcvData.SCS_ID, sizeof(myData.SCS_ID)); // empty data
            myData.statusindex = NAK_ret;

#ifdef DEEP_DEBUG
            printf("Returning the encrypted string to send to side 1\n");
#endif
            return myData;  // Send the NACK to Side 1, the server
        }
////////////////// NAK ///////////////////////
        strcaseelse("NAK") {   // This will be processed on side 1
#ifdef DEEP_DEBUG
            printf("\nNAK Message: ");
#endif
#ifdef DEEP_DEBUG
            printf("Side 1: Received a NAK from Side 2, NOW processing that in ProcessRcvd1stNAK\n");
#endif
            int PR1stNAK = ProcessRcvd1stNAK(msg + 3);
            if(PR1stNAK == 1) {
                // Clear each buffer according to its correct size
                rcvData = initBinaryData(); // MAKES EVERYTHIMG 0 OR nul IN RECEIVING OBJECT
                // copy received structure to local structure myData
                myData.Len[ELL] =  rcvData.Len[ELL];
                memcpy(myData.data, rcvData.data, sizeof(myData.data)); // empty data
                myData.Len[SCSID] =  rcvData.Len[SCSID];
                myData.Len[2] =  rcvData.Len[2];
                myData.Len[3] =  rcvData.Len[3];
                memcpy(myData.SCS_ID, rcvData.SCS_ID, sizeof(myData.SCS_ID)); // empty data
                myData.statusindex = SCS_setup; // status

                size_t lennn = strlen(T_DaKey_ptr);  // processed in ProcessRcvd1stNAK
                myData.Len[SCSID] = lennn;
                memcpy(myData.SCS_ID, T_DaKey_ptr, lennn);
                memcpy(SCS_ID, T_DaKey_ptr, lennn); //save to global var
#ifdef DEEP_DEBUG
                printf("NAK processing complete!\tRPR1stNAKet: %d\n", PR1stNAK);
                printf("Side 1 - PKey enabled\n");
#endif
                return myData;        // once here, the SCS is setup!
            } 
        }
        strcaseelse("DEC") {
////////////////////////// DEC ///////////////////////
#ifdef DEEP_DEBUG
        printf("\nDEC Message: ");
#endif
            size_t dlen = strlen(msg + 3);
            myData.Len[ELL] = dlen;
            memcpy(myData.data, msg + 3, dlen);
            myData.statusindex = DEC;
#ifdef DEEP_DEBUG
        printf("%s\n", myData.data );
#endif
            // returning clear text!!!!!  not over a network!
            return myData;
        }
////////////////////////// default ///////////////////////
        strdefault {
#ifdef DEEP_DEBUG
            printf("\nmsgtype: Invalid data\n"); // Should never get here
#endif
            // Clear each buffer according to its correct size
            rcvData = initBinaryData(); // MAKES EVERYTHIMG 0 OR nul IN RECEIVING OBJECT
            // copy received structure to local structure myData
            myData.Len[ELL] =  rcvData.Len[ELL];
            memcpy(myData.data, rcvData.data, sizeof(myData.data)); // empty data
            myData.Len[SCSID] =  rcvData.Len[SCSID];
            myData.Len[2] =  rcvData.Len[2];
            myData.Len[3] =  rcvData.Len[3];
            memcpy(myData.SCS_ID, rcvData.SCS_ID, sizeof(myData.SCS_ID)); // empty data
            myData.statusindex = MSGsERROR; // status

            return myData;  // We should never get here!
        }
    }
    // Should never get here
    printf("\nmsgtype: Should never get here\n");

    // Clear each buffer according to its correct size
    rcvData = initBinaryData(); // MAKES EVERYTHIMG 0 OR nul IN RECEIVING OBJECT
    // copy received structure to local structure myData
    myData.Len[ELL] =  rcvData.Len[ELL];
    memcpy(myData.data, rcvData.data, sizeof(myData.data)); // empty data
    myData.Len[SCSID] =  rcvData.Len[SCSID];
    myData.Len[2] =  rcvData.Len[2];
    myData.Len[3] =  rcvData.Len[3];
    memcpy(myData.SCS_ID, rcvData.SCS_ID, sizeof(myData.SCS_ID)); // empty data
    myData.statusindex = MSGsERROR; // status

    return myData;  // We should never get here!
}

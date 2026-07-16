#include <stdio.h>
#include <stdbool.h>
#include <string.h>
#include "common.h"
#include "Functions3.h"
#include "Pstr.h"
#include "Global2.h"
#include "api.h"
#include "HighLevelED.h"
#include "UberHighLevED.h"
#include "ProcMSG.h"
#ifdef DOTSO
#include <dlfcn.h>  // Required for dynamic loading of shared objects
#endif

//#define DEEP_DEBUG

// globals to further define BinaryDate
char Stellung[10][128] = {
    "Return from GenAnACK",
    "Return from NACK",
    "SCS setup complete",
    "Decryption message",
    "MSGs processed",
    "NULL Problem",
    "SCS NOT setup",
    "BufferOverRun decrypting",
    "MSGs NOT Processed",
    "RCVD Encrypted Txt"
};
// Gloals supporting BinaryData structure

char *retGenAnACK = Stellung[0];
char *retfromNACK = Stellung[1];
char *SCScomplete = Stellung[2];
char *DECMsg = Stellung[3];
char *NoMsgtypeProc = Stellung[4];
char *NULLProbmsgtype = Stellung[5];
char *SCSNOT = Stellung[6];
char *BufferOverRun = Stellung[7];
char *msgsNotPrcs = Stellung[8];
char *RcvdEncTxt = Stellung[9];

///////////////// initBinaryData ///////////////////////////////
BinaryData initBinaryData() {
    BinaryData localData;

        // Clear local buffer\\ struct
        for (register int w = 0; w < eBBUFSZ; w++)
            localData.data[w] = '\0';
        for (register int w = 0; w < NOMINALSZ; w++)
            localData.SCS_ID[w] = '\0';
        localData.Len[ELL] = 0;  localData.Len[SCSID] = 0;
        localData.Len[2] = 0;  localData.Len[3] = 0;  // tet data

        localData.statusindex = -1;

    return localData;
}

//////////////  getbdsize ///////////////////////////////
//  Desc:  Get the size of Binary data iwhout and with padding
size_t getbdsize() {
    size_t i,j = 0;
    BinaryData myData;
    BinaryData rcvData;

    // replace below with a function
    // Clear each buffer according to its correct size
    rcvData = initBinaryData(); // MAKES EVERYTHIMG 0 OR nul IN RECEIVING OBJECT

    // copy received structure to local structure myData
/////////////////////////////////////////////////////////////////////////////
////// Use this as a TEMPLATE  FPR COPYING THE BINARYDATE STRUCTURE /////////
    myData.Len[ELL] =  rcvData.Len[ELL];
    memcpy(myData.data, rcvData.data, sizeof(myData.data)); // empty data
    // when copying real data -memcpy(myData.data, rcvData.data, myData.Len[ELL]);
    myData.Len[SCSID] =  rcvData.Len[SCSID];
    myData.Len[2] =  rcvData.Len[2];
    myData.Len[3] =  rcvData.Len[3];
    memcpy(myData.SCS_ID, rcvData.SCS_ID, sizeof(myData.SCS_ID)); // empty data
    // when copying real data -memcpy(myData.SCS_ID, rcvData.SCS_ID, myData.Len[SCSID]);
    myData.statusindex = rcvData.statusindex;
/////////////////////////////////////////////////////////////////////////////

    // now determine the size of the structure
    j = sizeof(myData.Len[ELL]);
    i = j*2;
    //printf("size_t elements = %ld bytes ... total size %ld\n",  j, i);
    j = sizeof(myData.statusindex);
    i = i + j;
    //printf("statusindex %ld bytes ... running total size %ld\n", j, i);
    j = sizeof(myData.data);
    i = i + j;
    //printf("data %d bytes ... running total size %ld\n", j, i);
    j = sizeof(myData.SCS_ID);
    i = i + j;
    //printf("SCS_ID %ld bytes ... running total size %ld\n", j, i);
    j = sizeof(myData.statusindex);
    i = i + j;
    //printf("statusindex %ld bytes ... running total size %ld\n", j, i);
    //printf("calculated manually Binary Data without padding %ld\n", i);
    j = sizeof(BinaryData); // Most important
#ifdef DEEP_DEBUG    
    printf("BinayData sizeof: %ld\n", j);
#endif
    if (i > MaxPktSz) {
        fprintf(stderr,"Error 1022: BD size too large %ld\t %ld over\n", i, i-1420);
    }
    return i;
}

////////////////// GenAnACK ///////////////
BinaryData GenAnACK() {     
#ifdef DEEP_DEBUG
	printf("In GenAnACK()\n");
	printf("clearing the State Machine\n");
#endif
    CLRSteMach();   // wipe out the SM, starting over
    int type = 1;
	char Ping[GENBUFSZ];    
    char* ping = Ping;

    ping = GenPrivateKey(type);  //Generate a Ping Value with type - side
    Pstrcpy( (char *)rawpingptr, ping );

    BinaryData rcvData;
    BinaryData myData;
    // clear myData data object
    rcvData = initBinaryData(); // MAKES EVERYTHIMG 0 OR nul IN RECEIVING OBJECT
    // copy received structure to local structure myData
    myData.Len[ELL] =  rcvData.Len[ELL];
    memcpy(myData.data, rcvData.data, sizeof(myData.data)); // empty data
    // when copying real data memcpy(myData.data, rcvData.data, myData.Len[ELL]);
    myData.Len[SCSID] =  rcvData.Len[SCSID];
    myData.Len[2] =  rcvData.Len[2];
    myData.Len[3] =  rcvData.Len[3];
    memcpy(myData.SCS_ID, rcvData.SCS_ID, sizeof(myData.SCS_ID)); // empty data
    // when copying real data memcpy(myData.SCS_ID, rcvData.SCS_ID, myData.Len[SCSID]);
    myData.statusindex = rcvData.statusindex;
    // myData is now initialized asn ready

    // Encrpyt he return value
    size_t len =  strlen( (const char *)rawpingptr );
#ifdef DEEP_DEBUG
    printf("Sending \t%s\nafter it's encrpyted\n", rawpingptr);
#endif
    myData = PUEncrypt(rawpingptr, len);
    // Add status
    myData.statusindex = ACK_ret;
    
    return myData;    // the buffer to send back to network interface
}
///////////// MessageHandler /////////////////
BinaryData MessageHandler(BinaryData pData) {  
    //char Prefix[5];
    //char* Prefixptr = Prefix;
    char MSG[BBUFSZ];       //put decrypted data here
    char* msg = MSG; 
#ifdef DEEP_DEBUG
    printf("In MessageHandler:\n");
#endif
    BinaryData myData;
    BinaryData rcvData;

    //zero out rcvdata
    rcvData = initBinaryData(); // MAKES EVERYTHIMG 0 OR nul IN RECEIVING OBJECT
    // Make both local BD objects identical
    myData.Len[ELL] =  rcvData.Len[ELL];
    memcpy(myData.data, rcvData.data, sizeof(myData.data)); // empty data
    // when copying real data memcpy(myData.data, rcvData.data, myData.Len[ELL]);
    myData.Len[SCSID] =  rcvData.Len[SCSID];
    myData.Len[2] =  rcvData.Len[2];
    myData.Len[3] =  rcvData.Len[3];
    memcpy(myData.SCS_ID, rcvData.SCS_ID, sizeof(myData.SCS_ID)); // empty data
    // when copying real data memcpy(myData.SCS_ID, rcvData.SCS_ID, myData.Len[SCSID]);
    myData.statusindex = rcvData.statusindex;
    // myData ready to be used
    
    msg = PUDecrypt( pData );
    if (msg == NULL) {
        myData.statusindex = BUUFERNOT;
        return myData;
    }
    char* err = "Too Large to Decrypt!!!!";
    if (strcmp(msg, err) == 0) { // Should not happen
        // Problem, pData too large to decrypt!
        myData.statusindex = BUUFERNOT;
        return myData;
    }

#ifdef DEEP_DEBUG
    printf("msg:\n%s\n", msg);
#endif
    rcvData = msgtype( msg );

    // copy received structure to local structure myData
/////////////
    myData.Len[ELL] = rcvData.Len[ELL];
    memcpy(myData.data, rcvData.data, myData.Len[ELL]);
    myData.Len[SCSID] = rcvData.Len[SCSID];
    myData.Len[2] = rcvData.Len[2];
    myData.Len[3] = rcvData.Len[3];
    memcpy(myData.SCS_ID, rcvData.SCS_ID, myData.Len[SCSID]);
    myData.statusindex = rcvData.statusindex;

    if( myData.statusindex == SCS_setup ) { // SCS is set up!
        memcpy(myData.SCS_ID, myData.SCS_ID, myData.Len[SCSID]);
        return myData;
    }

    if (myData.data[0] == '\0') {
        // should never happen
        myData.statusindex = MSGsDONE;
        return myData;
    }

    if( myData.statusindex == MSGsDONE ) {
        // should never happen
        return myData;
    }

    // If the return is ASCII, it's the return of decryption, passs it back to the caller
    int ret = 0;  
    ret = isstr_ascii( (const char*)myData.data );
    if (ret == 0) {   
        // It's an ASCII string decrypting a DEC sent
        myData.statusindex = DEC;       // Needed or already there?
    }

    return myData;
}

//////////////////////////// EncryptNSend ////////////////////////////
// Desc:    Encrypt the input string and return the encrypted string with currnet PKey
// Params:  char* input, clear text string
//          the scsid for communicating to a specific channel in a 1-many model
//          this scsid was sent to the 1, server, when the scs was setup
// Return:  BinaryData myData object
//
// PWR point = check that where in CK that scsid was pulled inot the main sim!

BinaryData EncryptNSend(char* input, unsigned char* scsid) {
    unsigned char EncrVal[eBBUFSZ];
    unsigned char* EncrValptr = EncrVal;
    int ilen;
    BinaryData myData;
    BinaryData rcvData;

    rcvData = initBinaryData(); // MAKES EVERYTHIMG 0 OR null IN RECEIVING OBJECT
    // copy recvData to local myData variable
    myData.Len[ELL] =  rcvData.Len[ELL];
    memcpy(myData.data, rcvData.data, sizeof(myData.data)); // empty data
    myData.Len[SCSID] =  rcvData.Len[SCSID];
    myData.Len[2] =  rcvData.Len[2];
    myData.Len[3] =  rcvData.Len[3];
    memcpy(myData.SCS_ID, rcvData.SCS_ID, sizeof(myData.SCS_ID)); // empty data
    myData.statusindex = rcvData.statusindex;

/*    if((Rcvr_Side == false) && (S2_IsEncrypted == false)) {
        // SCS not set up! Return an error
        myData.statusindex = SCS_Not;   // check for this in the calling function
        return myData;
    }
*/
    ///n a 1-2-many model, the server, the sender may need to be set this 
    ///due to mult-threading server capabilities where the server was last
    // communicating with another "many" channel
    if (Sender_Side == true) {   // the server in the chat system
#ifdef DEEP_DEBUG
        size_t passed = strlen((const char *)scsid);
        size_t sss = strlen((const char *)SCS_ID_ptr);
        size_t glob = strlen((const char *)T_DaKey_ptr);
        printf("SCS_ID_ptr\n%ld\t%s\n", sss, SCS_ID_ptr );
        printf("scsid\n%ld\t%s\n", passed, scsid );
        printf("SCS_ID_ptr\nl%d\t%s\n", glob, T_DaKey_ptr );
#endif
        int one = strcmp((const char *)SCS_ID_ptr, (const char *)scsid);
        int two = strcmp((const char *)T_DaKey_ptr, (const char *)scsid);
        if(( one != 0) || ( two != 0) ) {
            // On the sender side, if the SCS_ID differs from T_DaKey_ptr,
            // the data was most likely used by a different server thread 
            // which needs to be corrected for this thread,
            // force it to be the same! to get to the correct client.
            ilen = strlen( (const char *)scsid );
            int three = strcmp((const char *)SCS_ID_ptr, (const char *)scsid);
            if(three != 0 )
                memcpy( SCS_ID_ptr, scsid, ilen);   // Careful
            int five = strcmp((const char *)T2_DaKey_ptr, (const char *)scsid);
            if(five != 0 )// should only be checked if 12M server
                memcpy( T2_DaKey_ptr, scsid, ilen); // Careful
#ifdef DEEP_DEBUG
            printf("scsid set to the correct scsid to communicate to the other side");
#endif
        }
    }

    ilen = strlen((const char *)input);
    if (ilen > BBUFSZ) {
        input[BBUFSZ-4] = '\0';  // Truncate the string at BBUFSZ characters, save room for the msgtype
        ilen = BBUFSZ-4;
    }
    memset(EncrVal, 0, sizeof(EncrVal));
    snprintf( (char *)EncrValptr, ilen+4, "%s%s", "DEC", input);
    size_t plen = strlen( (const char *)EncrValptr );

    // precautionary, size should already be truncated.....now just in case, shoud never execute!
    ilen = strlen((const char *)EncrValptr);
    if (plen > BBUFSZ) {
        EncrValptr[BBUFSZ] = '\0';  // Truncate the string at BBUFSZ characters
        ilen = BBUFSZ;
    }

    // OLD recvData = P3UEncrypt( EncrValptr, plen );
    rcvData = PUEncrypt( EncrValptr, plen );

    // copy received structure to local structure myData
     myData.Len[ELL] =  rcvData.Len[ELL];
    memcpy(myData.data, rcvData.data, myData.Len[ELL]);
    myData.Len[SCSID] =  rcvData.Len[SCSID];
    myData.Len[2] =  rcvData.Len[2];
    myData.Len[3] =  rcvData.Len[3];
    memcpy(myData.SCS_ID, rcvData.SCS_ID, myData.Len[SCSID]);
    myData.statusindex = DEC;   //Decryption message
    // local variable myData now contains which should be returned to the caller

    /*if ( Rcvr_Side == true ) {
        int four = strcmp((const char *)T2_DaKey_ptr, (const char *)scsid);
        if(four != 0 )
            memcpy( T2_DaKey_ptr, scsid, ilen);   // Careful, never execute?
    }*/

    return myData;
}

///////////////////////// getSCS_ID //////////////////////////////////
// Desc:    Get the ID of the Secure Communications Session, set when the session was completed
// Returns: the 64 byte session ID

char* getSCS_ID () {
    return(SCS_ID_ptr);
}
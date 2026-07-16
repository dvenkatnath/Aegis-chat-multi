#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <string.h>
#include <time.h>
#include <stdio.h>
#include <math.h>
#include <stdbool.h>
#include "Global2.h"
#include "SHA256.h"
#include "Functions3.h"
#include "Pstr.h"
#include "HighLevelED.h"
#include "UberHighLevED.h"
#include "common.h"

//#define DEEP_DEBUG

// Temp GVars for SM set up
char T_PingData[STDSZ]; // The ping string sent out by the initiator of a ping-pong
char T_PongData[STDSZ]; // The pong string sent out by the responder of a ping-pong
char T2_PingData[STDSZ]; // The ping string sent out by the initiator of a ping-pong
char T2_PongData[STDSZ]; // The pong string sent out by the responder of a ping-pong
char T2_RevPongdata[STDSZ]; // The reverse pong string is the actual pong string reversed order
char T2_DaKey[160]; /* holds ping+revpong concat (2x79 chars) before SHA */ // The concatenated ping and and revpong strings after a SHA256
char T2_F_DaKey[STDSZ]; // The Private Encrypt Key is THE key for SCS
char T_RevPongdata[STDSZ]; // The reverse pong string is the actual pong string reversed
char T_DaKey[160]; /* holds ping+revpong concat (2x79 chars) before SHA */ // The concatenated ping and and revpong strings after a SHA256
char T_F_DaKey[STDSZ]; // The Private Encrypt Key is THE key for SCS
char SCS_ID[STDSZ];	// SCS ID

char* T_PingData_ptr = T_PingData;
char* T_PongData_ptr = T_PongData;
char* T2_PingData_ptr = T2_PingData;
char* T2_PongData_ptr = T2_PongData;
char* T2_RevPongdata_ptr = T2_RevPongdata;
char* T_RevPongdata_ptr = T_RevPongdata;
char* T_DaKey_ptr = T_DaKey;
char* T_F_DaKey_ptr = T_F_DaKey;
char* T2_DaKey_ptr = T2_DaKey;
char* T2_F_DaKey_ptr = T2_F_DaKey;
char* SCS_ID_ptr = SCS_ID;

///////////////////////// is_prime //////////////////////////////////
/**
 * IS it a prime number
 *
 * Parameters:
 *   - int num
 *
 * Returns:
 *   - false: It is NOT a prime number
 *   - true: It IS a prime number
 */
bool is_prime(int num) {
    if (num <= 1) return false;      // 0 and 1 are not prime
    if (num <= 3) return true;       // 2 and 3 are prime
    if (num % 2 == 0 || num % 3 == 0) return false; // Divisible by 2 or 3

    register int i;
    // Check divisors from 5 to sqrt(num)
    for (i = 5; i * i <= num; i += 6) {
        if (num % i == 0 || num % (i + 2) == 0) {
            return false;
        }
    }
    return true;
}

////////////////////////////// next_prime ///////////////////////////////////
// Function to find and return the next prime number larger than the input
int next_prime(int num) {
    register int candidate;

    if (num <= 1) return 2; // The first prime number is 2
    candidate = num + 1; // Start checking from the next number
    while (!is_prime(candidate)) {
        candidate++;
    }
    return candidate; // Return the next prime number
}

//////////// GetUniqueNO ///////////////////////////////////
#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <time.h>

#define MAX_NUMBERS 1000000  // Adjust as needed
#define HASH_SIZE (MAX_NUMBERS * 2)

uint64_t GetUniqueNO() {
    static uint64_t *used_numbers = NULL;
    static size_t used_count = 0;
    static int initialized = 0;

    if (!initialized) {
        used_numbers = calloc(HASH_SIZE, sizeof(uint64_t));
        if (!used_numbers) {
            fprintf(stderr, "Memory allocation failed.\n");
            exit(EXIT_FAILURE);
        }
        srand((unsigned int)time(NULL));
        initialized = 1;
    }

    if (used_count >= MAX_NUMBERS) {
        fprintf(stderr, "Maximum number of unique numbers reached.\n");
        exit(EXIT_FAILURE);
    }

    uint64_t num;
    size_t index;
    while (1) {
        num = ((uint64_t)rand() << 32) | rand();
        index = num % HASH_SIZE;

        // Open addressing linear probe
        while (used_numbers[index] != 0 && used_numbers[index] != num) {
            index = (index + 1) % HASH_SIZE;
        }

        if (used_numbers[index] == 0) {
            used_numbers[index] = num;
            used_count++;
            return num;
        }
        // else, collision with same number — try again
    }
}

///////////////////  GetUniqueNOShaStr  ///////////////////////////
char* GetUniqueNOShaStr() {
    int random_number;
    int prime_randomNo;
    char buffer[GENBUFSZ];    // buffer for the string
    char Res[GENBUFSZ];
    char* Resptr = Res;

#ifdef DEEP_DEBUG
    printf("In GetUniqueNOShaStr Calling GetUniqueNO\n");
#endif
    random_number = GetUniqueNO();;
    prime_randomNo = next_prime((int)random_number);
    // get no into a string
    
    size_t buffer_size = sizeof(buffer);
    snprintf(buffer, buffer_size, "%d", prime_randomNo);    // hex "%016llx"
#ifdef DEEP_DEBUG
	printf("random_number: %s\n", buffer);
#endif

    Resptr = SHAPassed(buffer);
#ifdef DEEP_DEBUG
	printf("Return from SHAPassed %s\n", Resptr);
#endif
	return Resptr;
}

/////////////////////////// SHAPassedDeux //////////////////////////////
/* Double-SHA256, fully stack-based - no malloc, no heap leak.
 * Previously called SHAit() (which calls sha256() which malloc's 65 bytes)
 * twice, leaking the first allocation on every call. After ~1000 loop
 * iterations the heap was exhausted and malloc returned NULL, causing the
 * segfault at address 0x0.
 * out must be at least 65 bytes and is returned for convenience.
 */
char* SHAPassedDeux(char* str) {
    static char buf1[65];
    static char buf2[65];
    SHAit_buf(str,  buf1);   /* first SHA into buf1  */
    SHAit_buf(buf1, buf2);   /* second SHA into buf2 */
    return buf2;
}

/////////////////////////// SHAPassed //////////////////////////////
/* Single-SHA256, fully stack-based - no malloc, no heap leak. */
char* SHAPassed(char* str) {
    static char buf[65];
    SHAit_buf(str, buf);
    return buf;
}

/////////////////////////// Addtype2msg //////////////////////////////
// Prefix the type of type to the string
char* Addtype2msg(char* str, char* type) {
    char addingtype[GENBUFSZ];
    char* addingtypeptr = addingtype;
    char* atmp;

    atmp = Pstrcpy(addingtypeptr, type);
    atmp = Pstrncat(addingtypeptr, str, (int)strlen(str));
    return atmp;
}

///////////////////////// GenPrivateKey //////////////////////////////
char* GenPrivateKey(int type) {
    char str2[GENBUFSZ];
    char* str2sptr = str2;
    int len;
	int ilen = 0;
    char addingtype[GENBUFSZ];
    char* addingtypeptr = addingtype;
    static char Retval[128];  /* static: returned pointer must survive caller frame */
	char* retval = Retval;
    char* errret;
    char SSStmp[GENBUFSZ];
    char* Valptr = SSStmp;

#ifdef DEEP_DEBUG
	printf("In GenPrivateKey(%d)\n", type);
#endif
    str2sptr = GetUniqueNOShaStr();

    /* Use stack-based SHA to avoid malloc leaks in tight loops */
    char sha1buf[65], sha2buf[65];
    SHAit_buf(str2sptr, sha1buf);
    SHAit_buf(sha1buf,  sha2buf);   /* double-SHA forces 64-char output */
    Valptr = sha2buf;
    ilen = strlen(Valptr);
#ifdef DEEP_DEBUG
    //printf("Valptr: %s\tlen: %d\n", Valptr, ilen);
#endif

    if (type == 1) { // create ping
        // set pingdata global variable
        len = GENBUFSZ;
        if ( ilen < GENBUFSZ ) {
        // clearstate machine values
            CLRSteMach();           // Clear the state maching starting freshf
            Pstrcpy(T_PingData_ptr, Valptr);  // set PingData global variable
		    Sender_Side = true;  // set global variable
            //PRTSteMach();
#ifdef DEEP_DEBUG
            printf("Sender set PingData_ptr & Me_sender in SM\n");
#endif
            // Prefix the type of ACK to the string
            addingtypeptr = Pstrcpy(addingtypeptr, "ACK");

            long sn = getSN();
            char SN[25];
            char* SNptr = SN;
            snprintf(SNptr, 20, "%010ld", sn);  

            addingtypeptr = Pstrncat(addingtypeptr, SNptr, 25);         // Add in the Side 1 SerNo
			int retlen = strlen(addingtypeptr) + strlen(Valptr) + 1;    // for adding a null terminator
            snprintf(retval, sizeof(Retval), "%s%s", addingtypeptr, Valptr);  /* capped to buffer size */
            return retval;
        }
        else {
            // PWR Handle error: length exceeds buffer size
            errret = "-1";
			return errret;
        }
    }
    else { // type = 2  create pong - NAK ////////////////////////////////
        Pstrcpy(T2_PongData_ptr, Valptr);
        len = strlen(T2_PongData_ptr);
#ifdef DEEP_DEBUG
        printf("On Side 2: T2_PongData_ptr TSM updated\n%s  len %d\n", T2_PongData_ptr, len);
#endif
		//PRTSteMach();
		return T2_PongData_ptr;
    }
}    


////////////////////////////// ProcessRcvd1stNAK //////////////////////////////
int ProcessRcvd1stNAK(char* msg) {
    // With a raw string passed to this message, the following steps will occur
    // process elements of the TSM for side 1
    // SCS is complete
    // flag using the PKey from here on
    //char* type = rawstring;  
	char PONGdata[GENBUFSZ];
	char* pongdata = PONGdata;

#ifdef DEEP_DEBUG
	printf("In ProcessRcvd1stNAK, in Side 1 received a NAK from Side 2\n");
    printf("Incoming: %s\n", msg);
#endif
	pongdata = msg;
    Pstrcpy(T_PongData_ptr, pongdata);    // Copy the pongdata to the TSM global variable
    DoRevPong(T_PongData_ptr, T_RevPongdata_ptr);
    CreateTDa_Key(1);
#ifdef DEEP_DEBUG
    ND_PRTSteMach();        // last printing of TSM
#endif
    IsEncrypted = 1;    // Set PK encrpytion as a result of receiving the TSM data on Side 1
	return 1;
}

////////////////////////////////// /////// CreateTDa_Key //////////////////////////////
void CreateTDa_Key(int side) {
    /* Use local sha_result pointer for SHA step so T*_DaKey_ptr always points
     * to the T*_DaKey[160] array, never to SHAPassedDeux's internal static buffer.
     * Previously T2_DaKey_ptr got reassigned to SHAPassedDeux's 65-byte static buf
     * and the next snprintf into it overflowed. */
    char* sha_result;
    int TPLen;
    if (side == 2) {
        T2_DaKey_ptr = T2_DaKey;   /* reset to backing array in case it drifted */
        TPLen = (int)strlen(T2_PingData_ptr) + (int)strlen(T2_RevPongdata_ptr) + 1;
        if (TPLen > 160) TPLen = 160;
        snprintf(T2_DaKey_ptr, (size_t)TPLen, "%s%s", T2_PingData_ptr, T2_RevPongdata_ptr);
        sha_result = SHAPassedDeux(T2_DaKey_ptr);
        strncpy(T2_DaKey_ptr, sha_result, 159); T2_DaKey_ptr[159] = '\0';
        memcpy(BU_S2_DaKeyPTR, T2_DaKey_ptr, strlen(T2_DaKey_ptr) + 1);
        memcpy(SCS_ID_ptr, T2_DaKey_ptr, strlen(T2_DaKey_ptr) + 1);
    } else {
        T_DaKey_ptr = T_DaKey;     /* reset to backing array */
        TPLen = (int)strlen(T_PingData_ptr) + (int)strlen(T_RevPongdata_ptr) + 1;
        if (TPLen > 160) TPLen = 160;
        snprintf(T_DaKey_ptr, (size_t)TPLen, "%s%s", T_PingData_ptr, T_RevPongdata_ptr);
        sha_result = SHAPassedDeux(T_DaKey_ptr);
        strncpy(T_DaKey_ptr, sha_result, 159); T_DaKey_ptr[159] = '\0';
        memcpy(SCS_ID_ptr, T_DaKey_ptr, strlen(T_DaKey_ptr) + 1);
    }
}

// Note, the point here is to STOP Type 1 or 2 code and just have 1 calculation forcing common formatted next calcs.
// Side 2 always calcs first, then side 1.
// Processing assume ping, pong, revpong are all done before CreateKey is called, CreateKey could only calc DaKey * F_DaKey.
// Do a Create prKey function to do the smae with Ping Pong and Rev Pone data
// To Succeed, preKey does the calculations, populte T-vars, and CreatKey only doew only DaKey & F_DsKey,
// and t the end populates the SM vars

void ND_TSMP() {		//Print the global vaiables
#ifdef DEEP_DEBUG
	printf("SerNo: \t\t\t%s\n",C_SerNoPtr);
    printf("closedloopbrdgeNo:\t%s\n\n",C_closedloopbrdgeNoPtr);

    printf("T_PingData_ptr = \t%s\n", T_PingData_ptr);
    printf("T_PongData_ptr =\t%s\n", T_PongData_ptr);
    printf("T_RevPongdata_ptr =\t%s\n\n", T_RevPongdata_ptr);
    //printf("Pre =\t\t\t%s\n\n", T_DaKey_ptr);
    printf("T2_PingData_ptr = \t%s\n", T2_PingData_ptr);
    printf("T2_PongData_ptr =\t%s\n", T2_PongData_ptr);
    printf("T2_RevPongdata_ptr =\t%s\n", T2_RevPongdata_ptr);
    //printf("Pre =\t\t\t%s\n", T2_DaKey_ptr);
#endif
}

char* CreateFinalPK(char* DK_ptr) {
    char tempPK[GENBUFSZ];
    char* tempPKptr = tempPK;

    // SHAPassedDeux the DaKey to make F_DaKey
    tempPKptr = SHAPassedDeux(DK_ptr);
    Sim_complete = 1;   // simulation os completed

    return tempPKptr;
}

////////////////////////////// CreateDaKey //////////////////////////////

void CreateDaKey(int type) {
    // Create the Key for this communication session
    // General, to execute, all steps from the beginning must start over, because each communication session
    // is to have a unique key to secure the stream.
    // By the time this method is called, the ping-pong has been processed on each side,
    // leaving the final creating process each side of a communication will complete independently.
    // Final Process
    // 1. Point: the initiator of the process always creates the ping, and the responder creates the pong.
    //     To be clear, the responder creates the pong, they do not initiate a ping once a ping is received
    // 2. Obtain both the PingData and reversePongData global variables
    // 3. SHA256 each again.
    // 4. Concatenate the two strings together
    // 4a. SAVE THIS TO THE DaKey GLOVBAL VARIABLE
    // 5. SHA256 the DaKey saving it as F_DaKey
    // ASSUMES THE MAIN SM IS EMPTY except for the ACK & NACK processing
    
#ifdef DEEP_DEBUG
    printf("In CreateDaKay for Side %d\n", type);
#endif
    // Function Prototype declaration
	void Preverse_string(char* str);
	
    //PRTSteMach();       // view it
    //void ND_TSMP();

    // Update the TSM
    if(type == 1) {			//ACK
        CreateTDa_Key(1);
        //T_F_DaKey_ptr = CreateFinalPK(1, T_DaKey_ptr);

#ifdef DEEP_DEBUG
        printf("T_DaKey_ptr updated in TSM\n%s\n", T_DaKey_ptr);
#endif
    //PRTSteMach();
    }
    else {				// NAK
        CreateTDa_Key(2);
        //PRTSteMach();
#ifdef DEEP_DEBUG
        printf("T_DaKey_ptr updated in SM\n%s\n", T_DaKey_ptr);
#endif
    }
    
    //PRTSteMach();
	if(type == 1) {			//ACK
		//CheckSMVars();		// Check integrity of SM
 	} else {				// NAK
        T2_F_DaKey_ptr = CreateFinalPK(T2_DaKey_ptr);
        T2_F_DaKey_ptr = SHAPassedDeux(T2_F_DaKey_ptr);     // Sink Hole Side 2

        //PRTSteMach();
        //Pstrcpy(BU_S2_F_DaKeyPTR, T2_F_DaKey_ptr);		// backup this location
#ifdef DEEP_DEBUG
        //printf("S2_F_DaKey updated in SM\n%s\n", S2_F_DaKey_ptr);
        //printf("BU_S2_F_DaKeyPTR updated in SM\n%s\n", BU_S2_F_DaKeyPTR);
#endif
	}
}
/* NLN
////////////////////////////////// checkS2FDaKey ////////////////////////////////////////
// Desc: check to see if an element of the state machine is still there.  It so,
//Retrieve it from the BU variable and restore it
void checkS2FDaKey() {
	int len;
	len = strlen (S2_F_DaKey_ptr);
#ifdef _DEBUG
	//printf("In checkS2FDaKey checkS2FDaKey %s len = %d\n", S2_F_DaKey_ptr, len);
#endif
	if( len < 63) {
		// data has been stomped on, restore it
		//Pstrcpy(S2_DaKey_ptr, BU_S2_F_DaKeyPTR);
#ifdef _DEBUG
		//printf("S2_DaKey_ptr restored\n");
#endif
	}
}
*/
////////////////////////////////// DoRevPong /////////////////////
// Perform the TSM Reverse Pong op
void DoRevPong(char* base, char* revbase) {
    Pstrcpy(revbase, base);     // Place base into reverse location
    Preverse_string(revbase);
}

////////////////////////////////// GenNACK ////////////////////
BinaryData GenNACK() {

#ifdef _DEBUG
    //printf("In GenNACK\n");
#endif
    char Pong[GENBUFSZ];    // The Pong value to be generated or BBUFSZ
    char* pongptr = Pong;
    int type = 2;  // Pong
    //char buf[BBUFSZ];
    //char* bbuf = buf;   //the encrypted storage area
    unsigned char TypePong[BBUFSZ];
    unsigned char* TypePongPTR = TypePong;
    int TPlen;

    pongptr = GenPrivateKey(type);  //Generate a Pong Value by receiver      // called from msgtype
    TPlen = strlen(pongptr);

    // Populate TSM
    DoRevPong(T2_PongData_ptr, T2_RevPongdata_ptr);
    //PRTSteMach();
	CreateTDa_Key(2);
    // PWR check T2_DaKey_ptr and set SCS_ID_ptr
#ifdef _DEBUG
    //printf("T2_PongData_ptr value %s\n", T2_PongData_ptr);
#endif
    TPlen = strlen("NAK") + strlen(T2_PongData_ptr) + 1;
    snprintf((char *)TypePongPTR, TPlen, "%s%s", "NAK", T2_PongData_ptr);
    size_t tylen = strlen((const char *)TypePongPTR);

    BinaryData myData;
    BinaryData rcvData;

    rcvData = initBinaryData(); // MAKES EVERYTHIMG 0 OR nul IN RECEIVING OBJECT

    // copy received structure to initiated local structure myData
    myData.Len[ELL] =  rcvData.Len[ELL];
    memcpy(myData.data, rcvData.data, sizeof(myData.data)); // empty data
    myData.Len[SCSID] =  rcvData.Len[SCSID];
    myData.Len[2] =  rcvData.Len[2];
    myData.Len[3] =  rcvData.Len[3];
    memcpy(myData.SCS_ID, rcvData.SCS_ID, sizeof(myData.SCS_ID)); // empty data
    myData.statusindex = rcvData.statusindex;
    // PWR check T2_DaKey_ptr and set SCS_ID_ptr

    myData = PUEncrypt(TypePongPTR, tylen);
#if DEEP_DEBUG  
    printf( "Side 2-NAK message %s \n", TypePongPTR );
    printf("Side 2 - PKey enabled!\n");
#endif
    myData = PUEncrypt(TypePongPTR, tylen);
    myData.statusindex = NAK_ret;      // Add status
    size_t t2scsid = strlen(T2_PongData_ptr);
    myData.Len[SCSID] = t2scsid;
    memcpy(myData.SCS_ID, T2_PongData_ptr, t2scsid); // empty data

    Rcvr_Side = true;
    S2_IsEncrypted =  true;     // Set Side 2 to use TM PK for all next EDs

    return myData;
}

////////////////////// Util functions /////////////////////////

//////////////////////////// is_ascii ////////////////////////////
// name: is_ascii
// @param: const char* str - the string to check
// @return: int - 0 if all characters are ASCII, -1 if not
//
int isstr_ascii(const char* str) {
    while (*str) {
        if ((unsigned char)*str > 127) {
            return -1;  // Not ASCII
        }
        str++;
    }
    return 0;  // All characters are ASCII
}

//////////////////////////// SIM_Verified ////////////////////////////
// Desc: Do the private private keys match?  If so, the simulation is complete.
int SIM_Verified () {

if ((strlen(T_F_DaKey_ptr) == 64) && (strlen(T2_F_DaKey_ptr) == 64)) {
        if (Pstrcmp(T_F_DaKey_ptr, T2_F_DaKey_ptr) == 0) {
            return V_SUCCESS;
        } else {
            return V_FAIL;
        }
    }
    return V_SUCCESS;
}
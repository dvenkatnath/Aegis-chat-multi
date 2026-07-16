#include <stdbool.h>
#include <stdio.h>
#include <string.h>
#include "api.h"
#include "common.h"
#include "Pstr.h"
#include "Global2.h"

//#define DEEP_DEBUG


char* print_with_commas(long num) {
    char buffer[32];  // Enough space for formatted number
    char formatted[32]; // Output buffer
    char* fptr = formatted;
    register int len, i, j;// comma_count;

    sprintf(buffer, "%ld", num); // Convert to string
    len = strlen( (const char *)buffer );
    //comma_count = (len - 1) / 3; // Number of commas needed
    
    j = 0;
    for (i = 0; i < len; i++) {
        if (i > 0 && (len - i) % 3 == 0) {
            formatted[j++] = ','; // Insert comma
        }
        formatted[j++] = buffer[i]; // Copy digit
    }
    formatted[j] = '\0'; // Null-terminate string

    //printf("%s\n", fptr); // Print result

    return fptr;
}

char DaNum[64];
char* Danumptr = DaNum;

int main() {
    int tryGenAnACK = 0;

    char S1[BBUFSZ] = "1 WHEN in the Course of human Events, it becomes \
necessary for one People to dissolve the Political Bands which have connected \
them with another, and to assume among the Powers of the Earth, the separate \
and equal Station to which the Laws of Nature and of Nature’s God entitle them, \
a decent Respect to the Opinions of Mankind requires that they should declare \
the causes which impel them to the Separation. We hold these Truths to be \
self-evident, that all Men are created equal.";

// DEBUG    char S1[BBUFSZ] = "1 WHEN in the Course of human Events, it becomes";
    char* S1ptr = S1;

    char S2[BBUFSZ] = "2 We the People of the United States, in Order to \
form a more perfect Union, establish Justice, insure domestic Tranquility, \
provide for the common defence, promote the general Welfare, and secure the \
Blessings of Liberty to ourselves and our Posterity, do ordain and establish \
this Constitution for the United States of America";
    char* S2ptr = S2;

    unsigned char ScsID[STDSZ];
	unsigned char S1d[MaxPktSz];
	unsigned char* S1d_ptr = S1d;
    unsigned long iter = 1;   // iterations of the resets

    //////////////////// Testing ED Speed ////////////////////////////////
    clock_t start, end;
    double cpu_time_used;
restart0:
    int j = 1000;
    int k = 0;
    BinaryData myData;
    //BinaryData cData;
    BinaryData rcvData;

    size_t clclr = strlen( (const char *)S1 );

///////////////////////////
    start = clock();  // Start timing
    for (k = 0; k < j; k++) {
        rcvData = initBinaryData(); // MAKES EVERYTHIMG 0 OR nul IN RECEIVING OBJECT
        S1ptr[BBUFSZ-1] = '\0';   /* was [BBUFSZ] - off-by-one write past end of S1 */
        size_t S1len = strlen( (const char *)S1ptr );
        rcvData = PUEncrypt(S1ptr, clclr);
        // Copytp local BD buffer
        myData.Len[ELL] =  rcvData.Len[ELL];
        memcpy(myData.data, rcvData.data, myData.Len[ELL]);
        myData.Len[SCSID] =  rcvData.Len[SCSID];
        myData.Len[2] =  rcvData.Len[2];
        myData.Len[3] =  rcvData.Len[3];
        memcpy(myData.SCS_ID, rcvData.SCS_ID, myData.Len[SCSID]);
        myData.statusindex = rcvData.statusindex;

        S1d_ptr = PUDecrypt( myData );
        if(Pstrcmp( (const char *)S1ptr, (const char *)S1d_ptr ) == 0) {
#ifdef DEEP_DEBUG
            //printf("ED PASSED %d\n", k);
#endif
            int l = 0;
        } else {
            printf("\aED FAILED\n");
            break;
        }
    }
    end = clock();  // End timing
     // Calculate time in seconds
    cpu_time_used = ((double)(end - start)) / CLOCKS_PER_SEC;
#ifdef DEEP_DEBUG
    printf("%d E or D executions in %f seconds\n", k*2, cpu_time_used);
#endif
    if(k != j) {
        printf("\aOops???? restarting\n");
        //sleep(3);
        goto restart0;
    }
    //sleep(2);
    //printf("Press a key to Continue");
    //getchar();

/////////////////////////////////////////////////////////////////////
    // time simulation
    start = clock();  // Start timing
restart:
    //printf("\033[H\033[J");  // ANSI escape sequence to clear screen
    //printf("This main worlflow verification file assumes the simulation of the network interface NI\n");
	//printf("Only API calls available via the API or message handler will be used\n");
   // printf("In Main\n");

	/////////////////////////////////////////////////////////////////////
#ifdef DEEP_DEBUG
    printf("Generate an ACK via GenAnACK() - Setting up a secure communications session\n\n");
#endif
    //static char MSG[BBUFSZ];
    //static char* msg;
    //msg = MSG;
	tryGenAnACKagain:
///////////////////////////// Side 1 /////////////////////////////////////////////////////
    Sender_Side = true;
    Rcvr_Side = false;
#ifdef DEEP_DEBUG
    printf("calling GenAnACK from Side 1\n");
#endif
    rcvData = initBinaryData(); // MAKES EVERYTHIMG 0 OR nul IN RECEIVING OBJECT
    rcvData = GenAnACK();  // return value is .a sored memory, copy to local storage
    // Copy to local BD buffer
    myData.Len[ELL] =  rcvData.Len[ELL];
    memcpy(myData.data, rcvData.data, myData.Len[ELL]);
    myData.Len[SCSID] =  rcvData.Len[SCSID];
    myData.Len[2] =  rcvData.Len[2];
    myData.Len[3] =  rcvData.Len[3];
    memcpy(myData.SCS_ID, rcvData.SCS_ID, myData.Len[SCSID]);
    myData.statusindex = rcvData.statusindex;
    // local variable myData ready to use
#ifdef DEEP_DEBUG
    printf("\nThe encrypted GenAnACK return string is being returnd the other side, side 2");
#endif
	//printf("it sends the message out to the other side\n////////////////////////////////////");
///////////////////////////// Side 2 ////////////////////////////////////
    Sender_Side = false;
    Rcvr_Side = true;
#ifdef DEEP_DEBUG
	printf("\n***Side 2 now receives the ACK from Side 1, the NI sends it to MessageHandler()\n");
    printf("\n\n//MessageHandler - Side 2 now receives the ACK from Side 1\n");
#endif
    // ACK Message has coming into Side 2 ********** ret1p is what is returned to the NI************
    //printf("Calling MessageHandler \n");
    rcvData = initBinaryData(); // MAKES EVERYTHIMG 0 OR nul IN RECEIVING OBJECT

    rcvData = MessageHandler( myData );
    // Copy to local BD buffer
    myData.Len[ELL] =  rcvData.Len[ELL];
    memcpy(myData.data, rcvData.data, myData.Len[ELL]);
    myData.Len[SCSID] =  rcvData.Len[SCSID];
    myData.Len[2] =  rcvData.Len[2];
    myData.Len[3] =  rcvData.Len[3];
    memcpy(myData.SCS_ID, rcvData.SCS_ID, myData.Len[SCSID]);   // scs_id here!  Client app should save this!
    myData.statusindex = rcvData.statusindex;
   // local variable myData ready to use

    if(rcvData.data[0] == '\0') {
        printf("******PROBLEM - nak RETURN IS null***\n\n");
        if(tryGenAnACK > 0) {  // GenAnACK OR MESSAGE HANDLER error
            printf("GenAnACK mssaging FATAL ERROR!!!!\n");
            exit -1;
        }
        // try GenAnACK again
        tryGenAnACK++;
        goto tryGenAnACKagain;
    }
///////////////////////////// Side 1 ////////////////////////////////////////////
    Sender_Side = true;
    Rcvr_Side = false;
#ifdef DEEP_DEBUG
    printf("SIMULATION NOW ON Side1 - Received the NAK from Side2\n");
#endif
    // Send the Encrypted string that will be received from the NI
#ifdef DEEP_DEBUG
    printf("Calling MessageHandler \n");
#endif
    rcvData = initBinaryData(); // MAKES EVERYTHIMG 0 OR nul IN RECEIVING OBJECT
    rcvData = MessageHandler( myData );

    // clear out myData for next usage
    myData.Len[ELL] =  rcvData.Len[ELL];
    memcpy(myData.data, rcvData.data, myData.Len[ELL]);
    myData.Len[SCSID] =  rcvData.Len[SCSID];
    myData.Len[2] =  rcvData.Len[2];
    myData.Len[3] =  rcvData.Len[3];
    memcpy(myData.SCS_ID, rcvData.SCS_ID, myData.Len[SCSID]);   // SCSID here, Side 1 should save it
    myData.statusindex = rcvData.statusindex;
    // local variable myData ready to use 
    memcpy( ScsID, myData.SCS_ID, myData.Len[SCSID] ); 
    // local variable myData now ready to use
    // when this completes, the secure encrpyted session is established!
    bool Sim1 = true;
    if(myData.statusindex == SCS_setup) {
#ifdef _DEBUG
        printf("\n***Secure Communications Channel Established***\n");
#endif
        Sim1 = true;
    }
    
    // demo encryption and message to be sent to the NI
///////////////////////////// Side 1 ///////////////////////////////////////
    Sender_Side = true;     // assume a 1-to-many model as a server - multiple client communication
    Rcvr_Side = false;
#ifdef DEEP_DEBUG
    printf("\nEncrypt string to be sent from Side 1 to Side 2\n%s\n", S1ptr);
#endif
    rcvData = initBinaryData(); // MAKES EVERYTHIMG 0 OR nul
    myData = initBinaryData(); // MAKES EVERYTHIMG 0 OR nul

    size_t l4len = strlen( (const char *)S1 );
    rcvData = EncryptNSend( S1, ScsID );

    // Copy to local BD buffer
    myData.Len[ELL] =  rcvData.Len[ELL];
    memcpy(myData.data, rcvData.data, myData.Len[ELL]);
    myData.Len[SCSID] =  rcvData.Len[SCSID];
    myData.Len[2] =  rcvData.Len[2];
    myData.Len[3] =  rcvData.Len[3];
    memcpy(myData.SCS_ID, rcvData.SCS_ID, myData.Len[SCSID]);
    myData.statusindex = rcvData.statusindex;
    // local variable myData ready to use

///////////////////////////// Side 2 ///////////////////////////////
    Sender_Side = false;
    Rcvr_Side = true;
#ifdef DEEP_DEBUG
    printf("Now simulating side 2\n");
#endif

    rcvData = initBinaryData();// MAKES EVERYTHIMG 0 OR nul

    rcvData = MessageHandler( myData );
    myData = initBinaryData(); // MAKES EVERYTHIMG 0 OR nul

    // Copy to local BD buffer
    myData.Len[ELL] =  rcvData.Len[ELL];
    memcpy(myData.data, rcvData.data, myData.Len[ELL]);
    myData.Len[SCSID] =  rcvData.Len[SCSID];
    myData.Len[2] =  rcvData.Len[2];
    myData.Len[3] =  rcvData.Len[3];
    memcpy(myData.SCS_ID, rcvData.SCS_ID, myData.Len[SCSID]);
    myData.statusindex = rcvData.statusindex;
    // local variable myData ready to use

    //  Verify the Decrytion     
    size_t S1siz = strlen(S1);
#ifdef DEEP_DEBUG
    printf("S1\n%s\n",S1);
#endif
    int chkcmp1 = strcmp(S1ptr, myData.data);
    if( chkcmp1 == 0 ) {
        printf("Decryption S1\n%s\n",myData.data );
    } else {
        printf("PROBLEM!!!  Decryption failed\n");
    }
    // Do an encyrpted message from side 2 to 1
///////////////////////////// Side 2 ///////////////////////////////////
#ifdef DEEP_DEBUG
    printf("\nEncrypt string sent from Side 2 to side 1\n%s\n", S2ptr);
#endif

    l4len = strlen( (const char *)S2ptr );
    rcvData = EncryptNSend( S2ptr, ScsID );

    myData = initBinaryData(); // MAKES EVERYTHIMG 0 OR nul

    // Copy to local BD buffer
    myData.Len[ELL] =  rcvData.Len[ELL];
    memcpy(myData.data, rcvData.data, myData.Len[ELL]);
    myData.Len[SCSID] =  rcvData.Len[SCSID];
    myData.Len[2] =  rcvData.Len[2];
    myData.Len[3] =  rcvData.Len[3];
    memcpy(myData.SCS_ID, rcvData.SCS_ID, myData.Len[SCSID]);
    myData.statusindex = rcvData.statusindex;
    // local variable myData ready to use

///////////////////////////// Side 1 ////////////////////////////
    Sender_Side = true;   
    Rcvr_Side = false;
    rcvData = initBinaryData(); // MAKES EVERYTHIMG 0 OR nul IN RECEIVING OBJECT

    rcvData = MessageHandler( myData );
    myData = initBinaryData(); // MAKES EVERYTHIMG 0 OR nul

    // Copy to local BD buffer
    myData.Len[ELL] =  rcvData.Len[ELL];
    memcpy(myData.data, rcvData.data, myData.Len[ELL]);
    myData.Len[SCSID] =  rcvData.Len[SCSID];
    myData.Len[2] =  rcvData.Len[2];
    myData.Len[3] =  rcvData.Len[3];
    memcpy(myData.SCS_ID, rcvData.SCS_ID, myData.Len[SCSID]);
    myData.statusindex = rcvData.statusindex;
    // local variable myData ready to use

    //  Verify the Decry[tion
    size_t S2siz = strlen(S2);
#ifdef DEEP_DEBUG
    printf("S2\n%s\n",S2);
#endif
    chkcmp1 = strcmp(S2ptr, myData.data);
    if( chkcmp1 == 0 ) {
        printf("Decryption S2\n%s\n",myData.data );
    } else {
        printf("PROBLEM!!!  Decryption failed\n");
    }
       // Copy to local BD buffer
     
    bool Sim2 = true;

    if ( Sim1 == true && Sim2 == true) {
        //printf("\n!!!!!!!!!!!SIMULATION SUCCESSFUL!!!!!!!!!!!!!!!\n");
    } else {
        printf("simulation NOT SUCCESSFUL!!!!! Loop #%03ld  \n", iter++);
        //getchar();
        goto restart;
    }
    end = clock();  // End timing
     // Calculate simulation time in seconds
    cpu_time_used = ((double)(end - start)) / CLOCKS_PER_SEC;
    //printf("%d simulation execution in %f seconds\n", iter, cpu_time_used);
    if (iter % 10000 == 0) {

        Danumptr = print_with_commas( iter );
        //printf("Resetting - Side 2 is now Side 1   Loop #%03d  PRESS ANY KEY", iter++);
        printf("\r%s", Danumptr); // to the monitor, no newline
        fflush(stdout);
        //getchar();

        FILE *file = fopen("timerunning.txt", "a"); // Open in append mode
        if (file == NULL) {
           perror("Error opening file");
           exit(-7);
        }

        time_t now = time(NULL);
        struct tm *t = localtime(&now);
        Danumptr = print_with_commas( iter );
        fprintf(file, "%s - %02d-%02d %02d:%02d:%02d\n",
            Danumptr, t->tm_mon + 1, t->tm_mday, t->tm_hour, t->tm_min, t->tm_sec);
       
        fclose(file); // Close the file after writing

    }
    //printf("%ld", iter++);
    fflush(stdout);

    if (iter == 15000000 ) {
        printf("\nReached the high level mark %ld !!!\n", iter);
        getchar();
    }
    iter++;

    end == 0;  start = 0; cpu_time_used = 0;
    printf("Press KEY to continue\n");
    getchar();

    goto restart;

	return 0;
}



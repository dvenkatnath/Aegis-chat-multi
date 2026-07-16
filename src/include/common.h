// common.h
#ifndef COMMON_H
#define COMMON_H

#include <stddef.h>

#define _CRT_SECURE_NO_WARNINGS

// size_t INDEX
#define ELL     0       // actual length of the encrypted text
#define SCSID   1   // actual length of the encrypted text for SCS_ID

// Buffer size definitions
#define STDSZ       65      // limiter for 64 byte TEXT strings
#define GENBUFSZ    65      // SM standard variable size
#define NOMINALSZ   128     // use for encrypted text STDSZ & BD CSCID string
#define BBUFSZ      953     // Max size of a text buffer for BD
#define eBBUFSZ     1240    // Size of Data string in BD
#define MaxPktSz    1420    // Maximum size of BD
#define BDTARGETSZ  1408    // 11x of 128

//Some compilers may pad the C data structures. Rather than rely compiler 
//options when passing the below BinaryData structure over sockets, 
//compilers which often are  be different on different operating systems,
//the Binary Data structure is EXACTLY the size of 1408 (11 x 128 bytes).
// A multiple of 128 bytes allows consistent communications between  processors of 64 bit archtectures and processoes of
// ARM - Advanced Reduced instruction set Machine
typedef struct {
    unsigned char data[eBBUFSZ];	// Traditional object for encrypted text limit text to  BBUFSZ
    unsigned char SCS_ID[NOMINALSZ];// Encrypted SCS ID
    size_t Len[4];                  // 4 element size_t array
    char statusindex;               // status message index
} BinaryData;

// char array for status messages
#define ACK_ret     1   // return from GenAnACK
#define NAK_ret     2   // return from NACK on side 2
#define SCS_setup   3   // SCS setup complete
#define DEC         4   // Decryption message
#define MSGsDONE    5   // NULL data return from msgtype()
#define MSGsNULL     6   // SCS not setup
#define SCS_Not   7   // HOLDING item
#define BUUFERNOT   8   // Msgtype default execution
#define MSGsERROR 9   // Received Encrpypted text
#define S_RcvEncTxt   10

// leading to memory corruption.
// Solution: Explicitly set memory alignment on a 128 byte boundry

// Function declarations (prototypes)


#endif // COMMON_H

#include <stdio.h>
#include <stdbool.h>
#include <string.h>
#include "EncryptionToolsApi.h"
#include "Pstr.h"
#include "Global2.h"
#include "Functions3.h"
#include "UberHighLevED.h"


// void InitforED()
// Setups up the library for AES & MUST BE called before any ED actions
// Returns esize - size of encrypted buffer
int InitforED() {
#if _DEBUG
	//printf("In InitforED\n");
#endif
	// initialize the library
	encryptionToolsApi_init();
//	Encrptr = '\0';			// Clear the CT presistence storage for the last Encrpytion buffer
//	Decrptr = '\0';			// Clear the CT presistence storage for the last Decryption buffer

	register int i;
	char c;

	char password[(2 * MIN_PASSWORD_LEN) + 1];
	char* passwordptr = password;
	password[2 * MIN_PASSWORD_LEN] = '\0';

	// Determine where to get the CyberKnot private key is for the side we are on
	if(IsEncrypted == true && Sender_Side == true) {	// we are on ACK side - Side 1
		passwordptr = CreateFinalPK(T_DaKey_ptr);
#ifdef _DEBUG
		//printf("On SIDE 1*****Generate PK for Side 1\n");  //Uncomment to verify where the PK wsas sourced from
#endif
	} else if (S2_IsEncrypted == true && Rcvr_Side == true) {	// we are on the NACK side - Side 2
#ifdef _DEBUG
		//printf("On SIDE 2*****Generate PK for Side 2\n");  //Uncomment to verify where the PK wsas sourced from
#endif
		passwordptr = CreateFinalPK(T2_DaKey_ptr);
	} else {				// Use the default Password
		// generate the defailt password to be used
		for (i = 0; i != 2 * MIN_PASSWORD_LEN; i++) {
			c = (i % 95) + 32;
			password[i] = c;
		}
#ifdef _DEBUG
		//printf("Using DEFAULT PK\n");
#endif
	}
	// set the password in the library
#ifdef _DEBUG
	//printf("Password is: %s   *****************\n", passwordptr);
#endif
	int esize;	// size of encryoted buffer
	esize = encryptionToolsApi_setPswd(passwordptr);
/*
	// clear the password
	for (i=0; i<64; i++) {						//1
		password[i] = '\0';
	}
	for (i=0; i<64; i++) {						// 2
		password[i] = '1';
	}
*/
	/*
	passwordptr = SHAPassedDeux(passwordptr);	// 3	
	passwordptr = SHAPassedDeux(passwordptr);	// 4
	passwordptr = SHAPassedDeux(passwordptr);	// 5
	passwordptr = SHAPassedDeux(passwordptr);	// 6
	for (i=0; i<64; i++) {						// 7
		password[i] = '0';
	}
	password[0] = '\0';							// terminate the buffer value
	*/
}

// Must be called after all ED actions
void DeInitforED() {
	encryptionToolsApi_destructor();
}


int EncryptIt(char* S1, byte* bbuf, int bbufLen) {
	/* bbufLen = plaintext byte length; output buffer capacity is BBUFSZ */
	int plainLen;
	int encrbufsize;
	int plen;
	int check;
	int totalLen;

	if (S1 == NULL || bbuf == NULL || bbufLen <= 0) {
		return -1;
	}
	plainLen = (int)strlen(S1);
	if (plainLen > bbufLen) {
		return -1;
	}

	encrbufsize = encryptionToolsApi_encrypt(S1, bbuf, BBUFSZ);
	if (encrbufsize < 0) {
		return -1;
	}

	plen = encrbufsize;

	char Pref[5];
	char* pre = Pref;
	check = snprintf(pre, sizeof(Pref), "%04d", plen);
	if (check < 0 || check >= (int)sizeof(Pref)) {
		return -1;
	}

	char returndatabuf[BBUFSZ];
	char* rdb = returndatabuf;

	totalLen = plen + check;
	if (totalLen > BBUFSZ || totalLen > (int)sizeof(returndatabuf)) {
		return -1;
	}

	memset(returndatabuf, 0, sizeof(returndatabuf));
	rdb = returndatabuf;
	char* tstrdb = Pstrncat(rdb, pre, check);

	char* dataindex = tstrdb + check;
	memcpy(dataindex, bbuf, plen);

	memcpy(bbuf, rdb, totalLen);

#if PDEBUG
	printf("EncryptIt: Encryption retVal = %d   ZERO IS SUCCESS\n", retVal);
#endif
	return totalLen;
}

// int GetEncryptedLength(char* bbuf)
// Passeded - char* bbuf  pointer to the buffer containined the encryted buffer
// Returns - int the length of the encrypted buffer
/*
int GetEncryptedLength(char* str) {
	// Get the length of the encrypted buffer

	char PktLen[MaxPktSz];
	char* PktLenptr = PktLen;
	register int index = 0;
	char c;
	int k=0;

    if (str == NULL) {  // The string has encrypted content
       return(-1);
    }
	
	int j;
	for (j=0; (str[j] != '\0') && (j < MaxPktSz); j++) {
		k++;
	}
	int l = k;				// Calclated length of an encrypted string

	// Sinceit has content, get the 1st 4 characters without a loop
	PktLenptr[0] = str[0];
	PktLenptr[1] = str[1];
	PktLenptr[2] = str[2];
	PktLenptr[3] = str[3];
	PktLenptr[4] = '\0'; 
	int bbufLen = atoi(PktLen);		// actual length when encrypted

	//for (index=0; (str[index] != '\0') && (index < MaxPktSz); index++) {  // Iterate until the null terminator
	//	PktLenptr[index] = str[index];
    //}
	//PktLenptr[index] = '\0'; 

    return bbufLen;

	// OLD PktLenptr = Preturn_first_4_chars(bbuf);
	//int bbufLen = atoi(PktLenptr);
	//return bbufLen;
}
*/

// int DecryptIt(byte* bbuf, int bbufLen, char* rcvdPacket)
// Passeded -
//		byte* bbuf  pointer to the buffer contaiining the packet which is encrypted
//		int bbufLen - the length of that buffer
//		char* rcvdPacket - buffer for the received packet in clear text
// Returns - int retVal - 0 if successful, -1 if not

int DecryptIt(byte* bbuf, int bbufLen, char* rcvdPacket) {
	// Substitute the Prefix value for the actual length of the data in bbuf

	// Strip out the prefix leaving just the encrypted data buffer
	//byte* incomingptr = bbuf + 4;

	int i = encryptionToolsApi_getDecryptLen(bbuf, bbufLen);
	// = (char*)malloc(i);
	int retVal = encryptionToolsApi_decrypt(bbuf, bbufLen, rcvdPacket, i);  // if not 0, problem

	return retVal;
}
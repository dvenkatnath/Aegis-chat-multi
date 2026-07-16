#ifndef ENCRYPTION_TOOLS_API
#define ENCRYPTION_TOOLS_API

#include <stdlib.h>

typedef	unsigned char byte;


/*
 * This file defines the methods and contants available in the EncryptionTools Library.
 * This library is intended for use in the myKryptofon SIP Server (our adaptation of FreeSwitch).
 * This library is inter-operable with the EncryptionTools library used by the myKryptofon program,
 * and all of its brandings.
 *
 * Note: methods in this library will use malloc() and free() to obtain dynamically allocated memory.
 */


#define	MIN_PASSWORD_LEN		32		/* minimum length for a password, in characters */
#define SALT_LEN				8		/* length of a Salt array, in bytes */
#define	HMAC_LEN				20		/* length of an HMAC array, in bytes */


/*
 * initialize this library.  
 * This method MUST be called exactly one time, before any other methods in the library are called.
 */
void encryptionToolsApi_init();


/*
 * terminate the use of this library.  
 * This method MUST be called in order to free dynamically allocated memory retained by the library.
 * After calling this method, no other methods in this library can be called.
 */
void encryptionToolsApi_destructor();


/*
 * before any data can be encrypted or decrypted, this method must be called
 * to set the password to be used.  The password is a NULL ('\0') terminated array of 
 * ASCII characters (between 0x20 and 0x7E inclusive). 
 * The string must be at least MIN_PASSWORD_LEN characters, excluding the NULL.
 * Returns: -1 if the given password string is tno at least MIN_PASSWORD_LEN characters;
 *          else 0 is returned.
 */
int encryptionToolsApi_setPswd( const char* password );


/*
 * this method will return the minimum length required for the buffer passed to the
 * getEncryptedPswd() method.  This method would be called before calling getEncryptedPswd(),
 * a buffer of the returned length would be allocated, and that buffer would be 
 * passwd to the getEncryptedPswd() method.
 * Returns: -1 if a password has not been set; else the calculated length
 */
int encryptionToolsApi_getEncryptedPswdLen();


/*
 * this method will encrypt the password previously set with a call to the setPswd() method,
 * using a 'built-in' hardcoded password. The encrypted password along with its HMAC and a SALT
 * are Base-64 encoded and written to the given buffer upon successful return.
 * The returned string will typically be sent to myKryptofon clients via HTTP.
 *   buf    - a buffer containing the encrypted and Base-64 encoded password, upon return.
 *            The returned string will be null terminated.
 *   bufLen - the size of buf.
 * Returns: 0 upon success.
 *			-1 if 'bufLen' is not at least the required length, as returned by getEncryptedPswdLen()
 *			-2 if a password has not been set
 */
int encryptionToolsApi_getEncryptedPswd( char* buf, int bufLen );


/*
 * get the buffer size required to hold the result of encrypting the specified string
 * with the encrypt() method.
 */
int encryptionToolsApi_getEncryptLen( const char* src );


/*
 * encrypt the given string using the password previously specified with setPswd().
 * The given string along with its HMAC and a SALT are written
 * to the given buffer upon successful return.
 * The encrypted values written to the buffer are NOT Base-64 encoded.  They are binary.
 * The contents of the buffer can be sent to the myKryptofon client in a UDP packet.
 *   src    - the NULL terminated string to be encrypted.
 *   buf    - the buffer into which the encrypted string, HMAC and SALT are written
 *   bufLen - the size of buf.
 * Returns: 0 upon success.
 *          -1 if 'bufLen' is not at least the required length, as returned by getEncryptLen().
 *			-2 if a password has not been set
 */
int encryptionToolsApi_encrypt( const char* src, byte* buf, int bufLen );



/*
 * get the buffer size required to hold the result of decrypting the specified source data
 * with the decrypt() method.
 */
int encryptionToolsApi_getDecryptLen( const byte* src, int srcLen );


/*
 * decrypt the given binary src data, using the password previously specified with setPswd().
 * The src data is assumed to have been received in a UDP packet from a myKryptofon client,
 * and is assumed to contain an encrypted SIP message along with its HMAC and SALT.
 * The decrypted SIP message is written to the given buffer (buf) as a NULL terminated string.
 *   src    - the binary buffer containing an encrypted message along with its HMAC and SALT
 *            The message to be decrypted is assumed to start at byte 0 in this buffer.
 *   srcLen - the number of bytes in the src buffer containing the encrypted message, HMAC and SALT.
 *   buf    - the buffer into which the decrypted message is written, as a NULL terminated string.
 *   bufLen - the size of buf.
 * Returns: 0 upon success.
 *          -1 if 'bufLen' is not at least the required length, as returned by getDecryptLen().
 *          -2 if the decryption fails.
 *			-3 if a password has not been set
 */
int encryptionToolsApi_decrypt( const byte* src, int srcLen, char* buf, int bufLen );


#ifdef TESTCODE

// run internal tests
int eta_Test();

#endif // TESTCODE

#endif // ENCRYPTION_TOOLS_API

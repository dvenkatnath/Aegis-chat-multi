#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdarg.h>
#include <time.h>
#include "SHA256.h"
#include "Functions3.h"
#include "Global2.h"
#include "Pstr.h"

// Contains Private cross platform string functions


int Pstrcmp(const char *s1, const char *s2) {
    while (*s1 && (*s1 == *s2)) {
        s1++;
        s2++;
    }
    return *(unsigned char *)s1 - *(unsigned char *)s2;
}

////////////////////////// Pstrcpy //////////////////////////
// Copy a buffer from one to another, until the source has '\0'
char* Pstrcpy(char* destination, const char* source) {

    char* ptr = destination; // Save the start of destination pointer

    // Copy the source string to destination
    while (*source != '\0') {
        *ptr = *source;
        ptr++;
        source++;
    }

    *ptr = '\0'; // Add the null terminator at the end

#ifdef _DEBUG
	//printf("In Pstrcpy, Copy source:\n%s\nto Dest:\n%s\n", source, destination);
#endif
    return destination;
}

////////////////////////// Pstrlen //////////////////////////
long unsigned int Pstrlen(char *str) {
    long unsigned int length = 0;
    register int index = 0;

    if (str == NULL) {  // Prevents segmentation fault
        return(-1);
    }    

    while (str[index] != '\0') {  // Iterate until the null terminator
        index++;
    }
    length = index;
    return length;
}


////////////////////////// Pstrncpy //////////////////////////
// Copy a buffer from one to another, until the source has '\0' ot lrngth constrsaint
#include <stdio.h>

char* Pstrncpy(char* dest, const char* src, int n) {
    int i;

    // Copy characters from src to dest
    for (i = 0; i < n && src[i] != '\0'; i++) {
        dest[i] = src[i];
    }

    // Pad with null characters if src is shorter than n
    for (; i < n; i++) {
        dest[i] = '\0';
    }

    return dest;
}

////////////////////////// Preverse_string //////////////////////////
// Function to reverse a string
void Preverse_string(char* str) {
    int length = strlen(str);
    int start = 0;
    int end = length - 1;
    char temp;
#ifdef _DEBUG
	//printf("In Preverse_string BEG\n%s\n", str);
#endif

    // Swap characters from start to end
    while (start < end) {
        temp = str[start];
        str[start] = str[end];
        str[end] = temp;

        start++;
        end--;
    }
#ifdef _DEBUG
	//printf("In Preverse_string END\n%s\n", str);
#endif
}

////////////////////////// Pstrncat //////////////////////////
// Custom implementation of strncat
char* Pstrncat(char* dest, const char* src, int n) {
    char* dest_start = dest;

    // Move 'dest' pointer to the end of the current string
    int ctr = 0;
    while (*dest != '\0') {
        dest++;
        if (++ctr >= BBUFSZ) break;  // don't loop beyond the largest buffer size acceptable
    }
	//int ctr2 = ctr;  // check the loop

    // Append up to 'n' characters from 'src' to 'dest'
    while (n-- && *src != '\0') {
        *dest = *src;
        dest++;
        src++;
    }

    // Null-terminate the resulting string
    *dest = '\0';

    // Return the pointer to the beginning of the destination string
    return dest_start;
}

////////////////////////// Preturn_first_4_chars ////////////////////////// 
// Function to strip the first 4 characters and return them
char* Preturn_first_4_chars(char* str) {
    static char first_4[5];  // Buffer to hold the first 4 characters + null terminator
    long unsigned int len = strlen( str );

    // Manually copy the first 4 characters or less if the string is shorter
    size_t i;
    for (i = 0; i < 4 && i < len; i++) {
        first_4[i] = str[i];
    }
    first_4[i] = '\0';  // Null-terminate the copied string

    /*
    // Now strip the first 4 characters from the original string
    if (len > 4) {
        memmove(str, str + 4, len - 4 + 1);  // +1 for the null-terminator
    }
    else {
        str[0] = '\0';  // If the string has fewer than 4 characters, set it to an empty string
    }
    */
    return first_4;
}

////////////////////////// Preturn_next_3_chars //////////////////////////
// Function to strip the next 3 characters and return them
char* Preturn_next_3_chars(char* str) {
    static char next_3[4];  // Buffer to hold the 4 characters + null terminator
    size_t len = strlen(str);

    // Manually copy the first 3 characters or less if the string is shorter
    size_t i;
    for (i = 0; i < 3 && i < len; i++) {
        next_3[i] = str[i];
    }
    next_3[i] = '\0';  // Null-terminate the copied string
    return next_3;
}

////////////////////////// Preturn_next_10_chars //////////////////////////
// Function to strip the next 3 characters and return them
char* Preturn_next_10_chars(char* str) {
    static char next_10[11];  // Buffer to hold the 10 characters + null terminator
    size_t len = strlen(str);

    // Manually copy the first 10 characters or less if the string is shorter
    size_t i;
    for (i = 0; i < 10 && i < len; i++) {
        next_10[i] = str[i];
    }
    next_10[i] = '\0';  // Null-terminate the copied string
    return next_10;
}

///////////////////////////// Preturn_next_4_chars //////////////////////////
// Private sprintf code - Function to convert a single byte to a 2-digit hex string ////////////////
void byte_to_hex(unsigned char byte, char* hex) {
    const char hex_chars[] = "0123456789ABCDEF";

    hex[0] = hex_chars[(byte >> 4) & 0x0F]; // Get the upper 4 bits
    hex[1] = hex_chars[byte & 0x0F];        // Get the lower 4 bits
    hex[2] = '\0';                          // Null-terminate the hex representation
}

//////////////////////////////// hex_sprintf /////////////////////////////////////////
// Function to convert a string to its hex representation /////////////////////////////////////////
void hex_sprintf(char* destination, const unsigned char* source) {
    while (*source) {
        byte_to_hex(*source, destination);  // Convert each byte to a 2-character hex representation
        destination += 2;                   // Move to the next position in destination
        source++;
    }
    *destination = '\0'; // Null-terminate the destination string
}

////////////////////////// int_to_padded_str //////////////////////////
// Helper function to convert an integer to a zero-padded string
char* int_to_padded_str(int value, char* buffer, int min_width) {
    char temp[12];  // Temporary buffer for storing the number (enough for 32-bit int)
    int is_negative = 0;
    int index = 0;

    // Handle negative values
    if (value < 0) {
        is_negative = 1;
        value = -value;
    }

    // Convert integer to string in reverse order
    do {
        temp[index++] = (value % 10) + '0';
        value /= 10;
    } while (value > 0);

    // Add negative sign if needed
    if (is_negative) {
        temp[index++] = '-';
    }

    // Calculate the number of padding zeros required
    while (index < min_width) {
        temp[index++] = '0';
    }

    // Reverse the string into the output buffer
    int i;
    for (i = 0; i < index; i++) {
        buffer[i] = temp[index - 1 - i];
    }

    buffer[i] = '\0';  // Null-terminate the string
    return buffer;
}

///////////////////////////// Psprintf /////////////////////////////////////////
// Custom implementation of sprintf-like function
int Psprintf(char* buffer, const char* format, ...) {
    va_list args;   // Initialize the variable argument list
    va_start(args, format);

    char* buf_ptr = buffer;  // Pointer to traverse the output buffer
    const char* fmt_ptr = format;  // Pointer to traverse the format string

    while (*fmt_ptr != '\0') {
        if (*fmt_ptr == '%') {
            fmt_ptr++;  // Move past the '%'

            // Check for zero-padded format specifier like "%04d"
            if (*fmt_ptr == '0') {
                fmt_ptr++;  // Skip the '0'

                // Check for the width, assuming one digit for now (e.g., '4')
                int width = 0;
                while (*fmt_ptr >= '0' && *fmt_ptr <= '9') {
                    width = width * 10 + (*fmt_ptr - '0');
                    fmt_ptr++;
                }

                if (*fmt_ptr == 'd') {  // Handle the integer format with zero-padding
                    int value = va_arg(args, int);
                    char num_buffer[20];  // Buffer to hold the integer as a string with padding
                    int_to_padded_str(value, num_buffer, width);  // Convert the integer to a padded string
                    char* num_ptr = num_buffer;
                    while (*num_ptr) {
                        *buf_ptr++ = *num_ptr++;
                    }
                    fmt_ptr++;  // Move to the next character after 'd'
                    continue;  // Skip to the next format character
                }
            }

            // Handle other supported cases
            switch (*fmt_ptr) {
            case 'd': {  // Handle integer format without padding
                int value = va_arg(args, int);
                char num_buffer[12];  // Buffer to hold the integer as a string
                int_to_padded_str(value, num_buffer, 0);  // Convert the integer to a string without padding
                char* num_ptr = num_buffer;
                while (*num_ptr) {
                    *buf_ptr++ = *num_ptr++;
                }
                break;
            }
            case 'c': {  // Handle character format
                char value = (char)va_arg(args, int);  // char is promoted to int in va_arg
                *buf_ptr++ = value;
                break;
            }
            case 's': {  // Handle string format
                char* value = va_arg(args, char*);
                while (*value) {
                    *buf_ptr++ = *value++;
                }
                break;
            }
            default: {
                *buf_ptr++ = '%';  // Handle unknown format specifier by outputting '%'
                *buf_ptr++ = *fmt_ptr;
                break;
            }
            }
        }
        else {
            *buf_ptr++ = *fmt_ptr;  // Copy regular characters
        }
        fmt_ptr++;
    }

    *buf_ptr = '\0';  // Null-terminate the resulting string

    va_end(args);  // Clean up the variable argument list

    return buf_ptr - buffer;  // Return the length of the formatted string
}


#include <ctype.h>
////////////////////////// isascii_char //////////////////////////
int isascii_char(char ch) {

    if (isalnum((unsigned char)ch)) {
        return 0;
    }
    else {
		return - 1;
    }
}


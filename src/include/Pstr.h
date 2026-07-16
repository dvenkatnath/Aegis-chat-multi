#pragma once
#ifndef PSTR_H
#define PSTR_H

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "Pstr.h"

// Function prototypes
long unsigned int Pstrlen(char *str);
int Pstrcmp(const char *s1, const char *s2);
char* Pstrncpy(char* dest, const char* src, int n);
char* Pstrcpy(char* destination, const char* source);
void Preverse_string(char* str);
char* Pstrncat(char* dest, const char* src, int n);
char* Preturn_first_4_chars(char* str);
char* Preturn_next_3_chars(char* str);
char* Preturn_next_10_chars(char* str);
void byte_to_hex(unsigned char byte, char* hex);
void hex_sprintf(char* destination, const unsigned char* source);
int Psprintf(char* buffer, const char* format, ...);
int isascii_char(char);

#endif // PSTR_H

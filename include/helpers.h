#ifndef HELPERS_H
#define HELPERS_H

#include <stddef.h>

size_t strlen(const char* str);
int strcmp(const char* str1, const char* str2);
void* memcpy(void* dest, const void* src, size_t n);
void* memset(void* s, int c, size_t n);
char *strcpy(char* dest, const char* src);
int strncmp(const char* str1, const char* str2, size_t n);
char *strncpy(char* dest, const char* src, size_t n);
unsigned long kstrtoul(const char* s, int base);

#endif

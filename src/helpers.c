#include "../include/kernel.h"

size_t strlen(const char* str)
{
    size_t len = 0;
    while (str[len])
    {
        len++;
    }
    return len;
}

char* strcpy(char* dest, const char* src)
{
    char* original_dest = dest;
    while (*src)
    {
        *dest++ = *src++;
    }
    *dest = '\0'; 
    return original_dest;
}

int strcmp(const char* str1, const char* str2)
{
	while (*str1 && (*str1 == *str2))
    {
		str1++;
		str2++;
	}
	return *(const unsigned char*)str1 - *(const unsigned char*)str2;
}

void* memcpy(void* dest, const void* src, size_t n)
{
    unsigned char* d = (unsigned char*)dest;
    const unsigned char* s = (const unsigned char*)src;
    
    for (size_t i = 0; i < n; i++)
    {
        d[i] = s[i];
    }
    return dest;
}

void* memset(void* s, int c, size_t n)
{
    unsigned char* p = (unsigned char*)s;
    
    for (size_t i = 0; i < n; i++)
    {
        p[i] = (unsigned char)c;
    }
    return s;
}


int strncmp(const char* str1, const char* str2, size_t n)
{
    while (n && *str1 && (*str1 == *str2))
    {
        str1++;
        str2++;
        n--;
    }
    
    if (n == 0) return 0;
    return *(const unsigned char*)str1 - *(const unsigned char*)str2;
}

unsigned long kstrtoul(const char* s, int base)
{
    unsigned long value = 0;

    while (*s == ' ' || *s == '\t')
        s++;

    if (base == 0)
    {
        if (s[0] == '0' && (s[1] == 'x' || s[1] == 'X'))
        {
            base = 16;
            s += 2;
        }
        else
        {
            base = 10;
        }
    }
    else if (base == 16 && s[0] == '0' && (s[1] == 'x' || s[1] == 'X'))
    {
        s += 2;
    }

    while (*s)
    {
        int digit;

        if (*s >= '0' && *s <= '9')
            digit = *s - '0';
        else if (*s >= 'a' && *s <= 'f')
            digit = *s - 'a' + 10;
        else if (*s >= 'A' && *s <= 'F')
            digit = *s - 'A' + 10;
        else
            break;

        if (digit >= base)
            break;

        value = value * base + digit;
        s++;
    }

    return value;
}

char* strncpy(char* dest, const char* src, size_t n)
{
    char* original_dest = dest;
    
    while (n-- && *src)
    {
        *dest++ = *src++;
    }
    while (n-- > 0)
    {
        *dest++ = '\0';
    }
    
    return original_dest;
}
#ifndef KPRINTF_H
#define KPRINTF_H

#include <stddef.h>
#include <stdint.h>
#include <stdbool.h>
#include <stdarg.h>

void kputchar(char c);
void kputs(const char* str);
void reverse_string(char* str, int length);
int int_to_string(int num, char* str, int base);
int uint_to_string(unsigned int num, char* str, int base);
int kvprintf(void (*putc)(char), const char* fmt, va_list ap);
int kprintf_screen(int screen_id, const char* fmt, ...);
int kprintf_color(uint8_t color, const char* fmt, ...);
int kprintf(const char* fmt, ...);

#endif

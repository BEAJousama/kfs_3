#ifndef DEFS_H
#define DEFS_H

#include <stdint.h>
#include <stddef.h>
#include <stdbool.h>

// VGA constants
#define VGA_MEMORY 0xB8000
#define VGA_WIDTH 80
#define VGA_HEIGHT 25
#define MAX_SCREENS 4

// Scancodes
#define SC_F1 0x3B
#define SC_F2 0x3C
#define SC_F3 0x3D
#define SC_F4 0x3E
#define SC_CTRL 0x1D
#define SC_ALT 0x38
#define SC_SHIFT_L 0x2A
#define SC_SHIFT_R 0x36
#define SC_UP_ARROW 0x48
#define SC_DOWN_ARROW 0x50
#define SC_LEFT_ARROW 0x4B
#define SC_RIGHT_ARROW 0x4D
#define SC_ESC 0x01

// VGA colors
enum vga_color {
    VGA_COLOR_BLACK = 0,
    VGA_COLOR_BLUE = 1,
    VGA_COLOR_GREEN = 2,
    VGA_COLOR_CYAN = 3,
    VGA_COLOR_RED = 4,
    VGA_COLOR_MAGENTA = 5,
    VGA_COLOR_BROWN = 6,
    VGA_COLOR_LIGHT_GREY = 7,
    VGA_COLOR_DARK_GREY = 8,
    VGA_COLOR_LIGHT_BLUE = 9,
    VGA_COLOR_LIGHT_GREEN = 10,
    VGA_COLOR_LIGHT_CYAN = 11,
    VGA_COLOR_LIGHT_RED = 12,
    VGA_COLOR_LIGHT_MAGENTA = 13,
    VGA_COLOR_LIGHT_BROWN = 14,
    VGA_COLOR_WHITE = 15,
};

// CPU register state pushed by ISRs
struct regs {
    unsigned int gs, fs, es, ds;
    unsigned int edi, esi, ebp, esp, ebx, edx, ecx, eax;
    unsigned int int_no, err_code;
    unsigned int eip, cs, eflags, useresp, ss;
};

#endif

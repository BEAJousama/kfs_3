#ifndef TERMINAL_H
#define TERMINAL_H

#include <stddef.h>
#include <stdint.h>
#include "defs.h"

uint8_t vga_color_entry(enum vga_color fg, enum vga_color bg);
void terminal_set_color(uint8_t color);
uint8_t terminal_get_color(void);
void terminal_reset_color(void);

void update_cursor(size_t row, size_t col);
void scroll_screen(int screen_id);
void refresh_display(void);
void terminal_init_multiscreen(int num_screens);
int terminal_switch_screen(int screen_id);
int terminal_get_current_screen(void);
void terminal_putchar(char c);
void terminal_putchar_screen(int screen_id, char c);
void init_terminal(void);
void terminal_clear(void);
void terminal_clear_screen(int screen_id);
void terminal_scroll_up(void);
void terminal_scroll_down(void);


#endif

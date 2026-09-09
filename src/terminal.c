#include "../include/kernel.h"

#define SCROLL_BUFFER_SIZE (VGA_HEIGHT * 250)

typedef struct {
    size_t row;
    size_t column;
    uint8_t color;
    uint8_t default_color;
    uint16_t buffer[SCROLL_BUFFER_SIZE * VGA_WIDTH];
    int scroll_offset;
    size_t total_lines;
} screen_t;

static screen_t screens[MAX_SCREENS];
static int current_screen = 0;
static int num_screens = 1;
static uint16_t* vga_buffer = (uint16_t*)VGA_MEMORY;

static inline uint8_t vga_entry_color(enum vga_color fg, enum vga_color bg)
{
	return fg | bg << 4;
}

uint8_t vga_color_entry(enum vga_color fg, enum vga_color bg)
{
	return vga_entry_color(fg, bg);
}

void terminal_set_color(uint8_t color)
{
	screens[current_screen].color = color;
}

uint8_t terminal_get_color(void)
{
	return screens[current_screen].color;
}

void terminal_reset_color(void)
{
	screens[current_screen].color = screens[current_screen].default_color;
}

static inline uint16_t vga_entry(unsigned char uc, uint8_t color) 
{
	return (uint16_t) uc | (uint16_t) color << 8;
}

void update_cursor(size_t row, size_t col)
{
    uint16_t pos = row * VGA_WIDTH + col;
    
    outb(0x3D4, 0x0E);
    outb(0x3D5, (uint8_t) (pos >> 8));
    outb(0x3D4, 0x0F);
    outb(0x3D5, (uint8_t) (pos & 0xFF));
}

void refresh_display(void)
{
    screen_t* screen = &screens[current_screen];
    
    int display_start_line;
    if (screen->total_lines <= VGA_HEIGHT) {
        display_start_line = 0;
    } else {
        display_start_line = screen->total_lines - VGA_HEIGHT - screen->scroll_offset;
        if (display_start_line < 0) display_start_line = 0;
        if (display_start_line > SCROLL_BUFFER_SIZE - VGA_HEIGHT) {
            display_start_line = SCROLL_BUFFER_SIZE - VGA_HEIGHT;
        }
    }
    
    for (size_t i = 0; i < VGA_WIDTH * VGA_HEIGHT; i++)
    {
        size_t buffer_index = display_start_line * VGA_WIDTH + i;
        if (buffer_index < SCROLL_BUFFER_SIZE * VGA_WIDTH) {
            vga_buffer[i] = screen->buffer[buffer_index];
        } else {
            vga_buffer[i] = vga_entry(' ', screen->color);
        }
    }
    
    size_t cursor_row, cursor_col;
    if (screen->scroll_offset == 0) {
        cursor_row = screen->row - display_start_line;
        cursor_col = screen->column;
        if (cursor_row >= VGA_HEIGHT) {
            cursor_row = VGA_HEIGHT - 1;
        }
    } else {
        cursor_row = VGA_HEIGHT;
        cursor_col = 0;
    }
    
    update_cursor(cursor_row, cursor_col);
}

void terminal_scroll_up(void)
{
    screen_t* screen = &screens[current_screen];
    
    if (screen->total_lines > VGA_HEIGHT) {
        int max_scroll = screen->total_lines - VGA_HEIGHT;
        if (max_scroll > SCROLL_BUFFER_SIZE - VGA_HEIGHT) {
            max_scroll = SCROLL_BUFFER_SIZE - VGA_HEIGHT;
        }
        
        if (screen->scroll_offset < max_scroll) {
            screen->scroll_offset++;
            refresh_display();
        }
    }
}

void terminal_scroll_down(void)
{
    screen_t* screen = &screens[current_screen];
    
    if (screen->scroll_offset > 0) {
        screen->scroll_offset--;
        refresh_display();
    }
}

void terminal_init_multiscreen(int requested_screens)
{
    if (requested_screens > MAX_SCREENS) requested_screens = MAX_SCREENS;
    num_screens = requested_screens;
    
    for (int s = 0; s < num_screens; s++)
    {
        screens[s].row = 0;
        screens[s].column = 0;
        screens[s].scroll_offset = 0;
        screens[s].total_lines = 0;

        switch (s)
        {
            case 0:
                screens[s].color = vga_entry_color(VGA_COLOR_LIGHT_GREY, VGA_COLOR_BLACK);
                break;
            case 1:
                screens[s].color = vga_entry_color(VGA_COLOR_GREEN, VGA_COLOR_BLACK);
                break;
            case 2:
                screens[s].color = vga_entry_color(VGA_COLOR_CYAN, VGA_COLOR_BLACK);
                break;
            case 3:
                screens[s].color = vga_entry_color(VGA_COLOR_LIGHT_RED, VGA_COLOR_BLACK);
                break;
            default:
                screens[s].color = vga_entry_color(VGA_COLOR_WHITE, VGA_COLOR_BLACK);
                break;
        }
        screens[s].default_color = screens[s].color;
        for (size_t i = 0; i < SCROLL_BUFFER_SIZE * VGA_WIDTH; i++)
        {
            screens[s].buffer[i] = vga_entry(' ', screens[s].color);
        }
    }
    
    current_screen = 0;
    refresh_display();
}

int terminal_switch_screen(int screen_id)
{
    if (screen_id < 0 || screen_id >= num_screens) return -1;
    
    current_screen = screen_id;
    refresh_display();
    return 0;
}

int terminal_get_current_screen(void)
{
    return current_screen;
}

void terminal_putchar(char c)
{
    terminal_putchar_screen(current_screen, c);
}

void terminal_putchar_screen(int screen_id, char c)
{
    if (screen_id < 0 || screen_id >= num_screens) return;
    
    screen_t* screen = &screens[screen_id];
    
    if (c == '\n') 
    {
        screen->column = 0;
        screen->row++;
    } 
    else if (c == '\b')
    {
        if (screen->column > 0)
        {
            screen->column--;
        }
        else if (screen->row > 0)
        {
            screen->row--;
            screen->column = VGA_WIDTH - 1;
            while (screen->column > 0)
            {
                size_t index = screen->row * VGA_WIDTH + screen->column;
                if ((screen->buffer[index] & 0xFF) != ' ')
                {
                    screen->column++;
                    break;
                }
                screen->column--;
            }
        }
        size_t index = screen->row * VGA_WIDTH + screen->column;
        screen->buffer[index] = vga_entry(' ', screen->color);
    }
    else if (c == '\t')
    {
        do {
            size_t index = screen->row * VGA_WIDTH + screen->column;
            screen->buffer[index] = vga_entry(' ', screen->color);
            screen->column++;
            if (screen->column >= VGA_WIDTH) {
                screen->column = 0;
                screen->row++;
                break;
            }
        } while (screen->column % 4 != 0);
    }
    else if (c == '\r')
    {
        screen->column = 0;
    }
    else if (c >= 32 && c <= 126)
    {
        size_t index = screen->row * VGA_WIDTH + screen->column;
        screen->buffer[index] = vga_entry(c, screen->color);
        screen->column++;
        
        if (screen->column >= VGA_WIDTH) {
            screen->column = 0;
            screen->row++;
        }
    }
    
   if (screen->row >= screen->total_lines)
    {
        screen->total_lines = screen->row + 1;
    }
    
    if (screen_id == current_screen) {
        screen->scroll_offset = 0;
        refresh_display();
    }
}

void terminal_clear(void) {
    terminal_clear_screen(current_screen);
}

void terminal_clear_screen(int screen_id) {
    if (screen_id < 0 || screen_id >= num_screens) return;
    
    screen_t* screen = &screens[screen_id];
    screen->row = 0;
    screen->column = 0;
    screen->scroll_offset = 0;
    screen->total_lines = 0;
    
    for (size_t i = 0; i < SCROLL_BUFFER_SIZE * VGA_WIDTH; i++) {
        screen->buffer[i] = vga_entry(' ', screen->color);
    }
    
    if (screen_id == current_screen) {
        refresh_display();
    }
}

void init_terminal(void)
{
    terminal_init_multiscreen(4);
    kprintf_screen(0, "Screen 0: Hello World %d!!!!\n", 42);
    kprintf_screen(1, "Screen 1: Hello World %d!!!!\n", 42);
    kprintf_screen(2, "Screen 2: Hello World %d!!!!\n", 42);
    kprintf_screen(3, "Screen 3: Hello World %d!!!!\n", 42);
    for (int i = 0; i < 4; i++)
    {
        kprintf_screen(i, "Screen %d: Use ALT + { -> , <- } to switch screens\n", i);
        kprintf_screen(i, "Screen %d: Use UP/DOWN arrows to scroll\n", i);
        kprintf_screen(i, "Screen %d: Type to test keyboard input\n", i);
    }
    terminal_switch_screen(0);
}
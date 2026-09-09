#include "../include/kernel.h"

static unsigned char kbdus[128] = {
    0,  27, '1', '2', '3', '4', '5', '6', '7', '8', '9', '0', '-', '=', '\b',
    '\t', 'q', 'w', 'e', 'r', 't', 'y', 'u', 'i', 'o', 'p', '[', ']', '\n',
    0, 'a', 's', 'd', 'f', 'g', 'h', 'j', 'k', 'l', ';', '\'', '`', 0, '\\',
    'z', 'x', 'c', 'v', 'b', 'n', 'm', ',', '.', '/', 0, '*', 0, ' ', 0, 0,
    0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, '-', 0, 0, 0, '+', 0, 0, 0, 0,
    0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
    0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0
};

static unsigned char kbdus_shift[128] = {
    0,  27, '!', '@', '#', '$', '%', '^', '&', '*', '(', ')', '_', '+', '\b',
    '\t', 'Q', 'W', 'E', 'R', 'T', 'Y', 'U', 'I', 'O', 'P', '{', '}', '\n',
    0, 'A', 'S', 'D', 'F', 'G', 'H', 'J', 'K', 'L', ':', '"', '~', 0, '|',
    'Z', 'X', 'C', 'V', 'B', 'N', 'M', '<', '>', '?', 0, '*', 0, ' ', 0,
    0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, '-', 0, 0, 0, '+', 0, 0,
    0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
    0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0
};

static bool alt_pressed = false;
static bool shift_pressed = false;
static bool extended_scancode = false;

static void clear_keyboard_buffer(void) 
{
    while (inb(0x64) & 0x01)
    {
        inb(0x60);
    }
}

static void handle_screen_switch_arrows(unsigned char scancode)
{
    int current = terminal_get_current_screen();
    int new_screen = current;
    
    switch (scancode)
    {
        case SC_LEFT_ARROW:
            new_screen = (current - 1 + MAX_SCREENS) % MAX_SCREENS;
            break;
        case SC_RIGHT_ARROW:
            new_screen = (current + 1) % MAX_SCREENS;
            break;
        case SC_UP_ARROW:
            new_screen = 0;
            break;
        case SC_DOWN_ARROW:
            new_screen = MAX_SCREENS - 1;
            break;
        default:
            return;
    }
    terminal_switch_screen(new_screen);
}


void keyboard_handler(struct regs *r) {
    (void)r;
    
    unsigned char scancode = inb(0x60);

    if (scancode == 0xE0) {
        extended_scancode = true;
        return;
    }

    if (scancode & 0x80) {
        scancode &= 0x7F;

        if (extended_scancode) {
            extended_scancode = false;
            return;
        }

        if (scancode == SC_ALT) alt_pressed = false;
        if (scancode == SC_SHIFT_L || scancode == SC_SHIFT_R) shift_pressed = false;
        return;
    }

    if (extended_scancode) {
        extended_scancode = false;

        if (alt_pressed) {
            switch (scancode) {
                case SC_LEFT_ARROW:
                case SC_RIGHT_ARROW:
                case SC_UP_ARROW:
                case SC_DOWN_ARROW:
                    handle_screen_switch_arrows(scancode);
                    return;
            }
        } else {
            switch (scancode) {
                case SC_UP_ARROW:
                    terminal_scroll_up();
                    return;
                case SC_DOWN_ARROW:
                    terminal_scroll_down();
                    return;
            }
        }

        return;
    }

    if (scancode == SC_ALT) {
        alt_pressed = true;
        return;
    }
    if (scancode == SC_SHIFT_L || scancode == SC_SHIFT_R) {
        shift_pressed = true;
        return;
    }
    if (scancode == SC_ESC) {
        shell_toggle();
        return;
    }

    if (scancode < 128) {
        char c = shift_pressed ? kbdus_shift[scancode] : kbdus[scancode];

        if (c != 0) {
            if (is_shell_active()) {
                shell_handle_key(c);
            } else {
                terminal_putchar(c);
            }
        }
    }
}



void keyboard_install()
{
    clear_keyboard_buffer();
    irq_install_handler(1, keyboard_handler);
}
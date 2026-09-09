#include "../include/kernel.h"

static int default_print_screen = -1;

void kputchar(char c)
{
    if (default_print_screen >= 0)
    {
        terminal_putchar_screen(default_print_screen, c);
    } 
    else
    {
        terminal_putchar(c);
    }
}

void kputs(const char* str)
{
    for (size_t i = 0; str[i]; i++)
    {
        kputchar(str[i]);
    }
}

void reverse_string(char* str, int length)
{
    int start = 0;
    int end = length - 1;
    while (start < end)
    {
        char temp = str[start];
        str[start] = str[end];
        str[end] = temp;
        start++;
        end--;
    }
}

int int_to_string(int num, char* str, int base)
{
    int i = 0;
    bool is_negative = false;
    
    if (num == 0)
    {
        str[i++] = '0';
        str[i] = '\0';
        return i;
    }
    if (num < 0 && base == 10)
    {
        is_negative = true;
        num = -num;
    }
    while (num != 0)
    {
        int rem = num % base;
        if (rem > 9)
        {
            str[i++] = (rem - 10) + 'a';
        }
        else
        {
            str[i++] = rem + '0';
        }
        num = num / base;
    }    
    if (is_negative)
    {
        str[i++] = '-';
    }
    
    str[i] = '\0';
    reverse_string(str, i);
    return i;
}

int uint_to_string(unsigned int num, char* str, int base)
{
    int i = 0;
    
    if (num == 0)
    {
        str[i++] = '0';
        str[i] = '\0';
        return i;
    }
    while (num != 0)
    {
        int rem = num % base;
        if (rem > 9)
        {
            str[i++] = (rem - 10) + 'a';
        }
        else
        {
            str[i++] = rem + '0';
        }
        num = num / base;
    }
    
    str[i] = '\0';
    reverse_string(str, i);
    return i;
}

/* Emit `str` (length `len`) padded to `width`, honouring left/zero padding. */
static int emit_padded(void (*putc)(char), const char* str, int len,
                       int width, bool zero_pad, bool left_align)
{
    int count = 0;
    int pad = width - len;

    if (!left_align)
    {
        for (int i = 0; i < pad; i++, count++)
            putc(zero_pad ? '0' : ' ');
    }
    for (int i = 0; i < len; i++, count++)
        putc(str[i]);
    if (left_align)
    {
        for (int i = 0; i < pad; i++, count++)
            putc(' ');
    }
    return count;
}

int kvprintf(void (*putc)(char), const char* fmt, va_list ap)
{
    int count = 0;
    char buf[36];

    for (const char* p = fmt; *p; p++)
    {
        if (*p != '%')
        {
            putc(*p);
            count++;
            continue;
        }

        p++;
        bool left_align = false;
        bool zero_pad = false;
        int width = 0;

        while (*p == '-' || *p == '0')
        {
            if (*p == '-') left_align = true;
            else           zero_pad = true;
            p++;
        }
        while (*p >= '0' && *p <= '9')
        {
            width = width * 10 + (*p - '0');
            p++;
        }

        char spec = *p;
        switch (spec)
        {
            case 'd': {
                int len = int_to_string(va_arg(ap, int), buf, 10);
                count += emit_padded(putc, buf, len, width, zero_pad, left_align);
                break;
            }
            case 'u': {
                int len = uint_to_string(va_arg(ap, unsigned int), buf, 10);
                count += emit_padded(putc, buf, len, width, zero_pad, left_align);
                break;
            }
            case 'x': {
                int len = uint_to_string(va_arg(ap, unsigned int), buf, 16);
                count += emit_padded(putc, buf, len, width, zero_pad, left_align);
                break;
            }
            case 'X': {
                int len = uint_to_string(va_arg(ap, unsigned int), buf, 16);
                for (int i = 0; i < len; i++)
                {
                    if (buf[i] >= 'a' && buf[i] <= 'f')
                        buf[i] = buf[i] - 'a' + 'A';
                }
                count += emit_padded(putc, buf, len, width, zero_pad, left_align);
                break;
            }
            case 'p': {
                int len = uint_to_string(va_arg(ap, unsigned int), buf, 16);
                char pbuf[11];
                pbuf[0] = '0';
                pbuf[1] = 'x';
                for (int i = 0; i < 8; i++)
                    pbuf[2 + i] = (i < 8 - len) ? '0' : buf[i - (8 - len)];
                count += emit_padded(putc, pbuf, 10, width, false, left_align);
                break;
            }
            case 's': {
                const char* s = va_arg(ap, const char*);
                if (!s) s = "(null)";
                int len = 0;
                while (s[len]) len++;
                count += emit_padded(putc, s, len, width, false, left_align);
                break;
            }
            case 'c': {
                char c = (char)va_arg(ap, int);
                putc(c);
                count++;
                break;
            }
            case '%': {
                putc('%');
                count++;
                break;
            }
            default:
                putc('%');
                putc(spec);
                count += 2;
                break;
        }
    }
    return count;
}

int kprintf(const char* fmt, ...) {
    va_list ap;
    va_start(ap, fmt);
    int written = kvprintf(kputchar, fmt, ap);
    va_end(ap);
    return written;
}

int kprintf_color(uint8_t color, const char* fmt, ...)
{
    uint8_t old = terminal_get_color();
    va_list ap;

    terminal_set_color(color);
    va_start(ap, fmt);
    int written = kvprintf(kputchar, fmt, ap);
    va_end(ap);
    terminal_set_color(old);
    return written;
}

int kprintf_screen(int screen_id, const char* fmt, ...)
{
    va_list ap;
    int old_screen;
    int result;

    va_start(ap, fmt);

    old_screen = default_print_screen;
    default_print_screen = screen_id;
    result = kvprintf(kputchar, fmt, ap);
    default_print_screen = old_screen;
    
    va_end(ap);
    return result;
}
#include "../include/kernel.h"
#include "../include/panic.h"

extern const char *exception_messages[];

#define PANIC_COLOR   vga_color_entry(VGA_COLOR_WHITE, VGA_COLOR_RED)
#define WARN_COLOR    vga_color_entry(VGA_COLOR_BLACK, VGA_COLOR_LIGHT_BROWN)

static __attribute__((noreturn)) void halt_forever(void)
{
    __asm__ volatile ("cli");
    for (;;)
        __asm__ volatile ("hlt");
}

void panic(const char *fmt, ...)
{
    va_list ap;

    terminal_set_color(PANIC_COLOR);
    kprintf("\n");
    kprintf("  *** KERNEL PANIC ***\n  ");

    va_start(ap, fmt);
    kvprintf(kputchar, fmt, ap);
    va_end(ap);

    kprintf("\n  System halted. Reboot the machine.\n");
    terminal_reset_color();
    halt_forever();
}

void panic_nonfatal(const char *fmt, ...)
{
    va_list ap;
    uint8_t old = terminal_get_color();

    terminal_set_color(WARN_COLOR);
    kprintf("\n [warn] ");
    va_start(ap, fmt);
    kvprintf(kputchar, fmt, ap);
    va_end(ap);
    kprintf("\n");
    terminal_set_color(old);
}

void panic_from_regs(struct regs *r)
{
    const char *name = (r->int_no < 32)
        ? exception_messages[r->int_no]
        : "Unknown Interrupt";

    terminal_set_color(PANIC_COLOR);
    kprintf("\n  *** KERNEL PANIC: CPU EXCEPTION ***\n");
    kprintf("  #%d  %s   err_code=0x%08X\n\n", r->int_no, name, r->err_code);

    kprintf("  eax=0x%08X ebx=0x%08X ecx=0x%08X edx=0x%08X\n",
            r->eax, r->ebx, r->ecx, r->edx);
    kprintf("  esi=0x%08X edi=0x%08X ebp=0x%08X esp=0x%08X\n",
            r->esi, r->edi, r->ebp, r->esp);
    kprintf("  eip=0x%08X eflags=0x%08X\n", r->eip, r->eflags);
    kprintf("  cs=0x%04X ds=0x%04X es=0x%04X fs=0x%04X gs=0x%04X ss=0x%04X\n",
            r->cs, r->ds, r->es, r->fs, r->gs, r->ss);

    kprintf("\n  System halted.\n");
    terminal_reset_color();
    halt_forever();
}

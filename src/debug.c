#include "../include/kernel.h"
#include "../include/debug.h"

#define COL(fg) vga_color_entry((fg), VGA_COLOR_BLACK)

void dump_memory(uint32_t addr, uint32_t words)
{
    uint8_t c_addr = COL(VGA_COLOR_LIGHT_CYAN);
    uint8_t c_word = COL(VGA_COLOR_LIGHT_GREY);
    uint8_t c_asc  = COL(VGA_COLOR_LIGHT_GREEN);

    uint32_t *base = (uint32_t *)(addr & ~3u);

    if (words == 0)
        words = 16;

    for (uint32_t i = 0; i < words; i += 4) {
        kprintf_color(c_addr, "0x%08X:", (unsigned int)(base + i));

        for (uint32_t j = 0; j < 4; j++) {
            if (i + j < words)
                kprintf_color(c_word, "  0x%08X", base[i + j]);
            else
                kprintf_color(c_word, "  %10s", "");
        }

        kprintf_color(COL(VGA_COLOR_DARK_GREY), "  |");
        for (uint32_t j = 0; j < 4 && i + j < words; j++) {
            uint32_t v = base[i + j];
            for (int b = 0; b < 4; b++) {
                char ch = (char)((v >> (b * 8)) & 0xFF);
                kprintf_color(c_asc, "%c", (ch >= 32 && ch <= 126) ? ch : '.');
            }
        }
        kprintf_color(COL(VGA_COLOR_DARK_GREY), "|\n");
    }
    terminal_reset_color();
}

void print_eflags(void)
{
    uint32_t f;
    __asm__ volatile ("pushf\n\t pop %0" : "=r"(f));

    static const struct { int bit; const char *name; } flags[] = {
        {0, "CF"}, {2, "PF"}, {4, "AF"}, {6, "ZF"}, {7, "SF"},
        {8, "TF"}, {9, "IF"}, {10, "DF"}, {11, "OF"}, {14, "NT"},
        {16, "RF"}, {17, "VM"}, {18, "AC"}, {21, "ID"},
    };

    kprintf_color(COL(VGA_COLOR_LIGHT_CYAN), "eflags = 0x%08X  [ ", f);
    for (size_t i = 0; i < sizeof(flags) / sizeof(flags[0]); i++) {
        if (f & (1u << flags[i].bit))
            kprintf_color(COL(VGA_COLOR_LIGHT_GREEN), "%s ", flags[i].name);
    }
    kprintf_color(COL(VGA_COLOR_LIGHT_CYAN), "]\n");
    kprintf_color(COL(VGA_COLOR_DARK_GREY), "IOPL = %d\n", (f >> 12) & 3);
    terminal_reset_color();
}

void print_registers(void)
{
    uint32_t eax, ebx, ecx, edx, esi, edi, esp, ebp;
    uint32_t cr0, cr2, cr3;
    uint32_t cs, ds, es, fs, gs, ss;

    __asm__ volatile ("movl %%eax, %0" : "=m"(eax));
    __asm__ volatile ("movl %%ebx, %0" : "=m"(ebx));
    __asm__ volatile ("movl %%ecx, %0" : "=m"(ecx));
    __asm__ volatile ("movl %%edx, %0" : "=m"(edx));
    __asm__ volatile ("movl %%esi, %0" : "=m"(esi));
    __asm__ volatile ("movl %%edi, %0" : "=m"(edi));
    __asm__ volatile ("movl %%esp, %0" : "=m"(esp));
    __asm__ volatile ("movl %%ebp, %0" : "=m"(ebp));
    __asm__ volatile ("mov  %%cr0, %0" : "=r"(cr0));
    __asm__ volatile ("mov  %%cr2, %0" : "=r"(cr2));
    __asm__ volatile ("mov  %%cr3, %0" : "=r"(cr3));
    __asm__ volatile ("mov  %%cs, %0"  : "=r"(cs));
    __asm__ volatile ("mov  %%ds, %0"  : "=r"(ds));
    __asm__ volatile ("mov  %%es, %0"  : "=r"(es));
    __asm__ volatile ("mov  %%fs, %0"  : "=r"(fs));
    __asm__ volatile ("mov  %%gs, %0"  : "=r"(gs));
    __asm__ volatile ("mov  %%ss, %0"  : "=r"(ss));

    uint8_t c_reg = COL(VGA_COLOR_LIGHT_GREY);
    uint8_t c_lbl = COL(VGA_COLOR_LIGHT_CYAN);

    kprintf_color(c_lbl, "GP registers\n");
    kprintf_color(c_reg, "  eax=0x%08X  ebx=0x%08X  ecx=0x%08X  edx=0x%08X\n",
                  eax, ebx, ecx, edx);
    kprintf_color(c_reg, "  esi=0x%08X  edi=0x%08X  esp=0x%08X  ebp=0x%08X\n",
                  esi, edi, esp, ebp);

    kprintf_color(c_lbl, "Segment selectors\n");
    kprintf_color(c_reg, "  cs=0x%04X ds=0x%04X es=0x%04X fs=0x%04X gs=0x%04X ss=0x%04X\n",
                  cs & 0xFFFF, ds & 0xFFFF, es & 0xFFFF,
                  fs & 0xFFFF, gs & 0xFFFF, ss & 0xFFFF);

    kprintf_color(c_lbl, "Control registers\n");
    kprintf_color(c_reg, "  cr0=0x%08X  cr2=0x%08X  cr3=0x%08X\n", cr0, cr2, cr3);
    kprintf_color(COL(VGA_COLOR_DARK_GREY),
                  "  cr0.PE=%d PG=%d WP=%d\n",
                  cr0 & 1, (cr0 >> 31) & 1, (cr0 >> 16) & 1);

    print_eflags();
    terminal_reset_color();
}

void print_cpuid(void)
{
    uint32_t eax, ebx, ecx, edx;
    char vendor[13];

    __asm__ volatile ("cpuid"
        : "=a"(eax), "=b"(ebx), "=c"(ecx), "=d"(edx)
        : "a"(0));

    *(uint32_t *)&vendor[0] = ebx;
    *(uint32_t *)&vendor[4] = edx;
    *(uint32_t *)&vendor[8] = ecx;
    vendor[12] = '\0';

    kprintf_color(COL(VGA_COLOR_LIGHT_CYAN), "CPUID vendor : ");
    kprintf_color(COL(VGA_COLOR_WHITE), "%s\n", vendor);
    kprintf_color(COL(VGA_COLOR_DARK_GREY), "max std leaf : 0x%08X\n", eax);

    __asm__ volatile ("cpuid"
        : "=a"(eax), "=b"(ebx), "=c"(ecx), "=d"(edx)
        : "a"(1));

    int family = (eax >> 8) & 0xF;
    int model  = (eax >> 4) & 0xF;
    int step   = eax & 0xF;

    kprintf_color(COL(VGA_COLOR_LIGHT_CYAN), "family/model/stepping : ");
    kprintf_color(COL(VGA_COLOR_WHITE), "%d / %d / %d\n", family, model, step);

    kprintf_color(COL(VGA_COLOR_LIGHT_CYAN), "features : ");
    if (edx & (1 << 0))  kprintf_color(COL(VGA_COLOR_LIGHT_GREEN), "fpu ");
    if (edx & (1 << 4))  kprintf_color(COL(VGA_COLOR_LIGHT_GREEN), "tsc ");
    if (edx & (1 << 5))  kprintf_color(COL(VGA_COLOR_LIGHT_GREEN), "msr ");
    if (edx & (1 << 6))  kprintf_color(COL(VGA_COLOR_LIGHT_GREEN), "pae ");
    if (edx & (1 << 9))  kprintf_color(COL(VGA_COLOR_LIGHT_GREEN), "apic ");
    if (edx & (1 << 13)) kprintf_color(COL(VGA_COLOR_LIGHT_GREEN), "pge ");
    if (edx & (1 << 23)) kprintf_color(COL(VGA_COLOR_LIGHT_GREEN), "mmx ");
    if (edx & (1 << 25)) kprintf_color(COL(VGA_COLOR_LIGHT_GREEN), "sse ");
    if (edx & (1 << 26)) kprintf_color(COL(VGA_COLOR_LIGHT_GREEN), "sse2 ");
    if (ecx & (1 << 0))  kprintf_color(COL(VGA_COLOR_LIGHT_GREEN), "sse3 ");
    if (ecx & (1 << 5))  kprintf_color(COL(VGA_COLOR_LIGHT_GREEN), "vmx ");
    kprintf("\n");
    terminal_reset_color();
}

#include "../include/kernel.h"

struct gdt_entry {
	uint16_t limit_low;
	uint16_t base_low;
	uint8_t base_middle;
	uint8_t access;
	uint8_t granularity;
	uint8_t base_high;
} __attribute__((packed));

struct gdt_ptr {
	unsigned short limit;
	unsigned int base;
} __attribute__((packed));

struct gdt_entry gdt[7];
struct gdt_ptr gp;

extern void gdt_flush();

void gdt_set_gate(int num, unsigned long base, unsigned long limit, unsigned char access, unsigned char granularity) 
{
	gdt[num].base_low = (base & 0xFFFF);
	gdt[num].base_middle = (base >> 16) & 0xFF;
	gdt[num].base_high = (base >> 24) & 0xFF;

	gdt[num].limit_low = (limit & 0xFFFF);
	gdt[num].granularity = ((limit >> 16) & 0x0F);
	gdt[num].granularity |= (granularity & 0xF0);
	gdt[num].access = access;
}

#define COL(fg)   vga_color_entry((fg), VGA_COLOR_BLACK)

static const char *seg_type_str(int s, int type)
{
    if (!s) {
        switch (type) {
            case 0x2: return "LDT";
            case 0x9: return "TSS (avail)";
            case 0xB: return "TSS (busy)";
            case 0xE: return "Int Gate";
            case 0xF: return "Trap Gate";
            default:  return "System";
        }
    }
    if (type & 0x08) {
        if (type & 0x04) return (type & 0x02) ? "Code RX conf" : "Code X conf";
        return (type & 0x02) ? "Code RX" : "Code X";
    }
    if (type & 0x04) return (type & 0x02) ? "Data RW down" : "Data RO down";
    return (type & 0x02) ? "Data RW" : "Data RO";
}

void print_gdt_info(void)
{
    uint8_t c_head = COL(VGA_COLOR_LIGHT_CYAN);
    uint8_t c_dim  = COL(VGA_COLOR_DARK_GREY);
    uint8_t c_r0   = COL(VGA_COLOR_LIGHT_GREEN);
    uint8_t c_r3   = COL(VGA_COLOR_LIGHT_MAGENTA);

    kprintf_color(c_head, "GDTR  base=");
    kprintf_color(COL(VGA_COLOR_WHITE), "0x%08X", gp.base);
    kprintf_color(c_head, "  limit=");
    kprintf_color(COL(VGA_COLOR_WHITE), "0x%04X", gp.limit);
    kprintf_color(c_dim, "  (%d entries)\n", (gp.limit + 1) / 8);

    kprintf_color(c_head,
        " #  Sel   Base        Limit      Gran  D/B  Type          DPL  P\n");
    kprintf_color(c_dim,
        "--- ----- ----------- ---------- ----- ---- ------------- ---- --\n");

    for (int i = 0; i < 7; i++) {
        unsigned int base = (gdt[i].base_high << 24) |
                            (gdt[i].base_middle << 16) |
                            gdt[i].base_low;
        unsigned int limit = (gdt[i].limit_low |
                             ((gdt[i].granularity & 0x0F) << 16));

        unsigned char access = gdt[i].access;
        unsigned char gran   = gdt[i].granularity;

        int present = (access >> 7) & 1;
        int dpl     = (access >> 5) & 3;
        int s       = (access >> 4) & 1;
        int type    = (access & 0x0F);
        int g       = (gran >> 7) & 1;
        int db      = (gran >> 6) & 1;

        uint8_t row = c_dim;
        if (present) row = dpl == 0 ? c_r0 : (dpl == 3 ? c_r3 : COL(VGA_COLOR_WHITE));

        if (i == 0 && access == 0) {
            kprintf_color(c_dim, " 0  0x00  ---------- ----------  --    --   NULL          -    -\n");
            continue;
        }

        kprintf_color(row,
            " %d  0x%02X  0x%08X 0x%05X    %s  %s  %-13s %d    %s\n",
            i, i * 8, base, limit,
            g ? "4K" : "B ",
            db ? "32" : "16",
            seg_type_str(s, type),
            dpl,
            present ? "1" : "0");
    }
}

/*
 * Dump the kernel stack in a GDB-style layout ("x/Nxw $esp"): four 32-bit
 * words per row, each row prefixed with its address.  A handful of known
 * marker values are written into a local array first so the dump has
 * something recognisable to show.
 */
static int all_nibbles_equal(uint32_t v)
{
    uint32_t n = v & 0xF;
    return v == (n * 0x11111111u) && n != 0;
}

void print_kernel_stack(void)
{
    volatile uint32_t markers[13];
    for (int i = 0; i < 13; i++)
        markers[i] = ((uint32_t)(i + 1) & 0xF) * 0x11111111u;

    uint32_t esp = get_stack_pointer();
    uint32_t ebp = get_base_pointer();
    uint32_t *sp = (uint32_t *)esp;

    uint8_t c_addr = COL(VGA_COLOR_LIGHT_CYAN);
    uint8_t c_word = COL(VGA_COLOR_LIGHT_GREY);
    uint8_t c_mark = COL(VGA_COLOR_LIGHT_GREEN);
    uint8_t c_ptr  = COL(VGA_COLOR_LIGHT_RED);

    kprintf_color(COL(VGA_COLOR_LIGHT_CYAN), "esp = 0x%08X", esp);
    kprintf_color(COL(VGA_COLOR_DARK_GREY), "   ");
    kprintf_color(COL(VGA_COLOR_LIGHT_RED), "ebp = 0x%08X\n", ebp);
    (void)markers;

    for (int row = 0; row < 16; row++) {
        kprintf_color(c_addr, "0x%08X:", (unsigned int)(sp + row * 4));
        for (int col = 0; col < 4; col++) {
            uint32_t *cell = sp + row * 4 + col;
            uint32_t v = *cell;
            uint8_t c = c_word;
            if ((uint32_t)cell == esp || (uint32_t)cell == ebp) c = c_ptr;
            else if (all_nibbles_equal(v)) c = c_mark;
            kprintf_color(c, "  0x%08X", v);
        }
        kprintf("\n");
    }
    terminal_reset_color();
}

uint32_t get_stack_pointer() 
{
	uint32_t sp;
	__asm__ volatile("mov %%esp, %0" : "=r"(sp));
	return sp;
}


uint32_t get_base_pointer() 
{
	uint32_t bp;
	__asm__ volatile("mov %%ebp, %0" : "=r"(bp));
	return bp;
}

void gdt_install() 
{

	gp.limit = (sizeof(struct gdt_entry) * 7) - 1;
	gp.base = (unsigned int)&gdt;
	
    gdt_set_gate(0, 0, 0, 0, 0); // null descriptor
    gdt_set_gate(1, 0, 0xFFFFFFFF, 0x9A, 0xCF); // kernel code segment
    gdt_set_gate(2, 0, 0xFFFFFFFF, 0x92, 0xCF); // kernel data segment
	gdt_set_gate(3, 0, 0xFFFFFFFF, 0x96, 0xCF); // kernel stack segment

    gdt_set_gate(4, 0, 0xFFFFFFFF, 0xFA, 0xCF); // user code segment
    gdt_set_gate(5, 0, 0xFFFFFFFF, 0xF2, 0xCF); // user data segment
	gdt_set_gate(6, 0, 0xFFFFFFFF, 0xF6, 0xCF); // user stack segment

	gdt_flush();
}

#include "../include/kernel.h"

struct idt_entry {
	unsigned short base_lo; // Lower 16 bits of the address to jump to when this interrupt fires
	unsigned short sel; // The selector in the GDT or LDT that this IDT entry will use to find the code segment
	unsigned char always0;
	unsigned char flags;
	unsigned short base_hi;
} __attribute__((packed));

struct idt_ptr {
	unsigned short limit;
	unsigned int base;
} __attribute__((packed));

struct idt_entry idt[256];
struct idt_ptr idtp;

extern void idt_load();

void idt_set_gate(unsigned char num, unsigned long base, unsigned short sel, unsigned char flags) 
{
    idt[num].base_lo = (base & 0xFFFF);
    idt[num].base_hi = (base >> 16) & 0xFFFF;
    idt[num].sel = sel;
    idt[num].always0 = 0;
    idt[num].flags = flags;
}

void idt_install() 
{
	idtp.limit = (sizeof(struct idt_entry) * 256) - 1;
	idtp.base = (unsigned int)idt;

	memset(&idt, 0, sizeof(struct idt_entry) * 256);
	idt_load();
}
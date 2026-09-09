#ifndef GDT_H
#define GDT_H

void gdt_set_gate(int num, unsigned long base, unsigned long limit, unsigned char access, unsigned char granularity);
void gdt_install();
void print_gdt_info();
void print_kernel_stack();
uint32_t get_stack_pointer();
uint32_t get_base_pointer();

#endif

#include "../include/kernel.h"

extern uint8_t kernel_start[];
extern uint8_t kernel_end[];

static uint32_t detect_mem_upper(uint32_t magic, multiboot_info_t *mbi)
{
    if (magic == MULTIBOOT_BOOTLOADER_MAGIC && mbi &&
        (mbi->flags & MULTIBOOT_INFO_MEMORY))
        return mbi->mem_upper;

    return 31 * 1024;   /* fall back to "1 MiB + 31 MiB" */
}

void kernel_main(uint32_t magic, multiboot_info_t *mbi)
{
    gdt_install();
    idt_install();
    isrs_install();
    irq_install();
    timer_install();
    keyboard_install();

    uint32_t mem_upper = detect_mem_upper(magic, mbi);
    pmm_init(mem_upper);
    paging_init();

    __asm__ __volatile__("sti");

    init_terminal();

    kprintf_color(vga_color_entry(VGA_COLOR_LIGHT_CYAN, VGA_COLOR_BLACK),
                  "=== KFS_3 : paging + kmalloc / vmalloc ===\n");
    kprintf("kernel image : 0x%08X - 0x%08X\n",
            (uint32_t)kernel_start, (uint32_t)kernel_end);
    kprintf("RAM detected : %u KiB  (%u frames in pool)\n",
            1024 + mem_upper, pmm_total_frames());
    kprintf("paging       : enabled, first 64 MiB identity mapped\n");
    kprintf("kernel heap  : 0x%08X - 0x%08X\n", KHEAP_START, KHEAP_LIMIT);
    kprintf("virtual heap : 0x%08X - 0x%08X\n", VHEAP_START, VHEAP_LIMIT);
    kprintf("\nPress ESC to open the debug shell.\n");

    while (1)
        __asm__ volatile ("hlt");
}

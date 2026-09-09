#include "../include/kernel.h"
#include "../include/mm.h"
#include "../include/panic.h"

#define COL(fg) vga_color_entry((fg), VGA_COLOR_BLACK)

#define ID_PTS  (IDENTITY_END / (PAGE_SIZE * 1024))   /* 64 MiB / 4 MiB = 16 */

/* The page directory and the identity-mapping page tables live in .bss and
 * are therefore already reachable before paging is turned on. */
static uint32_t page_directory[1024]        __attribute__((aligned(PAGE_SIZE)));
static uint32_t identity_pt[ID_PTS][1024]   __attribute__((aligned(PAGE_SIZE)));

static inline void load_cr3(uint32_t pd)
{
    __asm__ volatile ("mov %0, %%cr3" :: "r"(pd) : "memory");
}

static inline void enable_paging(void)
{
    uint32_t cr0;
    __asm__ volatile ("mov %%cr0, %0" : "=r"(cr0));
    cr0 |= 0x80010000u;                 /* PG (bit 31) + WP (bit 16) */
    __asm__ volatile ("mov %0, %%cr0" :: "r"(cr0) : "memory");
}

static inline void invlpg(uint32_t virt)
{
    __asm__ volatile ("invlpg (%0)" :: "r"(virt) : "memory");
}

void paging_init(void)
{
    for (uint32_t d = 0; d < ID_PTS; d++) {
        for (uint32_t e = 0; e < 1024; e++)
            identity_pt[d][e] = ((d * 1024 + e) * PAGE_SIZE) | PAGE_PRESENT | PAGE_RW;
        page_directory[d] = ((uint32_t)identity_pt[d]) | PAGE_PRESENT | PAGE_RW;
    }
    for (uint32_t d = ID_PTS; d < 1024; d++)
        page_directory[d] = PAGE_RW;    /* not present */

    load_cr3((uint32_t)page_directory);
    enable_paging();
}

int map_page(uint32_t virt, uint32_t phys, uint32_t flags)
{
    uint32_t di = virt >> 22;
    uint32_t ti = (virt >> 12) & 0x3FF;
    uint32_t *pt;

    if (!(page_directory[di] & PAGE_PRESENT)) {
        uint32_t frame = pmm_alloc_frame();
        if (!frame)
            return -1;
        pt = (uint32_t *)frame;               /* identity mapped */
        memset(pt, 0, PAGE_SIZE);
        page_directory[di] = frame | PAGE_PRESENT | PAGE_RW | (flags & PAGE_USER);
    } else {
        pt = (uint32_t *)(page_directory[di] & PAGE_MASK);
    }

    pt[ti] = (phys & PAGE_MASK) | (flags & 0xFFF) | PAGE_PRESENT;
    invlpg(virt);
    return 0;
}

void unmap_page(uint32_t virt)
{
    uint32_t di = virt >> 22;
    uint32_t ti = (virt >> 12) & 0x3FF;

    if (!(page_directory[di] & PAGE_PRESENT))
        return;

    uint32_t *pt = (uint32_t *)(page_directory[di] & PAGE_MASK);
    pt[ti] = 0;
    invlpg(virt);
}

uint32_t virt_to_phys(uint32_t virt)
{
    uint32_t di = virt >> 22;
    uint32_t ti = (virt >> 12) & 0x3FF;

    if (!(page_directory[di] & PAGE_PRESENT))
        return 0;

    uint32_t *pt = (uint32_t *)(page_directory[di] & PAGE_MASK);
    if (!(pt[ti] & PAGE_PRESENT))
        return 0;

    return (pt[ti] & PAGE_MASK) | (virt & 0xFFF);
}

void paging_dump(uint32_t virt)
{
    uint32_t di = virt >> 22;
    uint32_t ti = (virt >> 12) & 0x3FF;
    uint32_t pde = page_directory[di];

    kprintf_color(COL(VGA_COLOR_LIGHT_CYAN), "linear   0x%08X\n", virt);
    kprintf_color(COL(VGA_COLOR_LIGHT_GREY),
                  "  dir idx  %4u   pde 0x%08X  %s\n",
                  di, pde, (pde & PAGE_PRESENT) ? "present" : "not present");

    if (!(pde & PAGE_PRESENT)) {
        terminal_reset_color();
        return;
    }

    uint32_t *pt = (uint32_t *)(pde & PAGE_MASK);
    uint32_t pte = pt[ti];

    kprintf_color(COL(VGA_COLOR_LIGHT_GREY),
                  "  tbl idx  %4u   pte 0x%08X  %s%s%s\n",
                  ti, pte,
                  (pte & PAGE_PRESENT) ? "P" : "-",
                  (pte & PAGE_RW)      ? "W" : "R",
                  (pte & PAGE_USER)    ? "U" : "S");

    if (pte & PAGE_PRESENT)
        kprintf_color(COL(VGA_COLOR_LIGHT_GREEN),
                      "  -> phys  0x%08X\n", (pte & PAGE_MASK) | (virt & 0xFFF));
    terminal_reset_color();
}

void page_fault_handler(struct regs *r)
{
    uint32_t cr2;
    __asm__ volatile ("mov %%cr2, %0" : "=r"(cr2));

    int present = r->err_code & 0x1;
    int write   = r->err_code & 0x2;
    int user    = r->err_code & 0x4;

    panic("page fault at 0x%08X (%s, %s, %s) eip=0x%08X",
          cr2,
          present ? "protection" : "not-present",
          write   ? "write"      : "read",
          user    ? "user"       : "kernel",
          r->eip);
}

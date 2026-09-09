#ifndef MM_H
#define MM_H

#include <stdint.h>
#include <stddef.h>

struct regs;

/* ----- geometry ------------------------------------------------------- */
#define PAGE_SIZE        0x1000u
#define PAGE_MASK        0xFFFFF000u
#define PAGE_ALIGN_DOWN(x) ((uint32_t)(x) & PAGE_MASK)
#define PAGE_ALIGN_UP(x)   (((uint32_t)(x) + PAGE_SIZE - 1) & PAGE_MASK)

/* ----- fixed kernel memory map -------------------------------------- */
#define IDENTITY_END     0x04000000u   /* first 64 MiB are identity mapped   */

#define KHEAP_START      0x00800000u   /*  8 MiB : kernel heap (kmalloc)     */
#define KHEAP_LIMIT      0x01000000u   /* 16 MiB                             */

#define PMM_POOL_START   0x01000000u   /* 16 MiB : physical frame pool       */

#define VHEAP_START      0x40000000u   /*  1 GiB : virtual heap (vmalloc)    */
#define VHEAP_LIMIT      0x50000000u

/* ----- page table / directory entry flags -------------------------- */
#define PAGE_PRESENT     0x001u
#define PAGE_RW          0x002u
#define PAGE_USER        0x004u
#define PAGE_ACCESSED    0x020u
#define PAGE_DIRTY       0x040u

/* =================================================================== */
/*  Physical memory manager (frame allocator)                           */
/* =================================================================== */
void      pmm_init(uint32_t mem_upper_kb);
uint32_t  pmm_alloc_frame(void);              /* 0 on failure            */
void      pmm_free_frame(uint32_t phys);
uint32_t  pmm_total_frames(void);
uint32_t  pmm_used_frames(void);

/* =================================================================== */
/*  Paging                                                              */
/* =================================================================== */
void      paging_init(void);
int       map_page(uint32_t virt, uint32_t phys, uint32_t flags);
void      unmap_page(uint32_t virt);
uint32_t  virt_to_phys(uint32_t virt);        /* 0 if unmapped           */
void      paging_dump(uint32_t virt);
void      page_fault_handler(struct regs *r);

/* =================================================================== */
/*  Kernel heap  (physical / identity mapped)                           */
/* =================================================================== */
void     *kbrk(int32_t increment);            /* sbrk-like; (void*)-1 err */
void     *kmalloc(size_t size);
void      kfree(void *ptr);
size_t    ksize(void *ptr);
void      kheap_stats(size_t *used, size_t *free, size_t *blocks);

/* =================================================================== */
/*  Virtual heap  (non-contiguous, demand mapped)                       */
/* =================================================================== */
void     *vbrk(int32_t increment);
void     *vmalloc(size_t size);
void      vfree(void *ptr);
size_t    vsize(void *ptr);
void      vheap_stats(size_t *used, size_t *free, size_t *blocks);

#endif

#include "../include/kernel.h"
#include "../include/mm.h"
#include "../include/panic.h"

/*
 * One first-fit free-list allocator, instantiated twice:
 *
 *   kheap  - lives in the identity-mapped window [KHEAP_START, KHEAP_LIMIT).
 *            kbrk() just moves a pointer, the frames are already mapped.
 *
 *   vheap  - lives in [VHEAP_START, VHEAP_LIMIT).  vbrk() maps a fresh
 *            (possibly non-contiguous) physical frame for every page it
 *            grows over, and releases them again when it shrinks.
 */

#define BLOCK_MAGIC 0xB10CB10Cu
#define ALIGN8(x)   (((x) + 7u) & ~7u)

typedef struct block {
    uint32_t magic;
    uint32_t size;          /* payload size in bytes            */
    uint32_t free;
    struct block *next;
    struct block *prev;
} block_t;

#define HDR ALIGN8(sizeof(block_t))

typedef struct {
    uint32_t start;
    uint32_t end;           /* current break                    */
    uint32_t limit;
    block_t *first;
    int      is_virtual;
} heap_t;

static heap_t kheap = { KHEAP_START, KHEAP_START, KHEAP_LIMIT, 0, 0 };
static heap_t vheap = { VHEAP_START, VHEAP_START, VHEAP_LIMIT, 0, 1 };

/* ------------------------------------------------------------------ */
/*  sbrk                                                               */
/* ------------------------------------------------------------------ */
static void *heap_sbrk(heap_t *h, int32_t inc)
{
    uint32_t old = h->end;

    if (inc == 0)
        return (void *)old;

    if (inc > 0) {
        uint32_t new_end = old + (uint32_t)inc;
        if (new_end > h->limit || new_end < old)
            return (void *)-1;

        if (h->is_virtual) {
            uint32_t p = PAGE_ALIGN_DOWN(h->start > old ? h->start : old);
            for (; p < PAGE_ALIGN_UP(new_end); p += PAGE_SIZE) {
                if (virt_to_phys(p))
                    continue;
                uint32_t frame = pmm_alloc_frame();
                if (!frame)
                    return (void *)-1;
                if (map_page(p, frame, PAGE_PRESENT | PAGE_RW) != 0) {
                    pmm_free_frame(frame);
                    return (void *)-1;
                }
                memset((void *)p, 0, PAGE_SIZE);
            }
        }
        h->end = new_end;
        return (void *)old;
    }

    /* shrink */
    uint32_t new_end = old - (uint32_t)(-inc);
    if (new_end < h->start)
        new_end = h->start;

    if (h->is_virtual) {
        for (uint32_t p = PAGE_ALIGN_UP(new_end); p < PAGE_ALIGN_UP(old); p += PAGE_SIZE) {
            uint32_t phys = virt_to_phys(p);
            if (phys) {
                pmm_free_frame(PAGE_ALIGN_DOWN(phys));
                unmap_page(p);
            }
        }
    }
    h->end = new_end;
    return (void *)old;
}

/* ------------------------------------------------------------------ */
/*  core allocator                                                     */
/* ------------------------------------------------------------------ */
static void split_block(block_t *b, uint32_t want)
{
    if (b->size < want + HDR + 8)
        return;

    block_t *rest = (block_t *)((uint8_t *)b + HDR + want);
    rest->magic = BLOCK_MAGIC;
    rest->size  = b->size - want - HDR;
    rest->free  = 1;
    rest->next  = b->next;
    rest->prev  = b;

    if (b->next)
        b->next->prev = rest;
    b->next = rest;
    b->size = want;
}

static void *heap_alloc(heap_t *h, size_t size)
{
    uint32_t want = ALIGN8(size ? size : 1);

    block_t *last = 0;
    for (block_t *b = h->first; b; last = b, b = b->next) {
        if (b->free && b->size >= want) {
            split_block(b, want);
            b->free = 0;
            return (void *)((uint8_t *)b + HDR);
        }
    }

    block_t *nb = heap_sbrk(h, (int32_t)(HDR + want));
    if ((void *)nb == (void *)-1)
        return 0;

    nb->magic = BLOCK_MAGIC;
    nb->size  = want;
    nb->free  = 0;
    nb->next  = 0;
    nb->prev  = last;

    if (last)
        last->next = nb;
    else
        h->first = nb;

    return (void *)((uint8_t *)nb + HDR);
}

static block_t *checked_block(heap_t *h, void *ptr, const char *who)
{
    if ((uint32_t)ptr < h->start + HDR || (uint32_t)ptr >= h->end) {
        panic_nonfatal("%s: pointer %p outside heap", who, ptr);
        return 0;
    }
    block_t *b = (block_t *)((uint8_t *)ptr - HDR);
    if (b->magic != BLOCK_MAGIC) {
        panic_nonfatal("%s: corrupt/!owned block %p", who, ptr);
        return 0;
    }
    return b;
}

static void heap_free(heap_t *h, void *ptr)
{
    if (!ptr)
        return;

    block_t *b = checked_block(h, ptr, "free");
    if (!b)
        return;

    b->free = 1;

    if (b->next && b->next->free) {
        b->size += HDR + b->next->size;
        b->next = b->next->next;
        if (b->next)
            b->next->prev = b;
    }
    if (b->prev && b->prev->free) {
        b->prev->size += HDR + b->size;
        b->prev->next = b->next;
        if (b->next)
            b->next->prev = b->prev;
        b = b->prev;
    }

    if (!b->next) {                       /* last block: give it back */
        if (b->prev)
            b->prev->next = 0;
        else
            h->first = 0;
        heap_sbrk(h, -(int32_t)(HDR + b->size));
    }
}

static void heap_stats(heap_t *h, size_t *used, size_t *freeb, size_t *blocks)
{
    size_t u = 0, f = 0, n = 0;
    for (block_t *b = h->first; b; b = b->next) {
        n++;
        if (b->free) f += b->size;
        else         u += b->size;
    }
    if (used)   *used = u;
    if (freeb)  *freeb = f;
    if (blocks) *blocks = n;
}

/* ================================================================== */
/*  public API                                                         */
/* ================================================================== */
void  *kbrk(int32_t inc)            { return heap_sbrk(&kheap, inc); }
void  *kmalloc(size_t size)         { return heap_alloc(&kheap, size); }
void   kfree(void *ptr)             { heap_free(&kheap, ptr); }
size_t ksize(void *ptr)
{
    block_t *b = checked_block(&kheap, ptr, "ksize");
    return b ? b->size : 0;
}
void kheap_stats(size_t *u, size_t *f, size_t *b) { heap_stats(&kheap, u, f, b); }

void  *vbrk(int32_t inc)            { return heap_sbrk(&vheap, inc); }
void  *vmalloc(size_t size)         { return heap_alloc(&vheap, size); }
void   vfree(void *ptr)             { heap_free(&vheap, ptr); }
size_t vsize(void *ptr)
{
    block_t *b = checked_block(&vheap, ptr, "vsize");
    return b ? b->size : 0;
}
void vheap_stats(size_t *u, size_t *f, size_t *b) { heap_stats(&vheap, u, f, b); }

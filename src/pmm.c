#include "../include/kernel.h"
#include "../include/mm.h"

/*
 * Physical frame allocator - a flat bitmap over the frame pool that starts
 * at PMM_POOL_START.  The pool is capped at IDENTITY_END so that every frame
 * we ever hand out is reachable through the identity mapping (needed to be
 * able to zero freshly allocated page tables).
 */

#define PMM_POOL_END   IDENTITY_END
#define PMM_FRAMES     ((PMM_POOL_END - PMM_POOL_START) / PAGE_SIZE)

static uint8_t frame_bitmap[PMM_FRAMES / 8];
static uint32_t total_frames = 0;
static uint32_t used_frames  = 0;

static inline void bm_set(uint32_t i)   { frame_bitmap[i >> 3] |=  (1u << (i & 7)); }
static inline void bm_clear(uint32_t i) { frame_bitmap[i >> 3] &= ~(1u << (i & 7)); }
static inline int  bm_test(uint32_t i)  { return frame_bitmap[i >> 3] & (1u << (i & 7)); }

void pmm_init(uint32_t mem_upper_kb)
{
    uint32_t ram_top = 0x100000u + mem_upper_kb * 1024u;
    if (ram_top > PMM_POOL_END)
        ram_top = PMM_POOL_END;

    memset(frame_bitmap, 0, sizeof(frame_bitmap));

    if (ram_top <= PMM_POOL_START) {
        total_frames = 0;
        return;
    }

    total_frames = (ram_top - PMM_POOL_START) / PAGE_SIZE;
    used_frames  = 0;

    /* frames past the detected RAM top are permanently unavailable */
    for (uint32_t i = total_frames; i < PMM_FRAMES; i++)
        bm_set(i);
}

uint32_t pmm_alloc_frame(void)
{
    for (uint32_t i = 0; i < total_frames; i++) {
        if (!bm_test(i)) {
            bm_set(i);
            used_frames++;
            return PMM_POOL_START + i * PAGE_SIZE;
        }
    }
    return 0;
}

void pmm_free_frame(uint32_t phys)
{
    if (phys < PMM_POOL_START || phys >= PMM_POOL_END)
        return;

    uint32_t i = (phys - PMM_POOL_START) / PAGE_SIZE;
    if (bm_test(i)) {
        bm_clear(i);
        used_frames--;
    }
}

uint32_t pmm_total_frames(void) { return total_frames; }
uint32_t pmm_used_frames(void)  { return used_frames; }

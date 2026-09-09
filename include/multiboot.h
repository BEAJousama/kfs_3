#ifndef MULTIBOOT_H
#define MULTIBOOT_H

#include <stdint.h>

#define MULTIBOOT_BOOTLOADER_MAGIC 0x2BADB002

#define MULTIBOOT_INFO_MEMORY   0x00000001
#define MULTIBOOT_INFO_MEM_MAP  0x00000040

#define MULTIBOOT_MEMORY_AVAILABLE 1

struct multiboot_mmap_entry {
    uint32_t size;      /* size of this entry, not counting `size` itself */
    uint64_t addr;
    uint64_t len;
    uint32_t type;
} __attribute__((packed));

struct multiboot_info {
    uint32_t flags;
    uint32_t mem_lower;   /* KiB, from 0 */
    uint32_t mem_upper;   /* KiB, from 1 MiB */
    uint32_t boot_device;
    uint32_t cmdline;
    uint32_t mods_count;
    uint32_t mods_addr;
    uint32_t syms[4];
    uint32_t mmap_length;
    uint32_t mmap_addr;   /* array of multiboot_mmap_entry */
    uint32_t drives_length;
    uint32_t drives_addr;
    uint32_t config_table;
    uint32_t boot_loader_name;
    uint32_t apm_table;
} __attribute__((packed));

typedef struct multiboot_info multiboot_info_t;
typedef struct multiboot_mmap_entry multiboot_mmap_entry_t;

#endif

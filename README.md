# KFS_3 — Memory

Paging, a physical frame allocator, and two heap allocators
(`kmalloc` / `vmalloc`) with a debug shell to poke at them.

## Building

Needs an `i686-elf` cross toolchain, `nasm`, `grub-mkrescue`
(`grub-pc-bin` + `xorriso`) and `qemu-system-i386`.

```sh
make                # build build/kernel.iso
make run            # build + boot in QEMU
make re             # clean rebuild
make CROSS_GCC=~/opt/cross/bin/i686-elf-gcc CROSS_AS=~/opt/cross/bin/i686-elf-as
```

The kernel binary is well under 10 MB.

## What is implemented

| Requirement                                   | Where |
|-----------------------------------------------|-------|
| Enable paging (4 KiB pages, CR3 / CR0.PG)     | [src/paging.c](src/paging.c) |
| Page directory / table structures & rights    | [src/paging.c](src/paging.c), [include/mm.h](include/mm.h) |
| Kernel vs user space (`PAGE_USER` / supervisor)| [include/mm.h](include/mm.h) |
| Create / fetch pages (`map_page`, `virt_to_phys`) | [src/paging.c](src/paging.c) |
| Physical memory manager (frame bitmap)        | [src/pmm.c](src/pmm.c) |
| `kmalloc` / `kfree` / `ksize` / `kbrk`        | [src/heap.c](src/heap.c) |
| `vmalloc` / `vfree` / `vsize` / `vbrk`        | [src/heap.c](src/heap.c) |
| Kernel panic (fatal vs recoverable)           | [src/panic.c](src/panic.c) |
| Page-fault handler (decodes CR2 + error code) | [src/paging.c](src/paging.c) |

## Theory in one paragraph

The CPU translates a 32-bit linear address in three steps: bits 31–22 index
the **page directory** (its physical base is in `CR3`), bits 21–12 index the
**page table** pointed at by that directory entry, and bits 11–0 are the byte
offset inside the resulting 4 KiB **page**. Directory and table entries share
a layout: a 20-bit physical frame number (the low 12 bits are always 0
because everything is 4 KiB-aligned) plus flag bits — `P` present, `R/W`
writable, `U/S` user-accessible, `A` accessed, `D` dirty, `PS` page size.
Paging gives every process its own 4 GiB view of memory and lets the kernel
grant or revoke pages dynamically.

## Memory map

```
0x00000000 - 0x00200000   low memory / BIOS / VGA           (identity)
0x00200000 - 0x00800000   kernel image + page tables + bss  (identity)
0x00800000 - 0x01000000   kernel heap  (kmalloc / kbrk)     (identity)
0x01000000 - 0x04000000   physical frame pool (pmm)         (identity)
0x00000000 - 0x04000000   << first 64 MiB are identity mapped >>
0x40000000 - 0x50000000   virtual heap (vmalloc / vbrk)     (demand mapped)
```

* **PMM** — a bitmap over the frame pool. `pmm_alloc_frame` / `pmm_free_frame`
  hand out 4 KiB physical frames; every frame stays inside the identity map so
  page tables allocated from it can be written directly.
* **Kernel heap** — lives in the identity-mapped window, so `kbrk` only has to
  move a pointer. `kmalloc` is a first-fit free list with block headers,
  splitting on allocation and coalescing on free; `ksize` reads the header.
* **Virtual heap** — same allocator, but `vbrk` calls `pmm_alloc_frame` +
  `map_page` for every page it grows across, so the backing frames are
  **not contiguous** in physical memory. Shrinking unmaps and frees them.

## Panics

`page_fault_handler` and the other fatal exceptions call `panic()`: a red
screen with a full register dump, then the CPU is halted. Recoverable traps
(`#BP`, `#DB`) go through `panic_nonfatal()` — a yellow banner — and execution
resumes. That is the "not every panic is fatal" distinction from the subject.

## Debug shell

Press **`ESC`** to open the shell (`Shell $>`, TAB completion, BACKSPACE
editing). Memory-related commands:

| Command | Description |
|---------|-------------|
| `pmm`   | frame pool: total / used / free |
| `paging <addr>` | walk the directory + table for a linear address, show the PTE flags and physical target |
| `kmalloc <size>` | allocate on the kernel heap, print address + `ksize` |
| `kfree <addr>` / `ksize <addr>` | free / query a kernel-heap block |
| `vmalloc <size>` | allocate on the virtual heap, print address, `vsize` and the (non-identity) physical frame |
| `vfree <addr>` / `vsize <addr>` | free / query a virtual-heap block |
| `heap`  | live usage of both heaps |
| `memtest` | allocate/free stress test across both allocators |
| `mem <addr> [words]` | raw hex + ASCII dump |
| `fault <div0\|bp\|gp\|ud\|pf>` | raise an exception (`pf` touches `0x30000000`, unmapped) |

Other commands (`gdt`, `stack`, `regs`, `eflags`, `cpuid`, `uptime`, `time`,
`colors`, `reboot`, `halt`) carry over from KFS_2. Four VGA screens:
`ALT`+`Left/Right` switch, `Up`/`Down` scroll the backlog.

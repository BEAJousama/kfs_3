# === KFS_3 =================================================================

NAME       := kfs_3

# === Toolchain ============================================================
# Primary path: an i686-elf cross toolchain (what the subject recommends).
# Fallback: the host gcc in 32-bit mode + the host ld. Works for C; needs
#   the 'gcc-multilib' package for the 32-bit headers.
# Override explicitly with:  make CROSS_GCC=~/opt/cross/bin/i686-elf-gcc
CROSS_GCC  ?= $(shell command -v i686-elf-gcc 2>/dev/null)
NASM       ?= nasm

ifneq ($(CROSS_GCC),)
  CC        := $(CROSS_GCC)
  LINK_CMD   = $(CC) -T $(LINKER) -ffreestanding -O2 -nostdlib -nodefaultlibs -o $@ $^ -lgcc
  TOOLCHAIN  := cross ($(CROSS_GCC))
else
  CC        := gcc -m32
  LINK_CMD   = ld -m elf_i386 -T $(LINKER) -o $@ $^
  TOOLCHAIN  := host gcc -m32 + ld
endif

# === Sources =============================================================
C_SOURCES  := $(wildcard src/*.c)
C_OBJECTS  := $(patsubst src/%.c,build/%.o,$(C_SOURCES))

BOOT_SRC   := src/boot.s
START_SRC  := src/start.asm
LINKER     := src/linker.ld

BOOT_OBJ   := build/boot.o
START_OBJ  := build/start.o
KERNEL_BIN := build/kernel.bin
ISO_IMAGE  := build/kernel.iso

CFLAGS     := -std=gnu99 -ffreestanding -O2 -Wall -Wextra \
              -fno-builtin -fno-stack-protector -fno-pic -nostdlib -nodefaultlibs

QEMU       ?= qemu-system-i386
QEMUFLAGS  ?= -m 512 -no-reboot

# === Rules ==============================================================
all: $(ISO_IMAGE)
	@echo "built with: $(TOOLCHAIN)"

build/%.o: src/%.c
	@mkdir -p build
	$(CC) -c $< -o $@ $(CFLAGS)

$(BOOT_OBJ): $(BOOT_SRC)
	@mkdir -p build
	$(CC) -c $< -o $@

$(START_OBJ): $(START_SRC)
	@mkdir -p build
	$(NASM) -f elf32 $< -o $@

$(KERNEL_BIN): $(BOOT_OBJ) $(C_OBJECTS) $(START_OBJ)
	$(LINK_CMD)

$(ISO_IMAGE): $(KERNEL_BIN)
	@mkdir -p isodir/boot/grub
	cp $< isodir/boot/kernel.bin
	cp boot/grub/grub.cfg isodir/boot/grub/grub.cfg
	grub-mkrescue -d /usr/lib/grub/i386-pc -o $@ isodir

# graphical window (host with a display)
run: $(ISO_IMAGE)
	$(QEMU) -cdrom $(ISO_IMAGE) $(QEMUFLAGS)

# text mode inside the current terminal (headless VM / SSH); leave with: Ctrl-a x
run-curses: $(ISO_IMAGE)
	$(QEMU) -cdrom $(ISO_IMAGE) $(QEMUFLAGS) -display curses

clean:
	rm -rf build isodir

re: clean all

.PHONY: all run run-curses clean re

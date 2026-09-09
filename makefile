# === KFS_3 =================================================================

NAME       := kfs_3

# === Toolchain (override on the command line if yours lives elsewhere) =====
#   make CROSS_GCC=~/opt/cross/bin/i686-elf-gcc
CROSS_GCC  ?= $(shell command -v i686-elf-gcc || echo i686-elf-gcc)
CROSS_AS   ?= $(shell command -v i686-elf-as  || echo i686-elf-as)
NASM       ?= nasm

# === Sources ==============================================================
C_SOURCES  := $(wildcard src/*.c)
C_OBJECTS  := $(patsubst src/%.c,build/%.o,$(C_SOURCES))

BOOT_SRC   := src/boot.s
START_SRC  := src/start.asm
LINKER     := src/linker.ld

BOOT_OBJ   := build/boot.o
START_OBJ  := build/start.o
KERNEL_BIN := build/kernel.bin
ISO_IMAGE  := build/kernel.iso

# === Flags ===============================================================
CFLAGS     := -std=gnu99 -ffreestanding -O2 -Wall -Wextra \
              -fno-builtin -fno-stack-protector -fno-pic -nostdlib -nodefaultlibs
LDFLAGS    := -T $(LINKER) -ffreestanding -O2 -nostdlib -nodefaultlibs

# === Rules ===============================================================
all: $(ISO_IMAGE)

build/%.o: src/%.c
	@mkdir -p build
	$(CROSS_GCC) -c $< -o $@ $(CFLAGS)

$(BOOT_OBJ): $(BOOT_SRC)
	@mkdir -p build
	$(CROSS_AS) $< -o $@

$(START_OBJ): $(START_SRC)
	@mkdir -p build
	$(NASM) -f elf32 $< -o $@

$(KERNEL_BIN): $(BOOT_OBJ) $(C_OBJECTS) $(START_OBJ)
	$(CROSS_GCC) $(LDFLAGS) -o $@ $^ -lgcc

$(ISO_IMAGE): $(KERNEL_BIN)
	@mkdir -p isodir/boot/grub
	cp $< isodir/boot/kernel.bin
	cp boot/grub/grub.cfg isodir/boot/grub/grub.cfg
	grub-mkrescue -d /usr/lib/grub/i386-pc -o $@ isodir

run: $(ISO_IMAGE)
	qemu-system-i386 -cdrom $(ISO_IMAGE)

clean:
	rm -rf build isodir

re: clean all

.PHONY: all run clean re

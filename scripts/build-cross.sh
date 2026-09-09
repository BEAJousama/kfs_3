#!/usr/bin/env bash
#
# Builds an i686-elf cross toolchain (binutils + gcc, C and C++) into
# /opt/cross and puts it on PATH. Optional — the Makefile falls back to
# `gcc -m32` without it. Takes ~30-45 minutes. Run as root:
#
#   sudo scripts/build-cross.sh
#
# Idempotent: skips the build if /opt/cross/bin/i686-elf-gcc already exists.

set -euo pipefail

TARGET=i686-elf
PREFIX=/opt/cross
BINUTILS_VER=2.41
GCC_VER=13.2.0

if [ -x "$PREFIX/bin/$TARGET-gcc" ]; then
    echo "already installed: $("$PREFIX/bin/$TARGET-gcc" --version | head -1)"
    exit 0
fi

export DEBIAN_FRONTEND=noninteractive
apt-get install -y --no-install-recommends \
    build-essential bison flex texinfo \
    libgmp3-dev libmpc-dev libmpfr-dev libisl-dev curl xz-utils

SRC=/usr/local/src/cross
mkdir -p "$SRC" && cd "$SRC"

[ -f "binutils-$BINUTILS_VER.tar.xz" ] || \
    curl -fLO "https://ftp.gnu.org/gnu/binutils/binutils-$BINUTILS_VER.tar.xz"
[ -f "gcc-$GCC_VER.tar.xz" ] || \
    curl -fLO "https://ftp.gnu.org/gnu/gcc/gcc-$GCC_VER/gcc-$GCC_VER.tar.xz"

rm -rf "binutils-$BINUTILS_VER" "gcc-$GCC_VER" build-binutils build-gcc
tar xf "binutils-$BINUTILS_VER.tar.xz"
tar xf "gcc-$GCC_VER.tar.xz"
( cd "gcc-$GCC_VER" && ./contrib/download_prerequisites )

mkdir build-binutils && cd build-binutils
../"binutils-$BINUTILS_VER"/configure \
    --target=$TARGET --prefix="$PREFIX" --with-sysroot \
    --disable-nls --disable-werror
make -j"$(nproc)"
make install
cd ..

mkdir build-gcc && cd build-gcc
../"gcc-$GCC_VER"/configure \
    --target=$TARGET --prefix="$PREFIX" \
    --disable-nls --enable-languages=c,c++ --without-headers
make -j"$(nproc)" all-gcc all-target-libgcc
make install-gcc install-target-libgcc

echo 'export PATH=/opt/cross/bin:$PATH' > /etc/profile.d/cross.sh
chmod +x /etc/profile.d/cross.sh
echo
echo "installed. open a new shell (or: source /etc/profile.d/cross.sh) then 'make re'."

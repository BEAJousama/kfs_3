#!/usr/bin/env bash
#
# Provisions a build VM for the KFS kernels:
#   - build tooling (nasm, grub-pc-bin, xorriso, qemu, ...)
#   - an i686-elf cross toolchain in /opt/cross  (binutils + gcc, C and C++)
#
# Idempotent: re-running skips the ~30-45 min cross-compile if it is already
# installed. Shared by kfs_2 and kfs3 (identical copy in each repo).

set -euo pipefail

TARGET=i686-elf
PREFIX=/opt/cross
BINUTILS_VER=2.41
GCC_VER=13.2.0

export DEBIAN_FRONTEND=noninteractive

echo "==> apt packages"
apt-get update -y
apt-get install -y --no-install-recommends \
    build-essential bison flex texinfo \
    libgmp3-dev libmpc-dev libmpfr-dev libisl-dev \
    nasm xorriso grub-pc-bin grub-common mtools \
    qemu-system-x86 \
    curl ca-certificates make file xz-utils

if [ -x "$PREFIX/bin/$TARGET-gcc" ]; then
    echo "==> cross toolchain already at $PREFIX - skipping build"
else
    echo "==> building $TARGET cross toolchain (this takes a while)"
    SRC=/usr/local/src/cross
    mkdir -p "$SRC"
    cd "$SRC"

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
    cd ..
fi

echo 'export PATH=/opt/cross/bin:$PATH' > /etc/profile.d/cross.sh
chmod +x /etc/profile.d/cross.sh

echo "==> done"
"$PREFIX/bin/$TARGET-gcc" --version | head -1
nasm --version
echo
echo "build a kernel with:  cd /home/vagrant/projects/kfs_2 && make"
echo "                      cd /home/vagrant/projects/kfs3  && make"

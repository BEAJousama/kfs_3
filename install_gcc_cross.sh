#!/bin/bash
set -e

su - obeaj <<'EOF'
    set -e
    export TARGET=i686-elf
    export PREFIX=$HOME/opt/cross
    export PATH=$PREFIX/bin:$PATH

    mkdir -p $HOME/src
    cd $HOME/src

    # Download sources
    curl -O https://ftp.gnu.org/gnu/binutils/binutils-2.41.tar.xz
    curl -O https://ftp.gnu.org/gnu/gcc/gcc-13.2.0/gcc-13.2.0.tar.xz

    tar -xf binutils-2.41.tar.xz
    tar -xf gcc-13.2.0.tar.xz

    # Build Binutils
    mkdir -p binutils-build
    cd binutils-build
    ../binutils-2.41/configure --target=$TARGET --prefix=$PREFIX --with-sysroot --disable-nls --disable-werror
    make
    make install
    cd ..

    # Build GCC
    cd gcc-13.2.0
    ./contrib/download_prerequisites
    cd ..
    mkdir -p gcc-build
    cd gcc-build
    ../gcc-13.2.0/configure --target=$TARGET --prefix=$PREFIX --disable-nls --enable-languages=c --without-headers
    make all-gcc
    make all-target-libgcc
    make install-gcc
    make install-target-libgcc

    export PATH="$HOME/opt/cross/bin:$PATH"
    source ~/.bashrc
EOF

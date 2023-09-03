#!/bin/bash
set -x # show cmds
set -e # fail globally
export PREFIX="$HOME/opt/cross"
export TARGET=i686-elf
export PATH="$PREFIX/bin:$PATH"
mkdir -p "$PREFIX"
mkdir -p temporarydir
cd temporarydir
wget -nc https://ftp.gnu.org/gnu/binutils/binutils-2.38.tar.gz
tar xpvf binutils-*.tar.gz
wget -nc https://ftp.gnu.org/gnu/gcc/gcc-11.4.0/gcc-11.4.0.tar.gz
tar xpvf gcc-*.tar.gz
cd binutils-2.38
mkdir -p build
cd build
../configure --target=$TARGET --prefix="$PREFIX" --with-sysroot --disable-nls --disable-werror
make
make install
cd ../../
cd gcc-11.4.0
mkdir -p build
cd build
../configure --target=$TARGET --prefix="$PREFIX" --disable-nls --enable-languages=c,c++ --without-headers
make all-gcc
make all-target-libgcc
make install-gcc
make install-target-libgcc
cd ../../

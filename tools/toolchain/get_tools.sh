#!/usr/bin/env bash
set -x # show cmds
set -e # fail globally

# Know where we at :p
SCRIPT=$(realpath "$0")
SCRIPTPATH=$(dirname "$SCRIPT")
cd "${SCRIPTPATH}"

export PREFIX="$HOME/opt/cross"
export TARGET=x86_64-cavos
export SYSROOT="$SCRIPTPATH/../../target"
export PATH="$PREFIX/bin:$PATH"

mkdir -p "$PREFIX"

# Ensure autotools
if ! test -f "$HOME/opt/autotools_gcc/bin/automake" || ! test -f "$HOME/opt/autotools_gcc/bin/autoconf"; then
	chmod +x get_autotools.sh
	./get_autotools.sh "1.15.1" "2.69" "$HOME/opt/autotools_gcc"
fi

export PATH=$HOME/opt/autotools_gcc/bin:$PATH

mkdir -p temporarydir
cd temporarydir

# Tarballs
wget -nc https://ftp.gnu.org/gnu/binutils/binutils-2.38.tar.gz
tar xpvf binutils-*.tar.gz
wget -nc https://ftp.gnu.org/gnu/gcc/gcc-11.4.0/gcc-11.4.0.tar.gz
tar xpvf gcc-*.tar.gz

# Binutils
cd binutils-2.38

# Binutils: Pre-build
patch -p1 <"${SCRIPTPATH}/../../patches/binutils.diff"
cd ld/
automake
cd ../

mkdir -p build
cd build
../configure --target="$TARGET" --prefix="$PREFIX" --with-sysroot="$SYSROOT" --enable-shared
make -j$(nproc)
make install
cd ../../

# GCC
cd gcc-11.4.0

# GCC: Pre-build
patch -p1 <"${SCRIPTPATH}/../../patches/gcc.diff"
cd libstdc++-v3/
autoconf
cd ../

./contrib/download_prerequisites

mkdir -p build
cd build
../configure --target="$TARGET" --prefix="$PREFIX" --enable-languages=c,c++ --with-sysroot="$SYSROOT" --enable-shared
make all-gcc -j$(nproc)
make all-target-libgcc -j$(nproc)
make install-gcc
make install-target-libgcc

# Building musl alone is essential before libstdc++ is built & installed
chmod +x "${SCRIPTPATH}/../../src/libs/musl/build.sh"
"${SCRIPTPATH}/../../src/libs/musl/build.sh" --noreplace

make all-target-libstdc++-v3 -j$(nproc)
make install-target-libstdc++-v3

cd ../../

cd ../
rm -rf temporarydir

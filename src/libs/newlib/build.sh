#!/bin/bash
set -x # show cmds
set -e # fail globally

# Know where we at :p
SCRIPT=$(realpath "$0")
SCRIPTPATH=$(dirname "$SCRIPT")

cd "${SCRIPTPATH}"

# --noreplace -> won't re-compile if it finds libc
if test -f "$SCRIPTPATH/cavos-out/i686-cavos/lib/libc.a"; then
	if [ "$#" -eq 1 ]; then
		exit 0
	fi
fi

# Ensure we have the sources!
if ! test -f "$SCRIPTPATH/newlib-4.1.0/README"; then
	wget -nc ftp://sourceware.org/pub/newlib/newlib-4.1.0.tar.gz
	tar xpvf newlib-4.1.0.tar.gz
	cd newlib-4.1.0
	patch -p1 <../patches/main.patch
	cd ../
fi

export PREFIX="$HOME/opt/autotools_newlib"

# No specific autotools versions installed
if ! test -f "$PREFIX/bin/automake"; then
	mkdir -p "$PREFIX"

	mkdir -p temporarydir
	cd temporarydir

	wget -nc https://ftp.gnu.org/gnu/automake/automake-1.11.tar.gz
	tar xpvf automake-1.11.tar.gz
	cd automake-1.11
	mkdir -p build
	cd build
	../configure --prefix="$PREFIX"
	make
	make install
	cd ../../

	wget -nc https://ftp.gnu.org/gnu/autoconf/autoconf-2.65.tar.gz
	tar xpvf autoconf-2.65.tar.gz
	cd autoconf-2.65
	mkdir -p build
	cd build
	../configure --prefix="$PREFIX"
	make
	make install
	cd ../../

	cd ../
	rm -rf temporarydir
fi

# Ensure everything's in PATH
if [[ ":$PATH:" != *":$PREFIX/bin:"* ]]; then
	export PATH=$PREFIX/bin:$PATH
fi

if [[ ":$PATH:" != *":$HOME/opt/cross/bin:"* ]]; then
	export PATH=$HOME/opt/cross/bin:$PATH
fi

# Optional, since we're matching versions
cd "${SCRIPTPATH}/newlib-4.1.0/newlib/libc/sys"
autoconf
cd "${SCRIPTPATH}/newlib-4.1.0/newlib/libc/sys/cavos"
autoreconf
cd "${SCRIPTPATH}"

# Booo! Scary!
export PREFIX="${SCRIPTPATH}/cavos-out"
mkdir -p cavos-build
cd cavos-build
../newlib-4.1.0/configure --prefix="$PREFIX" --target=i686-cavos
make clean
make all -j$(nproc)
make install

# Copy libraries (and update headers)
mkdir -p "$SCRIPTPATH/../target/usr/"
cp -r "$PREFIX/i686-cavos/"* "$SCRIPTPATH/../../../target/usr/"

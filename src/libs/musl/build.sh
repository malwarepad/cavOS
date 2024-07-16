#!/bin/bash
set -x # show cmds
set -e # fail globally

# Know where we at :p
SCRIPT=$(realpath "$0")
SCRIPTPATH=$(dirname "$SCRIPT")

MUSL_RELEASE="musl-1.2.5"

cd "${SCRIPTPATH}"

# --noreplace -> won't re-compile if it finds libc
if test -f "$SCRIPTPATH/cavos-out/lib/libc.a"; then
	if [ "$#" -eq 1 ]; then
		exit 0
	fi
fi

# Ensure we have the sources!
if ! test -f "$SCRIPTPATH/$MUSL_RELEASE/README"; then
	wget -nc "https://musl.libc.org/releases/$MUSL_RELEASE.tar.gz"
	tar xpvf "$MUSL_RELEASE.tar.gz"

	# No patches needed!
	# cd "$MUSL_RELEASE"
	# patch -p1 < xyz
	# cd ../
fi

# Ensure the cavOS toolchain's in PATH
if [[ ":$PATH:" != *":$HOME/opt/cross/bin:"* ]]; then
	export PATH=$HOME/opt/cross/bin:$PATH
fi

# Booo! Scary!
export PREFIX="${SCRIPTPATH}/cavos-out"
mkdir -p cavos-build
cd cavos-build
CC=x86_64-cavos-gcc ARCH=x86_64 CROSS_COMPILE=x86_64-cavos- "../$MUSL_RELEASE/configure" --target=x86_64-cavos --build=x86_64-cavos --host=x86_64-cavos --prefix="$PREFIX" --syslibdir="/lib" --enable-debug
make clean
make all -j$(nproc)
make install

# Copy libraries (and update headers)
mkdir -p "$SCRIPTPATH/../target/usr/"
cp -r "$PREFIX/lib" "$PREFIX/include" "$SCRIPTPATH/../../../target/usr/"

# libc.so (dynamic deafult) fixup (just use -static man)
# mv "$SCRIPTPATH/../../../target/usr/lib/libc.so" "$SCRIPTPATH/../../../target/usr/lib/libc.1.so"

# crt0 fixup
ln -sf "$SCRIPTPATH/../../../target/usr/lib/crt1.o" "$SCRIPTPATH/../../../target/usr/lib/crt0.o"

# required for proper dynamic linking
mkdir -p "$SCRIPTPATH/../../../target/lib/"
cp "$SCRIPTPATH/../../../target/usr/lib/libc.so" "$SCRIPTPATH/../../../target/lib/ld64.so.1"

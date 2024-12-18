#!/bin/bash
set -x # show cmds
set -e # fail globally

# Know where we at :p
SCRIPT=$(realpath "$0")
SCRIPTPATH=$(dirname "$SCRIPT")

# Useful
USR_PATHNAME="${SCRIPTPATH}/../../target/usr/"
PATCH_PATHNAME="${SCRIPTPATH}/../../patches"

# build_package_autotools($uri, $install_dir, $config_sub_path, $extra_parameters, $optional_patchname, $extra_install_parameters, $autoconf_extra)
source "${SCRIPTPATH}/../shared/cross_compile.sh"

# Some important stuff...
if [ ! -d "$USR_PATHNAME/../bin" ]; then
	cd "$USR_PATHNAME/../"
	ln -s usr/bin bin
fi
cd "$USR_PATHNAME/../../"

# Make sure we use GNU bash for our /bin/sh
if [ ! -f "$USR_PATHNAME/bin/sh" ]; then
	mkdir -p "$USR_PATHNAME/bin"
	ln -s bash "$USR_PATHNAME/bin/sh"
fi

# Make sure we are using the correct timezone
HOST_TIMEZONE=$(readlink -f /etc/localtime)
TARGET_TIMEZONE=$(readlink -f "$USR_PATHNAME/../etc/localtime" || echo definitely_not_lol)
if [ "${HOST_TIMEZONE}" != "${TARGET_TIMEZONE}" ]; then
	rm -f "$TARGET_TIMEZONE"
	ln -s "$HOST_TIMEZONE" "$TARGET_TIMEZONE"
fi

# GNU ncurses (useful library)
if [ ! -f "$USR_PATHNAME/bin/clear" ]; then
	build_package_autotools https://ftp.gnu.org/gnu/ncurses/ncurses-6.5.tar.gz "/usr" config.sub "--with-build-sysroot='$USR_PATHNAME' --with-sysroot=/" "" "DESTDIR='$USR_PATHNAME/../'"
	cp "$USR_PATHNAME/lib/libncursesw.a" "$USR_PATHNAME/lib/libncurses.a" # fat32 doesn't support ln
fi

# GNU Bash (statically compiled)
if [ ! -f "$USR_PATHNAME/bin/bash" ]; then
	build_package_autotools https://ftp.gnu.org/gnu/bash/bash-5.2.tar.gz "$USR_PATHNAME" support/config.sub "host_alias=x86_64-cavos --without-bash-malloc --enable-static-link" "$PATCH_PATHNAME/bash.diff" "" "autoconf -f"
fi

# GNU Coreutils (ls, pwd, whoami, etc)
if [ ! -f "$USR_PATHNAME/bin/ls" ]; then
	build_package_autotools https://ftp.gnu.org/gnu/coreutils/coreutils-9.5.tar.xz "$USR_PATHNAME" build-aux/config.sub
fi

# GNU findutils (find, etc)
if [ ! -f "$USR_PATHNAME/bin/find" ]; then
	build_package_autotools https://ftp.gnu.org/gnu/findutils/findutils-4.10.0.tar.xz "$USR_PATHNAME" build-aux/config.sub
fi

# GNU M4
if [ ! -f "$USR_PATHNAME/bin/m4" ]; then
	build_package_autotools https://ftp.gnu.org/gnu/m4/m4-1.4.19.tar.xz "$USR_PATHNAME" build-aux/config.sub
fi

# GNU grep
if [ ! -f "$USR_PATHNAME/bin/grep" ]; then
	build_package_autotools https://ftp.gnu.org/gnu/grep/grep-3.11.tar.xz "$USR_PATHNAME" build-aux/config.sub
fi

# GNU diffutils (diff)
if [ ! -f "$USR_PATHNAME/bin/diff" ]; then
	build_package_autotools https://ftp.gnu.org/gnu/diffutils/diffutils-3.10.tar.xz "$USR_PATHNAME" build-aux/config.sub
fi

# File (doesn't like relative paths for some reason)
if [ ! -f "$USR_PATHNAME/bin/file" ]; then
	USR_PATHNAME_FIXED=$(readlink -e "$USR_PATHNAME")
	DESTDIR_FIXED=$(readlink -e "$USR_PATHNAME/../")
	build_package_autotools https://astron.com/pub/file/file-5.45.tar.gz "$USR_PATHNAME_FIXED" config.sub "--prefix /usr" "" "DESTDIR=$DESTDIR_FIXED" "mkdir build && cd build && ../configure --disable-bzlib --disable-libseccomp --disable-xzlib --disable-zlib && make -j$(nproc) && cd .."
fi

# GNU Gawk
if [ ! -f "$USR_PATHNAME/bin/gawk" ]; then
	build_package_autotools https://ftp.gnu.org/gnu/gawk/gawk-5.3.0.tar.xz "$USR_PATHNAME" build-aux/config.sub
fi

# GNU gzip
if [ ! -f "$USR_PATHNAME/bin/gzip" ]; then
	build_package_autotools https://ftp.gnu.org/gnu/gzip/gzip-1.13.tar.xz "$USR_PATHNAME" build-aux/config.sub
fi

# GNUmake (I love make (sometimes (lie)))
if [ ! -f "$USR_PATHNAME/bin/make" ]; then
	build_package_autotools https://ftp.gnu.org/gnu/make/make-4.4.1.tar.gz "$USR_PATHNAME" build-aux/config.sub "--without-guile"
fi

# GNU patch
if [ ! -f "$USR_PATHNAME/bin/patch" ]; then
	build_package_autotools https://ftp.gnu.org/gnu/patch/patch-2.7.6.tar.xz "$USR_PATHNAME" build-aux/config.sub "" "$PATCH_PATHNAME/patch.diff"
fi

# GNU sed (I actually like this one)
if [ ! -f "$USR_PATHNAME/bin/sed" ]; then
	build_package_autotools https://ftp.gnu.org/gnu/sed/sed-4.9.tar.xz "$USR_PATHNAME" build-aux/config.sub
fi

# GNU tar
if [ ! -f "$USR_PATHNAME/bin/tar" ]; then
	build_package_autotools https://ftp.gnu.org/gnu/tar/tar-1.35.tar.xz "$USR_PATHNAME" build-aux/config.sub
fi

# xz-utils (scary, SCARY!)
if [ ! -f "$USR_PATHNAME/bin/xz" ]; then
	build_package_autotools https://github.com/tukaani-project/xz/releases/download/v5.4.6/xz-5.4.6.tar.xz "$USR_PATHNAME" build-aux/config.sub
	rm -f "$USR_PATHNAME/lib/liblzma.la"
fi

# GNU binutils (we're close!)
if [ ! -f "$USR_PATHNAME/bin/readelf" ]; then
	export PATH=$HOME/opt/autotools_gcc/bin:$PATH
	build_package_autotools https://ftp.gnu.org/gnu/binutils/binutils-2.38.tar.gz "$USR_PATHNAME" config.sub '--target=x86_64-cavos --prefix=/usr --enable-shared --disable-werror --enable-64-bit-bfd' "$PATCH_PATHNAME/binutils.diff" "DESTDIR=$USR_PATHNAME/../" "cd ld && automake && cd .."
fi

# GCC (for real this time, this is not a cross compiler)
if [ ! -f "$USR_PATHNAME/bin/gcc" ]; then
	export PATH=$HOME/opt/autotools_gcc/bin:$PATH
	build_package_autotools https://ftp.gnu.org/gnu/gcc/gcc-11.4.0/gcc-11.4.0.tar.gz "$USR_PATHNAME" config.sub '--host=x86_64-cavos --target=x86_64-cavos --prefix=/usr --enable-shared --disable-werror --enable-languages=c,c++' "$PATCH_PATHNAME/gcc.diff" "DESTDIR=$USR_PATHNAME/../" "cd libstdc++-v3 && autoconf && cd .. && ./contrib/download_prerequisites && sed -i 's/\-gnu\* | \-bsd\*/\-cavos\*/g' ./gmp/configfsf.sub && sed -i 's/\-gnu\* | \-bsd\*/\-cavos\*/g' ./mpfr/config.sub && sed -i 's/\-gnu\* | \-bsd\*/\-cavos\*/g' ./mpc/config.sub && sed -i 's/\-gnu\* | \-bsd\*/\-cavos\*/g' ./isl/config.sub"
fi

# Tzdata (pre-installed because we don't have Glibc's zic)
if [ ! -f "$USR_PATHNAME/share/zoneinfo/Europe/Amsterdam" ]; then
	mkdir timezonestuff
	cd timezonestuff

	wget http://ftp.us.debian.org/debian/pool/main/t/tzdata/tzdata_2024b-3_all.deb
	ar x tzdata_2024b-3_all.deb
	tar -xvf data.tar.xz

	mkdir -p "$USR_PATHNAME/share/zoneinfo/"
	cp -r usr/share/zoneinfo/* "$USR_PATHNAME/share/zoneinfo/"

	cd ..
	rm -rf timezonestuff
fi

# By this point, we have all compiler tools we need inside the target system...
# Hence, we can begin to use it for targetted compilations, instead of cross-compilations like so:

source "${SCRIPTPATH}/../shared/chroot.sh"
# chroot_establish "$USR_PATHNAME/../"
# cd "$USR_PATHNAME/../"

# The netwide assembler (nasm)
# if [ ! -f "$USR_PATHNAME/bin/nasm" ]; then
# 	wget -nc https://www.nasm.us/pub/nasm/releasebuilds/2.15.05/nasm-2.15.05.tar.xz
# 	tar xpvf nasm-2.15.05.tar.xz
# 	sudo chroot "$TARGET_DIR/" /usr/bin/bash -c "cd /nasm-2.15.05 && ./configure --prefix=/usr && make -j$(nproc) && make install && cd / && rm -rf /nasm-2.15.05 /nasm-2.15.05.tar.xz"
# fi

# Linux headers (needed for a lot of software)
# if [ ! -f "$USR_PATHNAME/include/asm/byteorder.h" ]; then
# 	wget -nc https://www.kernel.org/pub/linux/kernel/v6.x/linux-6.6.41.tar.xz
# 	tar xpvf linux-6.6.41.tar.xz
# 	cd linux-6.6.41
# 	make mrproper
# 	make headers
# 	find usr/include -type f ! -name '*.h' -delete
# 	cp -r usr/include/* "$USR_PATHNAME/include/"
# 	cd ..
# 	rm -rf linux-6.6.41 linux-6.6.41.tar.xz
# fi

# PCI utilities (lspci, update-pciids, etc)
# if [ ! -f "$USR_PATHNAME/sbin/lspci" ]; then
# 	wget -nc https://www.kernel.org/pub/software/utils/pciutils/pciutils-3.7.0.tar.xz
# 	tar xpvf pciutils-3.7.0.tar.xz
# 	sudo chroot "$TARGET_DIR/" /usr/bin/bash -c "cd /pciutils-3.7.0 && make PREFIX=/usr SHAREDIR=/usr/share/hwdata -j$(nproc) && make PREFIX=/usr SHAREDIR=/usr/share/hwdata install install-lib && cd / && rm -rf /pciutils-3.7.0 /pciutils-3.7.0.tar.xz"
# fi

# pkg-config
# if [ ! -f "$USR_PATHNAME/bin/pkg-config" ]; then
# 	wget -nc http://pkgconfig.freedesktop.org/releases/pkg-config-0.29.2.tar.gz
# 	tar xpvf pkg-config-0.29.2.tar.gz
# 	cd pkg-config-0.29.2
# 	patch -p1 <../../patches/pkg-config.diff
# 	cd ..
# 	sudo chroot "$TARGET_DIR/" /usr/bin/bash -c "cd /pkg-config-0.29.2 && ./configure --prefix=/usr --with-internal-glib --disable-host-tool && make -j$(nproc) && make install && cd / && rm -rf /pkg-config-0.29.2 /pkg-config-0.29.2.tar.gz"
# fi

# Vim (fuck nano)
# if [ ! -f "$USR_PATHNAME/bin/vim" ]; then
# 	wget -nc https://github.com/vim/vim/archive/v9.1.0041/vim-9.1.0041.tar.gz
# 	tar xpvf vim-9.1.0041.tar.gz
# 	echo '#define SYS_VIMRC_FILE  "/etc/vimrc"' >>vim-9.1.0041/src/feature.h
# 	echo '#define SYS_GVIMRC_FILE "/etc/gvimrc"' >>vim-9.1.0041/src/feature.h
# 	sudo chroot "$TARGET_DIR/" /usr/bin/bash -c "cd /vim-9.1.0041 && ./configure --prefix=/usr --with-tlib=ncursesw && make -j$(nproc) && make install && cd / && rm -rf /vim-9.1.0041 /vim-9.1.0041.tar.gz"
# fi

chroot_drop "$USR_PATHNAME/../"

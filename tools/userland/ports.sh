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

# GNU ncurses (useful library)
if [ ! -f "$USR_PATHNAME/bin/clear" ]; then
	build_package_autotools https://ftp.gnu.org/gnu/ncurses/ncurses-6.5.tar.gz "/usr" config.sub "--with-build-sysroot='$USR_PATHNAME' --with-sysroot=/" "" "DESTDIR='$USR_PATHNAME/../'"
	cp "$USR_PATHNAME/lib/libncursesw.a" "$USR_PATHNAME/lib/libncurses.a" # fat32 doesn't support ln
fi

# GNU Bash (statically compiled)
if [ ! -f "$USR_PATHNAME/bin/bash" ]; then
	build_package_autotools https://ftp.gnu.org/gnu/bash/bash-5.2.tar.gz "$USR_PATHNAME" support/config.sub "host_alias=x86_64-cavos --without-bash-malloc --enable-static-link" "$PATCH_PATHNAME/bash.diff" "" yes
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

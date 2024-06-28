#!/bin/bash
set -x # show cmds
set -e # fail globally

# Know where we at :p
SCRIPT=$(realpath "$0")
SCRIPTPATH=$(dirname "$SCRIPT")

# Useful
USR_PATHNAME="${SCRIPTPATH}/../../target/usr/"

# build_package_autotools($uri, $install_dir, $config_sub_path, $extra_parameters, $optional_patchname, $extra_install_parameters, $autoconf_extra)
source "${SCRIPTPATH}/../shared/cross_compile.sh"

# GNU Bash (statically compiled)
if [ ! -f "$USR_PATHNAME/bin/bash" ]; then
	build_package_autotools https://ftp.gnu.org/gnu/bash/bash-5.2.tar.gz "$USR_PATHNAME" support/config.sub "host_alias=x86_64-cavos --without-bash-malloc --enable-static-link" ~/dev/cavOS/patches/bash.diff "" yes
fi

# GNU Coreutils (ls, pwd, whoami, etc)
if [ ! -f "$USR_PATHNAME/bin/ls" ]; then
	build_package_autotools https://ftp.gnu.org/gnu/coreutils/coreutils-9.5.tar.xz "$USR_PATHNAME" build-aux/config.sub
fi

# GNU ncurses (useful library)
if [ ! -f "$USR_PATHNAME/bin/clear" ]; then
	build_package_autotools https://ftp.gnu.org/gnu/ncurses/ncurses-6.5.tar.gz "/usr" config.sub "--with-build-sysroot='$USR_PATHNAME' --with-sysroot=/" "" "DESTDIR='$USR_PATHNAME/../'"
fi

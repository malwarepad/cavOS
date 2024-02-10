#!/bin/bash
set -x # show cmds
set -e # fail globally

if [ "$#" -ne 3 ]; then
	echo "Usage: $0 <automake-version> <autoconf-version> <install-prefix>"
	exit 1
fi

automake_version=$1
autoconf_version=$2
install_prefix=$3

if [ -e "$install_prefix" ]; then
	exit 0
fi

mkdir -p "$install_prefix"

mkdir tmp_get_autotools
cd tmp_get_autotools

build_package() {
	package_name=$1
	version=$2
	install_dir=$3

	wget "https://ftp.gnu.org/gnu/$package_name/$package_name-$version.tar.gz"
	tar -xf "$package_name-$version.tar.gz"
	cd "$package_name-$version"
	./configure --prefix="$install_dir"
	make
	make install
	cd ..
}

build_package "automake" "$automake_version" "$install_prefix"
build_package "autoconf" "$autoconf_version" "$install_prefix"

cd ..
rm -rf tmp_get_autotools

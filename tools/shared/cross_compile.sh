#!/usr/bin/env bash
set -x # show cmds
set -e # fail globally

# Cross compile an autotools package for our custom target (x86_64-cavos)
build_package_autotools() {
	# Ensure the cavOS toolchain's in PATH
	if [[ ":$PATH:" != *":$HOME/opt/cross/bin:"* ]]; then
		export PATH=$HOME/opt/cross/bin:$PATH
	fi

	# Arguments
	uri=$1
	filename=$(echo "$uri" | sed 's/.*\///g')
	foldername=$(echo "$filename" | sed 's/\.tar.*//g')
	install_dir=$2
	config_sub_path=$3

	# EXTRA arguments
	extra_parameters=$4
	optional_patchname=$5
	extra_install_parameters=$6
	autoconf_extra=$7

	# Download and extract the tarball
	wget -nc "$uri"
	tar xpvf "$filename"
	cd "$foldername"

	# Add our target
	sed -i 's/\# Now accept the basic system types\./cavos\*\);;/g' "$config_sub_path"

	# Do any optional patches
	if [ -n "$optional_patchname" ]; then
		patch -p1 <"$optional_patchname"
	fi

	# Just in case it's needed
	if [ -n "$autoconf_extra" ]; then
		autoconf -f
	fi

	# Use a separate directory for compiling (good practice)
	mkdir -p build
	cd build

	# Compilation itself
	../configure --prefix="$install_dir" --host=x86_64-cavos "$extra_parameters"
	make -j$(nproc)
	if [ -n "$extra_install_parameters" ]; then
		make install "$extra_install_parameters"
	else
		make install
	fi

	# Cleanup
	cd ../../
	rm -rf "$foldername"
	rm -f "$filename"
}

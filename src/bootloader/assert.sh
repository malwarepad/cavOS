#!/usr/bin/env bash
set -x # show cmds
set -e # fail globally

# Know where we at :p
SCRIPT=$(realpath "$0")
SCRIPTPATH=$(dirname "$SCRIPT")
cd "${SCRIPTPATH}"

TARGET_VERSION="9.2.3"

fetchLimine() {
	rm -rf limine/
	git clone https://github.com/limine-bootloader/limine.git --branch=v9.x-binary --depth=1
	cd limine/
	make
	cd ../
	echo "$TARGET_VERSION" >.version
}

if [[ ! -f ".version" ]] || ! cat .version | grep -i "$TARGET_VERSION" || [[ ! -f "limine/limine" ]]; then
	fetchLimine
fi

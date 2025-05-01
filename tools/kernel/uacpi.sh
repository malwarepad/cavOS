#!/usr/bin/env bash
set -x # show cmds
set -e # fail globally

# Know where we at :p
SCRIPT=$(realpath "$0")
SCRIPTPATH=$(dirname "$SCRIPT")
PROJECT_ROOT="${SCRIPTPATH}/../../"
cd "${PROJECT_ROOT}/src/kernel/acpi/"

VERSION_FILE="$PROJECT_ROOT/.version_uacpi"
TARGET_VERSION="2.1.1"

fetchUACPI() {
	rm -rf uACPI/ repo/ uacpi/ ../include/uacpi/
	mkdir -p uacpi/
	wget "https://github.com/uACPI/uACPI/archive/refs/tags/$TARGET_VERSION.tar.gz" -O "$TARGET_VERSION.tar.gz"
	tar xpf "$TARGET_VERSION.tar.gz"
	rm -f "$TARGET_VERSION.tar.gz"
	mv "uACPI-$TARGET_VERSION" repo/
	mv "repo/source/"* uacpi/
	mv "repo/include/"* ../include/
	rm -rf uACPI/ repo/
	echo "$TARGET_VERSION" >"$VERSION_FILE"
}

if [[ ! -f "$VERSION_FILE" ]] || ! cat "$VERSION_FILE" | grep -i "$TARGET_VERSION" || [[ ! -d "uacpi" ]]; then
	fetchUACPI
fi

#!/usr/bin/env bash
set -x # show cmds
set -e # fail globally

# Know where we at :p
SCRIPT=$(realpath "$0")
SCRIPTPATH=$(dirname "$SCRIPT")
PROJECT_ROOT="${SCRIPTPATH}/../../"
cd "${PROJECT_ROOT}/src/kernel/acpi/"

VERSION_FILE="$PROJECT_ROOT/.version_uacpi"

# since the uACPI project doesn't have versions,
# I am forced to use an ugly hack like so
TARGET_VERSION="1"

fetchUACPI() {
	rm -rf uACPI/ repo/ uacpi/ ../include/uacpi/
	mkdir -p uacpi/
	git clone https://github.com/UltraOS/uACPI repo
	mv "repo/source/"* uacpi/
	mv "repo/include/"* ../include/
	rm -rf uACPI/ repo/
	echo "$TARGET_VERSION" >"$VERSION_FILE"
}

if [[ ! -f "$VERSION_FILE" ]] || ! cat "$VERSION_FILE" | grep -i "$TARGET_VERSION" || [[ ! -d "uacpi" ]]; then
	fetchUACPI
fi

#!/usr/bin/env bash
# set -x # show cmds
set -e # fail globally

# Embedded clang-format-19 (verified sha512sum from GitHub actions)
CLANG_FORMAT_URI="https://github.com/muttleyxd/clang-tools-static-binaries/releases/download/master-46b8640/clang-format-19_linux-amd64"
CLANG_FORMAT_SHA512="7fbbc586949d7dda7488b42a8fe2bd9cbe31e4eed9b57adf857f62a61d29c86d51372d01561d1be6a4288ee62dc888acb9477a15ff8c2dacc7da4b1b705c2aa1"
CLANG_FORMAT_TARGET="$HOME/opt/clang-format-cavos"

# Initial download (if needed)
if [ ! -f "$CLANG_FORMAT_TARGET" ]; then
	wget -nc -O "$CLANG_FORMAT_TARGET" "$CLANG_FORMAT_URI"
fi

# Pass 1. If this fails, just redownload and re-check
if [ "$(sha512sum "$CLANG_FORMAT_TARGET" | sed 's/ .*//g')" != "$CLANG_FORMAT_SHA512" ]; then
	# Pass 2. Re-download, re-check. might be just an update
	rm -f "$CLANG_FORMAT_TARGET"
	wget -nc -O "$CLANG_FORMAT_TARGET" "$CLANG_FORMAT_URI"
	if [ "$(sha512sum "$CLANG_FORMAT_TARGET" | sed 's/ .*//g')" != "$CLANG_FORMAT_SHA512" ]; then
		# Now it's actually malicious. Discard binary and exit
		rm -f "$CLANG_FORMAT_TARGET"
		echo "Binary failed sha256 validation! Report immidiately."
		exit 1
	fi
fi

# Just in case
chmod +x "$CLANG_FORMAT_TARGET"

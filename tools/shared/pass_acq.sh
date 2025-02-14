#!/usr/bin/env bash
# set -x # show cmds
set -e # fail globally

PASS_ACQ_URI=$1
PASS_ACQ_SHA512=$2
PASS_ACQ_TARGET=$3

# Initial download (if needed)
if [ ! -f "$PASS_ACQ_TARGET" ]; then
	wget -nc -O "$PASS_ACQ_TARGET" "$PASS_ACQ_URI"
fi

# Pass 1. If this fails, just redownload and re-check
if [ "$(sha512sum "$PASS_ACQ_TARGET" | sed 's/ .*//g')" != "$PASS_ACQ_SHA512" ]; then
	# Pass 2. Re-download, re-check. might be just an update
	rm -f "$PASS_ACQ_TARGET"
	wget -nc -O "$PASS_ACQ_TARGET" "$PASS_ACQ_URI"
	if [ "$(sha512sum "$PASS_ACQ_TARGET" | sed 's/ .*//g')" != "$PASS_ACQ_SHA512" ]; then
		# Now it's actually malicious. Discard binary and exit
		rm -f "$PASS_ACQ_TARGET"
		echo "Binary failed sha256 validation! Report immidiately."
		exit 1
	fi
fi

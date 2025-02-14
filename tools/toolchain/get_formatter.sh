#!/usr/bin/env bash
# set -x # show cmds
set -e # fail globally

# Embedded clang-format-19 (verified sha512sum from GitHub actions)
CLANG_FORMAT_URI="https://github.com/muttleyxd/clang-tools-static-binaries/releases/download/master-46b8640/clang-format-19_linux-amd64"
CLANG_FORMAT_SHA512="7fbbc586949d7dda7488b42a8fe2bd9cbe31e4eed9b57adf857f62a61d29c86d51372d01561d1be6a4288ee62dc888acb9477a15ff8c2dacc7da4b1b705c2aa1"
CLANG_FORMAT_TARGET="$HOME/opt/clang-format-cavos"

chmod +x "tools/shared/pass_acq.sh"
tools/shared/pass_acq.sh "$CLANG_FORMAT_URI" "$CLANG_FORMAT_SHA512" "$CLANG_FORMAT_TARGET"

# Just in case
chmod +x "$CLANG_FORMAT_TARGET"

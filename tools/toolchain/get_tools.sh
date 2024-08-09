#!/usr/bin/env bash

# Know where we at :p
SCRIPT=$(realpath "$0")
export SCRIPTPATH=$(dirname "$SCRIPT")
cd "${SCRIPTPATH}"

targets=("i386-cavos" "x86_64-cavos")

for i in "${targets[@]}"; do
    export TARGET=$i

    bash make_tool.sh
done
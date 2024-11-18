#!/bin/sh
shopt -s globstar

SCRIPT_DIR=$( cd -- "$( dirname -- "${BASH_SOURCE[0]}" )" &> /dev/null && pwd )

cd "$SCRIPT_DIR"

find . \( -name '*.hpp' -or -name '*.cpp' -or -name '*.mm' \) \( -path "./src/*" -or -path "./include/*" -or -path "./examples/*" \) -exec clang-format -i {} \;


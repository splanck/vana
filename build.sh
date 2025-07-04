#!/bin/bash
set -e

export PREFIX="$HOME/opt/cross"
export TARGET=i686-elf
export PATH="$PREFIX/bin:$PATH"

# Ensure the cross compiler tools exist in PATH
if ! command -v "$TARGET-gcc" >/dev/null 2>&1; then
  echo "Error: $TARGET-gcc not found in PATH. Install it or run build-toolchain.sh." >&2
  exit 1
fi

if ! command -v "$TARGET-ld" >/dev/null 2>&1; then
  echo "Error: $TARGET-ld not found in PATH. Install it or run build-toolchain.sh." >&2
  exit 1
fi

make all

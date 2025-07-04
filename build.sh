#!/usr/bin/env bash
set -e

export PREFIX="$HOME/opt/cross"
export TARGET=i686-elf
export PATH="$PREFIX/bin:$PATH"

# Ensure the cross compiler tools exist in PATH
if ! command -v i686-elf-gcc >/dev/null 2>&1; then
  echo "Error: i686-elf-gcc not found in PATH. Install it or run build-toolchain.sh." >&2
  exit 1
fi

if ! command -v i686-elf-ld >/dev/null 2>&1; then
  echo "Error: i686-elf-ld not found in PATH. Install it or run build-toolchain.sh." >&2
  exit 1
fi

make all

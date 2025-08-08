#!/bin/bash
set -e

# Usage:
#   ./build-toolchain.sh         # builds an i686-elf toolchain
#   TARGET=x86_64-elf ./build-toolchain.sh

export PREFIX="$HOME/opt/cross"
export TARGET="${TARGET:-i686-elf}"
export PATH="$PREFIX/bin:$PATH"
export BINVER="2.38"
export GCCVER="12.3.0"

# create and enter our source tree
SRCDIR="$HOME/source/$TARGET"
mkdir -p "$SRCDIR"
cd "$SRCDIR"

# install prerequisites
sudo apt-get update
sudo apt-get install -y \
  build-essential \
  bison flex \
  libgmp-dev libmpfr-dev libmpc-dev \
  libisl-dev zlib1g-dev libzstd-dev \
  texinfo python3 python3-pip \
  wget curl tar gzip bzip2 gawk m4 \
  libexpat1-dev libncurses-dev libelf-dev \
  autoconf automake libtool patchutils

# fetch sources
wget "https://ftp.gnu.org/gnu/binutils/binutils-$BINVER.tar.gz"
wget "https://ftp.gnu.org/gnu/gcc/gcc-$GCCVER/gcc-$GCCVER.tar.gz"

# extract
tar -xvzf "binutils-$BINVER.tar.gz"
tar -xvzf "gcc-$GCCVER.tar.gz"

#
# 1) build & install Binutils
#
mkdir -p build-binutils
cd build-binutils

../binutils-"$BINVER"/configure \
  --target="$TARGET" \
  --prefix="$PREFIX" \
  --with-sysroot \
  --disable-nls \
  --disable-werror

make
make install

#
# 2) build & install GCC (C and C++)
#
cd "$SRCDIR"     # back to the source root
mkdir -p build-gcc
cd build-gcc

../gcc-"$GCCVER"/configure \
  --target="$TARGET" \
  --prefix="$PREFIX" \
  --disable-nls \
  --enable-languages=c,c++ \
  --without-headers

make all-gcc
make all-target-libgcc

make install-gcc
make install-target-libgcc

echo "Cross-toolchain for $TARGET installed into $PREFIX"


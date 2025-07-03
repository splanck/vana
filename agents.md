## Prerequisites

- Ensure you have `nasm`, `i686-elf-gcc` (or compatible cross-compiler) and `i686-elf-ld` on your `PATH`.
- To bootstrap a cross-toolchain, simply run the root-level script:
  ```
  ./build-toolchain.sh
  ```
- The tools will be installed in $HOME/opt/cross when the build-toolchain.sh script completes. Be sure to give it several minutes since its building gcc and binutils.

You need to build the toolchain before running any builds or tests.



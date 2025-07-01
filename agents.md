## Prerequisites

- Ensure you have `nasm`, `i686-elf-gcc` (or compatible cross-compiler) and `i686-elf-ld` on your `PATH`.
- To bootstrap a cross-toolchain, simply run the root-level script:
  ```
  ./build-toolchain.sh
  ```
- The tools will be installed in $HOME/opt/cross when the build-toolchain.sh script completes. Be sure to give it several minutes since its building gcc and binutils.

You need to build the toolchain before running any builds or tests.

## Reference Kernel Examples

The `example_kernel_src/` directory contains small, self-contained demos of kernel subsystems (boot, C setup, simple syscall, VGA text, paging, etc.).
When implementing new features under `bootloader/` or `kernel/`, use those files as your guide—but do **not** modify anything in `example_kernel_src/`.

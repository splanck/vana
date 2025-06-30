## Prerequisites

- Ensure you have `nasm`, `i686-elf-gcc` (or compatible cross-compiler) and `i686-elf-ld` on your `PATH`.
- To bootstrap a cross-toolchain, simply run the root-level script:
  ```
  ./build-toolchain.sh
  ```

## Reference Kernel Examples

The `example_kernel_src/` directory contains small, self-contained demos of kernel subsystems (boot, C setup, simple syscall, VGA text, paging, etc.).
When implementing new features under `bootloader/` or `kernel/`, use those files as your guide—but do **not** modify anything in `example_kernel_src/`.

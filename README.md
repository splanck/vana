# Vana Kernel

Vana is an experimental 32‑bit x86 operating system written in C and assembly. It
implements a simple FAT16 bootloader, descriptor tables (GDT/IDT), paging based
memory management, a basic heap allocator, cooperative task scheduler and a
minimal set of system calls. User programs include a tiny shell and libc
routines to exercise the syscall interface.

For a deeper dive into the implementation details see
[docs/README.md](docs/README.md).

## Building the Cross‑Toolchain

The kernel requires the `i686-elf` cross‑compiler and `nasm`. If you do not
already have these tools available, run the provided script which downloads and
builds Binutils and GCC:

```bash
./build-toolchain.sh
```

The toolchain installs to `$HOME/opt/cross` and must be on your `PATH` before
building the kernel.

## Building the Kernel and User Programs

Once the cross‑compiler is installed, run either of the following commands from
the repository root to build the kernel and bundled user programs:

```bash
make all      # or simply ./build.sh
```

The resulting bootloader, kernel and operating system image are placed in
`bin/`.

## Running

After a successful build you can boot the kernel under QEMU using:

```bash
make run
```

This invokes `qemu-system-i386 -drive format=raw,file=./bin/os.bin`.

## Tests

There are currently no automated tests for the project.

## License and Contributions

This project is distributed under the terms of the BSD 2-Clause License. See the
[LICENSE](LICENSE) file for full details. Contributions are welcome through pull
requests.

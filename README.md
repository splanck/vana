# Vana Kernel

Vana is a small experimental operating system for 32-bit x86 machines. The project is written mostly in C with a minimal amount of assembly used for the bootloader and early initialization. It aims to provide a simple environment with multitasking, paging and a FAT16 file system so that user programs can run in ring 3.

## Features

- **Bootloader and kernel written from scratch** using NASM and GCC
- **Protected mode** setup with GDT, IDT and a TSS
- **Preemptive multitasking** and basic process management
- **Paging-based memory management** with a kernel heap
- **Simple FAT16 driver** for loading user programs from a disk image
- **Basic `int 0x80` system call interface** with a small libc/stdlib for userland
- Example programs including a tiny shell

## Repository layout

```
.
├── src/                # Kernel sources
│   ├── boot/           # Boot sector
│   ├── disk/           # ATA driver and block streaming helpers
│   ├── fs/             # File system layer and FAT16 implementation
│   ├── gdt/, idt/      # Descriptor tables and interrupt handling
│   ├── isr80h/         # System call handlers
│   ├── keyboard/       # Keyboard driver
│   ├── memory/         # Heap and paging code
│   ├── task/           # Task and process management
│   └── ...
├── programs/           # User space programs built with the provided libc
├── example_libc/       # Minimal C library used for early experiments
├── build.sh            # Helper to build the kernel using i686-elf tools
├── build-toolchain.sh  # Script for building a cross compiler toolchain
└── Makefile            # Main build logic
```

## Building

A cross compiler targeting **i686-elf** is required. You can install one through your package manager or build it locally using `./build-toolchain.sh` which places the tools under `~/opt/cross`.

Ensure `i686-elf-gcc` and `i686-elf-ld` are available in your `PATH`; the provided `build.sh` script sets up this environment and then invokes `make`.

```bash
./build.sh       # build kernel and user programs
make run         # boot the resulting image in QEMU
```

The `make run` target boots `bin/os.bin` using QEMU. During the build the Makefile mounts this image at `$(MOUNT_DIR)` (default `/mnt/d`) to copy user binaries. Adjust the path or run with suitable permissions if needed.

## Running

On boot the kernel loads `shell.elf` from the FAT16 partition and switches to it as the first user process. The shell provides a basic prompt capable of launching other programs from the disk image.

System calls are issued via interrupt `0x80` and cover simple file I/O, memory allocation and process management. The interface is defined in [`src/include/vana/syscall.h`](src/include/vana/syscall.h).

## License

Vana is distributed under the terms of the BSD 2-Clause license. See [LICENSE](LICENSE) for full details.

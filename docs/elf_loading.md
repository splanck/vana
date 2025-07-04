# ELF Loader and Paging Setup

The kernel supports running user programs stored as ELF executables. Loading an ELF file consists of reading the file into memory, validating the header and mapping each program segment into a task's address space. The paging layer then ensures those virtual addresses point at the loaded data with the appropriate permissions.

## Loading and Validation

`elf_load()` opens the file, allocates a buffer and reads the entire contents. It checks the ELF signature, class, data encoding and that a program header exists. Only 32‑bit little‑endian executables are accepted. Each program header is processed so the loader knows the required virtual and physical ranges.

## Mapping Segments

When `process_map_memory()` runs for an ELF process, it walks the program headers recorded by `elf_load()`. For every `PT_LOAD` entry the pages backing the segment are mapped into the task's directory with `paging_map_to()`. Writeable segments receive the `PAGING_IS_WRITEABLE` flag while read‑only sections are left protected.

The program's entry point comes from the ELF header (`e_entry`). The task's stack is mapped at `VANA_PROGRAM_VIRTUAL_STACK_ADDRESS_END` and grows downward. Because each task starts from the kernel's 4 GB identity mapping, the kernel remains accessible while user code resides at `VANA_PROGRAM_VIRTUAL_ADDRESS` and above.

Together this process ensures ELF executables appear at the correct virtual addresses and can safely transition into user mode.

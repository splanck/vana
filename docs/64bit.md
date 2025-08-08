# 64-bit (x86-64) Port Plan for Vana

## Target & Constraints

- **Mode:** x86-64 long mode with 4-level paging (LA57 off initially).
- **Boot:** BIOS stage-1 remains; add a stage-2 under `boot64/` that builds tables and jumps to 64-bit. UEFI can come later.
- **Kernel model:** higher-half, non-PIE, statically relocated. Optionally map physical memory directly for the memory manager.
- **Syscalls:** use `syscall`/`sysret` for user ↔ kernel transitions but keep an `int` pathway for early debugging.
- **ABI:** System V AMD64 for userland. Kernel follows SysV but disables the red zone.
- **Baseline features:** LM, NX, SYSCALL and SSE2. PAT/SMEP/SMAP can be detected and enabled later.

## Canonical Virtual Layout (recommended)

| Region | Virtual range |
| --- | --- |
| User space | `0x0000000000400000` – `0x00007fff_ffffffff` |
| Kernel text/data | `0xffffffff80000000` – `KERNEL_VMA` |
| Direct-map phys (optional) | `0xffff8880_00000000` – `0xffffc87f_ffffffff` |
| Guard pages | one below each kernel stack, marked NX |

## Phase 1 — Toolchain & Build System

**What to do**

- Produce an `x86_64-elf` binutils and gcc toolchain parallel to the existing i686 build.
- Add `ARCH=x86_64` make path with:
  - **CFLAGS (kernel):** `-m64 -mcmodel=kernel -fno-pic -fno-pie -mno-red-zone -fno-omit-frame-pointer -ffreestanding -fno-builtin -nostdlib -mno-mmx -mno-sse`
  - **LDFLAGS (kernel):** `-z max-page-size=0x1000` and `linker64.ld`
  - **NASM:** `-f elf64` and `BITS 64`
- Separate userland flags: `-O2 -m64 -fPIC` (red zone allowed).

**Done when** `readelf -h build/kernel64.o` shows `Class: ELF64` and `Machine: Advanced Micro Devices X86-64`.

## Phase 2 — Minimal Long-Mode Bring-Up (`boot64`)

**Files**

- `src/boot64/boot.asm` (new)
- `src/kernel.asm` (64-bit entry stub)
- `src/gdt/gdt64.asm|c` (tiny GDT)
- Temporary page table builder (pure asm or small C compiled with `-m32`).

**Exact sequence**

1. **Real mode stage**
   - Enable A20.
   - Load `kernel64.elf` to a known physical region (e.g. from 1 MiB).
2. **Build temporary paging**
   - Construct PML4 → PDPT → PD → PT that identity-map the low 16 MiB and map the kernel physical range to `KERNEL_VMA`.
   - Set PTE bits: `P=1`, `RW=1`, `NX=0` for kernel text (later set NX where appropriate), `US=0`, `G=1` for global kernel mappings.
3. **Protected mode prep**
   - Load a GDT with 32-bit segments and set `CR4.PAE=1`.
4. **Enter long mode**
   - Write `MSR_EFER (0xC0000080).LME=1`.
   - Set `CR3` to the PML4 physical address.
   - Set `CR0.PG=1` (and `PE=1`), then far jump to a 64-bit CS selector.
5. **64-bit entry (`kernel64_entry`)**
   - Load 64-bit GDT (flat, code=`0x9A`, data=`0x92`; long bit set in CS).
   - Set up a temporary kernel stack in the high half.
   - Zero segment registers except `GS/FS` as needed.
   - Jump to `kernel_main`.

**Gotchas**

- All target addresses must be canonical (bits 63:48 == bit 47).
- Identity-map page tables, transition code and stack.
- Do not rely on 32-bit `lgdt`/`lidt` encodings post-switch.
- To use NX later, set `EFER.NXE=1` and program NX bits.

**Done when** QEMU logs “hello from long mode” from `kernel_main` and can print a few values.

## Phase 3 — Linker Script & Relocation Model

- Create `linker64.ld` producing ELF64 output with `LMA` at the physical load (e.g. 1 MiB) and `VMA` at `KERNEL_VMA`.
- Provide symbols: `_kernel_start/_end`, `_text/_etext`, `_data/_edata`, `_bss/_ebss`, `_phys_to_virt_offset`.
- Ensure page-aligned segments and `max-page-size=0x1000`.
- Allow `R_X86_64_RELATIVE` relocations; avoid dynamic relocations.

**Done when** `readelf -lW kernel64.elf` shows `PT_LOAD` segments with low-memory `LMA` and high-half `VMA` with a constant offset.

## Phase 4 — Paging & Memory Manager (real tables, not just temp)

- Entries are 64-bit. Define common flag mask: `P(1<<0) RW(1<<1) US(1<<2) PWT(1<<3) PCD(1<<4) A(1<<5) D(1<<6) PS(1<<7) G(1<<8) NX(1ULL<<63)`.
- `CR4`: set `PAE`; later consider `SMEP` and `SMAP`.
- `EFER`: set `LME`, optionally `NXE`.
- Implement `paging64.{c,h}` with:
  - PML4 allocator and mapping helpers (`map_page`, `map_range`, `unmap`).
  - Higher-half map for kernel image (RX for text, RW for data, NX where appropriate).
  - Optional direct map region to cover low N GiB phys (RW, NX).
  - TLB shootdown API stub for future SMP (no-op now).
  - Early-boot bump allocator for tables before PMM online.

**Done when** page allocation and mapping work; `virt_to_phys`/`phys_to_virt` handle kernel and direct-map addresses. QEMU boots with NX enabled and deliberate NX fault traps.

## Phase 5 — Descriptors, IDT and TSS (long-mode format)

- **GDT**: minimal descriptors (kernel CS: 64-bit code, kernel DS: data, optional user CS/DS).
- Encode 64-bit TSS via system segment (two descriptors, 16 bytes) and load with `ltr`.
- **TSS64**: contains `rsp0/rsp1/rsp2` and `IST[0..6]`; dedicate ISTs for `#DF`, `NMI`, `#PF` and general faults.
- **IDT**: 16-byte gates with offset split low/mid/high; for faults/IRQs use DPL=0, for syscall-like ints DPL=3.

**Done when** `lidt` loads 16-byte gates and deliberate `#PF` lands on a known IST stack printing sane `RIP/CR2`.

## Phase 6 — Context Switching & ABI-Correct Entry/Exit

- Follow SysV AMD64 calling convention.
- Define a trap frame including: `r15..r12`, `rbx`, `rbp`, `rdi`, `rsi`, `rdx`, `rcx`, `r8`, `r9`, `rax`, `rip`, `cs`, `rflags`, `rsp`, `ss`, plus error code.
- Switcher saves callee-saved regs and ensures stack alignment before `iretq`.
- Manage FPU state with `fxsave`/`fxrstor` (later `xsave`).

**Done when** two kernel threads yield cooperatively and resume with registers intact; stack alignment verified.

## Phase 7 — Syscall Path (`syscall`/`sysret`)

- Program MSRs:
  - `IA32_STAR (0xC0000081)` – kernel/user selectors.
  - `IA32_LSTAR (0xC0000082)` – RIP target for `syscall` entry.
  - `IA32_FMASK (0xC0000084)` – RFLAGS mask cleared on entry.
- Entry sequence:
  - CPU stores user `RIP`→`rcx`, `RFLAGS`→`r11`, rises to CPL0, loads `CS/SS` from `STAR`.
  - `swapgs`, pivot to kernel stack (`rsp0` from TSS), build frame and call dispatcher with args in `rdi..r9` and syscall number in `rax`.
- Exit: restore registers, `swapgs`, set return `RIP`/`RFLAGS` from saved `rcx`/`r11` and `sysretq`.
- Handle edge cases: non-canonical addresses fall back to `iretq`; mask TF/IF via `SFMASK`; wrap user copies with `stac`/`clac` when SMAP enabled.
- Userland stubs: inline asm placing number in `rax` and args in regs, then `syscall`.

**Done when** a user test performs `write()` via syscall table and returns correct byte count with expected register prints.

## Phase 8 — Loader & ELF64 Support

- Handle `Elf64_Ehdr`, `Elf64_Phdr`, `Elf64_Shdr`.
- Map `PT_LOAD` segments at `VMA` with correct permissions and `p_align`.
- Support `R_X86_64_RELATIVE`, `R_X86_64_64`, `R_X86_64_32S`; keep kernel fully static (no PIE, `-fno-pic`).

**Done when** in-kernel loader parses ELF64 and starts a trivial 64-bit user binary.

## Phase 9 — Core C Source Audit

- Replace pointer-ish `uint32_t` with `uintptr_t`/`uint64_t`.
- Review size counters: `size_t` is 64-bit.
- `printf`/`vsnprintf`: handle `%p`, `%llx`, `%zu`.
- IO & drivers: ensure MMIO and port I/O use 64-bit regs (`dx` for ports).
- Memory: page allocator initially returns physical addresses <4 GiB; later support >4 GiB. Direct map simplifies `phys_to_virt`.

**Done when** kernel builds cleanly with `-Werror` for format/size/alignment warnings.

## Phase 10 — Exceptions, Interrupts and Stacks

- Populate IDT with 256 vectors, present and type=interrupt gate.
- Assign ISTs: `IST1` for NMI, `IST2` for `#DF`, `IST3` for `#PF`, `IST4` for general faults.
- Handlers push error codes where applicable; common stub normalizes frames for C handlers.
- Stick with PIC during early bring-up; move to APIC later.

**Done when** deliberate `#PF` prints `CR2`, `RIP` and recovers or halts gracefully.

## Phase 11 — Security & Robustness (initial set)

- Enable `NXE` and mark kernel data NX; only `.text` is RX.
- Guard pages under kernel stacks.
- Validate user pointers on syscall entry; avoid `rep movs` without `stac/clac` once SMAP is on.
- Later: enable `SMEP/SMAP`, consider `KPTI`, enforce W^X in userland loader.

## Phase 12 — Userland Rebuild

- Recompile libc stubs for x86-64 and update syscall inline asm.
- Ensure crt0/start files align stack to 16 bytes before calling `main`.
- Keep `int 0x80` compat shim while migrating if desired.

**Done when** shell or test binary runs, performs at least two syscalls and prints output.

## Phase 13 — Testing, Instrumentation and GDB

- **QEMU recipe:** `qemu-system-x86_64 -m 512 -no-reboot -d int,cpu_reset -serial stdio -s -S -drive file=build/vanadisk.img,format=raw`
  - `-s -S` lets GDB attach before first instruction.
- **GDB:** `target remote :1234`, `set arch i386:x86-64`, `add-symbol-file build/kernel64.elf <KERNEL_VMA+_text>`.
- Useful macros to inspect CR regs and walk PML4.
- Breakpoints on `#PF` handler and syscall entry.
- **Self-tests:** page table probe, context switch ping-pong, syscall regression, string/mem primitives with 64-bit sizes.

**Common failure modes**

- Triple-fault reboot: missing identity map for switch code or wrong `CR3`; invalid GDT/CS.
- IRQ/exception lost: IDT gates still 8 bytes or not present.
- Random crashes on task switch: stack mis-alignment or missing save of `r12–r15`.
- `sysret` `#GP`: non-canonical `RIP/RSP`; fall back to `iretq`.
- User copy faults with SMAP: missing `stac/clac` around copies.

## File-by-File Guide (mapped to your tree)

### Boot

- `src/boot/boot.asm`: keep stage-1 (BIOS). Remove 32-bit paging switch.
- `src/boot64/boot.asm`: A20, temp tables, `CR4.PAE`, `EFER.LME`, `CR0.PG`, far jump; identity map + higher-half map.
- `src/kernel.asm`: 64-bit stub to set stack (high half), load `GDT64`, zero segs, call `kernel_main`.

### Descriptors

- `src/gdt/gdt.h|c|asm`: 64-bit GDT and TSS; `ltr` during init.
- `src/idt/idt.h|c|asm`: 16-byte gate emitters, IST indices, `lidt`.
- `src/task/tss.asm|h`: 64-bit TSS layout with `rsp0` + ISTs.

### Paging/MM

- `src/memory/paging/paging64.{c,h}`: PML4/PDPT/PD/PT constructors; flags; map/unmap.
- `src/memory/heap/kheap.c`: `uintptr_t`/`size_t`, high-half addresses.
- `src/memory/memory.c`: audit `mem*` for 64-bit sizes and alignment.

### Context & Syscalls

- `src/task/task.asm`: save/restore `r15..r12`, `rbx`, `rbp`; frame layout; `iretq`.
- `src/task/task.c|process.c`: 64-bit `rip/rsp`; kernel stack alloc with guard page.
- `src/isr80h/`: entry stub at `LSTAR`; `swapgs`, pivot to `rsp0`, save `rcx/r11`, call dispatcher; dispatcher reads args from regs, `rax` carries syscall number; exit restores regs, `swapgs`, `sysretq`.

### ELF & Loader

- `src/loader/elf64.{c,h}`: parse ELF64 headers and program headers; map `PT_LOAD` with perms; respect `p_align`.
- If modules: keep static or define relocation subset.

### Kernel Core

- `src/config.h`: `KERNEL_VMA`, selectors, stack sizes, feature toggles (NXE).
- `src/kernel.c`: build tables, load `GDT/IDT/TSS`, enable `NXE`, program MSRs for syscall, enter scheduler.
- `src/io/io.asm`: 64-bit friendly wrappers (ports still `dx`, data sizes unchanged).

### Userland

- `example_libc`: update syscall inline asm; `%p`/`%zu`/`%ld` formatting; errno handling.
- `programs/*`: rebuild with `x86_64-elf-gcc`; ensure `_start` aligns stack to 16 bytes before `main`.

## Milestone Checklist (in order)

1. Kernel objects are ELF64; `kernel64.elf` links with high-half VMA.
2. Reach `kernel_main` through `boot64` with temporary tables (“hello from long mode”).
3. Switch to final PML4; `NXE` on; higher-half only (except identity window for early devices).
4. IDT64 + TSS64 + IST; deliberate `#PF` prints `CR2` and returns/halts cleanly.
5. Context switch between two kernel threads without corruption; FPU preserved.
6. User test binary calls a syscall and returns; `sysretq` path validated.
7. Userland shell (or simple app) runs; file/console syscalls work.
8. Regression tests: fault injection (NX, non-canonical), mapping/unmapping, large allocations.

## Deep-Dive Notes & Exact Bits (quick reference)

- **CR0:** set `PG` (bit 31) and `PE` (bit 0); keep `WP` (bit 16) once paging is stable.
- **CR4:** set `PAE` (bit 5); later `PGE` (bit 7), `OSFXSR` (bit 9), `OSXMMEXCPT` (bit 10), optionally `SMEP` (20), `SMAP` (21).
- **EFER (0xC0000080):** set `LME` (bit 8) before enabling `PG`; set `NXE` (bit 11) for NX.
- **STAR (0xC0000081):** upper/lower selector pairs (kernel/user `CS/SS`) encoded per AMD64 manual.
- **LSTAR (0xC0000082):** RIP for syscall entry (kernel VA).
- **SFMASK (0xC0000084):** mask `IF|DF|TF|NT|AC`; restore from saved `r11`.
- **IDT gate:** type `0xE` (interrupt), present `1`, DPL as needed, IST index `0..7`.
- **TSS64:** 104+ bytes; write `rsp0` each context if using per-thread kernel stacks; load with `ltr`.

## How to iterate safely (workflow)

- Keep 32-bit build working while bringing up 64-bit in parallel (`make vana32`, `make vana64`).
- Land one subsystem per commit/PR: boot, paging, IDT/TSS, context, syscall, userland.
- After each milestone, add a tiny self-test (panic on failure) and keep it permanently.
- Ask for concrete snippets (linker, boot64 skeleton, syscall stubs, trap frame structs) as needed.


# Task Manager ELF Retired C Provider

Captured: 2026-06-30 CEST.

## Scope

`qsoe-tm-elf` is the mandatory Rust provider for the portable task-manager ELF
view parser in:

```text
libtaskman/include/tm_elf.h
rust/crates/qsoe-tm-elf/src/lib.rs
```

It exports the existing `tm_elf_parse` ABI and preserves the retired C parser's
view shape for ELF64 little-endian RISC-V images. The C provider body
`libtaskman/src/elf.c` is removed.

It covers:

- ELF magic, class, byte-order, machine, and type checks;
- program-header-table bounds checks;
- `PT_LOAD` capture into the fixed 8-entry `tm_elf_view_t` array;
- `filesz <= memsz` validation;
- file-span validation for non-empty load segments;
- `PT_INTERP` bounds and NUL-termination validation;
- entry point, dynamic/executable flag, program-header metadata, and virtual
  address range reporting.

It intentionally preserves current C behavior where the loader depends on it,
including:

- allowing zero-file-size `PT_LOAD` entries without validating their file
  offset;
- storing interpreter pointers directly into the caller-owned ELF blob;
- wrapping virtual segment end arithmetic before the final empty-range check.

It does not replace:

- segment mapping or address-space construction;
- dynamic-linker admission;
- relocation parsing or relocation writes;
- CPIO lookup, script handling, process tables, capability ownership, or seL4
  object manipulation.

## Selector

Normal builds select Rust:

```sh
make -C nq/taskman
make -C lq taskman
```

C rollback is retired and now fails fast:

```sh
QSOE_RUST_TM_ELF=0 make -C nq/taskman
QSOE_RUST_TM_ELF=0 make -C lq taskman
```

The top-level evidence target is:

```sh
make tm-elf-evidence
make tm-elf-runtime-smoke
make tm-elf-rc-smoke
```

Multiple taskman Rust providers may be selected together. The shared
`qsoe-tm-providers` archive packages the selected provider crates behind one
no-std panic handler. Legacy targets such as `make rust-tm-elf-provider`
still produce the historical single-provider output path for focused evidence.

## Evidence

Local validation on 2026-06-30:

```sh
make check-tm-elf-model
cargo test --manifest-path rust/Cargo.toml -p qsoe-tm-elf --features host-tests --lib
cargo clippy --manifest-path rust/Cargo.toml -p qsoe-tm-elf --features host-tests -- -D warnings
bash -n scripts/check-tm-elf-model.sh scripts/build-rust-tm-elf-provider.sh scripts/tm-elf-evidence.sh scripts/tm-elf-runtime-smoke.sh scripts/tm-elf-rc-smoke.sh scripts/apply-component-overrides.sh scripts/boot-smoke.sh
make -n tm-elf-rc-smoke container-tm-elf-rc-smoke
make rust-tm-elf-provider
make tm-elf-evidence
make tm-elf-runtime-smoke
make tm-elf-rc-smoke
```

`make tm-elf-evidence` verified:

- Rust host tests pass for layout, valid parses, malformed inputs, too many
  load segments, zero-file-size loads, and wrapped segment ends;
- Rust staticlib builds for `riscv64imac-unknown-none-elf`;
- Rust provider archive members report RVC soft-float ABI;
- Rust provider archive exports `tm_elf_parse`;
- NQ and LQ Rust-only `libtaskman.a` contain zero `elf.o`;
- NQ and LQ reject `QSOE_RUST_TM_ELF=0`;
- NQ and LQ taskman links complete in Rust-only mode, export `tm_elf_parse`,
  and pass the evidence script's ELF flag and section audit.

`make tm-elf-runtime-smoke` verified the Rust-selected parser in a booted LQ
image. The smoke injects a sysinit fragment, verifies the staged
`/usr/bin/sysinfo` binary has a program interpreter, rebuilds QSOE/L with
`QSOE_RUST_TM_ELF=1`, and waits for:

```text
tm-elf-runtime-smoke: /usr/bin/sysinfo dynamic ELF spawn ok
```

`/usr/bin/sysinfo` is a dynamic ELF, so successful spawn exercises
`tm_elf_parse` for the main image plus the loader path for `rtld` and `libc`.
This evidence proves ABI compatibility, archive selection, linked artifact
shape, and focused dynamic ELF spawn behavior under the Rust ELF parser.

`make tm-elf-rc-smoke` verifies the same dynamic ELF spawn path in the default
selector mode and requires C `elf.o` to be absent from NQ and LQ
`libtaskman.a`. Attempts to use `QSOE_RUST_TM_ELF=0` or
`TM_ELF_RC_ROLLBACK=1` fail fast after retirement.

## C Retirement

C rollback no longer exists:

- `QSOE_RUST_TM_ELF=1` is mandatory and excludes `elf.o`;
- `QSOE_RUST_TM_ELF=0` is rejected;
- `libtaskman/src/elf.c` is removed;
- the Rust path links
  the shared taskman Rust provider archive.

Do not reintroduce C rollback without a new issue, explicit rollback
justification, and fresh PR evidence.

# Task Manager ELF C Retirement

Captured: 2026-06-30 CEST.

## Scope

The task-manager `tm_elf` provider is Rust-only. The retirement removes:

- `libtaskman/src/elf.c`
- `tests/tm_elf_model_test.c`

The Rust provider remains in `rust/crates/qsoe-tm-elf/src/lib.rs` and exports
the existing `tm_elf_parse` ABI declared by `libtaskman/include/tm_elf.h`.

This retirement covers only the portable read-only ELF view parser. Segment
mapping, dynamic-linker admission, relocation parsing and writes, process
tables, CPIO lookup, script handling, capability ownership, and seL4 object
manipulation remain C.

## Selector State

`QSOE_RUST_TM_ELF=1` is mandatory in umbrella, NQ, LQ, and standalone
`libtaskman` builds. `QSOE_RUST_TM_ELF=0` now fails fast.

The rollback smoke target was removed:

```sh
make tm-elf-rc-rollback-smoke
make container-tm-elf-rc-rollback-smoke
```

The retained checks are:

```sh
make check-tm-elf-model
make tm-elf-evidence
make tm-elf-runtime-smoke
make tm-elf-rc-smoke
```

`make tm-elf-evidence` verifies Rust host tests, soft-float archive shape,
`tm_elf_parse` export, NQ/LQ taskman links without C `elf.o`, and retired
selector rejection for NQ and LQ.

`make tm-elf-rc-smoke` verifies the retired/default path and boots QSOE/L
through dynamic `/usr/bin/sysinfo` spawn. Successful spawn exercises
`tm_elf_parse` for the main dynamic ELF and loader path while relocation and
process management remain C.

## Adjacent Evidence

Adjacent task-manager evidence scripts that previously pinned
`QSOE_RUST_TM_ELF=0` now pin `QSOE_RUST_TM_ELF=1`:

- `scripts/tm-fdt-evidence.sh`
- `scripts/tm-pathmgr-evidence.sh`
- `scripts/tm-sysmap-evidence.sh`

This keeps those tests focused on their own provider while respecting the
retired tm_elf selector.

## Reintroduction Rule

Do not reintroduce C `tm_elf` rollback without a new issue, explicit rollback
justification, and fresh PR evidence.

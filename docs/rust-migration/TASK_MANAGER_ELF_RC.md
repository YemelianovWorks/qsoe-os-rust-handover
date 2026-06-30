# `qsoe-tm-elf` Rust-Default Release Candidate

Captured: 2026-06-30 CEST.

This note records the `tm_elf` Rust-default release-candidate path with C
rollback still available.

## Rust Migration: `tm_elf`

Status: Rust default RC.
Release or build: `qsoe-tm-elf-rc1`, introduced by the
`codex/tm-elf-rust-default-rc` branch.

### Language Change

- Previous default implementation: C `libtaskman/src/elf.c`
- New RC default implementation: Rust `qsoe-tm-elf`
- Rust artifact or crate: `rust/crates/qsoe-tm-elf`
- Taskman Rust link model: selected providers are packaged through the shared
  `build/rust/tm-providers/libqsoe_tm_providers.a` archive
- C implementation status during this RC: rollback-only through
  `QSOE_RUST_TM_ELF=0`
- User-visible behavior changes: none expected for ELF view parsing or dynamic
  process spawn semantics

The RC changes only the selected provider for the portable read-only
`tm_elf_parse` view parser. Segment mapping, dynamic-linker admission,
relocation parsing and writes, process tables, CPIO lookup, script handling,
capability ownership, and seL4 object manipulation remain C.

## Rollback

- Rollback available during RC: yes
- Rollback selector: `QSOE_RUST_TM_ELF=0`
- Rollback command:

```sh
make tm-elf-rc-rollback-smoke
```

Default RC smoke:

```sh
make tm-elf-rc-smoke
```

Rollback window: open. Do not remove `libtaskman/src/elf.c` until #26's C
retirement checklist is satisfied and a separate removal PR records fresh
evidence.

## Test Evidence

- C model fixture: `make check-tm-elf-model`
- Rust host tests: `cargo test --manifest-path rust/Cargo.toml -p qsoe-tm-elf --features host-tests --lib`
- Artifact and membership audit: `make tm-elf-evidence`
- Existing Rust-selected runtime smoke: `make tm-elf-runtime-smoke`
- Rust-default RC smoke: `make tm-elf-rc-smoke`
- C rollback RC smoke: `make tm-elf-rc-rollback-smoke`

The RC smoke builds NQ and LQ taskman in the default selector mode and verifies
that C `elf.o` is absent from `libtaskman.a`. The rollback smoke repeats the
archive-membership check with `QSOE_RUST_TM_ELF=0`, where C `elf.o` must be
present. Both modes boot QSOE/L with a staged sysinit fragment that verifies
`/usr/bin/sysinfo` is dynamic and runs it successfully.

## Known Limitations

- No C source is removed by this RC.
- The RC covers QSOE/L QEMU runtime behavior, not a full hardware release.
- Only the portable taskman ELF view parser is selected through Rust. The
  loader, relocation, process, CPIO, script, and seL4 object paths remain C.

## Review Notes

- Unsafe review: no new Rust unsafe code in this RC target wiring.
- Data or on-disk format migration: none.
- Operator impact: use `make tm-elf-rc-smoke` to validate the Rust default RC
  path and `make tm-elf-rc-rollback-smoke` to validate rollback.

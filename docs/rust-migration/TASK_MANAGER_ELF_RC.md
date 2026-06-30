# `qsoe-tm-elf` Rust-Default Release Candidate

Captured: 2026-06-30 CEST.

This note records the former `tm_elf` Rust-default release-candidate path. The
C rollback path has since been retired; see
`TASK_MANAGER_ELF_RETIREMENT.md` for the removal evidence.

## Rust Migration: `tm_elf`

Status: retired after Rust default RC.
Release or build: `qsoe-tm-elf-rc1`, introduced by the
`codex/tm-elf-rust-default-rc` branch.

### Language Change

- Previous default implementation: C `libtaskman/src/elf.c`
- Current implementation: Rust `qsoe-tm-elf`
- Rust artifact or crate: `rust/crates/qsoe-tm-elf`
- Taskman Rust link model: selected providers are packaged through the shared
  `build/rust/tm-providers/libqsoe_tm_providers.a` archive
- C implementation status: retired; `QSOE_RUST_TM_ELF=0` fails fast
- User-visible behavior changes: none expected for ELF view parsing or dynamic
  process spawn semantics

The RC changes only the selected provider for the portable read-only
`tm_elf_parse` view parser. Segment mapping, dynamic-linker admission,
relocation parsing and writes, process tables, CPIO lookup, script handling,
capability ownership, and seL4 object manipulation remain C.

## Rollback

- Rollback available now: no
- Retired selector: `QSOE_RUST_TM_ELF=0`
- Retired rollback command:

```sh
TM_ELF_RC_ROLLBACK=1 scripts/tm-elf-rc-smoke.sh
```

Default RC smoke:

```sh
make tm-elf-rc-smoke
```

Rollback window: closed by the tm_elf C retirement PR. Do not reintroduce
`libtaskman/src/elf.c` without a new issue, explicit rollback justification,
and fresh PR evidence.

## Test Evidence

- Rust host model tests: `make check-tm-elf-model`
- Rust host tests: `cargo test --manifest-path rust/Cargo.toml -p qsoe-tm-elf --features host-tests --lib`
- Artifact and membership audit: `make tm-elf-evidence`
- Existing Rust-selected runtime smoke: `make tm-elf-runtime-smoke`
- Rust-default RC smoke: `make tm-elf-rc-smoke`
- Retired selector checks: `QSOE_RUST_TM_ELF=0` and
  `TM_ELF_RC_ROLLBACK=1` fail fast

The RC smoke builds NQ and LQ taskman in the default selector mode and verifies
that C `elf.o` is absent from `libtaskman.a`. It boots QSOE/L with a staged
sysinit fragment that verifies `/usr/bin/sysinfo` is dynamic and runs it
successfully. The former rollback selector now fails before boot.

## Known Limitations

- C source was removed by the later retirement PR.
- The RC covers QSOE/L QEMU runtime behavior, not a full hardware release.
- Only the portable taskman ELF view parser is selected through Rust. The
  loader, relocation, process, CPIO, script, and seL4 object paths remain C.

## Review Notes

- Unsafe review: no new Rust unsafe code in this RC target wiring.
- Data or on-disk format migration: none.
- Operator impact: use `make tm-elf-rc-smoke` to validate the Rust-only path.

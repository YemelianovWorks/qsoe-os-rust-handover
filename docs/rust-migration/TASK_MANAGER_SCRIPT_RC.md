# `qsoe-tm-script` Rust-Default Release Candidate

Captured: 2026-06-30 CEST.

This note records the `tm_script` Rust-default release-candidate path with C
rollback still available.

## Rust Migration: `tm_script`

Status: Rust default RC.
Release or build: `qsoe-tm-script-rc1`, introduced by the
`codex/tm-script-rust-default-rc` branch.

### Language Change

- Previous default implementation: C `libtaskman/src/script.c`
- New RC default implementation: Rust `qsoe-tm-script`
- Rust artifact or crate: `rust/crates/qsoe-tm-script`
- Taskman Rust link model: selected providers are packaged through the shared
  `build/rust/tm-providers/libqsoe_tm_providers.a` archive
- C implementation status during this RC: rollback-only through
  `QSOE_RUST_TM_SCRIPT=0`
- User-visible behavior changes: none expected for shebang parsing, direct
  script spawn, interpreter path handling, or optional argument parsing

The RC changes only the selected provider for the portable task-manager
shebang parser. Interpreter loading, argv construction, CPIO lookup, ELF
loading, relocation, process tables, and seL4 invocation code remain C.

## Rollback

- Rollback available during RC: yes
- Rollback selector: `QSOE_RUST_TM_SCRIPT=0`
- Rollback command:

```sh
make tm-script-rc-rollback-smoke
```

Default RC smoke:

```sh
make tm-script-rc-smoke
```

Rollback window: open. Do not remove `libtaskman/src/script.c` until #26's C
retirement checklist is satisfied and a separate removal PR records fresh
evidence.

## Test Evidence

- C model fixture: `make check-tm-script-model`
- Rust host tests: `cargo test --manifest-path rust/Cargo.toml -p qsoe-tm-script --features host-tests`
- Artifact and membership audit: `make tm-script-evidence`
- Existing Rust-selected runtime smoke: `make tm-script-runtime-smoke`
- Rust-default RC smoke: `make tm-script-rc-smoke`
- C rollback RC smoke: `make tm-script-rc-rollback-smoke`

The RC smoke builds NQ and LQ taskman in the default selector mode and verifies
that C `script.o` is absent from `libtaskman.a`. The rollback smoke repeats
the archive-membership check with `QSOE_RUST_TM_SCRIPT=0`, where C `script.o`
must be present. Both modes boot QSOE/L with a staged executable
`/usr/bin/tm_script_probe` shell script and run it directly from sysinit, which
forces taskman spawn to parse the shebang before loading `/bin/sh`.

## Known Limitations

- No C source is removed by this RC.
- The RC covers QSOE/L QEMU runtime behavior, not a full hardware release.
- Only the portable shebang parser is selected through Rust. Task-manager
  spawn orchestration, CPIO file access, loader, process lifecycle, and seL4
  object code remain C.

## Review Notes

- Unsafe review: no new Rust unsafe code in this RC target wiring.
- Data or on-disk format migration: none.
- Operator impact: use `make tm-script-rc-smoke` to validate the Rust default
  RC path and `make tm-script-rc-rollback-smoke` to validate rollback.

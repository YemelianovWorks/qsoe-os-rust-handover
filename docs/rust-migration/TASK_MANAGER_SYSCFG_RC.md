# `qsoe-tm-syscfg` Historical Rust-Default Release Candidate

Captured: 2026-06-30 CEST.

This note records the former `tm_syscfg` Rust-default release-candidate path.
The C rollback window is now closed; see
`TASK_MANAGER_SYSCFG_RETIREMENT.md` for the retirement record.

## Rust Migration: `tm_syscfg`

Status: historical Rust-default RC, later retired.
Release or build: `qsoe-tm-syscfg-rc1`, introduced by the
`codex/tm-syscfg-rust-default-rc` branch.

### Language Change

- Previous default implementation: C `libtaskman/src/syscfg.c`
- RC default implementation: Rust `qsoe-tm-syscfg`
- Rust artifact or crate: `rust/crates/qsoe-tm-syscfg`
- Taskman Rust link model: selected providers are packaged through the shared
  `build/rust/tm-providers/libqsoe_tm_providers.a` archive
- C implementation status during the RC: rollback-only through
  `QSOE_RUST_TM_SYSCFG=0`
- Current C implementation status: removed from `libtaskman`

The RC changed only the selected provider for the portable
`libtaskman/src/syscfg.c` TLV builder/walker. LQ's private FDT-backed runtime
syscfg builder, FDT parsing, sysmap construction, `/sys` file serving, process
tables, IPC decoding, and seL4 object code remained C.

## Former Rollback

The former rollback selector was:

```sh
QSOE_RUST_TM_SYSCFG=0
```

The former rollback smoke was:

```sh
make tm-syscfg-rc-rollback-smoke
```

Both now fail fast after C provider retirement.

## Test Evidence

- C model fixture: historical `make check-tm-syscfg-model`
- Rust host tests: `cargo test --manifest-path rust/Cargo.toml -p qsoe-tm-syscfg --features host-tests`
- Artifact and membership audit: `make tm-syscfg-evidence`
- Existing Rust-selected runtime smoke: `make tm-syscfg-runtime-smoke`
- Rust-default RC smoke: `make tm-syscfg-rc-smoke`
- Former C rollback RC smoke: `make tm-syscfg-rc-rollback-smoke`

The RC smoke built NQ and LQ taskman in the default selector mode and verified
that C `syscfg.o` was absent from `libtaskman.a`. The rollback smoke repeated
the archive-membership check with `QSOE_RUST_TM_SYSCFG=0`, where C `syscfg.o`
was present. Both modes booted QSOE/L with a staged sysinit fragment that read
`/sys/board`, checked `/sys/cmdline` for the virtio mainfs argument, and ran
`/usr/bin/sysinfo`.

## Known Limitations

- The RC covered QSOE/L QEMU runtime behavior, not a full hardware release.
- Only the portable taskman syscfg TLV helper was selected through Rust. The
  LQ private runtime syscfg builder remained C.

## Review Notes

- Unsafe review: no new Rust unsafe code was added in the RC target wiring.
- Data or on-disk format migration: none.
- Operator impact after retirement: use `make tm-syscfg-rc-smoke` to validate
  the Rust-only path; no rollback target remains.

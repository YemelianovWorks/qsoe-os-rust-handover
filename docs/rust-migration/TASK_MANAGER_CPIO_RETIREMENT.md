# `tm_cpio` C Provider Retirement

Captured: 2026-06-30 CEST.

This note records the task-manager CPIO archive provider C retirement after the
Rust-default release-candidate path documented in `TASK_MANAGER_CPIO_RC.md`
and the shared task-manager provider archive documented in
`TASK_MANAGER_PROVIDERS.md`.

## Rust Migration: `tm_cpio`

Status: Retired C provider.
Release or build: `qsoe-tm-cpio-retired`, introduced by the
`codex/tm-cpio-c-retirement` branch.

### Language Change

- Previous default implementation: C `libtaskman/src/cpio.c`
- New default implementation: Rust `qsoe-tm-cpio`
- Rust artifact or crate: `rust/crates/qsoe-tm-cpio`, linked through the
  shared `build/rust/tm-providers/libqsoe_tm_providers.a` archive
- Removed C source: `libtaskman/src/cpio.c`
- Removed C host fixture: `tests/tm_cpio_model_test.c`
- Public ABI retained: `libtaskman/include/tm_cpio.h`; taskman C path-manager,
  spawn, and file-access code still call the `tm_cpio_*` functions exported by
  Rust
- User-visible behavior changes: none expected for boot CPIO lookup, symlink
  resolution, directory iteration, CPIO-backed file reads, or symlink-backed
  spawn

## Rollback

- Rollback available: no C rollback target remains.
- `QSOE_RUST_TM_CPIO=0` now fails fast in taskman builds and provider archive
  builds.
- `TM_CPIO_RC_ROLLBACK=1 scripts/tm-cpio-rc-smoke.sh` now fails fast.
- Historical rollback evidence lives in `TASK_MANAGER_CPIO_RC.md`.

## Test Evidence

- Rust host tests: `make check-tm-cpio-model`
- Rust provider archive audit, NQ/LQ taskman membership, and retired selector
  rejection: `make tm-cpio-evidence`
- Runtime smoke: `make tm-cpio-runtime-smoke`
- Retired compatibility smoke: `make tm-cpio-rc-smoke`

The runtime smoke boots QSOE/L with the Rust-only CPIO provider, stages a
temporary sysinit fragment, and checks CPIO-root symlink listing
(`/etc -> /usr/conf`, `/home -> /usr/home`), `/etc/passwd` access through the
CPIO symlink into mounted `/usr`, direct `/sbin/init` reads from the boot CPIO,
and `/bin/sh` symlink spawn.

## Review Notes

- Unsafe review: no new Rust unsafe code in the retirement wiring.
- Data or on-disk format migration: none.
- Link review: `tm_cpio` is mandatory in the shared provider archive, so
  taskman can still combine it with other Rust task-manager providers through
  one no-std panic handler.
- Operator impact: `tm_cpio` rollback flags now fail fast; use
  `make tm-cpio-evidence`, `make tm-cpio-runtime-smoke`, or
  `make tm-cpio-rc-smoke` to validate the Rust-only provider path.

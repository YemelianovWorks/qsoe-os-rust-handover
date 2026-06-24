# `pipe-rs` Rust-Default Release Candidate

Captured: 2026-06-24 15:58 CEST.

This note records the `pipe` Rust-default release-candidate path. It does not
retire the C implementation. Normal source builds still preserve the C daemon,
and the RC image path keeps a one-command C rollback drill.

## Rust Migration: `pipe`

Status: Rust default RC.
Release or build: `pipe-rs-rc1`, introduced by the
`codex/pipe-rust-default-rc` branch.

### Language Change

- Previous default implementation: C `/sbin/pipe`
- New RC default implementation: Rust `pipe-rs`
- Rust artifact or crate: `rust/bins/pipe-rs`, linked to
  `build/rust/qsoe-pipe-rs.elf`
- C implementation status: rollback-only for the RC image path; still present
  in tree and still used by non-RC normal builds
- User-visible behavior changes: none expected for libc `pipe(2)` callers

Known startup-log difference:

- C prints:
  - `[pipe] alive, pid=N`
  - `[pipe] registered at /dev/pipe on chid=N`
- Rust prints:
  - `[pipe-rs] alive`
  - `[pipe-rs] /dev/pipe registered`
  - `[pipe-rs] entering MsgReceive loop`

## Rollback

- Rollback available: yes
- Rollback flag: `QSOE_PIPE_RC_ROLLBACK=1`
- Rollback command:

```sh
make pipe-rc-rollback-smoke
```

Default RC data-path smoke:

```sh
make pipe-rc-data-smoke
```

Rollback window: still open until the C retirement gate in `RETIREMENT.md` is
satisfied and a separate removal PR is reviewed.

Rollback limitations: none known for the QSOE/L smoke paths. The rollback image
uses the same C `/sbin/pipe` artifact as the pre-RC path.

## Test Evidence

- Host tests: `make rust-quality`
- Artifact audit: `make rust-pipe-link-smoke`
- Existing opt-in data-path smoke: `make rust-pipe-data-smoke`
- Rust-default RC data-path smoke: `make pipe-rc-data-smoke`
- C rollback data-path smoke: `make pipe-rc-rollback-smoke`

The data-path smoke boots QSOE/L, starts `/sbin/pipe`, runs
`/usr/bin/test_pipe_data`, and verifies registration, libc `pipe(2)` round
trip, EOF behavior, helper exit, and boot-to-login.

## Known Limitations

- No C source is removed by this RC.
- The RC covers QSOE/L QEMU data-path behavior, not a full hardware release.
- C retirement remains blocked by #26 until the retirement checklist is
  reviewed; this RC evidence does not remove or disable the C implementation.

## Review Notes

- Unsafe review: no new Rust unsafe code in this RC target wiring.
- Data or on-disk format migration: none.
- Operator impact: use `make pipe-rc-data-smoke` to validate the Rust default
  RC path and `make pipe-rc-rollback-smoke` to validate rollback.

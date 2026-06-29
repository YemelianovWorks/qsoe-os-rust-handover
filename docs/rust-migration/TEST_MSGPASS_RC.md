# `test_msgpass-rs` Rust-Default Test-Image Release Candidate

Captured: 2026-06-28.

Superseded by: `TEST_MSGPASS_RETIREMENT.md` on 2026-06-29. This file records
the historical RC window and rollback drill that made the later C retirement
eligible. The rollback command named below is intentionally removed after
retirement.

This note records the `test_msgpass` Rust-default release-candidate path for
test images. It does not retire the C implementation. Normal source builds
still preserve the C helper, and the RC smoke keeps a one-command C rollback
drill.

## Rust Migration: `test_msgpass`

Status: Rust default RC.
Release or build: `test-msgpass-rs-rc1`, introduced by the
`codex/test-msgpass-rust-default-rc` branch.

### Language Change

- Previous default test-image helper: C `/usr/bin/test_msgpass`
- New RC default test-image helper: Rust `test_msgpass-rs`
- Rust artifact or crate: `rust/bins/test-msgpass-rs`, linked to
  `build/rust/qsoe-test-msgpass-rs.elf`
- C implementation status during this RC: rollback-only for the RC smoke path;
  still present in the `quser` component and still selectable by
  `QSOE_RUST_TEST_MSGPASS=0`. After `TEST_MSGPASS_RETIREMENT.md`, the helper
  is removed and the flag value `0` is rejected.
- User-visible behavior changes: none expected for the suite `[msgpass]`
  section

Known startup-log difference:

- C prints only failure diagnostics.
- Rust prints:
  - `[test_msgpass-rs] alive`
  - `[test_msgpass-rs] /dev/msgpass registered`
  - receive/shutdown progress markers used for debugging the MCS no-reply path

## Rollback

- Rollback available: yes
- Rollback flag: `QSOE_TEST_MSGPASS_RC_ROLLBACK=1`
- Rollback command:

```sh
make test-msgpass-rc-rollback-smoke
```

Default RC smoke:

```sh
make test-msgpass-rc-smoke
```

Rollback window: closed by the dedicated C retirement PR after this RC evidence
and #26 checklist review.

Rollback limitations: none known for the QSOE/L-targeted `[msgpass]` smoke
path. The rollback image stages the same C `/usr/bin/test_msgpass` artifact as
the pre-RC path.

## Test Evidence

- Host tests: `make rust-quality`
- Artifact audit: `make rust-test-msgpass-link-smoke`
- Existing opt-in smoke: `make rust-test-msgpass-smoke`
- Rust-default RC smoke: `make test-msgpass-rc-smoke`
- C rollback smoke: `make test-msgpass-rc-rollback-smoke`

The smoke boots QSOE/L, runs the existing `/usr/bin/suite`, and verifies the
targeted `[msgpass]` markers: path resolution, 4 MiB minus 2 byte round trip,
halfword-swapped payload, clean server exit, and the QSOE/L no-reply skip.

## Known Limitations

- No C source was removed by this RC; the removal is recorded separately in
  `TEST_MSGPASS_RETIREMENT.md`.
- The RC covers the QSOE/L-targeted suite `[msgpass]` path, not the full
  hardware matrix.
- The no-reply ESRVRFAULT subcase remains a documented QSOE/L limitation and is
  skipped before entering a blocking `MsgSend`.
- C retirement was still blocked by #26 during this RC; this evidence later
  became the input to the separate retirement PR.

## Review Notes

- Unsafe review: no new Rust unsafe code in this RC target wiring.
- Data or on-disk format migration: none.
- Operator impact: use `make test-msgpass-rc-smoke` to validate the Rust
  default RC path and `make test-msgpass-rc-rollback-smoke` to validate
  rollback.

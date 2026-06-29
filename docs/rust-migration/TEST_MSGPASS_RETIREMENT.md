# `test_msgpass` C Helper Retirement

Captured: 2026-06-29.

This note records the first tracked C implementation retirement in the Rust
migration: the in-guest `test_msgpass` helper. The retired component is a test
helper, not a production service, which keeps the first #26 removal exercise
low-risk while still proving the retirement process end to end.

## Rust Migration: `test_msgpass`

Status: Retired.
Release or build: `test-msgpass-rs-retire-c1`, introduced by the
`codex/retire-c-test-msgpass` branch.

### Language Change

- Previous default implementation: C `/usr/bin/test_msgpass`
- New default implementation: Rust `test_msgpass-rs`
- Rust artifact or crate: `rust/bins/test-msgpass-rs`, linked to
  `build/rust/qsoe-test-msgpass-rs.elf`
- C implementation status: removed from tracked `quser/test/msgpass` source
  and normal qrvfs image-selection paths
- User-visible behavior changes: none expected for the suite `[msgpass]`
  section; Rust retains the debug startup markers introduced during RC

Retirement scope:

- `make test-msgpass-artifact` always stages Rust
  `build/rust/selected/usr/bin/test_msgpass.elf`.
- `FSQRV_BINS` defaults to the selected Rust helper instead of the former
  `quser/build/test/msgpass/test_msgpass.elf` path.
- The tracked `quser` override removes `test/msgpass` from the component list
  and deletes the nested C helper Makefile/source.
- `QSOE_RUST_TEST_MSGPASS=0` and `QSOE_TEST_MSGPASS_RC_ROLLBACK=1` are rejected
  because no C rollback artifact remains.

### Rollback

- Rollback available: no
- Rollback flag, artifact, or package: none after retirement
- Rollback command or procedure: none; recover by reverting the retirement PR
- Rollback window: closed after the `TEST_MSGPASS_RC.md` Rust-default RC and C
  rollback evidence
- Rollback limitations: intentional. This is the first retired component, and
  future production removals should remain more conservative.

### Test Evidence

- Prior RC evidence: `TEST_MSGPASS_RC.md`
- Prior trusted CI evidence: `container-rust-test-msgpass-smoke` accepted by
  trusted `main` CI run `28102250069` for #97
- Shell syntax: `bash -n scripts/apply-component-overrides.sh scripts/select-test-msgpass-artifact.sh scripts/rust-test-msgpass-smoke.sh scripts/test-msgpass-rc-smoke.sh scripts/rust-pipe-data-smoke.sh scripts/capture-elf-baseline.sh` -> pass
- Component override replay: `./scripts/apply-component-overrides.sh` -> pass
- Retired C flag rejection: `QSOE_RUST_TEST_MSGPASS=0 make test-msgpass-artifact` -> fails fast with status 2
- Retired rollback rejection: `QSOE_TEST_MSGPASS_RC_ROLLBACK=1 scripts/test-msgpass-rc-smoke.sh` -> fails fast with status 2
- Artifact audit: `make rust-test-msgpass-link-smoke` -> pass
- Selected artifact: `make test-msgpass-artifact` -> pass
- Rust quality gate: `make rust-check` -> pass
- Component smoke: `make rust-test-msgpass-smoke` -> pass
- Retired compatibility smoke: `make test-msgpass-rc-smoke` -> pass
- qrvfs production-root regression: `make check-qrvfs-rust-writer-production-root` -> pass
- Cross-smoke regression: `make rust-pipe-data-smoke` -> pass

### Known Limitations

- Missing test coverage: the retirement smoke covers the QSOE/L-targeted
  suite `[msgpass]` path, not a full hardware matrix.
- Unsupported hardware or image modes: none newly introduced; this helper is
  test-image only.
- Behavior differences from C: Rust emits debug startup and registration
  markers.
- Open follow-up issues: use #26 as the policy checklist for this first
  retirement and repeat the process for any future C removal.

### Review Notes

- Unsafe review: no new Rust unsafe code in the retirement wiring.
- Data or on-disk format migration: none.
- Operator impact: `test_msgpass` rollback flags now fail fast; use
  `make test-msgpass-rc-smoke` for the current Rust-only path.
- Link review: the Rust helper no-reply branch exits directly instead of
  importing `ProcessTerminate`; the suite owns QSOE/L server termination before
  entering the MCS-blocking no-reply send.

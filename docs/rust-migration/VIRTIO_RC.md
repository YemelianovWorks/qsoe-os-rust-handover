# `devb-virtio-rs` Rust-Default Release Candidate

Captured: 2026-06-28 20:46 CEST.

This historical note records the `devb-virtio` Rust-default
release-candidate path. It has been superseded by the C retirement recorded in
`VIRTIO_RETIREMENT.md`.

## Rust Migration: `devb-virtio`

Status: superseded by C retirement.
Release or build: `devb-virtio-rs-rc1`, introduced by the
`codex/virtio-rust-default-rc` branch.

### Language Change

- Previous default implementation: C `/sbin/devb-virtio`
- New RC default implementation: Rust `devb-virtio-rs`
- Rust artifact or crate: `rust/bins/devb-virtio-rs`, linked to
  `build/rust/qsoe-devb-virtio-rs.elf`
- C implementation status: retired by `VIRTIO_RETIREMENT.md`; this note keeps
  the prior RC and rollback evidence
- User-visible behavior changes: none expected for `/dev/vblk0`, `/usr`, or
  qrvfs readers

Known startup-log difference:

- C prints:
  - `devb-virtio: /dev/vblk0 ready (...)`
- Rust prints:
  - `[devb-virtio-rs] alive`
  - `[devb-virtio-rs] /dev/vblk0 ready`

## Rollback

- Rollback available now: no
- Historical rollback flag: `QSOE_VIRTIO_RC_ROLLBACK=1`
- Historical rollback command:

```sh
make virtio-rc-rollback-smoke
```

Default RC file-read smoke:

```sh
make virtio-rc-file-smoke
```

Rollback window: closed by the C retirement recorded in
`VIRTIO_RETIREMENT.md`.

Rollback limitations: the historical rollback path is no longer available.
`QSOE_VIRTIO_RC_ROLLBACK=1` is now rejected, and the top-level
`virtio-rc-rollback-smoke` target has been removed.

## Test Evidence

- Host tests: `make rust-quality`
- Artifact audit: `make rust-virtio-link-smoke`
- Existing opt-in file-read smoke: `make rust-virtio-file-smoke`
- Rust-default RC file-read smoke: `make virtio-rc-file-smoke`
- Historical C rollback file-read smoke: `make virtio-rc-rollback-smoke`

The file-read smoke boots QSOE/L, starts `/sbin/devb-virtio`, mounts the qrvfs
image from `/dev/vblk0` at `/usr`, and verifies `/bin/cat` can read
`/usr/conf/passwd` through the selected block driver before reaching `login:`.

## Known Limitations

- This RC did not remove C source; the later retirement PR removed it.
- The RC covers QSOE/L QEMU file-read behavior, not a full hardware release.
- Writes remain disabled through the resource-server surface, matching the C
  driver behavior.
- C retirement is now complete; this note remains as prior RC and rollback
  evidence.

## Review Notes

- Unsafe review: no new Rust unsafe code in this RC target wiring.
- Data or on-disk format migration: none.
- Operator impact: use `make virtio-rc-file-smoke` to validate the Rust-only
  compatibility path. There is no current C rollback command.

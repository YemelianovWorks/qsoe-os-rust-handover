# Task Manager Path Registry Rust-Default RC

Captured: 2026-06-30 CEST.

`tm_pathmgr` now has a Rust-default release-candidate path. Normal umbrella,
standalone `libtaskman`, and applied NQ/LQ taskman builds select Rust with:

```text
QSOE_RUST_TM_PATHMGR=1
```

The C implementation remains available as rollback:

```text
QSOE_RUST_TM_PATHMGR=0
TM_PATHMGR_RC_ROLLBACK=1
```

## Scope

The RC covers the portable path registry behind `tm_pathmgr.h`: fixed-pool
namespace nodes, registration, unregister-by-pid, longest-prefix resolution,
PMDIR child iteration, repath, symlink expansion, and CPIO symlink expansion
through the existing `tm_cpio_find_file` ABI.

It does not replace path IO dispatch, process creation, channel delivery,
device-server policy, filesystem serving, or seL4 object manipulation.

## Evidence

The focused evidence gate is:

```sh
make tm-pathmgr-evidence
```

It runs the C host model, Rust host tests, Rust archive audit, and NQ/LQ
taskman link audits for both Rust-default and C rollback membership.

The live RC gates are:

```sh
make tm-pathmgr-rc-smoke
make tm-pathmgr-rc-rollback-smoke
```

Both verify NQ/LQ `libtaskman.a` membership, then boot QSOE/L through the
runtime namespace smoke. The Rust-default path verifies C `pathmgr.o` is absent
and the shared Rust provider exports all `tm_pathmgr_*` ABI symbols. The C
rollback path verifies `pathmgr.o` remains present.

## Retirement

C is not retired. Keep `libtaskman/src/pathmgr.c` buildable through
`QSOE_RUST_TM_PATHMGR=0` until the RC has trusted CI evidence, #26's retirement
checklist is satisfied, and a separate removal PR explicitly removes the C
provider.

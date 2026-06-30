# Task Manager Sysfs C Retirement

Captured: 2026-06-30 CEST.

## Scope

The portable task-manager `tm_sysfs` provider is Rust-only. The retirement
removes:

- `libtaskman/src/tm_sysfs.c`
- `tests/tm_sysfs_model_test.c`

The Rust provider remains in `rust/crates/qsoe-tm-sysfs/src/lib.rs` and
exports the existing `tm_sysfs_*` ABI declared by
`libtaskman/include/tm_sysfs.h`.

This retirement covers only the portable read-only `/sys` model for
`board`, `builddate`, `cmdline`, `osname`, and `version`. Syscfg/FDT
discovery, sysmap construction, path dispatch, process lifecycle, IPC, and
seL4 object code remain C.

## Selector State

`QSOE_RUST_TM_SYSFS=1` is mandatory in umbrella, NQ, LQ, and standalone
`libtaskman` builds. `QSOE_RUST_TM_SYSFS=0` now fails fast.

The rollback smoke target was removed:

```sh
make tm-sysfs-rc-rollback-smoke
make container-tm-sysfs-rc-rollback-smoke
```

The retained checks are:

```sh
make check-tm-sysfs-model
make tm-sysfs-evidence
make tm-sysfs-runtime-smoke
make tm-sysfs-rc-smoke
```

`make tm-sysfs-evidence` verifies Rust host tests, soft-float archive shape,
all exported `tm_sysfs_*` symbols in the Rust provider archive, NQ/LQ taskman
links without C `tm_sysfs.o` in `libtaskman.a`, and retired selector rejection
for NQ, LQ, and standalone `libtaskman`.

`make tm-sysfs-rc-smoke` verifies the retired/default path and boots QSOE/L
through `/sys` readdir plus all five portable `/sys` file reads.

## Adjacent Evidence

Adjacent task-manager evidence scripts that previously pinned
`QSOE_RUST_TM_SYSFS=0` now pin `QSOE_RUST_TM_SYSFS=1`:

- `scripts/tm-cpio-evidence.sh`
- `scripts/tm-fdt-evidence.sh`
- `scripts/tm-pathmgr-evidence.sh`
- `scripts/tm-rsrcdb-evidence.sh`
- `scripts/tm-script-evidence.sh`

This keeps those tests focused on their own provider while respecting the
retired tm_sysfs selector.

## Reintroduction Rule

Do not reintroduce C `tm_sysfs` rollback without a new issue, explicit
rollback justification, and fresh PR evidence.

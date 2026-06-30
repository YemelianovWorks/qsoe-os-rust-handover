# Task Manager Syscfg C Retirement

Captured: 2026-06-30 CEST.

## Scope

The portable task-manager `tm_syscfg` provider is Rust-only. The retirement
removes:

- `libtaskman/src/syscfg.c`
- `tests/tm_syscfg_model_test.c`

The Rust provider remains in `rust/crates/qsoe-tm-syscfg/src/lib.rs` and
exports the existing `tm_syscfg_*` ABI declared by
`libtaskman/include/tm_syscfg.h`.

This retirement covers only the portable caller-owned TLV builder/walker in
`libtaskman`. LQ's private FDT-backed runtime builder
`lq/taskman/sys/syscfg.c` remains C by design.

## Selector State

`QSOE_RUST_TM_SYSCFG=1` is mandatory in umbrella, NQ, LQ, and standalone
`libtaskman` builds. `QSOE_RUST_TM_SYSCFG=0` now fails fast.

The rollback smoke target was removed:

```sh
make tm-syscfg-rc-rollback-smoke
make container-tm-syscfg-rc-rollback-smoke
```

The retained checks are:

```sh
make check-tm-syscfg-model
make tm-syscfg-evidence
make tm-syscfg-runtime-smoke
make tm-syscfg-rc-smoke
```

`make tm-syscfg-evidence` verifies Rust host tests, soft-float archive shape,
all exported `tm_syscfg_*` symbols in the Rust provider archive, NQ/LQ taskman
links without C `syscfg.o` in `libtaskman.a`, and retired selector rejection
for NQ and LQ.

`make tm-syscfg-rc-smoke` verifies the retired/default path and boots QSOE/L
through `/sys/board`, `/sys/cmdline`, and `/usr/bin/sysinfo` syscfg consumers.

## Adjacent Evidence

Adjacent task-manager evidence scripts that previously pinned
`QSOE_RUST_TM_SYSCFG=0` now pin `QSOE_RUST_TM_SYSCFG=1`:

- `scripts/tm-fdt-evidence.sh`
- `scripts/tm-pathmgr-evidence.sh`
- `scripts/tm-rsrcdb-evidence.sh`

This keeps those tests focused on their own provider while respecting the
retired tm_syscfg selector.

## Reintroduction Rule

Do not reintroduce C `tm_syscfg` rollback without a new issue, explicit
rollback justification, and fresh PR evidence.

# Task Manager FDT C Retirement

Captured: 2026-07-01 CEST.

## Scope

This retires the LQ C `tm_fdt` provider after the Rust-default RC documented in
`TASK_MANAGER_FDT_RC.md`.

Removed by component overrides:

```text
lq/taskman/sys/fdt.c
```

Kept as ABI surface for taskman C callers:

```text
lq/taskman/sys/fdt.h
```

Rust `qsoe-tm-fdt` is mandatory through the shared `qsoe-tm-providers` archive.
`QSOE_RUST_TM_FDT=0` now fails fast for LQ taskman and provider builds, and the
former `tm-fdt-rc-rollback-smoke` make target is removed.

## Evidence Gate

The retirement gate is:

```sh
bash -n scripts/tm-fdt-evidence.sh scripts/tm-fdt-runtime-smoke.sh scripts/tm-fdt-rc-smoke.sh scripts/build-rust-tm-providers.sh scripts/apply-component-overrides.sh
./scripts/apply-component-overrides.sh
cargo test --manifest-path rust/Cargo.toml -p qsoe-tm-fdt --features host-tests
make tm-fdt-evidence
make tm-fdt-runtime-smoke
make tm-fdt-rc-smoke
make tm-providers-evidence
QSOE_RUST_TM_FDT=0 make -C lq taskman
QSOE_RUST_TM_FDT=0 make -C lq/taskman all
QSOE_RUST_TM_FDT=0 scripts/build-rust-tm-providers.sh build/tm-fdt-retired-selector.a
```

The last three commands are expected to fail with a retired-selector diagnostic.

## Acceptance

- `lq/taskman/sys/fdt.c` is absent after component overrides.
- LQ taskman dry-run plans omit `/sys/fdt.o`.
- LQ taskman links through `libqsoe_tm_providers.a`.
- The Rust provider archive exports all nine `tm_fdt_*` ABI symbols.
- The runtime smoke reaches `/chosen`, syscfg/sysmap, `/sys`, and `sysinfo`
  consumers on the Rust-only path.
- Adjacent taskman evidence no longer pins `QSOE_RUST_TM_FDT=0` to isolate
  other providers.

## Residual Boundary

This retirement removes only the duplicate C FDT parser provider. Broader PCI,
memory topology, and boot-platform validation remain separate platform work and
should not reintroduce a taskman-local C parser rollback.

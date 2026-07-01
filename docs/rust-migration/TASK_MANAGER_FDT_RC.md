# Task Manager FDT Rust-Default RC

Captured: 2026-06-30 CEST.

Status: historical Rust-default RC; superseded by
`TASK_MANAGER_FDT_RETIREMENT.md`.

`tm_fdt` entered a Rust-default release-candidate window for QSOE/L before C
retirement. Normal LQ taskman builds used `qsoe-tm-fdt` through the shared
`qsoe-tm-providers` archive while the C parser in `lq/taskman/sys/fdt.c`
remained available as explicit rollback with `QSOE_RUST_TM_FDT=0`.

## Historical Selectors

```sh
make tm-fdt-rc-smoke
make tm-fdt-rc-rollback-smoke
make tm-fdt-evidence
QSOE_RUST_TM_FDT=0 make -C lq taskman
```

The RC smoke verified that the LQ taskman link plan omitted `sys/fdt.o`, built
the Rust-default taskman, and booted through the FDT runtime smoke. The rollback
smoke verified that the C rollback link plan included `sys/fdt.o` and booted
the same `/chosen`, `/sys`, and `sysinfo` consumers with
`QSOE_RUST_TM_FDT=0`.

## Evidence Boundary

The RC covered the current QEMU/LQ boot consumers:

- `/chosen` bootargs;
- syscfg and sysmap construction markers;
- `/sys/board`;
- `/sys/cmdline`;
- `/usr/bin/sysinfo`.

Current state: C `sys/fdt.c` is retired, `QSOE_RUST_TM_FDT=0` is rejected, and
`make tm-fdt-rc-smoke` now validates the Rust-only path.

# Task Manager FDT Parser Rust Provider

Captured: 2026-07-01 CEST.

## Scope

`qsoe-tm-fdt` is the retired-C Rust provider for the LQ task-manager
device-tree blob parser. The C ABI header remains in:

```text
lq/taskman/sys/fdt.h
```

The previous C implementation source is removed by component overrides:

```text
lq/taskman/sys/fdt.c
```

The Rust provider exports the existing `tm_fdt_*` ABI and preserves the minimal
big-endian FDT walker behavior used by LQ boot consumers.

It covers:

- header magic and last-compatible-version validation;
- total-size reporting;
- absolute path lookup with root handling, NOP skipping, and `name@unit`
  matching;
- raw property lookup through the strings block;
- u32, u64, and NUL-terminated string property helpers;
- first compatible-node search;
- indexed `reg` tuple decoding with caller-supplied address/size cell counts.

It does not replace:

- FDT discovery from bootinfo;
- syscfg construction policy;
- sysmap construction;
- initrd loading;
- path, process, memory, capability, IRQ, or seL4 invocation code.

## Selector

Normal LQ taskman builds select Rust through the shared provider archive:

```sh
make -C lq taskman
```

`QSOE_RUST_TM_FDT=0` is now rejected after C retirement. The rollback target is
removed, and the retirement evidence checks that `sys/fdt.o` is absent from the
LQ taskman link plan.

Active targets:

```sh
make rust-tm-fdt-provider
make tm-fdt-evidence
make tm-fdt-runtime-smoke
make tm-fdt-rc-smoke
```

Multiple taskman Rust providers are selected together through
`qsoe-tm-providers`, which packages provider crates behind one no-std panic
handler. The historical `make rust-tm-fdt-provider` target still writes the
focused archive path used by component evidence.

## Evidence

Historical RC evidence is captured in `TASK_MANAGER_FDT_RC.md`. The retirement
path now verifies:

- Rust host tests pass for header validation, size reporting, path lookup,
  raw/string/u32 property reads, compatible-node search, malformed string
  rejection, and `reg` tuple decoding;
- Rust provider archive members report RVC soft-float ABI;
- Rust provider archive exports all nine `tm_fdt_*` ABI symbols;
- `lq/taskman/sys/fdt.c` is absent after component overrides;
- LQ Rust-only taskman omits `sys/fdt.o` and links
  `libqsoe_tm_providers.a`;
- linked taskman ELFs pass the evidence script's ELF flag and section audit;
- `QSOE_RUST_TM_FDT=0` is rejected by LQ taskman and provider-archive builds;
- `make tm-fdt-runtime-smoke` boots through `/chosen` bootargs,
  syscfg/sysmap construction, `/sys`, and `sysinfo` consumers.

Expected runtime markers:

```text
Boot command line: mainfs=/dev/vblk0
syscfg built from FDT
sysmap page built
tm-fdt-runtime-smoke: /sys/board FDT compatible ok
tm-fdt-runtime-smoke: /chosen bootargs ok
tm-fdt-runtime-smoke: /usr/bin/sysinfo FDT sysinfo ok
```

## Retirement Boundary

The retired provider covers the parser ABI and the QEMU/LQ boot-consumer path.
It intentionally does not move broader PCI discovery, memory-topology policy,
or kernel-adjacent boot decisions into Rust. Those remain separate platform
validation concerns rather than a reason to keep a duplicate C `tm_fdt` parser
rollback.

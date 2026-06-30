# Task Manager Syscfg Retired Rust Provider

Captured: 2026-06-30 CEST.

## Scope

`qsoe-tm-syscfg` is the mandatory Rust provider for the portable task-manager
syscfg TLV builder/walker behind the existing `tm_syscfg.h` ABI. The retired C
provider was `libtaskman/src/syscfg.c`.

This covers:

- caller-owned `tm_syscfg_state_t` initialization;
- raw TLV emit;
- little-endian `u32` and `u64` emit;
- ASCIZ emit, including the existing empty-string skip behavior;
- END sentinel finalization;
- finalized blob get;
- raw find plus typed `u32` and `u64` find helpers.

This does not replace:

- LQ's private global `lq/taskman/sys/syscfg.c` FDT-backed builder;
- FDT parsing;
- sysmap page construction;
- boot platform-data policy;
- `/sys` file serving;
- process creation, capability ownership, relocation, loader admission, or
  seL4 object manipulation.

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

When a taskman link needs Rust providers, the selected provider is packaged in
the shared `build/rust/tm-providers/libqsoe_tm_providers.a` archive. Legacy
targets such as `make rust-tm-syscfg-provider` still produce the historical
single-provider output path for focused evidence.

## Evidence

Validation commands:

```sh
make check-tm-syscfg-model
cargo test --manifest-path rust/Cargo.toml -p qsoe-tm-syscfg --features host-tests
cargo clippy --manifest-path rust/Cargo.toml -p qsoe-tm-syscfg --features host-tests -- -D warnings
make rust-tm-syscfg-provider
make tm-syscfg-evidence
make tm-syscfg-runtime-smoke
make tm-syscfg-rc-smoke
```

`make tm-syscfg-evidence` verifies:

- Rust host tests pass for the exported ABI behavior;
- Rust staticlib builds for `riscv64imac-unknown-none-elf`;
- Rust provider archive members report RVC soft-float ABI;
- Rust provider archive exports all `tm_syscfg_*` symbols;
- NQ and LQ `libtaskman.a` contain zero C `syscfg.o` members;
- NQ and LQ reject `QSOE_RUST_TM_SYSCFG=0`;
- NQ and LQ taskman links complete in Rust-only mode.

`make tm-syscfg-runtime-smoke` verifies the Rust-selected taskman build in a
booted LQ image. The smoke:

- rebuilds QSOE/L with mandatory `QSOE_RUST_TM_SYSCFG=1` and
  `QSOE_RUST_TM_PROCFS=1`;
- verifies `libtaskman.a` no longer contains C `syscfg.o`;
- verifies the selected Rust provider archive exports `tm_syscfg_init`;
- waits for taskman's `syscfg built from FDT` and `sysmap page built` boot
  markers;
- reads `/sys/board` and `/sys/cmdline`, checking that cmdline carries
  `mainfs=/dev/vblk0`;
- runs `/usr/bin/sysinfo`, which reads `/sys` identity data and syscfg-derived
  CPU/sysmap state.

This runtime smoke proves that a Rust-only portable `tm_syscfg` taskman build
still boots and serves syscfg-backed consumers. It does not prove that LQ's
private global `lq/taskman/sys/syscfg.c` was replaced; that private FDT-backed
runtime builder remains C by design.

## Reintroduction Rule

Do not reintroduce C `tm_syscfg` rollback without a new issue, explicit
rollback justification, and fresh PR evidence.

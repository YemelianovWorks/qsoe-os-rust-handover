# QSOE Rust Toolchain Spike

This document records the first Rust scaffold. It is opt-in and does not change
the normal C build or boot images.

## Local Toolchain

The initial pinned toolchain is:

```text
rustc 1.95.0
cargo 1.95.0
```

The pin lives in `rust/rust-toolchain.toml`.

The local macOS host has Rust installed, but does not currently have the
RISC-V GNU linker or `readelf` tools on `PATH`.

## Workspace

Initial layout:

```text
rust/
  Cargo.toml
  rust-toolchain.toml
  .cargo/config.toml
  targets/riscv64-qsoe-user.json
  crates/qsoe-abi/
  bins/minimal-rs/
```

Crates:

- `qsoe-abi`: `no_std` constants and initial `#[repr(C)]` ABI structures.
- `qsoe-minimal-rs`: `no_std` staticlib exporting C ABI `main()`.

## Checks

Run host checks:

```sh
make rust-check
```

This performs:

- `cargo fmt --check`.
- `cargo check --workspace`.
- `cargo clippy --workspace -- -D warnings`.

Run the compile-only RISC-V spike after installing the Rust target:

```sh
rustup target add riscv64gc-unknown-none-elf
QSOE_RUST_COMPILE=1 scripts/rust-check.sh
```

## Target Strategy

Two targets are present:

- `riscv64gc-unknown-none-elf`: Rust built-in bare-metal target used for the
  first compile-only spike.
- `rust/targets/riscv64-qsoe-user.json`: QSOE-specific target description for
  the eventual QSOE userland contract.

The custom target records:

- RISC-V 64-bit little-endian.
- `lp64d` ABI.
- `panic=abort`.
- PIC relocation model for Rust code that will be linked into QSOE userland.
- GNU-CC linker flavor with `riscv64-linux-gnu-gcc`.
- no PIE as a target default.

The built-in target is used first because stable Cargo can consume its prebuilt
`core` once installed. The custom target remains the contract target and should
be promoted after the build can reliably provide `core` for it.

## QSOE Link Smoke

After a normal QSOE source build has produced `nq/build/libc/crt0.o` and
`nq/build/libc/libc.so`, run:

```sh
make rust-qsoe-link-smoke
```

The script links:

```text
crt0.o + libqsoe_minimal_rs.a + libc.so + libgcc
```

with the existing QSOE userland link contract:

- `-nostdlib`.
- `-no-pie`.
- dynamic interpreter `/lib/ld-qsoe.so.1`.
- `-z now`.
- unresolved symbols in shared libraries deferred the same way as current
  C userland links.

The output is:

```text
build/rust/qsoe-minimal-rs.elf
```

The script then runs `scripts/audit-elf.sh --strict-qsoe-user` when the link
succeeds.

## Current Limitation

On the current macOS host, the QSOE link smoke is expected to stop before link
because `riscv64-linux-gnu-gcc` and compatible `readelf` tools are not present.
Run the full link smoke in the Debian/Ubuntu/container source-build
environment.

The checked-in Debian container path is:

```sh
make container-toolchain-build
make container-source-build
make container-rust-qsoe-link-smoke
```

See `TOOLCHAIN.md` for runtime setup and QEMU version caveats.

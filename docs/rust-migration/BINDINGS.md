# QSOE Rust Bindings

This document records the first Rust ABI and resource-server binding layer.
The scope is deliberately narrow: compile-time shape checks and raw FFI first,
then small wrappers that can support a `slogger-rs` pilot.

## Crates

### `qsoe-abi`

`qsoe-abi` is `no_std` and contains C-compatible public ABI shapes:

- QSOE scalar aliases such as `PidT`, `ModeT`, `OffT`, `SizeT`, and `SsizeT`.
- IPC constants such as `TASKMAN_COID`, `TASKMAN_CHID`, `QSOE_MI_PULSE`, and
  `QSOE_RCVID_SAVED`.
- `#[repr(C)]` message structs:
  - `QsoeMsgInfo`
  - `QsoeCredInfo`
  - `QsoeClientInfo`
  - `QsoePulse`

Layout tests assert the current RV64 C ABI sizes and alignments.

### `qsoe-ffi`

`qsoe-ffi` is `no_std` and exposes raw `extern "C"` bindings for actual
linkable QSOE libc symbols:

- channel and connection calls.
- `MsgSend` / `MsgReceive` / `MsgReply` / pulse calls.
- `_r` message and connection variants where available.
- `procmgr_detach`.
- path-manager registration and resolve.
- debug write.
- `qsoe_mmap` and `qsoe_alloc_phys`.

`qsoe_errno` is not bound because it is a C macro over the current thread
block, not a linkable symbol. Rust code should prefer `_r` forms or C helper
APIs that already return negative errno.

### `qsoe-ressrv`

`qsoe-ressrv` is `no_std` and mirrors `quser/ressrv/include/ressrv.h`:

- `Attr`
- `Open`
- `Handle`
- `ProviderVtable`
- `Provider`
- `Server`
- opaque `Call`

It also exposes raw C lifecycle functions and thin wrappers for:

- provider initialization.
- provider listen.
- dispatch loop entry.

Layout tests assert the current RV64 C ABI sizes and alignments.

## Current Boundary

The bindings are suitable for compiling Rust code that can describe QSOE
resource-server objects and link against the C framework later.

They are not yet a full safe resource-server framework. In particular:

- callback implementors still receive raw provider/handle pointers.
- method callbacks are `unsafe extern "C"` functions.
- buffer validity remains the caller/framework contract.
- no Rust allocator contract is assumed.
- `qsoe_errno` access is intentionally absent.

## Validation

Run:

```sh
make rust-check
```

This includes:

- formatting.
- workspace `cargo check`.
- clippy with warnings denied.
- layout tests for `qsoe-abi` and `qsoe-ressrv`.

Run the RISC-V compile-only path:

```sh
QSOE_RUST_COMPILE=1 scripts/rust-check.sh
```

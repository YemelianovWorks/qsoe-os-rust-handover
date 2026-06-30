# Task Manager CPIO Provider

Captured: 2026-06-30 CEST.

`tm_cpio` is a bounded task-manager Rust provider for the portable in-memory
`newc` archive model. The C provider is retired; the remaining C header is the
ABI consumed by task-manager C code:

```text
libtaskman/include/tm_cpio.h
rust/crates/qsoe-tm-cpio
```

## Scope

The Rust provider exports the existing `tm_cpio.h` C ABI:

```text
int tm_cpio_check_valid(const uint8_t *data, uint64_t size);
void tm_cpio_iterate(const uint8_t *data, uint64_t size,
                     tm_cpio_callback_t cb, void *user);
int tm_cpio_find_file(const uint8_t *data, uint64_t size,
                      const char *filename, tm_cpio_file_info_t *info);
int tm_cpio_resolve_path(const uint8_t *data, uint64_t size,
                         const char *path,
                         char *out_path, unsigned out_cap,
                         tm_cpio_file_info_t *out_info);
int tm_cpio_dirent_at(const uint8_t *data, uint64_t size,
                      const char *prefix,
                      uint32_t index,
                      unsigned *out_type,
                      char *out_name_buf, unsigned out_name_cap,
                      unsigned *out_namelen);
int tm_cpio_dir_exists(const uint8_t *data, uint64_t size,
                       const char *prefix);
```

It owns only byte-level archive walking, exact file lookup, symlink resolution,
directory-entry synthesis, and directory-existence checks over caller-owned
archive bytes. It does not replace taskman spawn, CPIO-backed file descriptor
state, path-manager registration, ELF loading, relocation, process creation,
or any seL4 object manipulation.

The provider intentionally preserves the C walker's forgiving behavior:
iteration stops at the first malformed header or `TRAILER!!!` entry rather than
turning malformed later records into hard errors. It also follows the C
implementation's absolute pointer-alignment behavior when the archive pointer
itself is not 4-byte aligned.

## Selector

Normal taskman builds always select the Rust provider. `QSOE_RUST_TM_CPIO=0`
now fails fast in top-level, `libtaskman`, NQ, LQ, and provider-archive build
paths because there is no C rollback provider left.

```text
build/rust/tm-providers/libqsoe_tm_providers.a
```

The archive is built for `riscv64imac-unknown-none-elf` so it matches
taskman's soft-float ABI.

`libtaskman/Makefile` no longer builds `cpio.o`, and the NQ/LQ taskman links
add the shared provider archive. Multiple taskman Rust providers may be
selected together. The shared
`qsoe-tm-providers` archive packages the selected provider crates behind one
no-std panic handler.

## Evidence

The Rust host model is covered by:

```sh
make check-tm-cpio-model
```

That target runs `qsoe-tm-cpio` host tests for archive iteration, exact lookup,
symlink resolution, directory existence, directory-entry synthesis, short
output buffers, missing paths, and malformed-archive stopping behavior.

The Rust-only evidence gate is:

```sh
make tm-cpio-evidence
```

It runs the Rust host tests, builds and audits the Rust staticlib, checks
exported archive and linked taskman symbols, verifies all archive members are
RVC soft-float, links both NQ and LQ taskman without C `cpio.o`, and verifies
that `QSOE_RUST_TM_CPIO=0` is rejected.

The focused runtime smoke is:

```sh
make tm-cpio-runtime-smoke
```

It rebuilds QSOE/L with mandatory Rust `tm_cpio` and `tm_procfs`, injects a
temporary sysinit fragment, and boots the image. The fragment verifies CPIO-root
symlink readlink output (`/etc -> /usr/conf` and `/home -> /usr/home`),
`/etc/passwd` access through the CPIO symlink into mounted `/usr`, direct
`/sbin/init` reads from the boot CPIO, and `/bin/sh` symlink spawn.

The retired compatibility smoke is:

```sh
make tm-cpio-rc-smoke
```

It preserves the former RC smoke entry point while enforcing the retired
Rust-only selector. `TM_CPIO_RC_ROLLBACK=1` and `QSOE_RUST_TM_CPIO=0` both fail
fast.

The multi-provider link gate is:

```sh
make tm-providers-evidence
```

It currently selects `tm_cpio` and `tm_procfs` together to prove the shared
archive, single panic handler, final taskman ELF audits, and dual-provider
`/proc` smoke.

## Current State

`tm_cpio` is a retired C provider. The Rust `qsoe-tm-cpio` implementation is
mandatory in taskman, and no C rollback target remains.

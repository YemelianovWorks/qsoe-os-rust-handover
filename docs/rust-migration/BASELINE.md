# QSOE Rust Migration Baseline

This baseline records the release tree and preflight assumptions for the Rust
migration work. It should be updated whenever the baseline release moves.

## Release Components

The umbrella release is `os` `v0.1`.

| Component | Version | Commit |
| --- | --- | --- |
| `os` | `v0.1` | `9941a1bd879e889767800d3b331e6ac3fb0b2ab0` |
| `lq` | `v0.14` | `a7e72fc815e892e8f0c212bf26f9b4429ec8154a` |
| `nq` | `v0.17` | `f5bae1029f008d3c731bc1cee09214feec24d391` |
| `libc` | `v0.6` | `d71bf2169a7c3e49648c5b8ab6fbb483547eb1b7` |
| `quser` | `v0.5` | `8dc6cec5e00a92753c96807ac9430e0b3a1725d0` |
| `mr-bml` | `v0.5` | `a31e6f492726ab61fdc7c9e5b5b3a5a174824472` |

`component.list` maps the `os` `v0.1` release to `lq` `0.14`, `nq` `0.17`,
`libc` `0.6`, and `quser` `0.5`.

## Local Host Modes

### macOS: run prebuilt or already-built images

The local macOS host has:

- `qemu-system-riscv64` at `/opt/homebrew/bin/qemu-system-riscv64`.
- QEMU `11.0.1`.

The macOS path is suitable for running QEMU images that are already built. It
is not the expected source-build environment because the release Makefiles
expect Linux-style package/tool names such as:

- `riscv64-linux-gnu-gcc`.
- `python3-kconfiglib`.
- OpenSBI firmware paths used by `nq/emu.sh`.

Use this mode for boot smoke after artifacts exist:

```sh
scripts/boot-smoke.sh -k lq
```

### Debian/Ubuntu or container: source build

Use a Debian/Ubuntu environment, or a container with equivalent packages, for
source builds:

```sh
apt install gcc-riscv64-linux-gnu qemu-system-misc opensbi python3-kconfiglib
```

The checked-in container path is documented in `TOOLCHAIN.md`.

Expected source-build flow:

```sh
make prepare
make
```

Then boot through the per-kernel launchers or the smoke wrapper:

```sh
scripts/boot-smoke.sh -k lq
scripts/boot-smoke.sh -k nq
```

## Boot Smoke Contract

The boot smoke helper is:

```sh
scripts/boot-smoke.sh [-k lq|nq] [-t seconds] [-o log] [-- <emu args>]
```

Default mode boots QSOE/L because the current local QEMU path has already
successfully reached the QSOE/L login prompt.

The smoke check waits for:

- `QSOE/` banner.
- `[slogger] alive`.
- `fs-qrv: mounted qrvfs at /usr`.
- `login:`.

QSOE/L-specific markers:

- `spawning /sbin/init`.
- `dispatcher ready`.
- `devb-virtio: /dev/vblk0 ready`.

QSOE/N-specific markers:

- `QSOE/N taskman starting`.
- `taskman: cmdline:`.
- `devb-nvme: ready`.

Successful smoke logs are kept under `build/` by default. Failed boots also
keep the log and print the last 80 lines.

## Known Baseline Boot Warnings

These messages are known in the current QSOE/L debug boot and should not be
treated as Rust migration regressions unless their frequency or surrounding
behavior changes:

```text
<<seL4(CPU 0) [decodeUntypedInvocation/204 ...]: Untyped Retype: Insufficient memory ...>>
```

The current QSOE/L QEMU board may also report:

```text
rtc: this board carries no battery-backed clock the kernel recognizes
```

The expected behavior after these messages is continued boot through storage
driver startup, qrvfs mount, sysinit, and login.

## Artifact Audit Contract

The artifact audit helper is:

```sh
scripts/audit-elf.sh [--strict-qsoe-user] <elf> [<elf> ...]
```

It reports:

- ELF header.
- Program headers and interpreter.
- Dynamic section.
- Relocations.
- TLS and unwind-related sections.
- Undefined dynamic symbols.

`--strict-qsoe-user` fails on TLS, unwind sections, or relocation types outside
the current userland relocation baseline:

- `R_RISCV_RELATIVE`.
- `R_RISCV_64`.
- `R_RISCV_JUMP_SLOT`.

The script requires one of:

- `llvm-readelf`.
- `readelf`.
- `riscv64-linux-gnu-readelf`.
- `greadelf`.

No compatible `readelf` tool was found on the current macOS PATH during this
baseline pass, so artifact audit execution needs that tool installed or a
Linux/container environment.

## Existing In-Guest Test Helpers

Known in-guest test helpers to preserve and reuse:

- `quser/test/msgpass/main.c`: focused message-passing helper.
- `quser/test/syncspace/main.c`: sync-space helper.
- `quser/test/suite/*`: broader in-guest suite covering IPC, sync, timers,
  stdio, process behavior, and related userland surfaces.

The qrvfs image staging path installs these, when built, under `/usr/bin`:

- `suite`.
- `test_msgpass`.
- `test_syncspace`.
- `time`.
- `sysinfo`.

## First Rust Preflight Gate

Before a Rust binary enters any boot image:

1. The C baseline boots to login with `scripts/boot-smoke.sh`.
2. The Rust artifact passes `scripts/audit-elf.sh --strict-qsoe-user`.
3. The Rust artifact is opt-in only.
4. The C artifact remains the default rollback path.

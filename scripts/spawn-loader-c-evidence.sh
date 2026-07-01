#!/usr/bin/env bash
set -eu

ROOT="$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)"
MAKE="${MAKE:-make}"
WORKDIR="${SPAWN_LOADER_C_EVIDENCE_WORKDIR:-$ROOT/build/spawn-loader-c-evidence}"
BOOT_LOG="$WORKDIR/boot-spawn-loader-c.log"
SUMMARY="$WORKDIR/summary.txt"
SPAWN_SRC="$ROOT/lq/taskman/proc/spawn.c"

fail() {
    printf 'spawn-loader-c-evidence: %s\n' "$*" >&2
    exit 1
}

require_log_regex() {
    pattern="$1"
    label="$2"
    if ! grep -Eq "$pattern" "$BOOT_LOG"; then
        fail "missing boot evidence: $label ($pattern)"
    fi
}

reject_log_fixed() {
    pattern="$1"
    label="$2"
    if grep -Fq "$pattern" "$BOOT_LOG"; then
        fail "unexpected boot failure evidence: $label ($pattern)"
    fi
}

require_source_fixed() {
    pattern="$1"
    label="$2"
    if ! grep -Fq "$pattern" "$SPAWN_SRC"; then
        fail "missing source evidence: $label ($pattern)"
    fi
}

mkdir -p "$WORKDIR"

"$ROOT/scripts/apply-component-overrides.sh"
"$MAKE" -C "$ROOT/lq" --no-print-directory

QSOE_BOOT_EXTRA_PATTERNS="$(printf '%s\n' \
    'spawning /sbin/init' \
    'dispatcher ready')" \
QSOE_BOOT_VIRTIO_PATTERN="/dev/vblk0 ready" \
    "$ROOT/scripts/boot-smoke.sh" -k lq -t 180 -o "$BOOT_LOG" -- --debug=1

require_log_regex 'spawn: bin/(sh|qsh) e_type=.*interp=yes' 'shebang interpreter resolved through the C spawn/loader path'
require_log_regex 'spawn: .*e_type=.*interp=yes' 'dynamic ELF interpreter path observed during boot'

reject_log_fixed 'spawn: shebang interpreter not found' 'missing shebang interpreter'
reject_log_fixed 'spawn: nested shebang not supported' 'nested shebang rejection'
reject_log_fixed 'spawn: not an ELF' 'bad ELF rejection'
reject_log_fixed 'spawn: not ELF64' 'non-ELF64 rejection'
reject_log_fixed 'spawn: rtld not in cpio' 'missing runtime linker'
reject_log_fixed 'spawn: lib/libc.so not in cpio' 'missing libc'
reject_log_fixed 'tm_spawn returned non-zero' 'boot init spawn failure'

require_source_fixed 'spawn: shebang interpreter not found' 'missing interpreter failure path'
require_source_fixed 'spawn: nested shebang not supported' 'nested shebang failure path'
require_source_fixed 'spawn: not an ELF' 'bad ELF failure path'
require_source_fixed 'spawn: not ELF64' 'ELF class failure path'
require_source_fixed 'spawn: rtld not in cpio' 'runtime linker failure path'
require_source_fixed 'spawn: lib/libc.so not in cpio' 'libc failure path'
require_source_fixed 'reloc skip:' 'relocation skip diagnostic path'
require_source_fixed 'tm_spawn_read_args:' 'argument readback diagnostic path'

{
    printf 'spawn/loader C evidence complete\n'
    printf 'boot_log=%s\n' "$BOOT_LOG"
    printf 'source=%s\n' "$SPAWN_SRC"
    printf 'observed=shebang interpreter, dynamic ELF interpreter, clean boot without spawn/loader failure logs\n'
} > "$SUMMARY"

printf 'spawn-loader-c-evidence: wrote %s\n' "$SUMMARY"

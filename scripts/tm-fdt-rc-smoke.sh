#!/usr/bin/env bash
#
# Validate the retired tm_fdt Rust-only selector.

set -eu

usage() {
    cat <<'EOF_USAGE'
usage: scripts/tm-fdt-rc-smoke.sh [-t seconds] [-o log] [--keep-running] [-- <emu args>]

Builds LQ taskman with retired Rust tm_fdt, verifies C sys/fdt.o is absent from
the link plan, then reuses the live tm_fdt runtime smoke.

Environment:
  TM_FDT_RC_ROLLBACK  no longer supported after C tm_fdt retirement
  QSOE_RUST_TM_FDT    must remain 1 after C tm_fdt retirement
  TM_FDT_RC_WORKDIR   output directory, default build/tm-fdt-rc
EOF_USAGE
}

ROOT=$(cd "$(dirname "$0")/.." && pwd)
MAKE=${MAKE:-make}
workdir=${TM_FDT_RC_WORKDIR:-"$ROOT/build/tm-fdt-rc"}
has_log=0

for arg in "$@"; do
    case "$arg" in
        -h|--help)
            usage
            exit 0
            ;;
        -o|--log)
            has_log=1
            ;;
        --)
            break
            ;;
    esac
done

fail() {
    echo "tm-fdt-rc-smoke.sh: $*" >&2
    exit 1
}

capture_lq_plan() {
    local log=$1

    "$MAKE" -C "$ROOT/lq/taskman" --no-print-directory -B -n all \
        LIBTASKMAN_A="$ROOT/lq/build/libtaskman/libtaskman.a" \
        LIBTASKMAN_INC="$ROOT/libtaskman/include" \
        QSOE_RUST_TM_CPIO=1 \
        QSOE_RUST_TM_CRED=1 \
        QSOE_RUST_TM_ELF=1 \
        QSOE_RUST_TM_FDT=1 \
        QSOE_RUST_TM_PATHMGR=1 \
        QSOE_RUST_TM_PROCFS=1 \
        QSOE_RUST_TM_PSEUDODEV=1 \
        QSOE_RUST_TM_RSRCDB=1 \
        QSOE_RUST_TM_SCRIPT=1 \
        QSOE_RUST_TM_SYSCFG=1 \
        QSOE_RUST_TM_SYSMAP=1 \
        QSOE_RUST_TM_SYSFS=1 \
        > "$log"
}

require_fdt_plan() {
    local label=$1
    local log=$2
    local expected=$3
    local count

    count=$(grep -Fo '/sys/fdt.o' "$log" | wc -l | tr -d ' ')
    printf '%s sys/fdt.o plan count: %s\n' "$label" "$count" |
        tee "$workdir/$label-fdt-plan-membership.txt"
    [ "$count" -eq "$expected" ] ||
        fail "$label expected $expected sys/fdt.o dry-run entries, got $count"
}

mkdir -p "$workdir"
"$ROOT/scripts/apply-component-overrides.sh"

case "${TM_FDT_RC_ROLLBACK:-0}" in
    0|false|FALSE|no|NO)
        ;;
    1|true|TRUE|yes|YES)
        echo "tm-fdt-rc-smoke.sh: TM_FDT_RC_ROLLBACK is retired; C tm_fdt rollback no longer exists" >&2
        exit 2
        ;;
    *)
        echo "tm-fdt-rc-smoke.sh: TM_FDT_RC_ROLLBACK must be 0 or 1" >&2
        exit 2
        ;;
esac

case "${QSOE_RUST_TM_FDT:-1}" in
    1|true|TRUE|yes|YES)
        selected=1
        mode=rust-retired
        ;;
    0|false|FALSE|no|NO)
        echo "tm-fdt-rc-smoke.sh: C tm_fdt is retired; QSOE_RUST_TM_FDT must be 1" >&2
        exit 2
        ;;
    *)
        echo "tm-fdt-rc-smoke.sh: QSOE_RUST_TM_FDT must be 1 after C retirement" >&2
        exit 2
        ;;
esac

if [ -e "$ROOT/lq/taskman/sys/fdt.c" ]; then
    fail "lq/taskman/sys/fdt.c should be retired"
fi

echo "tm-fdt-rc-smoke.sh: mode=$mode"

plan_log="$workdir/lq-$mode-taskman-dry-run.txt"
echo "tm-fdt-rc-smoke.sh: verifying LQ taskman selector"
capture_lq_plan "$plan_log"
require_fdt_plan "lq-$mode" "$plan_log" 0

echo "tm-fdt-rc-smoke.sh: building LQ taskman selector"
"$MAKE" -C "$ROOT/lq" --no-print-directory \
    QSOE_RUST_TM_CPIO=1 \
    QSOE_RUST_TM_CRED=1 \
    QSOE_RUST_TM_ELF=1 \
    QSOE_RUST_TM_FDT="$selected" \
    QSOE_RUST_TM_PATHMGR=1 \
    QSOE_RUST_TM_PROCFS=1 \
    QSOE_RUST_TM_PSEUDODEV=1 \
    QSOE_RUST_TM_RSRCDB=1 \
    QSOE_RUST_TM_SCRIPT=1 \
    QSOE_RUST_TM_SYSCFG=1 \
    QSOE_RUST_TM_SYSMAP=1 \
    QSOE_RUST_TM_SYSFS=1 \
    taskman

runtime_args=("$@")
if [ "$has_log" -eq 0 ]; then
    runtime_args=(-o "$workdir/boot-smoke-lq-tm-fdt-rc-$mode.log" "${runtime_args[@]}")
fi

export QSOE_RUST_TM_FDT=1
export QSOE_RUST_TM_PROCFS=1
export TM_FDT_RUNTIME_SMOKE_WORKDIR="$workdir"

exec "$ROOT/scripts/tm-fdt-runtime-smoke.sh" "${runtime_args[@]}"

#!/usr/bin/env bash
#
# Validate the tm_pseudodev Rust-default RC selector while keeping C rollback alive.

set -eu

usage() {
    cat <<'EOF'
usage: scripts/tm-pseudodev-rc-smoke.sh [-t seconds] [-o log] [--keep-running] [-- <emu args>]

Builds LQ taskman with the tm_pseudodev RC selection, verifies whether C
sys/devnull.o and sys/devzero.o are present in the link plan, then reuses the
live LQ tm_pseudodev runtime smoke.

Environment:
  TM_PSEUDODEV_RC_ROLLBACK  set to 1 to validate the C rollback path
  QSOE_RUST_TM_PSEUDODEV    default 1 for Rust RC; set 0 only for rollback validation
  TM_PSEUDODEV_RC_WORKDIR   output directory, default build/tm-pseudodev-rc
EOF
}

ROOT=$(cd "$(dirname "$0")/.." && pwd)
MAKE=${MAKE:-make}
workdir=${TM_PSEUDODEV_RC_WORKDIR:-"$ROOT/build/tm-pseudodev-rc"}
rollback=${TM_PSEUDODEV_RC_ROLLBACK:-0}
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
    echo "tm-pseudodev-rc-smoke.sh: $*" >&2
    exit 1
}

capture_lq_taskman_plan() {
    "$MAKE" -C "$ROOT/lq/taskman" --no-print-directory -B -n all \
        LIBTASKMAN_A="$ROOT/lq/build/libtaskman/libtaskman.a" \
        LIBTASKMAN_INC="$ROOT/libtaskman/include" \
        QSOE_RUST_TM_CPIO=1 \
        QSOE_RUST_TM_CRED=1 \
        QSOE_RUST_TM_ELF=1 \
        QSOE_RUST_TM_FDT=1 \
        QSOE_RUST_TM_PATHMGR=1 \
        QSOE_RUST_TM_PROCFS=1 \
        QSOE_RUST_TM_PSEUDODEV="$selected" \
        QSOE_RUST_TM_RSRCDB=1 \
        QSOE_RUST_TM_SCRIPT=1 \
        QSOE_RUST_TM_SYSCFG=1 \
        QSOE_RUST_TM_SYSMAP=1 \
        QSOE_RUST_TM_SYSFS=1 \
        > "$plan_log"
}

require_plan_contains() {
    local label=$1
    local needle=$2

    grep -Fq "$needle" "$plan_log" ||
        fail "$label dry-run link plan is missing $needle"
}

require_plan_omits() {
    local label=$1
    local needle=$2

    if grep -Fq "$needle" "$plan_log"; then
        fail "$label dry-run link plan unexpectedly contains $needle"
    fi
}

mkdir -p "$workdir"
"$ROOT/scripts/apply-component-overrides.sh"

case "$rollback" in
    0|false|FALSE|no|NO)
        rollback=0
        default_selected=1
        ;;
    1|true|TRUE|yes|YES)
        rollback=1
        default_selected=0
        ;;
    *)
        echo "tm-pseudodev-rc-smoke.sh: TM_PSEUDODEV_RC_ROLLBACK must be 0 or 1" >&2
        exit 2
        ;;
esac

case "${QSOE_RUST_TM_PSEUDODEV:-$default_selected}" in
    1|true|TRUE|yes|YES)
        selected=1
        mode=rust-default
        ;;
    0|false|FALSE|no|NO)
        selected=0
        mode=c-rollback
        ;;
    *)
        echo "tm-pseudodev-rc-smoke.sh: QSOE_RUST_TM_PSEUDODEV must be 0 or 1" >&2
        exit 2
        ;;
esac

if [ "$rollback" -eq 1 ] && [ "$selected" -ne 0 ]; then
    echo "tm-pseudodev-rc-smoke.sh: TM_PSEUDODEV_RC_ROLLBACK=1 requires QSOE_RUST_TM_PSEUDODEV=0" >&2
    exit 2
fi

plan_log="$workdir/lq-$mode-taskman-dry-run.txt"

echo "tm-pseudodev-rc-smoke.sh: mode=$mode rollback=$rollback"
echo "tm-pseudodev-rc-smoke.sh: verifying LQ taskman selector"
"$MAKE" -C "$ROOT/lq" --no-print-directory \
    QSOE_RUST_TM_PROCFS=1 \
    QSOE_RUST_TM_PSEUDODEV="$selected" \
    taskman
capture_lq_taskman_plan

if [ "$selected" -eq 1 ]; then
    require_plan_omits lq-rust-default '/sys/devnull.o'
    require_plan_omits lq-rust-default '/sys/devzero.o'
    require_plan_contains lq-rust-default 'libqsoe_tm_providers.a'
else
    require_plan_contains lq-c-rollback '/sys/devnull.o'
    require_plan_contains lq-c-rollback '/sys/devzero.o'
fi

runtime_args=("$@")
if [ "$has_log" -eq 0 ]; then
    runtime_args=(-o "$workdir/boot-smoke-lq-tm-pseudodev-rc-$mode.log" "${runtime_args[@]}")
fi

export QSOE_RUST_TM_PSEUDODEV="$selected"
export QSOE_RUST_TM_PROCFS=1
export TM_PSEUDODEV_RUNTIME_SMOKE_WORKDIR="$workdir"
if [ "$selected" -eq 0 ]; then
    export TM_PSEUDODEV_RUNTIME_ALLOW_C=1
fi

exec "$ROOT/scripts/tm-pseudodev-runtime-smoke.sh" "${runtime_args[@]}"

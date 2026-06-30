#!/usr/bin/env bash
#
# Validate the retired tm_syscfg Rust selector.

set -eu

usage() {
    cat <<'EOF'
usage: scripts/tm-syscfg-rc-smoke.sh [-t seconds] [-o log] [--keep-running] [-- <emu args>]

Builds NQ and LQ taskman with the retired Rust tm_syscfg provider, verifies
that C syscfg.o is absent, then reuses the live tm_syscfg runtime smoke.

Environment:
  TM_SYSCFG_RC_ROLLBACK  unsupported after C tm_syscfg retirement
  QSOE_RUST_TM_SYSCFG    must remain 1 after C tm_syscfg retirement
  TM_SYSCFG_RC_WORKDIR   output directory, default build/tm-syscfg-rc
EOF
}

ROOT=$(cd "$(dirname "$0")/.." && pwd)
MAKE=${MAKE:-make}
workdir=${TM_SYSCFG_RC_WORKDIR:-"$ROOT/build/tm-syscfg-rc"}
rollback=${TM_SYSCFG_RC_ROLLBACK:-0}
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

find_tool() {
    local tool
    for tool in "$@"; do
        if command -v "$tool" >/dev/null 2>&1; then
            command -v "$tool"
            return 0
        fi
    done
    return 1
}

AR=$(find_tool riscv64-linux-gnu-ar ar llvm-ar) || {
    echo "tm-syscfg-rc-smoke.sh: no ar tool found" >&2
    exit 127
}

fail() {
    echo "tm-syscfg-rc-smoke.sh: $*" >&2
    exit 1
}

object_count() {
    local archive=$1
    local object=$2

    "$AR" t "$archive" |
        awk -v object="$object" '$0 == object { n++ } END { print n + 0 }'
}

require_syscfg_count() {
    local label=$1
    local archive=$2
    local expected=$3
    local count

    [ -f "$archive" ] || fail "missing archive for $label: $archive"
    count=$(object_count "$archive" syscfg.o)
    printf '%s syscfg.o count: %s\n' "$label" "$count" |
        tee "$workdir/$label-syscfg-membership.txt"
    [ "$count" -eq "$expected" ] ||
        fail "$label expected $expected syscfg.o members, got $count"
}

mkdir -p "$workdir"
"$ROOT/scripts/apply-component-overrides.sh"

case "${QSOE_RUST_TM_SYSCFG:-1}" in
    1|true|TRUE|yes|YES)
        selected=1
        ;;
    0|false|FALSE|no|NO)
        echo "tm-syscfg-rc-smoke.sh: C tm_syscfg is retired; QSOE_RUST_TM_SYSCFG must be 1" >&2
        exit 2
        ;;
    *)
        echo "tm-syscfg-rc-smoke.sh: QSOE_RUST_TM_SYSCFG must be 1 after C retirement" >&2
        exit 2
        ;;
esac

case "$rollback" in
    0|false|FALSE|no|NO)
        mode=rust-retired
        expected_syscfg_count=0
        ;;
    1|true|TRUE|yes|YES)
        echo "tm-syscfg-rc-smoke.sh: C tm_syscfg rollback is retired" >&2
        exit 2
        ;;
    *)
        echo "tm-syscfg-rc-smoke.sh: TM_SYSCFG_RC_ROLLBACK must be 0 after C retirement" >&2
        exit 2
        ;;
esac

echo "tm-syscfg-rc-smoke.sh: mode=$mode rollback=$rollback"

echo "tm-syscfg-rc-smoke.sh: verifying NQ taskman selector"
"$MAKE" -C "$ROOT/nq/taskman" --no-print-directory \
    QSOE_RUST_TM_SYSCFG=1 \
    QSOE_RUST_TM_PROCFS=1
require_syscfg_count "nq-$mode" "$ROOT/nq/build/libtaskman/libtaskman.a" "$expected_syscfg_count"

echo "tm-syscfg-rc-smoke.sh: verifying LQ taskman selector"
"$MAKE" -C "$ROOT/lq" --no-print-directory \
    QSOE_RUST_TM_SYSCFG=1 \
    QSOE_RUST_TM_PROCFS=1 \
    taskman
require_syscfg_count "lq-$mode" "$ROOT/lq/build/libtaskman/libtaskman.a" "$expected_syscfg_count"

runtime_args=("$@")
if [ "$has_log" -eq 0 ]; then
    runtime_args=(-o "$workdir/boot-smoke-lq-tm-syscfg-rc-$mode.log" "${runtime_args[@]}")
fi

export QSOE_RUST_TM_SYSCFG="$selected"
export QSOE_RUST_TM_PROCFS=1
export TM_SYSCFG_RUNTIME_SMOKE_WORKDIR="$workdir"

exec "$ROOT/scripts/tm-syscfg-runtime-smoke.sh" "${runtime_args[@]}"

#!/usr/bin/env bash
#
# Boot the pipe Rust-default release-candidate image, or its C rollback, and
# verify pipe(2) data flow through the selected /sbin/pipe.

set -eu

usage() {
    cat <<'EOF'
usage: scripts/pipe-rc-data-smoke.sh [-t seconds] [-o log] [--keep-running] [-- <emu args>]

Builds and boots the pipe release-candidate image. The RC default selects
pipe-rs at /sbin/pipe. Set QSOE_PIPE_RC_ROLLBACK=1 to prove the C rollback
image through the same pipe(2) data-path smoke.

Environment:
  QSOE_PIPE_RC_ROLLBACK   set 1 to select the C rollback artifact
  RUST_PIPE_DATA_WORKDIR  output directory, default build/pipe-rc
EOF
}

ROOT=$(cd "$(dirname "$0")/.." && pwd)
rollback=${QSOE_PIPE_RC_ROLLBACK:-0}

case "$rollback" in
    0|false|FALSE|no|NO)
        export QSOE_RUST_PIPE=1
        mode=rust-default
        ;;
    1|true|TRUE|yes|YES)
        export QSOE_RUST_PIPE=0
        mode=c-rollback
        ;;
    *)
        echo "pipe-rc-data-smoke.sh: QSOE_PIPE_RC_ROLLBACK must be 0 or 1" >&2
        exit 2
        ;;
esac

if [ "${1:-}" = "-h" ] || [ "${1:-}" = "--help" ]; then
    usage
    exit 0
fi

export RUST_PIPE_DATA_WORKDIR=${RUST_PIPE_DATA_WORKDIR:-"$ROOT/build/pipe-rc"}

echo "pipe-rc-data-smoke.sh: mode=$mode rollback=$rollback"
exec "$ROOT/scripts/rust-pipe-data-smoke.sh" "$@"

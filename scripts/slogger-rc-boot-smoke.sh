#!/usr/bin/env bash
#
# Boot the slogger Rust-default release-candidate image, or its C rollback.

set -eu

usage() {
    cat <<'EOF'
usage: scripts/slogger-rc-boot-smoke.sh [-t seconds] [-o log] [--prepare-only] [--keep-running] [-- <emu args>]

Builds and boots the slogger release-candidate image. The RC default selects
slogger-rs at /sbin/slogger. Set QSOE_SLOGGER_RC_ROLLBACK=1 to prove the C
rollback image through the same boot path.

Environment:
  QSOE_SLOGGER_RC_ROLLBACK  set 1 to select the C rollback artifact
  RUST_SLOGGER_WORKDIR      output directory, default build/slogger-rc
EOF
}

ROOT=$(cd "$(dirname "$0")/.." && pwd)
rollback=${QSOE_SLOGGER_RC_ROLLBACK:-0}

case "$rollback" in
    0|false|FALSE|no|NO)
        export QSOE_RUST_SLOGGER=1
        mode=rust-default
        ;;
    1|true|TRUE|yes|YES)
        export QSOE_RUST_SLOGGER=0
        mode=c-rollback
        ;;
    *)
        echo "slogger-rc-boot-smoke.sh: QSOE_SLOGGER_RC_ROLLBACK must be 0 or 1" >&2
        exit 2
        ;;
esac

if [ "${1:-}" = "-h" ] || [ "${1:-}" = "--help" ]; then
    usage
    exit 0
fi

export RUST_SLOGGER_WORKDIR=${RUST_SLOGGER_WORKDIR:-"$ROOT/build/slogger-rc"}

echo "slogger-rc-boot-smoke.sh: mode=$mode rollback=$rollback"
exec "$ROOT/scripts/rust-slogger-boot-smoke.sh" "$@"

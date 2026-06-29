#!/usr/bin/env bash
#
# Boot the retired test_msgpass Rust image and verify the existing suite
# [msgpass] path.

set -eu

usage() {
    cat <<'EOF'
usage: scripts/test-msgpass-rc-smoke.sh [-t seconds] [-o log] [--keep-running] [-- <emu args>]

Builds and boots the test_msgpass image. C test_msgpass is retired; the image
selects test_msgpass-rs at /usr/bin/test_msgpass in the temporary qrvfs test
image.

Environment:
  QSOE_TEST_MSGPASS_RC_ROLLBACK  unsupported after C retirement
  RUST_TEST_MSGPASS_WORKDIR      output directory, default build/test-msgpass-rc
EOF
}

ROOT=$(cd "$(dirname "$0")/.." && pwd)
rollback=${QSOE_TEST_MSGPASS_RC_ROLLBACK:-0}

case "$rollback" in
    0|false|FALSE|no|NO)
        export QSOE_RUST_TEST_MSGPASS=1
        mode=rust-retired
        ;;
    1|true|TRUE|yes|YES)
        echo "test-msgpass-rc-smoke.sh: C test_msgpass rollback is retired" >&2
        exit 2
        ;;
    *)
        echo "test-msgpass-rc-smoke.sh: QSOE_TEST_MSGPASS_RC_ROLLBACK must be 0 after C retirement" >&2
        exit 2
        ;;
esac

if [ "${1:-}" = "-h" ] || [ "${1:-}" = "--help" ]; then
    usage
    exit 0
fi

export RUST_TEST_MSGPASS_WORKDIR=${RUST_TEST_MSGPASS_WORKDIR:-"$ROOT/build/test-msgpass-rc"}

echo "test-msgpass-rc-smoke.sh: mode=$mode rollback=$rollback"
exec "$ROOT/scripts/rust-test-msgpass-smoke.sh" "$@"

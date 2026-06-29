#!/usr/bin/env bash
#
# Stage the selected test_msgpass helper at a stable path for test-image
# packaging. The C helper is retired; this always stages test_msgpass-rs.

set -eu

ROOT=$(cd "$(dirname "$0")/.." && pwd)
SELECTED_TEST_MSGPASS_ELF=${SELECTED_TEST_MSGPASS_ELF:-"$ROOT/build/rust/selected/usr/bin/test_msgpass.elf"}
RUST_TEST_MSGPASS_ELF=${RUST_TEST_MSGPASS_ELF:-"$ROOT/build/rust/qsoe-test-msgpass-rs.elf"}

case "${QSOE_RUST_TEST_MSGPASS:-1}" in
    1|true|TRUE|yes|YES)
        ;;
    0|false|FALSE|no|NO)
        echo "select-test-msgpass-artifact.sh: C test_msgpass is retired; use Rust test_msgpass-rs" >&2
        exit 2
        ;;
    *)
        echo "select-test-msgpass-artifact.sh: QSOE_RUST_TEST_MSGPASS must be 1 after C retirement" >&2
        exit 2
        ;;
esac

RUST_PACKAGE=qsoe-test-msgpass-rs \
    RUST_OUT="$RUST_TEST_MSGPASS_ELF" \
    "$ROOT/scripts/rust-qsoe-link-smoke.sh"

if [ ! -f "$RUST_TEST_MSGPASS_ELF" ]; then
    echo "select-test-msgpass-artifact.sh: missing Rust test_msgpass ELF: $RUST_TEST_MSGPASS_ELF" >&2
    exit 1
fi

mkdir -p "$(dirname "$SELECTED_TEST_MSGPASS_ELF")"
cp "$RUST_TEST_MSGPASS_ELF" "$SELECTED_TEST_MSGPASS_ELF"
chmod 0755 "$SELECTED_TEST_MSGPASS_ELF"

echo "select-test-msgpass-artifact.sh: selected rust test_msgpass -> $SELECTED_TEST_MSGPASS_ELF"

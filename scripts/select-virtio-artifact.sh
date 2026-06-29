#!/usr/bin/env bash
#
# Stage the selected virtio block driver at a stable path for later image
# packaging. The C driver is retired; this always stages devb-virtio-rs.

set -eu

ROOT=$(cd "$(dirname "$0")/.." && pwd)
MAKE=${MAKE:-make}
SELECTED_VIRTIO_ELF=${SELECTED_VIRTIO_ELF:-"$ROOT/build/rust/selected/sbin/devb-virtio.elf"}
RUST_VIRTIO_ELF=${RUST_VIRTIO_ELF:-"$ROOT/build/rust/qsoe-devb-virtio-rs.elf"}
RESSRV_DIR=${RESSRV_DIR:-"$ROOT/quser/build/ressrv"}

case "${QSOE_RUST_VIRTIO:-1}" in
    1|true|TRUE|yes|YES)
        ;;
    0|false|FALSE|no|NO)
        echo "select-virtio-artifact.sh: C devb-virtio is retired; use Rust devb-virtio-rs" >&2
        exit 2
        ;;
    *)
        echo "select-virtio-artifact.sh: QSOE_RUST_VIRTIO must be 1 after C retirement" >&2
        exit 2
        ;;
esac

"$MAKE" -C "$ROOT/quser/ressrv" --no-print-directory
RUST_PACKAGE=qsoe-devb-virtio-rs \
    RUST_OUT="$RUST_VIRTIO_ELF" \
    RUST_EXTRA_LDFLAGS="-L$RESSRV_DIR" \
    RUST_EXTRA_LDLIBS="-lressrv" \
    "$ROOT/scripts/rust-qsoe-link-smoke.sh"

if [ ! -f "$RUST_VIRTIO_ELF" ]; then
    echo "select-virtio-artifact.sh: missing Rust virtio ELF: $RUST_VIRTIO_ELF" >&2
    exit 1
fi

mkdir -p "$(dirname "$SELECTED_VIRTIO_ELF")"
cp "$RUST_VIRTIO_ELF" "$SELECTED_VIRTIO_ELF"
chmod 0755 "$SELECTED_VIRTIO_ELF"

echo "select-virtio-artifact.sh: selected rust virtio -> $SELECTED_VIRTIO_ELF"

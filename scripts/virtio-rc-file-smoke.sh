#!/usr/bin/env bash
#
# Boot the retired devb-virtio Rust image and verify /usr file reads through
# /sbin/devb-virtio.

set -eu

usage() {
    cat <<'EOF'
usage: scripts/virtio-rc-file-smoke.sh [-t seconds] [-o log] [--keep-running] [-- <emu args>]

Builds and boots the retired devb-virtio image. The image selects
devb-virtio-rs at /sbin/devb-virtio. C devb-virtio rollback is retired and no
longer selectable.

Environment:
  QSOE_VIRTIO_RC_ROLLBACK  unsupported after C retirement
  RUST_VIRTIO_FILE_WORKDIR output directory, default build/virtio-rc
EOF
}

ROOT=$(cd "$(dirname "$0")/.." && pwd)
rollback=${QSOE_VIRTIO_RC_ROLLBACK:-0}

case "$rollback" in
    0|false|FALSE|no|NO)
        export QSOE_RUST_VIRTIO=1
        mode=rust-retired
        ;;
    1|true|TRUE|yes|YES)
        echo "virtio-rc-file-smoke.sh: C devb-virtio rollback is retired" >&2
        exit 2
        ;;
    *)
        echo "virtio-rc-file-smoke.sh: QSOE_VIRTIO_RC_ROLLBACK must be 0 after C retirement" >&2
        exit 2
        ;;
esac

if [ "${1:-}" = "-h" ] || [ "${1:-}" = "--help" ]; then
    usage
    exit 0
fi

export RUST_VIRTIO_FILE_WORKDIR=${RUST_VIRTIO_FILE_WORKDIR:-"$ROOT/build/virtio-rc"}
export RUST_VIRTIO_WORKDIR=${RUST_VIRTIO_WORKDIR:-"$ROOT/build/virtio-rc"}

echo "virtio-rc-file-smoke.sh: mode=$mode rollback=$rollback"
exec "$ROOT/scripts/rust-virtio-file-smoke.sh" "$@"

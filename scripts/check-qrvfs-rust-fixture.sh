#!/usr/bin/env bash
#
# Compare the Rust qrvfs inspector against the current C treeqrvfs output.

set -eu

ROOT=$(cd "$(dirname "$0")/.." && pwd)
IMG="$ROOT/build/fixtures/qrvfs/qrvfs-fixture.img"
C_TREE="$ROOT/build/fixtures/qrvfs/tree.log"
RUST_TREE="$ROOT/build/fixtures/qrvfs/rust-tree.log"
MANIFEST="$ROOT/rust/Cargo.toml"

. "$ROOT/scripts/rust-env.sh"
qsoe_cargo_set_target_dir "$ROOT" host

if ! command -v cargo >/dev/null 2>&1; then
    echo "check-qrvfs-rust-fixture.sh: cargo not found" >&2
    exit 127
fi

"$ROOT/scripts/check-qrvfs-fixture.sh" >/dev/null

cargo run \
    --quiet \
    --manifest-path "$MANIFEST" \
    -p qsoe-qrvfs \
    --bin qrvfs-tree \
    -- "$IMG" > "$RUST_TREE"

if ! diff -u "$C_TREE" "$RUST_TREE"; then
    echo "check-qrvfs-rust-fixture.sh: Rust qrvfs output diverges from treeqrvfs" >&2
    exit 1
fi

echo "check-qrvfs-rust-fixture.sh: ok"
echo "  image: $IMG"
echo "  c:     $C_TREE"
echo "  rust:  $RUST_TREE"

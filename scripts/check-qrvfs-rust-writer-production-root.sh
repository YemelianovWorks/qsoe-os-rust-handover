#!/usr/bin/env bash
#
# Build the normal staged qrvfs root, then write and inspect a Rust image from it.

set -eu

ROOT=$(cd "$(dirname "$0")/.." && pwd)
TOOLS="$ROOT/build/host-tools"
FIXTURE="$ROOT/build/fixtures/qrvfs-rust-production-root"
ROOTDIR="$ROOT/build/fsqrv-root"
C_IMG="$ROOT/build/fsqrv.img"
RUST_IMG="$FIXTURE/fsqrv-rust.img"
C_TREE="$FIXTURE/tree-c-writer.log"
RUST_TREE="$FIXTURE/tree-rust-writer.log"
C_BUILD_LOG="$FIXTURE/mkfs-c.log"
RUST_BUILD_LOG="$FIXTURE/mkfs-rust.log"
MANIFEST="$ROOT/rust/Cargo.toml"
CC=${CC:-cc}

. "$ROOT/scripts/rust-env.sh"
qsoe_cargo_set_target_dir "$ROOT" host

if ! command -v cargo >/dev/null 2>&1; then
    echo "check-qrvfs-rust-writer-production-root.sh: cargo not found" >&2
    exit 127
fi

mkdir -p "$TOOLS" "$FIXTURE"

make -C "$ROOT" --no-print-directory fsqrv-image > "$C_BUILD_LOG"
if [ ! -f "$C_IMG" ] || [ ! -d "$ROOTDIR" ]; then
    echo "check-qrvfs-rust-writer-production-root.sh: fsqrv-image was not built" >&2
    echo "--- $C_BUILD_LOG ---" >&2
    cat "$C_BUILD_LOG" >&2
    exit 1
fi

"$CC" -O2 -Wall -Wno-unused-variable -I "$ROOT/quser/fs/qrv" \
    -o "$TOOLS/treeqrvfs" "$ROOT/host_tools/treeqrvfs.c"

cargo run \
    --quiet \
    --manifest-path "$MANIFEST" \
    -p qsoe-qrvfs \
    --bin mkfs-qrv-rs \
    -- -s 16 "$RUST_IMG" "$ROOTDIR" > "$RUST_BUILD_LOG"

"$TOOLS/treeqrvfs" "$C_IMG" > "$C_TREE"
"$TOOLS/treeqrvfs" "$RUST_IMG" > "$RUST_TREE"

require() {
    pattern=$1
    file=$2
    if ! grep -Fq "$pattern" "$file"; then
        echo "check-qrvfs-rust-writer-production-root.sh: missing pattern in $file: $pattern" >&2
        echo "--- $file ---" >&2
        cat "$file" >&2
        exit 1
    fi
}

for tree in "$C_TREE" "$RUST_TREE"; do
    require "qrvfs v2, 4096 blocks, 128 inodes" "$tree"
    require "home" "$tree"
    require "user" "$tree"
    require ".profile" "$tree"
    require "conf" "$tree"
    require "shadow" "$tree"
    require "group" "$tree"
    require "passwd" "$tree"
    require "sbin" "$tree"
    require "sysinit" "$tree"
    require "level1.sh" "$tree"
    require "login" "$tree"
    require "getty" "$tree"
    require "bin" "$tree"
    require "suite" "$tree"
    require "test_msgpass" "$tree"
    require "test_syncspace" "$tree"
    require "time" "$tree"
    require "sysinfo" "$tree"
    require "6 directories, 12 files" "$tree"
done

c_count=$(tail -n 1 "$C_TREE")
rust_count=$(tail -n 1 "$RUST_TREE")
if [ "$c_count" != "$rust_count" ]; then
    echo "check-qrvfs-rust-writer-production-root.sh: C/Rust tree counts diverge" >&2
    echo "C:    $c_count" >&2
    echo "Rust: $rust_count" >&2
    exit 1
fi

echo "check-qrvfs-rust-writer-production-root.sh: ok"
echo "  root:      $ROOTDIR"
echo "  c image:   $C_IMG"
echo "  rust image: $RUST_IMG"

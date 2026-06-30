#!/usr/bin/env bash
#
# Run the host-side Rust fixture for libtaskman's portable tm_sysfs model.

set -eu

ROOT=$(cd "$(dirname "$0")/.." && pwd)
cargo test --manifest-path "$ROOT/rust/Cargo.toml" \
    -p qsoe-tm-sysfs \
    --features host-tests \
    --lib

echo "check-tm-sysfs-model.sh: ok"

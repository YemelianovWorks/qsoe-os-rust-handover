#!/usr/bin/env bash
#
# Run the host-side tests for the retired Rust tm_elf model.

set -eu

ROOT=$(cd "$(dirname "$0")/.." && pwd)
cargo test --manifest-path "$ROOT/rust/Cargo.toml" \
    -p qsoe-tm-elf \
    --features host-tests \
    --lib

echo "check-tm-elf-model.sh: ok"

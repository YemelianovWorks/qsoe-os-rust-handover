#!/usr/bin/env bash
#
# Run the Rust host-side model for libtaskman's portable tm_syscfg ABI.

set -eu

ROOT=$(cd "$(dirname "$0")/.." && pwd)

cargo test --manifest-path "$ROOT/rust/Cargo.toml" -p qsoe-tm-syscfg --features host-tests --lib

echo "check-tm-syscfg-model.sh: ok"

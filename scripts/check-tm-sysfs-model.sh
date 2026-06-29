#!/usr/bin/env bash
#
# Build and run the host-side fixture for libtaskman's portable tm_sysfs model.

set -eu

ROOT=$(cd "$(dirname "$0")/.." && pwd)
BUILD="$ROOT/build/fixtures/tm-sysfs"
CC=${CC:-cc}

mkdir -p "$BUILD"

"$CC" -std=c11 -O2 -Wall -Wextra -Werror \
    -I "$ROOT/libtaskman/include" \
    -o "$BUILD/tm_sysfs_model_test" \
    "$ROOT/tests/tm_sysfs_model_test.c" \
    "$ROOT/libtaskman/src/tm_sysfs.c"

"$BUILD/tm_sysfs_model_test"

echo "check-tm-sysfs-model.sh: ok"
echo "  binary: $BUILD/tm_sysfs_model_test"

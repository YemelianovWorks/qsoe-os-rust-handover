#!/usr/bin/env bash
#
# Build and run the host-side fixture for libtaskman's portable tm_cpio model.

set -eu

ROOT=$(cd "$(dirname "$0")/.." && pwd)
BUILD="$ROOT/build/fixtures/tm-cpio"
CC=${CC:-cc}

mkdir -p "$BUILD"

"$CC" -std=c11 -O2 -Wall -Wextra -Werror \
    -I "$ROOT/libtaskman/include" \
    -o "$BUILD/tm_cpio_model_test" \
    "$ROOT/tests/tm_cpio_model_test.c" \
    "$ROOT/libtaskman/src/cpio.c"

"$BUILD/tm_cpio_model_test"

echo "check-tm-cpio-model.sh: ok"
echo "  binary: $BUILD/tm_cpio_model_test"

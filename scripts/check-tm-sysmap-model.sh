#!/usr/bin/env bash
#
# Build and run the host-side fixture for LQ taskman's sysmap builder.

set -eu

ROOT=$(cd "$(dirname "$0")/.." && pwd)
BUILD="$ROOT/build/fixtures/tm-sysmap"
CC=${CC:-cc}

mkdir -p "$BUILD"

"$CC" -std=c11 -O2 -Wall -Wextra -Werror \
    -include "$ROOT/tests/tm_sysmap_model_prelude.h" \
    -I "$ROOT/libc/include" \
    -I "$ROOT/lq/taskman" \
    -o "$BUILD/tm_sysmap_model_test" \
    "$ROOT/tests/tm_sysmap_model_test.c" \
    "$ROOT/lq/taskman/sys/sysmap.c"

"$BUILD/tm_sysmap_model_test"

echo "check-tm-sysmap-model.sh: ok"
echo "  binary: $BUILD/tm_sysmap_model_test"

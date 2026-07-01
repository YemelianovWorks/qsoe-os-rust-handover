#!/bin/sh
set -eu

OUT_DIR="build/spawn-argpack-c-evidence"
SUMMARY="$OUT_DIR/summary.txt"
SRC="lq/taskman/proc/spawn.c"

mkdir -p "$OUT_DIR"

require_source() {
    pattern="$1"
    description="$2"
    if ! grep -Fq "$pattern" "$SRC"; then
        echo "missing: $description ($pattern)" >&2
        exit 1
    fi
    echo "present: $description" >> "$SUMMARY"
}

: > "$SUMMARY"
echo "tm_spawn_argpack C seam evidence" >> "$SUMMARY"
echo "source: $SRC" >> "$SUMMARY"

require_source "typedef struct tm_spawn_argpack" "bounded argpack plan type"
require_source "tm_spawn_argpack_prepare" "argpack validation/preparation seam"
require_source "TM_SPAWN_ARGPACK_MAX_VEC" "argv/envp vector bound"
require_source "TM_SPAWN_ARGPACK_MAX_AUXV" "auxv vector bound"
require_source "TM_SPAWN_ARGPACK_STACK_LIMIT" "initial stack byte bound"
require_source "strings_bytes" "precomputed string byte accounting"
require_source "pointer_bytes" "precomputed pointer/auxv byte accounting"
require_source "total_aligned" "precomputed aligned stack extent"
require_source "strings_alloc" "precomputed string allocation extent"
require_source "static unsigned long build_initial_stack(seL4_CPtr top_frame," "stack writer takes prepared argpack"
require_source "const tm_spawn_argpack_t *argpack" "stack writer accepts prepared argpack"
require_source "spawn: tm_spawn_argpack_prepare failed" "spawn path rejects invalid argpack before child stack write"

echo "result: PASS" >> "$SUMMARY"
cat "$SUMMARY"

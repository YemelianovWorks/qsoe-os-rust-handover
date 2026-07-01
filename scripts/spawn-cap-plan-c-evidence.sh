#!/bin/sh
set -eu

OUT_DIR="build/spawn-cap-plan-c-evidence"
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
echo "tm_cap_plan C seam evidence" >> "$SUMMARY"
echo "source: $SRC" >> "$SUMMARY"

require_source "typedef struct tm_cap_plan" "bounded cap plan type"
require_source "typedef struct tm_cap_op" "typed cap operation entries"
require_source "TM_CAP_PLAN_MAX_OPS" "cap operation bound"
require_source "TM_CAP_PLAN_STDIO_COUNT" "stdio operation bound"
require_source "TM_CAP_OP_MINT" "mint operation kind"
require_source "TM_CAP_OP_COPY" "copy operation kind"
require_source "tm_cap_plan_prepare" "cap plan preparation seam"
require_source "tm_cap_plan_commit" "C-owned cap authority commit seam"
require_source "QSOE_CAP_TASKMAN_EP" "taskman endpoint publication"
require_source "QSOE_CAP_OWN_UNTYPED" "child untyped publication"
require_source "QSOE_CAP_CNODE_SELF" "child CNode self publication"
require_source "QSOE_CAP_STDOUT_CONNECT" "stdio connection cap publication"
require_source "tm_connection_register_existing(plan->pid" "stdio connection records tied to cap plan"
require_source "spawn: tm_cap_plan_prepare failed" "spawn path rejects invalid cap plan before commit"

echo "result: PASS" >> "$SUMMARY"
cat "$SUMMARY"

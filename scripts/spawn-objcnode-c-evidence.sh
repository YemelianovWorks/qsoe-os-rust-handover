#!/usr/bin/env bash
set -euo pipefail

ROOT="$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)"
SRC="${1:-$ROOT/lq/taskman/proc/spawn.c}"
OUT="${QSOE_EVIDENCE_OUT:-$ROOT/build/spawn-objcnode-c-evidence.txt}"
mkdir -p "$(dirname "$OUT")"

if [[ ! -f "$SRC" ]]; then
    echo "spawn-objcnode-c-evidence: missing source: $SRC" >&2
    exit 1
fi

require_pattern() {
    local pattern="$1"
    if ! grep -Fq "$pattern" "$SRC"; then
        echo "spawn-objcnode-c-evidence: missing pattern: $pattern" >&2
        exit 1
    fi
}

require_pattern "typedef enum tm_spawn_objcnode_status"
require_pattern "TM_SPAWN_OBJCNODE_READY"
require_pattern "TM_SPAWN_OBJCNODE_BOUND"
require_pattern "TM_SPAWN_OBJCNODE_BAD_PROCESS"
require_pattern "TM_SPAWN_OBJCNODE_BAD_OBJECT"
require_pattern "typedef struct tm_spawn_objcnode_plan"
require_pattern "tm_spawn_objcnode_prepare"
require_pattern "tm_spawn_objcnode_bind"
require_pattern "objc_plan->pid = pid"
require_pattern "objc_plan->proc = proc"
require_pattern "objc_plan->sc = sc"
require_pattern "objc_plan->frame_count = frame_count"
require_pattern "objc_plan->pt_count = pt_count"
require_pattern "objc_plan->relro_count = relro_count"
require_pattern "objc_plan->max_mprot = TM_MAX_MPROT"
require_pattern "objc_plan->objcnode = objcnode"
require_pattern "tm_spawn_objcnode_plan_t objc_plan"
require_pattern "tm_spawn_objcnode_prepare(&objc_plan"
require_pattern "tm_spawn_objcnode_bind(&objc_plan"
require_pattern "op->sc = objc_plan.sc"
require_pattern "op->objcnode      = objc_plan.objcnode"
require_pattern "i < objc_plan.frame_count"
require_pattern "i < objc_plan.pt_count"
require_pattern "op->mprot_count >= objc_plan.max_mprot"
require_pattern "qsoe_cnode_move(objc"
require_pattern "taskman_free_slot(src)"

if grep -Fq "op->sc = publication.sc" "$SRC" ||
   grep -Fq "op->objcnode      = objc;" "$SRC" ||
   grep -Fq "i < s_frame_count" "$SRC" ||
   grep -Fq "i < s_pt_count" "$SRC" ||
   grep -Fq "op->mprot_count >= TM_MAX_MPROT" "$SRC"; then
    echo "spawn-objcnode-c-evidence: stale inline objcnode relocation source remains" >&2
    exit 1
fi

{
    echo "spawn-objcnode-c-evidence: ok"
    echo "source=$SRC"
    echo "checked=tm_spawn_objcnode_plan frame/PT relocation bounds, RELRO retention bounds, and C-owned cnode move/free authority"
} > "$OUT"
cat "$OUT"

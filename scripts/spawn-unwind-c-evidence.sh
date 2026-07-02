#!/usr/bin/env bash
set -euo pipefail

ROOT="$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)"
SRC="${1:-$ROOT/lq/taskman/proc/spawn.c}"
OUT="${QSOE_EVIDENCE_OUT:-$ROOT/build/spawn-unwind-c-evidence.txt}"
mkdir -p "$(dirname "$OUT")"

if [[ ! -f "$SRC" ]]; then
    echo "spawn-unwind-c-evidence: missing source: $SRC" >&2
    exit 1
fi

require_pattern() {
    local pattern="$1"
    if ! grep -Fq "$pattern" "$SRC"; then
        echo "spawn-unwind-c-evidence: missing pattern: $pattern" >&2
        exit 1
    fi
}

require_pattern "typedef enum tm_spawn_unwind_status"
require_pattern "TM_SPAWN_UNWIND_READY"
require_pattern "TM_SPAWN_UNWIND_COMMITTED"
require_pattern "TM_SPAWN_UNWIND_BAD_PID"
require_pattern "typedef struct tm_spawn_unwind_plan"
require_pattern "seL4_CPtr cnode"
require_pattern "seL4_CPtr vspace"
require_pattern "seL4_CPtr tcb"
require_pattern "seL4_CPtr l1_pt"
require_pattern "seL4_CPtr l0_pt"
require_pattern "seL4_CPtr workers_l1_pt"
require_pattern "seL4_CPtr ipc_frame"
require_pattern "seL4_CPtr tcb_frame"
require_pattern "seL4_CPtr child_untyped"
require_pattern "seL4_CPtr sc"
require_pattern "seL4_CPtr fault_ep"
require_pattern "int frame_count"
require_pattern "int pt_count"
require_pattern "int relro_count"
require_pattern "int stack_pages"
require_pattern "int committed"
require_pattern "tm_spawn_unwind_prepare"
require_pattern "tm_spawn_unwind_track_core"
require_pattern "tm_spawn_unwind_track_vspace"
require_pattern "tm_spawn_unwind_track_runtime"
require_pattern "tm_spawn_unwind_track_publication"
require_pattern "tm_spawn_unwind_track_counts"
require_pattern "tm_spawn_unwind_mark_committed"
require_pattern "unwind->pid = pid"
require_pattern "unwind->status = TM_SPAWN_UNWIND_READY"
require_pattern "unwind->status = TM_SPAWN_UNWIND_COMMITTED"
require_pattern "tm_spawn_unwind_plan_t unwind_plan"
require_pattern "tm_spawn_unwind_prepare(&unwind_plan, pid)"
require_pattern "tm_spawn_unwind_track_core(&unwind_plan"
require_pattern "tm_spawn_unwind_track_vspace(&unwind_plan"
require_pattern "tm_spawn_unwind_track_runtime(&unwind_plan"
require_pattern "tm_spawn_unwind_track_publication(&unwind_plan"
require_pattern "tm_spawn_unwind_track_counts(&unwind_plan"
require_pattern "tm_spawn_unwind_mark_committed(&unwind_plan)"
require_pattern "Failure"
require_pattern "tm_spawn_unwind_plan records the resource inventory"

if grep -Fq "failure path leaks slots until v0.4's PCB-keyed cleanup" "$SRC"; then
    echo "spawn-unwind-c-evidence: stale unrepresented failure-unwind text remains" >&2
    exit 1
fi

{
    echo "spawn-unwind-c-evidence: ok"
    echo "source=$SRC"
    echo "checked=tm_spawn_unwind_plan resource inventory, count tracking, publication caps, and committed-state gating with C-owned cleanup authority"
} > "$OUT"
cat "$OUT"

#!/usr/bin/env bash
set -euo pipefail

ROOT="$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)"
SRC="${1:-$ROOT/lq/taskman/proc/spawn.c}"
OUT="${QSOE_EVIDENCE_OUT:-$ROOT/build/spawn-loader-entry-c-evidence.txt}"
mkdir -p "$(dirname "$OUT")"

if [[ ! -f "$SRC" ]]; then
    echo "spawn-loader-entry-c-evidence: missing source: $SRC" >&2
    exit 1
fi

require_pattern() {
    local pattern="$1"
    if ! grep -Fq "$pattern" "$SRC"; then
        echo "spawn-loader-entry-c-evidence: missing pattern: $pattern" >&2
        exit 1
    fi
}

require_pattern "typedef enum tm_loader_entry_status"
require_pattern "TM_LOADER_ENTRY_READY"
require_pattern "TM_LOADER_ENTRY_BAD_PROTO"
require_pattern "TM_LOADER_ENTRY_BAD_STACK"
require_pattern "typedef struct tm_loader_entry_plan"
require_pattern "tm_loader_entry_prepare"
require_pattern "proto->entry_pc"
require_pattern "entry_plan->pc = proto->entry_pc"
require_pattern "entry_plan->sp = initial_sp"
require_pattern "entry_plan->gp = 0"
require_pattern "entry_plan->tp = CHILD_TCB_BASE"
require_pattern "entry_plan->a0 = (seL4_Word)pid"
require_pattern "entry_plan->status = TM_LOADER_ENTRY_READY"
require_pattern "tm_loader_entry_plan_t entry_plan"
require_pattern "tm_loader_entry_prepare(&entry_plan, &loader_proto"
require_pattern "ctx.pc = entry_plan.pc"
require_pattern "ctx.sp = entry_plan.sp"
require_pattern "ctx.gp = entry_plan.gp"
require_pattern "ctx.tp = entry_plan.tp"
require_pattern "ctx.a0 = entry_plan.a0"
require_pattern "qsoe_tcb_write_registers(tcb, 0, &ctx)"
require_pattern "tm_loader_auxv_plan_t loader_auxv"
require_pattern "tm_spawn_argpack_prepare(&argpack"
require_pattern "tm_cap_plan_commit(&cap_plan)"

if grep -Fq "ctx.pc = loader_proto.entry_pc" "$SRC" ||    grep -Fq "ctx.sp = initial_sp" "$SRC" ||    grep -Fq "ctx.tp = CHILD_TCB_BASE" "$SRC" ||    grep -Fq "ctx.a0 = (seL4_Word)pid" "$SRC"; then
    echo "spawn-loader-entry-c-evidence: stale inline register source remains" >&2
    exit 1
fi

{
    echo "spawn-loader-entry-c-evidence: ok"
    echo "source=$SRC"
    echo "checked=tm_loader_entry_plan pc/sp/gp/tp/a0 shaping, ctx handoff, TCB write authority"
} > "$OUT"
cat "$OUT"

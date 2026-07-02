#!/usr/bin/env bash
set -euo pipefail

ROOT="$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)"
SRC="${1:-$ROOT/lq/taskman/proc/spawn.c}"
OUT="${QSOE_EVIDENCE_OUT:-$ROOT/build/spawn-tcb-handoff-c-evidence.txt}"
mkdir -p "$(dirname "$OUT")"

if [[ ! -f "$SRC" ]]; then
    echo "spawn-tcb-handoff-c-evidence: missing source: $SRC" >&2
    exit 1
fi

require_pattern() {
    local pattern="$1"
    if ! grep -Fq "$pattern" "$SRC"; then
        echo "spawn-tcb-handoff-c-evidence: missing pattern: $pattern" >&2
        exit 1
    fi
}

require_pattern "typedef enum tm_tcb_handoff_status"
require_pattern "TM_TCB_HANDOFF_CONFIG_READY"
require_pattern "TM_TCB_HANDOFF_READY"
require_pattern "TM_TCB_HANDOFF_BAD_OBJECTS"
require_pattern "TM_TCB_HANDOFF_BAD_FAULT"
require_pattern "typedef struct tm_tcb_handoff_plan"
require_pattern "tm_tcb_handoff_prepare"
require_pattern "tm_tcb_handoff_bind_sched_fault"
require_pattern "handoff->cnode_data = 52UL"
require_pattern "handoff->vspace_data = 0"
require_pattern "handoff->ipc_buffer = CHILD_IPC_BUFFER"
require_pattern "handoff->sched_core = 0"
require_pattern "handoff->sched_authority = seL4_CapInitThreadTCB"
require_pattern "handoff->sched_mcp = TM_PRIO_USER_DEFAULT"
require_pattern "handoff->sched_prio = TM_PRIO_USER_DEFAULT"
require_pattern "handoff->fault_badge = TM_FAULT_BADGE_FLAG | (seL4_Word)pid"
require_pattern "handoff->reply_slot = QSOE_CAP_REPLY"
require_pattern "tm_tcb_handoff_plan_t tcb_handoff"
require_pattern "tm_tcb_handoff_prepare(&tcb_handoff"
require_pattern "qsoe_tcb_configure(tcb_handoff.tcb"
require_pattern "tm_sched_context_create(tcb_handoff.sched_core)"
require_pattern "taskman_alloc_empty_slot"
require_pattern "tm_tcb_handoff_bind_sched_fault(&tcb_handoff"
require_pattern "qsoe_cnode_mint(tcb_handoff.cspace_root"
require_pattern "qsoe_tcb_set_sched_params(tcb_handoff.tcb"
require_pattern "qsoe_untyped_retype(tcb_handoff.reply_untyped"
require_pattern "qsoe_tcb_write_registers(tcb, 0, &ctx)"
require_pattern "tm_loader_entry_plan_t entry_plan"

if grep -Fq "seL4_Word cnode_data = 52UL" "$SRC" ||    grep -Fq "qsoe_tcb_configure(tcb," "$SRC" ||    grep -Fq "TM_FAULT_BADGE_FLAG | (seL4_Word)pid);" "$SRC" ||    grep -Fq "qsoe_untyped_retype(s_untyped" "$SRC"; then
    echo "spawn-tcb-handoff-c-evidence: stale inline TCB handoff source remains" >&2
    exit 1
fi

{
    echo "spawn-tcb-handoff-c-evidence: ok"
    echo "source=$SRC"
    echo "checked=tm_tcb_handoff_plan configure/sched/fault/reply shaping with C authority calls"
} > "$OUT"
cat "$OUT"

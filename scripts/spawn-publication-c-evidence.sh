#!/usr/bin/env bash
set -euo pipefail

ROOT="$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)"
SRC="${1:-$ROOT/lq/taskman/proc/spawn.c}"
OUT="${QSOE_EVIDENCE_OUT:-$ROOT/build/spawn-publication-c-evidence.txt}"
mkdir -p "$(dirname "$OUT")"

if [[ ! -f "$SRC" ]]; then
    echo "spawn-publication-c-evidence: missing source: $SRC" >&2
    exit 1
fi

require_pattern() {
    local pattern="$1"
    if ! grep -Fq "$pattern" "$SRC"; then
        echo "spawn-publication-c-evidence: missing pattern: $pattern" >&2
        exit 1
    fi
}

require_pattern "typedef enum tm_spawn_publication_status"
require_pattern "TM_SPAWN_PUBLICATION_READY"
require_pattern "TM_SPAWN_PUBLICATION_CONNECTION_READY"
require_pattern "TM_SPAWN_PUBLICATION_RESUME_READY"
require_pattern "TM_SPAWN_PUBLICATION_BAD_OBJECTS"
require_pattern "TM_SPAWN_PUBLICATION_BAD_CHANNEL"
require_pattern "typedef struct tm_spawn_publication_plan"
require_pattern "tm_spawn_publication_prepare"
require_pattern "tm_spawn_publication_bind_taskman_channel"
require_pattern "tm_spawn_publication_mark_resume_ready"
require_pattern "publication->first_child_slot = QSOE_CAP_WELL_KNOWN_END"
require_pattern "publication->child_untyped = child_untyped"
require_pattern "publication->fault_ep = fault_ep"
require_pattern "publication->sc = sc"
require_pattern "publication->workers_l1_pt = workers_l1_pt"
require_pattern "publication->taskman_owner_pid = QSOE_PID_TASKMAN"
require_pattern "publication->taskman_owner_chid = 1"
require_pattern "publication->taskman_ep_slot = QSOE_CAP_TASKMAN_EP"
require_pattern "publication->taskman_scoid = (seL4_Word)pid"
require_pattern "publication->taskman_flags = 0"
require_pattern "tm_spawn_publication_plan_t publication"
require_pattern "tm_spawn_publication_prepare(&publication"
require_pattern "tm_process_register(publication.pid"
require_pattern "tm_process_lookup(publication.pid)"
require_pattern "prec->untyped_budget = publication.child_untyped"
require_pattern "prec->fault_ep       = publication.fault_ep"
require_pattern "publication.name_base"
require_pattern "publication.workers_l1_pt"
require_pattern "tm_channel_index(publication.taskman_owner_pid"
require_pattern "tm_spawn_publication_bind_taskman_channel(&publication"
require_pattern "tm_connection_register_existing(publication.pid"
require_pattern "publication.taskman_ep_slot"
require_pattern "publication.primary_idx"
require_pattern "publication.taskman_scoid"
require_pattern "publication.taskman_flags"
require_pattern "op->sc = publication.sc"
require_pattern "qsoe_cnode_move(objc"
require_pattern "taskman_free_slot(src)"
require_pattern "tm_spawn_publication_mark_resume_ready(&publication)"
require_pattern "qsoe_tcb_resume(publication.tcb)"

if grep -Fq "tm_process_register(pid, cnode" "$SRC" ||    grep -Fq "tm_process_lookup(pid);" "$SRC" ||    grep -Fq "const char *base = elf_name ? elf_name : \"?\"" "$SRC" ||    grep -Fq "tm_connection_register_existing(pid, QSOE_CAP_TASKMAN_EP" "$SRC" ||    grep -Fq "qsoe_tcb_resume(tcb)" "$SRC"; then
    echo "spawn-publication-c-evidence: stale inline publication/resume source remains" >&2
    exit 1
fi

{
    echo "spawn-publication-c-evidence: ok"
    echo "source=$SRC"
    echo "checked=tm_spawn_publication_plan process record, taskman connection, object relocation, and resume gating with C authority calls"
} > "$OUT"
cat "$OUT"

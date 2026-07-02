#!/usr/bin/env bash
set -euo pipefail

ROOT=${QSOE_ROOT:-$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)}
SPAWN_C=${QSOE_SPAWN_SOURCE:-$ROOT/lq/taskman/proc/spawn.c}

fail() {
    echo "spawn-unwind-cleanup-c-evidence: $*" >&2
    exit 1
}

require_pattern() {
    local pattern=$1
    local label=$2
    if ! grep -Eq "$pattern" "$SPAWN_C"; then
        fail "missing ${label}: ${pattern}"
    fi
}

[ -f "$SPAWN_C" ] || fail "spawn source not found: $SPAWN_C"

require_pattern 'typedef enum tm_spawn_cleanup_status' 'cleanup status enum'
require_pattern 'TM_SPAWN_CLEANUP_REQUIRED' 'cleanup required state'
require_pattern 'TM_SPAWN_CLEANUP_NOT_REQUIRED' 'cleanup not-required state'
require_pattern 'typedef enum tm_spawn_cleanup_op_kind' 'cleanup operation enum'
require_pattern 'TM_SPAWN_CLEANUP_OP_RECLAIM_RECORDED_FRAMES' 'recorded frame cleanup operation'
require_pattern 'TM_SPAWN_CLEANUP_OP_RECLAIM_RECORDED_PTS' 'recorded page-table cleanup operation'
require_pattern 'TM_SPAWN_CLEANUP_OP_DELETE_FAULT_ENDPOINT' 'fault endpoint cleanup operation'
require_pattern 'TM_SPAWN_CLEANUP_OP_REVOKE_SCHED_CONTEXT' 'scheduler context cleanup operation'
require_pattern 'TM_SPAWN_CLEANUP_OP_REVOKE_CHILD_UNTYPED' 'child untyped cleanup operation'
require_pattern 'TM_SPAWN_CLEANUP_OP_REVOKE_CHILD_CNODE' 'child cnode cleanup operation'
require_pattern '#define TM_SPAWN_CLEANUP_MAX_OPS 8' 'bounded cleanup operation count'
require_pattern 'typedef struct tm_spawn_unwind_cleanup_plan' 'cleanup plan struct'
require_pattern 'tm_spawn_unwind_cleanup_add' 'cleanup action builder'
require_pattern 'tm_spawn_unwind_cleanup_prepare' 'cleanup preparation helper'
require_pattern 'tm_spawn_unwind_cleanup_commit' 'cleanup commit seam helper'
require_pattern 'tm_spawn_unwind_return' 'failure return helper'
require_pattern 'cleanup_planned' 'unwind cleanup planned marker'
require_pattern 'failure_rc' 'unwind failure return marker'
require_pattern 'return tm_spawn_unwind_return\(&unwind_plan, -ENOMEM\);' 'ENOMEM failure return routed through cleanup seam'
require_pattern 'return tm_spawn_unwind_return\(&unwind_plan, -EINVAL\);' 'EINVAL failure return routed through cleanup seam'
require_pattern 'return tm_spawn_unwind_return\(&unwind_plan, -ENOEXEC\);' 'ENOEXEC failure return routed through cleanup seam'

if awk '
    /static int tm_spawn_unwind_cleanup_prepare/ { in_cleanup = 1 }
    /static int tm_spawn_unwind_return/ { in_cleanup = 0 }
    in_cleanup && /qsoe_cnode_revoke|qsoe_cnode_delete|taskman_free_slot|qsoe_tcb_resume|qsoe_untyped_retype/ { bad = 1 }
    END { exit bad ? 1 : 0 }
' "$SPAWN_C"; then
    :
else
    fail "cleanup plan helper must not perform seL4 cleanup authority calls"
fi

echo "spawn-unwind-cleanup-c-evidence: OK"

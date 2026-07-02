#!/usr/bin/env bash
set -euo pipefail

ROOT="$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)"
SRC="${1:-$ROOT/lq/taskman/proc/spawn.c}"
OUT="${QSOE_EVIDENCE_OUT:-$ROOT/build/spawn-loader-auxv-c-evidence.txt}"
mkdir -p "$(dirname "$OUT")"

if [[ ! -f "$SRC" ]]; then
    echo "spawn-loader-auxv-c-evidence: missing source: $SRC" >&2
    exit 1
fi

require_pattern() {
    local pattern="$1"
    if ! grep -Fq "$pattern" "$SRC"; then
        echo "spawn-loader-auxv-c-evidence: missing pattern: $pattern" >&2
        exit 1
    fi
}

require_pattern "typedef enum tm_loader_auxv_status"
require_pattern "TM_LOADER_AUXV_STATIC"
require_pattern "TM_LOADER_AUXV_READY"
require_pattern "TM_LOADER_AUXV_BAD_PROTO"
require_pattern "TM_LOADER_AUXV_BAD_MAP"
require_pattern "TM_LOADER_AUXV_TOO_MANY"
require_pattern "typedef struct tm_loader_auxv_plan"
require_pattern "struct aux_pair auxv[TM_SPAWN_ARGPACK_MAX_AUXV]"
require_pattern "tm_loader_auxv_init_static"
require_pattern "tm_loader_auxv_add"
require_pattern "auxv_plan->auxc >= TM_SPAWN_ARGPACK_MAX_AUXV"
require_pattern "tm_loader_auxv_admit_dynamic"
require_pattern "proto->dyn_link"
require_pattern "map_plan->status != TM_LOADER_MAP_READY"
require_pattern "tm_loader_auxv_add(auxv_plan, AT_PHDR_"
require_pattern "tm_loader_auxv_add(auxv_plan, AT_PHENT_"
require_pattern "tm_loader_auxv_add(auxv_plan, AT_PHNUM_"
require_pattern "tm_loader_auxv_add(auxv_plan, AT_BASE_"
require_pattern "tm_loader_auxv_add(auxv_plan, AT_ENTRY_"
require_pattern "tm_loader_auxv_add(auxv_plan, AT_PAGESZ_"
require_pattern "tm_loader_auxv_add(auxv_plan, AT_KPRELOAD_"
require_pattern "map_plan->libc_load_base"
require_pattern "tm_loader_auxv_plan_t loader_auxv"
require_pattern "tm_loader_auxv_init_static(&loader_auxv)"
require_pattern "tm_loader_auxv_admit_dynamic(&loader_auxv"
require_pattern "tm_spawn_argpack_prepare(&argpack"
require_pattern "loader_auxv.auxv"
require_pattern "loader_auxv.auxc"
require_pattern "qsoe_tcb_write_registers"

if grep -Fq "auxv[auxc++]" "$SRC"; then
    echo "spawn-loader-auxv-c-evidence: stale inline auxv append remains" >&2
    exit 1
fi

{
    echo "spawn-loader-auxv-c-evidence: ok"
    echo "source=$SRC"
    echo "checked=tm_loader_auxv_plan dynamic auxv entries, bounds, map/proto handoff, argpack handoff"
} > "$OUT"
cat "$OUT"

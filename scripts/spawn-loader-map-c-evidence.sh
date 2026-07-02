#!/usr/bin/env bash
set -euo pipefail

ROOT="$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)"
SRC="${1:-$ROOT/lq/taskman/proc/spawn.c}"
OUT="${QSOE_EVIDENCE_OUT:-$ROOT/build/spawn-loader-map-c-evidence.txt}"
mkdir -p "$(dirname "$OUT")"

if [[ ! -f "$SRC" ]]; then
    echo "spawn-loader-map-c-evidence: missing source: $SRC" >&2
    exit 1
fi

require_pattern() {
    local pattern="$1"
    if ! grep -Fq "$pattern" "$SRC"; then
        echo "spawn-loader-map-c-evidence: missing pattern: $pattern" >&2
        exit 1
    fi
}

require_pattern "typedef enum tm_loader_map_status"
require_pattern "TM_LOADER_MAP_LIBC_LOAD_FAILED"
require_pattern "TM_LOADER_MAP_RTLD_LOAD_FAILED"
require_pattern "TM_LOADER_MAP_LIBC_PARSE_FAILED"
require_pattern "TM_LOADER_MAP_RTLD_PARSE_FAILED"
require_pattern "TM_LOADER_MAP_MAIN_PARSE_FAILED"
require_pattern "typedef struct tm_loader_map_plan"
require_pattern "tm_loader_map_dynamic"
require_pattern "admit->status != TM_LOADER_ADMIT_READY"
require_pattern "map_plan->libc_load_base = DL_LIBC_LOAD_VA"
require_pattern "map_plan->rtld_load_base = DL_RTLD_LOAD_VA"
require_pattern "load_elf_segments(vspace, admit->libc_blob"
require_pattern "load_elf_segments(vspace, admit->rtld_blob"
require_pattern "tm_elf_parse(admit->libc_blob"
require_pattern "tm_elf_parse(admit->rtld_blob"
require_pattern "tm_elf_parse(main_blob"
require_pattern "map_plan->status = TM_LOADER_MAP_READY"
require_pattern "tm_loader_map_dynamic(&loader_map"
require_pattern "tm_reloc_apply(&loader_map.libc_view"
require_pattern "tm_reloc_init_resolver(&loader_map.libc_view"
require_pattern "tm_reloc_apply(&loader_map.rtld_view"
require_pattern "tm_reloc_apply(&loader_map.main_view"
require_pattern "loader_map.libc_load_base"
require_pattern "loader_map.rtld_load_base"
require_pattern "tm_loader_proto_admit_dynamic(&loader_proto"
require_pattern "reloc_write_cb"
require_pattern "qsoe_tcb_write_registers"

if grep -Fq "tm_reloc_apply(&libc_view" "$SRC" ||    grep -Fq "tm_reloc_apply(&rtld_view" "$SRC" ||    grep -Fq "tm_reloc_apply(&main_view" "$SRC"; then
    echo "spawn-loader-map-c-evidence: stale inline ELF view relocation call remains" >&2
    exit 1
fi

{
    echo "spawn-loader-map-c-evidence: ok"
    echo "source=$SRC"
    echo "checked=tm_loader_map_plan load bases, ELF views, load/parse failures, relocation handoff"
} > "$OUT"
cat "$OUT"

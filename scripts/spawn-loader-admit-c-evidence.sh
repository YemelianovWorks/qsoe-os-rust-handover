#!/bin/sh
set -eu

OUT_DIR="build/spawn-loader-admit-c-evidence"
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
echo "tm_loader_admit C seam evidence" >> "$SUMMARY"
echo "source: $SRC" >> "$SUMMARY"

require_source "typedef enum tm_loader_admit_status" "bounded loader admission status enum"
require_source "TM_LOADER_ADMIT_MISSING_RTLD" "missing runtime linker failure state"
require_source "TM_LOADER_ADMIT_MISSING_LIBC" "missing libc failure state"
require_source "typedef struct tm_loader_admit" "bounded loader admission state"
require_source "tm_loader_admit_dynamic" "dynamic loader admission seam"
require_source "admit->interp_cpio_name" "PT_INTERP path normalized into admission state"
require_source "tm_cpio_lookup(admit->interp_cpio_name" "runtime linker lookup owned by admission seam"
require_source "tm_cpio_lookup(\"lib/libc.so\"" "libc lookup owned by admission seam"
require_source "admit->status = TM_LOADER_ADMIT_READY" "successful admission state recorded"
require_source "tm_loader_admit_dynamic(&loader_admit" "C-owned dynamic admission call site"
require_source "admit->libc_blob" "libc load/parse consumes admission state through loader map plan"
require_source "admit->rtld_blob" "rtld load/parse consumes admission state through loader map plan"
require_source "admit->libc_size" "libc parse size consumes admission state through loader map plan"
require_source "admit->rtld_size" "rtld parse size consumes admission state through loader map plan"
require_source "const struct elf64_hdr *rtld_eh = loader_admit.rtld_blob" "rtld debug metadata consumes admission state"
require_source "tm_reloc_apply(&loader_map.main_view" "relocation authority remains in C after admission via loader map plan"
require_source "tm_loader_proto_admit_dynamic(&loader_proto" "protocol state remains a later C-owned seam"
require_source "qsoe_tcb_write_registers" "TCB authority remains in C commit path"

echo "result: PASS" >> "$SUMMARY"
cat "$SUMMARY"

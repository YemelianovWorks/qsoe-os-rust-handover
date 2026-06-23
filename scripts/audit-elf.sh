#!/usr/bin/env bash
#
# Audit ELF artifacts before admitting them into a QSOE image.
#
# The script is intentionally read-only. It reports the properties that matter
# for the current QSOE loader contract and can optionally fail on features that
# are known to be unsafe for first-stage Rust userland experiments.

set -u

usage() {
    cat <<'EOF'
usage: scripts/audit-elf.sh [--strict-qsoe-user] <elf> [<elf> ...]

Reports:
  - ELF header
  - interpreter / program headers
  - dynamic section
  - relocations
  - TLS and unwind-related sections
  - undefined dynamic symbols

With --strict-qsoe-user, fail if an artifact uses TLS, unwind sections, or
relocation types outside the current QSOE userland baseline:
  R_RISCV_RELATIVE, R_RISCV_64, R_RISCV_JUMP_SLOT
EOF
}

strict=0
files=()

while [ "$#" -gt 0 ]; do
    case "$1" in
        --strict-qsoe-user)
            strict=1
            shift
            ;;
        -h|--help)
            usage
            exit 0
            ;;
        --)
            shift
            while [ "$#" -gt 0 ]; do
                files+=("$1")
                shift
            done
            ;;
        -*)
            echo "audit-elf.sh: unknown option: $1" >&2
            usage >&2
            exit 2
            ;;
        *)
            files+=("$1")
            shift
            ;;
    esac
done

if [ "${#files[@]}" -eq 0 ]; then
    usage >&2
    exit 2
fi

find_readelf() {
    local t
    for t in llvm-readelf readelf riscv64-linux-gnu-readelf greadelf; do
        if command -v "$t" >/dev/null 2>&1; then
            command -v "$t"
            return 0
        fi
    done
    return 1
}

READELF=$(find_readelf) || {
    echo "audit-elf.sh: no readelf tool found." >&2
    echo "Install llvm-readelf, readelf, riscv64-linux-gnu-readelf, or greadelf." >&2
    exit 127
}

status=0

print_section() {
    printf '\n-- %s --\n' "$1"
}

has_tls_or_unwind() {
    "$READELF" -S "$1" 2>/dev/null |
        grep -E '(\.(tdata|tbss|eh_frame|gcc_except_table|debug_frame)| TLS )' >/dev/null
}

unsupported_relocs() {
    "$READELF" -r "$1" 2>/dev/null |
        awk '
            /R_/ {
                rel = ""
                for (i = 1; i <= NF; i++) {
                    if ($i ~ /^R_/) {
                        rel = $i
                        break
                    }
                }
                if (rel != "" &&
                    rel != "R_RISCV_RELATIVE" &&
                    rel != "R_RISCV_64" &&
                    rel != "R_RISCV_JUMP_SLOT") {
                    print rel
                }
            }
        ' | sort -u
}

for f in "${files[@]}"; do
    if [ ! -f "$f" ]; then
        echo "audit-elf.sh: not a file: $f" >&2
        status=1
        continue
    fi

    printf '==> %s\n' "$f"
    printf 'readelf: %s\n' "$READELF"

    print_section "ELF header"
    "$READELF" -h "$f" || status=1

    print_section "Program headers"
    "$READELF" -l "$f" || status=1

    print_section "Dynamic section"
    "$READELF" -d "$f" 2>/dev/null || echo "(no dynamic section)"

    print_section "Relocations"
    "$READELF" -r "$f" 2>/dev/null || echo "(no relocation sections)"

    print_section "TLS / unwind sections"
    if "$READELF" -S "$f" 2>/dev/null |
        grep -E '(\.(tdata|tbss|eh_frame|gcc_except_table|debug_frame)| TLS )'; then
        :
    else
        echo "(none found)"
    fi

    print_section "Undefined dynamic symbols"
    "$READELF" -Ws "$f" 2>/dev/null |
        awk '$7 == "UND" && $4 != "SECTION" { print }' || echo "(symbol table unavailable)"

    if [ "$strict" -eq 1 ]; then
        bad_relocs=$(unsupported_relocs "$f")
        if [ -n "$bad_relocs" ]; then
            echo "audit-elf.sh: unsupported relocations in $f:" >&2
            echo "$bad_relocs" >&2
            status=1
        fi
        if has_tls_or_unwind "$f"; then
            echo "audit-elf.sh: TLS or unwind sections found in $f" >&2
            status=1
        fi
    fi

    printf '\n'
done

exit "$status"

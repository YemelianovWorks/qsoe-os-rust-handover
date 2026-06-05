/*
 * <libtaskman/elf.h> -- ELF64 little-endian (RISC-V) parser.
 *
 * Used by every QSOE taskman to load user images at runtime: the body
 * is OS-independent (pure byte-level inspection of a caller-provided
 * buffer), so NQ and LQ share it.  No allocations, no syscalls, no
 * kernel coupling.
 *
 * Scope is deliberately narrow -- we don't need a general ELF library,
 * just enough to drive a dynamic loader:
 *   - validate the magic / arch / ELF class
 *   - enumerate PT_LOAD program headers
 *   - locate PT_INTERP and return its path string
 *   - report the image's entry point and overall vaddr range
 *
 * Anything else (sections, symbols, relocations, ELF64 BE, ARM/x86)
 * is out of scope and lives in rtld when needed.
 *
 * Copyright (c) 2026 Yuri Zaporozhets <yuriz@qsoe.net>
 * SPDX-License-Identifier: Apache-2.0
 */
#ifndef LIBTASKMAN_ELF_H
#define LIBTASKMAN_ELF_H

#include <stdint.h>

/* PT_LOAD permission bits (raw from the ELF p_flags field).  We
 * expose them as POSIX-style PROT_* so callers feed them straight
 * into the per-OS mmap surface. */
#define ELF_PROT_READ    0x4   /* PF_R */
#define ELF_PROT_WRITE   0x2   /* PF_W */
#define ELF_PROT_EXEC    0x1   /* PF_X */

/* One PT_LOAD slot in the parsed view.  Bytes-from-the-file lie at
 * [blob + file_offset, blob + file_offset + file_size); they should
 * land at [vaddr + load_bias, vaddr + load_bias + file_size) in the
 * caller's VSpace.  The tail [vaddr + file_size, vaddr + mem_size)
 * is zero-filled (the .bss region).  perms is a PROT_* mask
 * (ELF_PROT_READ|WRITE|EXEC) translated 1:1 from p_flags. */
typedef struct tm_elf_phdr {
    uint64_t file_offset;
    uint64_t file_size;
    uint64_t vaddr;
    uint64_t mem_size;
    uint32_t perms;             /* PROT_* mask */
    uint32_t _pad;
} tm_elf_phdr_t;

/* Hard limit on PT_LOAD count.  qsh / rtld / libc.so each have at
 * most 3-4 (text, rodata, data, tls) -- 8 is comfortable headroom. */
#define TM_ELF_MAX_PHDRS  8

typedef struct tm_elf_view {
    const void   *blob;         /* caller's pointer, retained for memcpy */
    uint64_t      blob_size;    /* caller's buffer size, kept for bounds */

    uint64_t      entry;        /* e_entry from the header (pre-bias) */
    uint64_t      vaddr_lo;     /* lowest vaddr across all PT_LOAD */
    uint64_t      vaddr_hi;     /* highest vaddr + mem_size across all PT_LOAD */

    int           is_dyn;       /* 1 = ET_DYN (PIE / .so), 0 = ET_EXEC */

    unsigned      n_phdrs;      /* PT_LOAD count (<= TM_ELF_MAX_PHDRS) */
    tm_elf_phdr_t phdrs[TM_ELF_MAX_PHDRS];

    /* PT_INTERP, if present.  interp_path is a pointer into `blob`
     * (NUL-terminated string inside the file).  NULL if not present. */
    const char   *interp_path;
    uint64_t      interp_len;

    /* Phdr table location in the file -- needed for AT_PHDR/PHNUM/PHENT
     * in the aux-vector after loading. */
    uint64_t      phdr_off;     /* offset within file */
    uint16_t      phdr_entsize;
    uint16_t      phdr_count;
} tm_elf_view_t;

/* Parse `blob` (size `blob_size`) into `out`.  Returns 0 on success,
 * -1 on any malformed-ELF / unsupported-arch condition.  After a
 * successful parse the caller may inspect out->phdrs (the PT_LOAD
 * vector) and out->interp_path; the original `blob` must remain live
 * because phdrs index into it via file_offset. */
int tm_elf_parse(const void *blob, uint64_t blob_size, tm_elf_view_t *out);

#endif /* LIBTASKMAN_ELF_H */

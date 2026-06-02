/*
 * <libtaskman/reloc.h> -- RV64 ELF relocation walker, shared across
 *                         every QSOE taskman.
 *
 * Used to pre-relocate ET_DYN images (rtld, libc.so) and resolve
 * JUMP_SLOT/R_RISCV_64 references in ET_EXEC user programs (qsh)
 * AGAINST a previously-loaded image (typically libc.so).
 *
 * Pure byte-level over a tm_elf_view_t (the file blob is kept live by
 * the caller; this code reads its DT_RELA / DT_JMPREL / DT_SYMTAB /
 * DT_STRTAB tables straight from the blob without touching the
 * loaded image's VSpace).  Writes go through a caller-supplied 8-byte
 * write callback so the same body works for:
 *
 *   - NQ (Skimmer):     single-VSpace; callback does *(uint64_t*)v = q.
 *   - LQ (seL4):        child VSpace; callback scratch-maps the child
 *                       frame containing v, writes, and unmaps.
 *
 * Handles three relocation types:
 *
 *   R_RISCV_RELATIVE  -- internal pointer.  *loc = bias + r_addend.
 *   R_RISCV_64        -- absolute 64-bit reference.  Resolves via the
 *                        image's own dynsym if non-UND; via the
 *                        external resolver otherwise.
 *   R_RISCV_JUMP_SLOT -- PLT slot.  Same resolution as R_RISCV_64.
 *
 * Other types are silently skipped (counted in *out_skipped) -- they're
 * either TLS-related (handled by rtld at startup) or unused at our
 * scale.  We deliberately do NOT touch GNU_IFUNC; rtld can deal.
 *
 * Copyright (c) 2026 Yuri Zaporozhets <r_tty@yahoo.co.uk>
 * SPDX-License-Identifier: Apache-2.0
 */
#ifndef LIBTASKMAN_RELOC_H
#define LIBTASKMAN_RELOC_H

#include <stdint.h>
#include <tm_elf.h>

/* External symbol resolver -- one of these describes a previously-
 * loaded image whose exports are visible to subsequent loads.  All
 * pointers are CALLER-VIEW (i.e., into the file blob taskman holds
 * for the OTHER image); st_value is added to `base` to yield the
 * runtime VA inside the resolver-image's address space.
 *
 * Populate via tm_reloc_init_resolver() after loading libc.so; pass
 * the result as `ext` when loading rtld and qsh. */
typedef struct tm_reloc_resolver {
    unsigned long  base;        /* runtime bias of the resolver image */
    const void    *symtab;      /* elf64_sym_t[] in caller's view */
    const char    *strtab;      /* dynstr in caller's view */
    unsigned long  nsyms;       /* symtab entry count (DT_HASH/DT_GNU_HASH) */
} tm_reloc_resolver_t;

/* 8-byte write callback.  vaddr is in the target image's address
 * space (runtime VA after bias).  Returns 0 on success, -1 on
 * failure (failure stops the walk).  `user` is the caller-supplied
 * cookie passed to tm_reloc_apply(). */
typedef int (*tm_reloc_write_q_fn)(void *user, uint64_t vaddr, uint64_t value);

/* Optional per-skip logger.  Called once for every external symbol
 * that the walker had to leave NULL (no resolution path).  Lets the
 * caller surface a "WARN: reloc skip <name>" line per
 * feedback_stubs_announce -- silent NULL slots crash hours later
 * with no context, so we always announce them at load time too.
 * `name` is a NUL-terminated pointer into the file blob; do NOT
 * retain past the tm_reloc_apply() call.  Pass NULL to suppress. */
typedef void (*tm_reloc_skip_log_fn)(void *user, const char *name);

/* Apply all RELATIVE / R_RISCV_64 / JUMP_SLOT relocations in `view`.
 *
 *   view       : parsed file image (blob still live).
 *   bias       : runtime load offset (0 for ET_EXEC; the picked base
 *                for ET_DYN).
 *   ext        : optional external resolver (NULL = no external syms).
 *   write_cb   : per-reloc 8-byte write seam.
 *   user       : passed to write_cb verbatim.
 *   out_applied,
 *   out_total,
 *   out_skipped: optional counters.  Each may be NULL.
 *
 * Returns 0 on success, -1 if the dynamic section is malformed.
 * (Note: a perfectly fine image with no PT_DYNAMIC also returns 0
 * with 0/0/0 counters -- a static-linked ET_EXEC just has nothing
 * to do.) */
int tm_reloc_apply(const tm_elf_view_t *view,
                    unsigned long bias,
                    const tm_reloc_resolver_t *ext,
                    tm_reloc_write_q_fn write_cb,
                    tm_reloc_skip_log_fn skip_log,
                    void *user,
                    unsigned long *out_applied,
                    unsigned long *out_total,
                    unsigned long *out_skipped);

/* Populate `out` so the loaded image can serve as `ext` for later
 * tm_reloc_apply() calls.  Reads DT_SYMTAB / DT_STRTAB / one of
 * DT_HASH / DT_GNU_HASH straight from the file blob.  Returns 0 on
 * success, -1 if the dynamic-table entries we need are missing. */
int tm_reloc_init_resolver(const tm_elf_view_t *view,
                            unsigned long bias,
                            tm_reloc_resolver_t *out);

#endif /* LIBTASKMAN_RELOC_H */

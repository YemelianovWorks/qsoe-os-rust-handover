/*
 * libtaskman/src/elf.c -- ELF64 little-endian (RISC-V) parser body.
 *
 * Pure byte-level over a caller-provided buffer.  No allocations, no
 * syscalls, no kernel coupling.  Walks the program-header table once,
 * captures every PT_LOAD into out->phdrs[] and PT_INTERP into
 * out->interp_path; computes the vaddr lo/hi span and the entry
 * point.  Returns -1 on any malformed-ELF condition (bad magic, wrong
 * class/endian/arch/type, PHT outside the blob, PT_LOAD count > limit,
 * PT_INTERP string outside the blob).
 *
 * The view's blob/phdrs pointers reference the caller's buffer; the
 * caller keeps the buffer live until it has consumed the segments
 * (the typical pattern: parse, then memcpy each PT_LOAD's bytes from
 * file_offset into the freshly-mapped destination, then drop the
 * buffer).
 *
 * Copyright (c) 2026 Yuri Zaporozhets <r_tty@yahoo.co.uk>
 * SPDX-License-Identifier: Apache-2.0
 */

#include <tm_elf.h>

/* ELF64 header constants we care about (no <elf.h> dependency -- keeps
 * libtaskman self-contained on the freestanding NQ taskman build). */
#define EI_NIDENT       16
#define EI_CLASS        4
#define EI_DATA         5
#define ELFCLASS64      2
#define ELFDATA2LSB     1
#define ET_EXEC         2
#define ET_DYN          3
#define EM_RISCV        243
#define PT_LOAD         1
#define PT_INTERP       3

/* Same layout as <elf.h>'s Elf64_Ehdr / Elf64_Phdr; redefined locally
 * so libtaskman builds without a libc include. */
typedef struct {
    unsigned char e_ident[EI_NIDENT];
    uint16_t e_type;
    uint16_t e_machine;
    uint32_t e_version;
    uint64_t e_entry;
    uint64_t e_phoff;
    uint64_t e_shoff;
    uint32_t e_flags;
    uint16_t e_ehsize;
    uint16_t e_phentsize;
    uint16_t e_phnum;
    uint16_t e_shentsize;
    uint16_t e_shnum;
    uint16_t e_shstrndx;
} elf64_ehdr_t;

typedef struct {
    uint32_t p_type;
    uint32_t p_flags;
    uint64_t p_offset;
    uint64_t p_vaddr;
    uint64_t p_paddr;
    uint64_t p_filesz;
    uint64_t p_memsz;
    uint64_t p_align;
} elf64_phdr_t;

/* Bounded read: ensure [off, off+span) fits in [0, blob_size). */
static int span_in_blob(uint64_t off, uint64_t span, uint64_t blob_size)
{
    if (span > blob_size) return 0;
    if (off > blob_size - span) return 0;
    return 1;
}

int tm_elf_parse(const void *blob, uint64_t blob_size, tm_elf_view_t *out)
{
    if (!blob || !out || blob_size < sizeof(elf64_ehdr_t)) return -1;

    const unsigned char *bp = (const unsigned char *) blob;
    const elf64_ehdr_t  *eh = (const elf64_ehdr_t *) blob;

    /* Magic and class/endian/arch validation. */
    if (eh->e_ident[0] != 0x7f || eh->e_ident[1] != 'E' ||
        eh->e_ident[2] != 'L'  || eh->e_ident[3] != 'F')
        return -1;
    if (eh->e_ident[EI_CLASS] != ELFCLASS64)  return -1;
    if (eh->e_ident[EI_DATA]  != ELFDATA2LSB) return -1;
    if (eh->e_machine != EM_RISCV)            return -1;
    if (eh->e_type != ET_EXEC && eh->e_type != ET_DYN) return -1;
    if (eh->e_phentsize < sizeof(elf64_phdr_t))        return -1;

    uint64_t pht_bytes = (uint64_t) eh->e_phnum * eh->e_phentsize;
    if (!span_in_blob(eh->e_phoff, pht_bytes, blob_size))
        return -1;

    out->blob         = blob;
    out->blob_size    = blob_size;
    out->entry        = eh->e_entry;
    out->is_dyn       = (eh->e_type == ET_DYN);
    out->vaddr_lo     = (uint64_t) -1;
    out->vaddr_hi     = 0;
    out->n_phdrs      = 0;
    out->interp_path  = 0;
    out->interp_len   = 0;
    out->phdr_off     = eh->e_phoff;
    out->phdr_entsize = eh->e_phentsize;
    out->phdr_count   = eh->e_phnum;

    /* Walk the PHT once.  Tolerate unknown p_type values (they're not
     * an error -- ELF reserves many).  Only react to PT_LOAD and
     * PT_INTERP. */
    for (unsigned i = 0; i < eh->e_phnum; i++) {
        const elf64_phdr_t *ph =
            (const elf64_phdr_t *) (bp + eh->e_phoff + i * eh->e_phentsize);

        if (ph->p_type == PT_LOAD) {
            if (ph->p_filesz > ph->p_memsz) return -1;
            if (ph->p_filesz != 0 &&
                !span_in_blob(ph->p_offset, ph->p_filesz, blob_size))
                return -1;
            if (out->n_phdrs >= TM_ELF_MAX_PHDRS) return -1;

            tm_elf_phdr_t *slot = &out->phdrs[out->n_phdrs++];
            slot->file_offset = ph->p_offset;
            slot->file_size   = ph->p_filesz;
            slot->vaddr       = ph->p_vaddr;
            slot->mem_size    = ph->p_memsz;
            slot->perms       = ph->p_flags & (ELF_PROT_READ |
                                               ELF_PROT_WRITE |
                                               ELF_PROT_EXEC);
            slot->_pad        = 0;

            uint64_t seg_lo = ph->p_vaddr;
            uint64_t seg_hi = ph->p_vaddr + ph->p_memsz;
            if (seg_lo < out->vaddr_lo) out->vaddr_lo = seg_lo;
            if (seg_hi > out->vaddr_hi) out->vaddr_hi = seg_hi;
        } else if (ph->p_type == PT_INTERP) {
            if (ph->p_filesz == 0) return -1;
            if (!span_in_blob(ph->p_offset, ph->p_filesz, blob_size))
                return -1;
            const char *s = (const char *) (bp + ph->p_offset);
            /* PT_INTERP payload should be NUL-terminated within the
             * declared filesz; verify rather than trust. */
            uint64_t n = 0;
            while (n < ph->p_filesz && s[n] != '\0') n++;
            if (n == ph->p_filesz) return -1;       /* no terminator */
            out->interp_path = s;
            out->interp_len  = n;
        }
    }

    if (out->n_phdrs == 0)  return -1;
    if (out->vaddr_hi <= out->vaddr_lo) return -1;
    return 0;
}

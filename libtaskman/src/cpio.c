/*
 * libtaskman/src/cpio.c -- newc-format CPIO walker body.
 *
 * Pure byte arithmetic over a caller-provided buffer; no allocations,
 * no syscalls.  Lifted from the kernel-side walker that drives
 * boot-time taskman loading (nq/kernel/cpio.c), with skimmer types
 * replaced by stdint and the libtaskman tm_* naming convention.
 *
 * Copyright (c) 2020, 2026 Yuri Zaporozhets <r_tty@yahoo.co.uk>
 * SPDX-License-Identifier: Apache-2.0
 */

#include <tm_cpio.h>

typedef struct __attribute__((packed)) {
    char magic[6];
    char ino[8];
    char mode[8];
    char uid[8];
    char gid[8];
    char nlink[8];
    char mtime[8];
    char filesize[8];
    char devmajor[8];
    char devminor[8];
    char rdevmajor[8];
    char rdevminor[8];
    char namesize[8];
    char check[8];
} cpio_newc_header_t;

static int s_memeq(const void *a, const void *b, unsigned n)
{
    const unsigned char *p = a, *q = b;
    for (unsigned i = 0; i < n; i++) if (p[i] != q[i]) return 0;
    return 1;
}

static unsigned s_strlen(const char *s)
{
    unsigned n = 0;
    while (s[n] != '\0') n++;
    return n;
}

static int s_strneq(const char *a, const char *b, unsigned n)
{
    for (unsigned i = 0; i < n; i++) {
        if (a[i] != b[i]) return 0;
        if (a[i] == '\0') return 1;
    }
    return 1;
}

static uint32_t parse_hex8(const char *hex)
{
    uint32_t val = 0;
    for (int i = 0; i < 8; i++) {
        val <<= 4;
        if (hex[i] >= '0' && hex[i] <= '9')
            val |= (uint32_t) (hex[i] - '0');
        else if (hex[i] >= 'a' && hex[i] <= 'f')
            val |= (uint32_t) (hex[i] - 'a' + 10);
        else if (hex[i] >= 'A' && hex[i] <= 'F')
            val |= (uint32_t) (hex[i] - 'A' + 10);
        else
            return 0;
    }
    return val;
}

static inline const uint8_t *align_4(const uint8_t *ptr)
{
    return (const uint8_t *) (((uintptr_t) ptr + 3) & ~(uintptr_t) 3);
}

int tm_cpio_check_valid(const uint8_t *data, uint64_t size)
{
    if (size < sizeof(cpio_newc_header_t)) return 0;
    const cpio_newc_header_t *hdr = (const cpio_newc_header_t *) data;
    return s_memeq(hdr->magic, TM_CPIO_NEWC_MAGIC, 6);
}

void tm_cpio_iterate(const uint8_t *data, uint64_t size,
                     tm_cpio_callback_t cb, void *user)
{
    const uint8_t *ptr = data;
    const uint8_t *end = ptr + size;

    while (ptr < end) {
        if (ptr + sizeof(cpio_newc_header_t) > end) break;
        const cpio_newc_header_t *hdr = (const cpio_newc_header_t *) ptr;
        if (!s_memeq(hdr->magic, TM_CPIO_NEWC_MAGIC, 6)) break;

        uint32_t namesize = parse_hex8(hdr->namesize);
        uint32_t filesize = parse_hex8(hdr->filesize);
        uint32_t mode     = parse_hex8(hdr->mode);

        ptr += sizeof(cpio_newc_header_t);
        if (ptr + namesize > end) break;
        const char *filename = (const char *) ptr;
        ptr += namesize;
        ptr = align_4(ptr);

        /* End-of-archive sentinel. */
        if (s_strneq(filename, "TRAILER!!!", 11)) break;
        if (ptr + filesize > end) break;

        tm_cpio_file_info_t info;
        unsigned i;
        for (i = 0; i + 1 < sizeof info.filename && filename[i]; i++)
            info.filename[i] = filename[i];
        info.filename[i] = '\0';
        info.filesize = filesize;
        info.mode     = mode;
        info.data     = ptr;

        if (cb != 0) cb(&info, user);

        ptr += filesize;
        ptr = align_4(ptr);
    }
}

typedef struct {
    const char           *target;
    tm_cpio_file_info_t  *result;
    int                   found;
} find_ctx_t;

static void find_cb(const tm_cpio_file_info_t *fi, void *user)
{
    find_ctx_t *ctx = user;
    if (ctx->found) return;
    if (s_strneq(fi->filename, ctx->target, sizeof fi->filename) &&
        fi->filename[s_strlen(ctx->target)] == '\0') {
        *ctx->result = *fi;
        ctx->found = 1;
    }
}

int tm_cpio_find_file(const uint8_t *data, uint64_t size,
                      const char *filename, tm_cpio_file_info_t *info)
{
    find_ctx_t ctx = { filename, info, 0 };
    tm_cpio_iterate(data, size, find_cb, &ctx);
    return ctx.found;
}

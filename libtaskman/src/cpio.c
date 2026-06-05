/*
 * libtaskman/src/cpio.c -- newc-format CPIO walker body.
 *
 * Pure byte arithmetic over a caller-provided buffer; no allocations,
 * no syscalls.  Lifted from the kernel-side walker that drives
 * boot-time taskman loading (nq/kernel/cpio.c), with skimmer types
 * replaced by stdint and the libtaskman tm_* naming convention.
 *
 * Copyright (c) 2020, 2026 Yuri Zaporozhets <yuriz@qsoe.net>
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

/* Strip a leading '/' so callers can pass POSIX-shape paths and the
 * walker sees the cpio-form ("bin/sh" not "/bin/sh"). */
static const char *cpio_strip_slash(const char *p)
{
    return (p && p[0] == '/') ? p + 1 : p;
}

/* Bounded copy.  Returns 0 on success, -1 if `src_len` doesn't fit. */
static int cpio_copy_str(char *dst, unsigned cap,
                         const char *src, unsigned src_len)
{
    if (cap == 0) return -1;
    if (src_len + 1 > cap) { dst[0] = '\0'; return -1; }
    for (unsigned i = 0; i < src_len; i++) dst[i] = src[i];
    dst[src_len] = '\0';
    return 0;
}

int tm_cpio_resolve_path(const uint8_t *data, uint64_t size,
                         const char *path,
                         char *out_path, unsigned out_cap,
                         tm_cpio_file_info_t *out_info)
{
    char cur[64];
    const char *clean = cpio_strip_slash(path);
    if (cpio_copy_str(cur, sizeof cur, clean, s_strlen(clean)) != 0)
        return -1;

    for (int hop = 0; hop < TM_CPIO_MAX_SYMLINKS; hop++) {
        tm_cpio_file_info_t info;
        if (!tm_cpio_find_file(data, size, cur, &info))
            return 0;
        if ((info.mode & TM_CPIO_S_IFMT) != TM_CPIO_S_IFLNK) {
            *out_info = info;
            return (cpio_copy_str(out_path, out_cap,
                                  cur, s_strlen(cur)) == 0) ? 1 : -1;
        }
        /* Symlink: replace `cur` with the target.  Absolute targets
         * (leading '/') override the whole path; relative targets are
         * resolved against the symlink's directory. */
        const char *tgt = (const char *) info.data;
        unsigned tlen = info.filesize;
        if (tlen == 0 || tlen >= sizeof cur) return -1;

        char next[64];
        if (tgt[0] == '/') {
            if (cpio_copy_str(next, sizeof next, tgt + 1, tlen - 1) != 0)
                return -1;
        } else {
            int last_slash = -1;
            for (unsigned i = 0; cur[i]; i++)
                if (cur[i] == '/') last_slash = (int) i;
            unsigned dir_keep = (last_slash >= 0) ? (unsigned) (last_slash + 1) : 0;
            if (dir_keep + tlen >= sizeof next) return -1;
            for (unsigned i = 0; i < dir_keep; i++) next[i] = cur[i];
            for (unsigned i = 0; i < tlen; i++) next[dir_keep + i] = tgt[i];
            next[dir_keep + tlen] = '\0';
        }
        if (cpio_copy_str(cur, sizeof cur, next, s_strlen(next)) != 0)
            return -1;
    }
    return -1;       /* loop / chain too deep */
}

/* Common prefix-and-component extractor.  Given a cpio entry name
 * and a directory prefix, return whether `name` is a descendant of
 * `prefix`, and if so the basename of the first path component after
 * the prefix.  `*out_type` is DT_DIR when the entry sits more than
 * one component deep (the component is a synthetic directory), or
 * DT_LNK / DT_REG when it IS the leaf. */
static int cpio_child_of(const char *name, const char *prefix, unsigned prefix_len,
                         uint32_t mode,
                         const char **out_comp, unsigned *out_complen,
                         unsigned *out_type)
{
    for (unsigned i = 0; i < prefix_len; i++) {
        if (name[i] == '\0' || name[i] != prefix[i]) return 0;
    }
    const char *tail = name + prefix_len;
    if (*tail == '\0') return 0;            /* the prefix entry itself */
    const char *p = tail;
    while (*p && *p != '/') p++;
    *out_comp    = tail;
    *out_complen = (unsigned) (p - tail);
    if (*p == '/') {
        *out_type = TM_CPIO_DT_DIR;
    } else {
        *out_type = ((mode & TM_CPIO_S_IFMT) == TM_CPIO_S_IFLNK)
                    ? TM_CPIO_DT_LNK : TM_CPIO_DT_REG;
    }
    return 1;
}

typedef struct {
    const char *prefix;
    unsigned    prefix_len;
    uint32_t    target;
    uint32_t    seen;
    char        emitted[TM_CPIO_DIRENT_MAX][TM_CPIO_DIRENT_NAME_MAX];
    unsigned    emitted_count;
    char       *out_name_buf;
    unsigned    out_name_cap;
    unsigned   *out_namelen;
    unsigned   *out_type;
    int         found;          /*  1=hit, 0=not yet, -1=buffer too small */
} dirent_ctx_t;

static int dirent_already_emitted(dirent_ctx_t *ctx,
                                  const char *comp, unsigned complen)
{
    for (unsigned i = 0; i < ctx->emitted_count; i++) {
        unsigned el = s_strlen(ctx->emitted[i]);
        if (el != complen) continue;
        int eq = 1;
        for (unsigned j = 0; j < complen; j++) {
            if (ctx->emitted[i][j] != comp[j]) { eq = 0; break; }
        }
        if (eq) return 1;
    }
    return 0;
}

static void dirent_at_cb(const tm_cpio_file_info_t *fi, void *user)
{
    dirent_ctx_t *ctx = user;
    if (ctx->found != 0) return;

    const char *comp; unsigned complen, type;
    if (!cpio_child_of(fi->filename, ctx->prefix, ctx->prefix_len,
                       fi->mode, &comp, &complen, &type))
        return;
    if (dirent_already_emitted(ctx, comp, complen)) return;

    /* Record under dedup; ignore (don't dedup) if either buffer cap
     * is exceeded -- the next entry with the same component name then
     * appears as a fresh hit, which is wrong, but we log the cap miss
     * to make it visible.  The TM_CPIO_DIRENT_MAX cap is sized for
     * our initrd; bump if needed. */
    if (ctx->emitted_count < TM_CPIO_DIRENT_MAX &&
        complen + 1 <= TM_CPIO_DIRENT_NAME_MAX) {
        for (unsigned i = 0; i < complen; i++)
            ctx->emitted[ctx->emitted_count][i] = comp[i];
        ctx->emitted[ctx->emitted_count][complen] = '\0';
        ctx->emitted_count++;
    }

    if (ctx->seen == ctx->target) {
        if (complen + 1 > ctx->out_name_cap) { ctx->found = -1; return; }
        for (unsigned i = 0; i < complen; i++) ctx->out_name_buf[i] = comp[i];
        ctx->out_name_buf[complen] = '\0';
        *ctx->out_namelen = complen;
        *ctx->out_type    = type;
        ctx->found = 1;
        return;
    }
    ctx->seen++;
}

int tm_cpio_dirent_at(const uint8_t *data, uint64_t size,
                      const char *prefix,
                      uint32_t index,
                      unsigned *out_type,
                      char *out_name_buf, unsigned out_name_cap,
                      unsigned *out_namelen)
{
    dirent_ctx_t ctx;
    ctx.prefix        = prefix;
    ctx.prefix_len    = s_strlen(prefix);
    ctx.target        = index;
    ctx.seen          = 0;
    ctx.emitted_count = 0;
    ctx.out_name_buf  = out_name_buf;
    ctx.out_name_cap  = out_name_cap;
    ctx.out_namelen   = out_namelen;
    ctx.out_type      = out_type;
    ctx.found         = 0;
    tm_cpio_iterate(data, size, dirent_at_cb, &ctx);
    return (ctx.found == 1) ? 1 : (ctx.found == -1 ? -1 : 0);
}

typedef struct {
    const char *prefix;
    unsigned    prefix_len;
    int         hit;
} exists_ctx_t;

static void exists_cb(const tm_cpio_file_info_t *fi, void *user)
{
    exists_ctx_t *ctx = user;
    if (ctx->hit) return;
    for (unsigned i = 0; i < ctx->prefix_len; i++) {
        if (fi->filename[i] == '\0' || fi->filename[i] != ctx->prefix[i]) return;
    }
    if (fi->filename[ctx->prefix_len] != '\0') ctx->hit = 1;
}

int tm_cpio_dir_exists(const uint8_t *data, uint64_t size,
                       const char *prefix)
{
    exists_ctx_t ctx = { prefix, s_strlen(prefix), 0 };
    tm_cpio_iterate(data, size, exists_cb, &ctx);
    return ctx.hit;
}

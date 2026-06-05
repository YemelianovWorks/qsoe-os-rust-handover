/*
 * libtaskman/src/syscfg.c -- system-configuration blob builder + walker.
 *
 * The builder produces a flat TLV stream (tm_syscfg_tag_t header +
 * payload, terminated by TM_SYSCFG_TAG_END) in a caller-provided
 * buffer; the walker reads it back tag-by-tag.  Both sides are pure
 * byte-level logic -- no libtaskman_seams() calls, no kernel ops.
 * The platform-data source (FDT, board firmware, etc.) is the per-OS
 * taskman's concern: it walks that and calls our emit_* helpers in
 * sequence.
 *
 * Lifted from LQ taskman's sys/syscfg.c (the in-buffer body), with
 * the global s_blob replaced by a caller-owned tm_syscfg_state_t.
 *
 * Copyright (c) 2026 Yuri Zaporozhets <yuriz@qsoe.net>
 * SPDX-License-Identifier: Apache-2.0
 */

#include <tm_syscfg.h>

static unsigned s_len(const char *str)
{
    unsigned n = 0;
    while (str && str[n]) ++n;
    return n;
}

void tm_syscfg_init(tm_syscfg_state_t *s, void *buf, unsigned cap)
{
    if (!s) return;
    s->buf   = (unsigned char *)buf;
    s->cap   = cap;
    s->len   = 0;
    s->ready = 0;
}

int tm_syscfg_emit(tm_syscfg_state_t *s, uint16_t id,
                   const void *payload, unsigned len)
{
    if (!s || !s->buf) return -1;
    if (s->ready) return -1;
    if (s->len + 4 + len > s->cap) return -1;
    s->buf[s->len + 0] = (unsigned char)(id & 0xff);
    s->buf[s->len + 1] = (unsigned char)((id >> 8) & 0xff);
    s->buf[s->len + 2] = (unsigned char)(len & 0xff);
    s->buf[s->len + 3] = (unsigned char)((len >> 8) & 0xff);
    if (len && payload) {
        const unsigned char *p = (const unsigned char *)payload;
        for (unsigned i = 0; i < len; ++i) s->buf[s->len + 4 + i] = p[i];
    }
    s->len += 4 + len;
    return 0;
}

int tm_syscfg_emit_u32(tm_syscfg_state_t *s, uint16_t id, uint32_t v)
{
    /* Little-endian: matches the walker's expectation in find_u32. */
    unsigned char le[4] = {
        (unsigned char)( v        & 0xff),
        (unsigned char)((v >>  8) & 0xff),
        (unsigned char)((v >> 16) & 0xff),
        (unsigned char)((v >> 24) & 0xff),
    };
    return tm_syscfg_emit(s, id, le, 4);
}

int tm_syscfg_emit_u64(tm_syscfg_state_t *s, uint16_t id, uint64_t v)
{
    unsigned char le[8];
    for (int i = 0; i < 8; ++i) le[i] = (unsigned char)((v >> (i * 8)) & 0xff);
    return tm_syscfg_emit(s, id, le, 8);
}

int tm_syscfg_emit_asciz(tm_syscfg_state_t *s, uint16_t id, const char *str)
{
    unsigned len = s_len(str);
    if (len == 0) return 0;             /* nothing to emit */
    /* Include trailing NUL so clients can use it as a C string
     * directly without copying. */
    return tm_syscfg_emit(s, id, str, len + 1);
}

int tm_syscfg_finalize(tm_syscfg_state_t *s)
{
    if (!s) return -1;
    if (s->ready) return 0;
    if (tm_syscfg_emit(s, TM_SYSCFG_TAG_END, 0, 0) != 0) return -1;
    s->ready = 1;
    return 0;
}

int tm_syscfg_get(const tm_syscfg_state_t *s,
                  const void **out_blob, unsigned *out_len)
{
    if (!s || !s->ready) return -1;
    if (out_blob) *out_blob = s->buf;
    if (out_len)  *out_len  = s->len;
    return 0;
}

int tm_syscfg_find(const tm_syscfg_state_t *s, unsigned tag_id,
                   const void **out_ptr, unsigned *out_len)
{
    if (!s || !s->ready) return -1;
    unsigned off = 0;
    while (off + 4 <= s->len) {
        uint16_t id  = (uint16_t)(s->buf[off] | (s->buf[off + 1] << 8));
        uint16_t len = (uint16_t)(s->buf[off + 2] | (s->buf[off + 3] << 8));
        if (id == TM_SYSCFG_TAG_END) return -1;
        if (id == tag_id) {
            if (out_ptr) *out_ptr = &s->buf[off + 4];
            if (out_len) *out_len = len;
            return 0;
        }
        off += 4 + len;
    }
    return -1;
}

int tm_syscfg_find_u32(const tm_syscfg_state_t *s, unsigned tag_id,
                       uint32_t *out)
{
    const void *p; unsigned len;
    if (tm_syscfg_find(s, tag_id, &p, &len) != 0) return -1;
    if (len != 4) return -1;
    const unsigned char *bp = (const unsigned char *)p;
    *out = (uint32_t)bp[0] |
           ((uint32_t)bp[1] <<  8) |
           ((uint32_t)bp[2] << 16) |
           ((uint32_t)bp[3] << 24);
    return 0;
}

int tm_syscfg_find_u64(const tm_syscfg_state_t *s, unsigned tag_id,
                       uint64_t *out)
{
    const void *p; unsigned len;
    if (tm_syscfg_find(s, tag_id, &p, &len) != 0) return -1;
    if (len != 8) return -1;
    const unsigned char *bp = (const unsigned char *)p;
    uint64_t v = 0;
    for (int i = 7; i >= 0; --i) v = (v << 8) | bp[i];
    *out = v;
    return 0;
}

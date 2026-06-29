/*
 * Host tests for LQ taskman's sysmap builder.
 *
 * These tests link the existing C implementation directly and capture the
 * tm_sysmap_* ABI/layout before the Rust provider is wired into taskman.
 */
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <qsoe/syscfg.h>
#include <qsoe/sysmap.h>
#include <sys/sysmap.h>

#define CHECK(expr) do { \
    if (!(expr)) { \
        fprintf(stderr, "%s:%d: check failed: %s\n", __FILE__, __LINE__, #expr); \
        exit(1); \
    } \
} while (0)

static unsigned char s_syscfg[512];
static unsigned s_syscfg_len;
static int s_syscfg_ready;

static uint16_t rd16(const unsigned char *b, unsigned off)
{
    return (uint16_t)b[off] | ((uint16_t)b[off + 1] << 8);
}

static uint32_t rd32(const unsigned char *b, unsigned off)
{
    return (uint32_t)b[off] |
           ((uint32_t)b[off + 1] << 8) |
           ((uint32_t)b[off + 2] << 16) |
           ((uint32_t)b[off + 3] << 24);
}

static uint64_t rd64(const unsigned char *b, unsigned off)
{
    return (uint64_t)rd32(b, off) | ((uint64_t)rd32(b, off + 4) << 32);
}

static void wr16(unsigned char *b, unsigned off, uint16_t value)
{
    b[off] = (unsigned char)value;
    b[off + 1] = (unsigned char)(value >> 8);
}

static void wr32(unsigned char *b, unsigned off, uint32_t value)
{
    b[off] = (unsigned char)value;
    b[off + 1] = (unsigned char)(value >> 8);
    b[off + 2] = (unsigned char)(value >> 16);
    b[off + 3] = (unsigned char)(value >> 24);
}

static void wr64(unsigned char *b, unsigned off, uint64_t value)
{
    wr32(b, off, (uint32_t)value);
    wr32(b, off + 4, (uint32_t)(value >> 32));
}

static void reset_syscfg(void)
{
    memset(s_syscfg, 0, sizeof s_syscfg);
    s_syscfg_len = 0;
    s_syscfg_ready = 0;
}

static void emit_syscfg(uint16_t id, const void *payload, unsigned len)
{
    CHECK(s_syscfg_len + 4 + len <= sizeof s_syscfg);
    wr16(s_syscfg, s_syscfg_len, id);
    wr16(s_syscfg, s_syscfg_len + 2, (uint16_t)len);
    if (len != 0) memcpy(s_syscfg + s_syscfg_len + 4, payload, len);
    s_syscfg_len += 4 + len;
}

static void finish_syscfg(void)
{
    emit_syscfg(TM_SYSCFG_TAG_END, NULL, 0);
    s_syscfg_ready = 1;
}

int tm_syscfg_get(const void **out_blob, unsigned *out_len)
{
    if (!s_syscfg_ready) return -1;
    if (out_blob) *out_blob = s_syscfg;
    if (out_len) *out_len = s_syscfg_len;
    return 0;
}

int tm_syscfg_find(unsigned tag_id, const void **out_ptr, unsigned *out_len)
{
    unsigned off = 0;
    if (!s_syscfg_ready) return -1;
    while (off + 4 <= s_syscfg_len) {
        unsigned id = rd16(s_syscfg, off);
        unsigned len = rd16(s_syscfg, off + 2);
        if (id == TM_SYSCFG_TAG_END) break;
        if (id == tag_id) {
            if (out_ptr) *out_ptr = s_syscfg + off + 4;
            if (out_len) *out_len = len;
            return 0;
        }
        off += 4 + len;
    }
    return -1;
}

int tm_syscfg_find_u64(unsigned tag_id, uint64_t *out)
{
    const void *ptr = NULL;
    unsigned len = 0;
    if (tm_syscfg_find(tag_id, &ptr, &len) != 0 || len != 8) return -1;
    if (out) *out = rd64((const unsigned char *)ptr, 0);
    return 0;
}

int tm_syscfg_find_u32(unsigned tag_id, uint32_t *out)
{
    const void *ptr = NULL;
    unsigned len = 0;
    if (tm_syscfg_find(tag_id, &ptr, &len) != 0 || len != 4) return -1;
    if (out) *out = rd32((const unsigned char *)ptr, 0);
    return 0;
}

static const unsigned char *get_page(unsigned *out_len)
{
    const void *page = NULL;
    unsigned len = 0;
    CHECK(tm_sysmap_get(&page, &len) == 0);
    CHECK(page != NULL);
    CHECK(len == QSOE_SYSMAP_BYTES);
    if (out_len) *out_len = len;
    return (const unsigned char *)page;
}

static const unsigned char *find_tlv(const unsigned char *page,
                                     uint16_t tag, unsigned *out_len)
{
    unsigned off = rd16(page, 6);
    unsigned end = rd32(page, 8);
    while (off + 4 <= end) {
        uint16_t id = rd16(page, off);
        unsigned len = rd16(page, off + 2);
        if (id == QSOE_SYSMAP_TAG_END) return NULL;
        if (id == tag) {
            if (out_len) *out_len = len;
            return page + off + 4;
        }
        off += (4 + len + 7) & ~7U;
    }
    return NULL;
}

static void test_get_before_build_fails(void)
{
    CHECK(tm_sysmap_get(NULL, NULL) == -1);
}

static void test_minimal_syscfg_builds_header_and_end(void)
{
    const unsigned char *page;

    reset_syscfg();
    finish_syscfg();

    CHECK(tm_sysmap_build() == 0);
    page = get_page(NULL);
    CHECK(rd32(page, 0) == QSOE_SYSMAP_MAGIC);
    CHECK(rd16(page, 4) == QSOE_SYSMAP_VERSION);
    CHECK(rd16(page, 6) == sizeof(qsoe_sysmap_hdr_t));
    CHECK(rd32(page, 8) == 24);
    CHECK(rd16(page, sizeof(qsoe_sysmap_hdr_t)) == QSOE_SYSMAP_TAG_END);
}

static void test_emits_timebase_plic_pci_and_designware_fields(void)
{
    unsigned char ecam[20];
    unsigned char prefetch_window[28];
    unsigned char mem_window[28];
    unsigned char dw[20];
    const unsigned char *page;
    const unsigned char *body;
    unsigned len = 0;
    uint64_t timebase = 10000000;
    uint32_t ncpu = 4;

    reset_syscfg();
    emit_syscfg(TM_SYSCFG_TAG_TIMEBASE_HZ, &timebase, sizeof timebase);
    emit_syscfg(TM_SYSCFG_TAG_NUM_CPUS, &ncpu, sizeof ncpu);

    memset(ecam, 0, sizeof ecam);
    wr64(ecam, 0, 0x30000000ULL);
    wr64(ecam, 8, 0x10000000ULL);
    wr32(ecam, 16, 0x7f);
    emit_syscfg(TM_SYSCFG_TAG_PCI_ECAM, ecam, sizeof ecam);

    memset(prefetch_window, 0, sizeof prefetch_window);
    wr64(prefetch_window, 0, 0x11110000ULL);
    wr64(prefetch_window, 8, 0x22220000ULL);
    wr64(prefetch_window, 16, 0x33330000ULL);
    wr32(prefetch_window, 24,
         TM_SYSCFG_PCI_WINDOW_MEM | TM_SYSCFG_PCI_WINDOW_PREFETCH);
    emit_syscfg(TM_SYSCFG_TAG_PCI_WINDOW,
                prefetch_window, sizeof prefetch_window);

    memset(mem_window, 0, sizeof mem_window);
    wr64(mem_window, 0, 0x40000000ULL);
    wr64(mem_window, 8, 0x40000000ULL);
    wr64(mem_window, 16, 0x04000000ULL);
    wr32(mem_window, 24, TM_SYSCFG_PCI_WINDOW_MEM);
    emit_syscfg(TM_SYSCFG_TAG_PCI_WINDOW, mem_window, sizeof mem_window);

    memset(dw, 0, sizeof dw);
    wr64(dw, 0, 0x50000000ULL);
    wr64(dw, 8, 0x1000ULL);
    wr32(dw, 16, 33);
    emit_syscfg(TM_SYSCFG_TAG_DW_MSI, dw, sizeof dw);
    finish_syscfg();

    CHECK(tm_sysmap_build() == 0);
    page = get_page(NULL);
    CHECK(rd32(page, 8) == 176);

    body = find_tlv(page, QSOE_SYSMAP_TAG_MTIME_FREQ, &len);
    CHECK(body != NULL);
    CHECK(len == sizeof(qsoe_sysmap_mtime_freq_t));
    CHECK(rd32(body, 0) == 10000000);
    CHECK(rd32(body, 4) == 0);

    body = find_tlv(page, QSOE_SYSMAP_TAG_PLIC, &len);
    CHECK(body != NULL);
    CHECK(len == sizeof(qsoe_sysmap_plic_t));
    CHECK(rd32(body, 20) == 4);

    body = find_tlv(page, QSOE_SYSMAP_TAG_PCI_ECAM, &len);
    CHECK(body != NULL);
    CHECK(len == sizeof(qsoe_sysmap_pci_ecam_t));
    CHECK(rd64(body, 0) == 0x30000000ULL);
    CHECK(rd64(body, 8) == 0x10000000ULL);
    CHECK(rd64(body, 16) == 0x50000000ULL);
    CHECK(rd64(body, 24) == 0x1000ULL);
    CHECK(body[32] == 0);
    CHECK(body[33] == 0x7f);
    CHECK(rd32(body, 36) == 33);
    CHECK(rd64(body, 40) == 0x40000000ULL);
    CHECK(rd64(body, 48) == 0x40000000ULL);
    CHECK(rd64(body, 56) == 0x04000000ULL);
}

int main(void)
{
    test_get_before_build_fails();
    test_minimal_syscfg_builds_header_and_end();
    test_emits_timebase_plic_pci_and_designware_fields();
    puts("tm_sysmap_model_test: ok");
    return 0;
}

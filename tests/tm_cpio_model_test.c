/*
 * Host tests for the portable tm_cpio archive model.
 *
 * These tests link the existing C implementation directly and capture the C
 * ABI/behavior before the Rust provider is wired into task-manager paths.
 */
#include <stddef.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <tm_cpio.h>

#define CHECK(expr) do { \
    if (!(expr)) { \
        fprintf(stderr, "%s:%d: check failed: %s\n", __FILE__, __LINE__, #expr); \
        exit(1); \
    } \
} while (0)

#define CHECK_STREQ(actual, expected) do { \
    if (strcmp((actual), (expected)) != 0) { \
        fprintf(stderr, "%s:%d: string mismatch\nexpected: <%s>\nactual:   <%s>\n", \
                __FILE__, __LINE__, (expected), (actual)); \
        exit(1); \
    } \
} while (0)

#define ARCHIVE_CAP 2048U
#define CPIO_REG 0100644U
#define CPIO_LNK 0120777U

struct archive_builder {
    uint8_t bytes[ARCHIVE_CAP];
    size_t len;
};

struct collect_ctx {
    char names[8][64];
    unsigned count;
};

static void pad4(struct archive_builder *b)
{
    while ((b->len & 3U) != 0) {
        CHECK(b->len < ARCHIVE_CAP);
        b->bytes[b->len++] = 0;
    }
}

static void push_bytes(struct archive_builder *b, const void *src, size_t n)
{
    CHECK(b->len + n <= ARCHIVE_CAP);
    memcpy(b->bytes + b->len, src, n);
    b->len += n;
}

static void push_hex(struct archive_builder *b, uint32_t value)
{
    char field[9];
    int rc = snprintf(field, sizeof field, "%08x", value);
    CHECK(rc == 8);
    push_bytes(b, field, 8);
}

static void push_entry(struct archive_builder *b, const char *name,
                       uint32_t mode, const char *data, uint32_t data_len)
{
    uint32_t namesize = (uint32_t) strlen(name) + 1U;

    push_bytes(b, "070701", 6);
    push_hex(b, 1);
    push_hex(b, mode);
    push_hex(b, 0);
    push_hex(b, 0);
    push_hex(b, 1);
    push_hex(b, 0);
    push_hex(b, data_len);
    push_hex(b, 0);
    push_hex(b, 0);
    push_hex(b, 0);
    push_hex(b, 0);
    push_hex(b, namesize);
    push_hex(b, 0);
    push_bytes(b, name, namesize);
    pad4(b);
    push_bytes(b, data, data_len);
    pad4(b);
}

static void make_fixture(struct archive_builder *b)
{
    memset(b, 0, sizeof *b);
    push_entry(b, "bin/qsh", CPIO_REG, "elf", 3);
    push_entry(b, "bin/sh", CPIO_LNK, "qsh", 3);
    push_entry(b, "etc/init", CPIO_LNK, "/bin/qsh", 8);
    push_entry(b, "usr/conf/passwd", CPIO_REG, "root\n", 5);
    push_entry(b, "usr/conf/shadow", CPIO_REG, "shadow\n", 7);
    push_entry(b, "TRAILER!!!", CPIO_REG, "", 0);
}

static void collect_cb(const tm_cpio_file_info_t *fi, void *user)
{
    struct collect_ctx *ctx = user;
    CHECK(ctx->count < sizeof(ctx->names) / sizeof(ctx->names[0]));
    snprintf(ctx->names[ctx->count], sizeof(ctx->names[ctx->count]),
             "%s", fi->filename);
    ctx->count++;
}

static void test_iterate_and_find(void)
{
    struct archive_builder b;
    struct collect_ctx ctx;
    tm_cpio_file_info_t info;

    make_fixture(&b);
    memset(&ctx, 0, sizeof ctx);
    CHECK(tm_cpio_check_valid(b.bytes, b.len) == 1);
    tm_cpio_iterate(b.bytes, b.len, collect_cb, &ctx);

    CHECK(ctx.count == 5);
    CHECK_STREQ(ctx.names[0], "bin/qsh");
    CHECK_STREQ(ctx.names[1], "bin/sh");
    CHECK_STREQ(ctx.names[2], "etc/init");
    CHECK_STREQ(ctx.names[3], "usr/conf/passwd");
    CHECK_STREQ(ctx.names[4], "usr/conf/shadow");

    memset(&info, 0, sizeof info);
    CHECK(tm_cpio_find_file(b.bytes, b.len, "usr/conf/passwd", &info) == 1);
    CHECK_STREQ(info.filename, "usr/conf/passwd");
    CHECK(info.filesize == 5);
    CHECK(memcmp(info.data, "root\n", 5) == 0);
    CHECK(tm_cpio_find_file(b.bytes, b.len, "missing", &info) == 0);
}

static void test_resolve_path(void)
{
    struct archive_builder b;
    tm_cpio_file_info_t info;
    char out[64];

    make_fixture(&b);
    memset(&info, 0, sizeof info);
    memset(out, 0, sizeof out);

    CHECK(tm_cpio_resolve_path(b.bytes, b.len, "/bin/sh",
                               out, sizeof out, &info) == 1);
    CHECK_STREQ(out, "bin/qsh");
    CHECK_STREQ(info.filename, "bin/qsh");

    CHECK(tm_cpio_resolve_path(b.bytes, b.len, "/etc/init",
                               out, sizeof out, &info) == 1);
    CHECK_STREQ(out, "bin/qsh");

    CHECK(tm_cpio_resolve_path(b.bytes, b.len, "/missing",
                               out, sizeof out, &info) == 0);
    CHECK(tm_cpio_resolve_path(b.bytes, b.len, "/etc/init",
                               out, 4, &info) == -1);
}

static void expect_dirent(const uint8_t *data, size_t size, const char *prefix,
                          uint32_t index, const char *expected,
                          unsigned expected_type)
{
    char name[32];
    unsigned namelen = 0;
    unsigned type = 0;

    memset(name, 0, sizeof name);
    CHECK(tm_cpio_dirent_at(data, size, prefix, index, &type,
                            name, sizeof name, &namelen) == 1);
    CHECK_STREQ(name, expected);
    CHECK(namelen == strlen(expected));
    CHECK(type == expected_type);
}

static void test_dirent_and_exists(void)
{
    struct archive_builder b;
    char name[4];
    unsigned namelen = 0;
    unsigned type = 0;

    make_fixture(&b);
    CHECK(tm_cpio_dir_exists(b.bytes, b.len, "") == 1);
    CHECK(tm_cpio_dir_exists(b.bytes, b.len, "usr/") == 1);
    CHECK(tm_cpio_dir_exists(b.bytes, b.len, "missing/") == 0);

    expect_dirent(b.bytes, b.len, "", 0, "bin", TM_CPIO_DT_DIR);
    expect_dirent(b.bytes, b.len, "", 1, "etc", TM_CPIO_DT_DIR);
    expect_dirent(b.bytes, b.len, "", 2, "usr", TM_CPIO_DT_DIR);
    expect_dirent(b.bytes, b.len, "bin/", 0, "qsh", TM_CPIO_DT_REG);
    expect_dirent(b.bytes, b.len, "bin/", 1, "sh", TM_CPIO_DT_LNK);
    expect_dirent(b.bytes, b.len, "usr/", 0, "conf", TM_CPIO_DT_DIR);
    CHECK(tm_cpio_dirent_at(b.bytes, b.len, "usr/", 1, &type,
                            name, sizeof name, &namelen) == 0);
    CHECK(tm_cpio_dirent_at(b.bytes, b.len, "usr/conf/", 0, &type,
                            name, sizeof name, &namelen) == -1);
}

static void test_malformed_archive_stops(void)
{
    struct archive_builder b;
    struct collect_ctx ctx;
    size_t second_header;

    make_fixture(&b);
    second_header = 124;
    CHECK(memcmp(b.bytes + second_header, "070701", 6) == 0);
    b.bytes[second_header] = 'x';

    memset(&ctx, 0, sizeof ctx);
    tm_cpio_iterate(b.bytes, b.len, collect_cb, &ctx);
    CHECK(ctx.count == 1);
    CHECK_STREQ(ctx.names[0], "bin/qsh");

    make_fixture(&b);
    b.bytes[0] = 'x';
    CHECK(tm_cpio_check_valid(b.bytes, b.len) == 0);
    CHECK(tm_cpio_find_file(b.bytes, b.len, "bin/qsh", NULL) == 0);
}

int main(void)
{
    test_iterate_and_find();
    test_resolve_path();
    test_dirent_and_exists();
    test_malformed_archive_stops();
    puts("tm_cpio_model_test: ok");
    return 0;
}

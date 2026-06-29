/*
 * Host tests for the portable tm_sysfs model.
 *
 * These tests link the existing C implementation directly and exercise the
 * documented C ABI before a Rust provider is wired into task-manager paths.
 */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <tm_sysfs.h>

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

static void expect_content(unsigned idx, const char *expected)
{
    unsigned len = 999;
    const char *data = tm_sysfs_content(idx, &len);
    CHECK(data != NULL);
    CHECK(len == strlen(expected));
    CHECK_STREQ(data, expected);
}

static void test_snapshot_and_content(void)
{
    tm_sysfs_init("QSOE/L", "qemu,virt", NULL, "1.2.3", "");

    expect_content(0, "qemu,virt\n");
    expect_content(1, "\n");
    expect_content(2, "\n");
    expect_content(3, "QSOE/L\n");
    expect_content(4, "1.2.3\n");

    {
        unsigned len = 123;
        CHECK(tm_sysfs_content(99, &len) == NULL);
        CHECK(len == 0);
        CHECK(tm_sysfs_content(99, NULL) == NULL);
    }
}

static void test_truncation(void)
{
    char board[141];
    unsigned len = 0;
    const char *data;

    memset(board, 'a', sizeof(board));
    board[sizeof(board) - 1] = 0;
    tm_sysfs_init("os", board, "cmd", "ver", "date");

    data = tm_sysfs_content(0, &len);
    CHECK(data != NULL);
    CHECK(len == 129);
    for (unsigned i = 0; i < 128; i++)
        CHECK(data[i] == 'a');
    CHECK(data[128] == '\n');
    CHECK(data[129] == 0);
}

static void expect_resolve(const char *path, int expected_kind, unsigned expected_idx)
{
    unsigned idx = 99;
    int kind = tm_sysfs_resolve(path, &idx);
    CHECK(kind == expected_kind);
    if (expected_kind == 2)
        CHECK(idx == expected_idx);
    else
        CHECK(idx == 99);
}

static void test_resolution(void)
{
    expect_resolve("/sys", 1, 0);
    expect_resolve("/sys/", 1, 0);
    expect_resolve("/sys/board", 2, 0);
    expect_resolve("/sys/builddate", 2, 1);
    expect_resolve("/sys/cmdline", 2, 2);
    expect_resolve("/sys/osname", 2, 3);
    expect_resolve("/sys/version", 2, 4);

    CHECK(tm_sysfs_resolve("/sys/version", NULL) == 2);
    CHECK(tm_sysfs_path_exists("/sys") == 1);
    CHECK(tm_sysfs_path_exists("/sys/version") == 1);

    expect_resolve("sys", 0, 0);
    expect_resolve("/sysx", 0, 0);
    expect_resolve("/sys/unknown", 0, 0);
    expect_resolve("/sys/board/", 0, 0);
    CHECK(tm_sysfs_path_exists("/sys/missing") == 0);
}

static void test_entry_order(void)
{
    CHECK(tm_sysfs_nentries() == 5);
    CHECK_STREQ(tm_sysfs_entry_name(0), "board");
    CHECK_STREQ(tm_sysfs_entry_name(1), "builddate");
    CHECK_STREQ(tm_sysfs_entry_name(2), "cmdline");
    CHECK_STREQ(tm_sysfs_entry_name(3), "osname");
    CHECK_STREQ(tm_sysfs_entry_name(4), "version");
    CHECK(tm_sysfs_entry_name(5) == NULL);
}

int main(void)
{
    test_snapshot_and_content();
    test_truncation();
    test_resolution();
    test_entry_order();

    puts("tm_sysfs_model_test: ok");
    return 0;
}

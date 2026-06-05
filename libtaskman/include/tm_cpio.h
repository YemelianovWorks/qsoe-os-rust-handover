/*
 * <libtaskman/cpio.h> -- newc-format CPIO archive walker.
 *
 * Walks an in-memory CPIO archive byte-by-byte; finds files by exact
 * name.  No allocations, no syscalls, no kernel coupling -- the body
 * is pure byte arithmetic on a caller-provided buffer.
 *
 * Used by every QSOE taskman to read the boot initrd after mapping it
 * into its own VSpace (NQ: TM_PRIV_QUERY_INITRD -> TM_PRIV_VSPACE_MAP;
 * LQ: similar shape over seL4 caps).  Adapted from the kernel-side
 * walker that drives the boot-time taskman load -- same shape,
 * stdint types, no skimmer-side dependency.
 *
 * Copyright (c) 2020, 2026 Yuri Zaporozhets <yuriz@qsoe.net>
 * SPDX-License-Identifier: Apache-2.0
 */
#ifndef LIBTASKMAN_CPIO_H
#define LIBTASKMAN_CPIO_H

#include <stdint.h>

#define TM_CPIO_NEWC_MAGIC      "070701"

typedef struct tm_cpio_file_info {
    char           filename[64];   /* truncated if archive name exceeded */
    uint32_t       filesize;
    uint32_t       mode;
    const uint8_t *data;           /* points into the caller's buffer */
} tm_cpio_file_info_t;

/* Quick magic check. */
int tm_cpio_check_valid(const uint8_t *data, uint64_t size);

/* Walk every entry in [data, data + size); invoke `cb(file, user)` for
 * each non-TRAILER record.  Stops on TRAILER!!! or any malformed
 * header. */
typedef void (*tm_cpio_callback_t)(const tm_cpio_file_info_t *file,
                                   void *user);
void tm_cpio_iterate(const uint8_t *data, uint64_t size,
                     tm_cpio_callback_t cb, void *user);

/* Convenience: walk until `filename` matches exactly.  Returns 1 on
 * hit (fills *info) and 0 on miss. */
int tm_cpio_find_file(const uint8_t *data, uint64_t size,
                      const char *filename, tm_cpio_file_info_t *info);

/* Mode-field bits used to spot symlink entries in the cpio archive.
 * Standard POSIX numbering (matches octal 0170000 / 0120000). */
#define TM_CPIO_S_IFMT   0xF000U
#define TM_CPIO_S_IFLNK  0xA000U

/* How many symlink hops tm_cpio_resolve_path will follow before
 * giving up.  Eight is generous for any sane tree and matches what
 * most Unix kernels limit symlink chains to. */
#define TM_CPIO_MAX_SYMLINKS 8

/* Resolve `path` through any chain of symlinks in the archive.  A
 * leading '/' on `path` is stripped automatically.  On success
 * (return 1) `out_path` holds the cpio-form (no leading slash) name
 * of the final concrete entry and `out_info` points at its data.
 * Returns 0 if any element in the chain is absent, or -1 if the
 * symlink budget is exceeded or `out_path` is too small. */
int tm_cpio_resolve_path(const uint8_t *data, uint64_t size,
                         const char *path,
                         char *out_path, unsigned out_cap,
                         tm_cpio_file_info_t *out_info);

/* d_type values yielded by tm_cpio_dirent_at.  Numerically aligned
 * with POSIX DT_* so the caller can copy straight into struct dirent. */
#define TM_CPIO_DT_UNKNOWN  0
#define TM_CPIO_DT_DIR      4
#define TM_CPIO_DT_REG      8
#define TM_CPIO_DT_LNK      10

/* Directory enumeration.  `prefix` is the cpio-relative directory
 * (e.g. "" for the archive root, "bin/" for everything under /bin --
 * trailing '/' required when non-empty).  `index` selects which
 * unique immediate child to yield (0 = first).  An immediate child
 * is the first path component after `prefix`; entries one level deep
 * (e.g. "bin/qsh" relative to prefix="") yield "bin" as a synthetic
 * DIR entry, with later cpio entries that share that first component
 * de-duplicated.
 *
 * Caller provides the name buffer (`out_name_buf`, `out_name_cap`);
 * on hit it's filled NUL-terminated.  On hit (return 1) *out_type
 * holds one of the DT_* values and *out_namelen the basename length
 * (NOT counting the trailing NUL).  Returns 0 past the last unique
 * child, -1 if the caller buffer is too small. */
int tm_cpio_dirent_at(const uint8_t *data, uint64_t size,
                      const char *prefix,
                      uint32_t index,
                      unsigned *out_type,
                      char *out_name_buf, unsigned out_name_cap,
                      unsigned *out_namelen);

/* Hard cap on unique immediate children per directory the walker
 * tracks for dedup.  Comfortably above our flat initrd's ~20-entry
 * shape; bump if a future cpio outgrows it. */
#define TM_CPIO_DIRENT_MAX       16
#define TM_CPIO_DIRENT_NAME_MAX  32

/* Probe: does any cpio entry have `prefix` as a name prefix (and at
 * least one byte after)?  Used to decide whether a path opens as a
 * directory.  `prefix` should include the trailing '/' (or be empty
 * for the archive root).  Returns 1 if at least one such entry
 * exists, 0 otherwise. */
int tm_cpio_dir_exists(const uint8_t *data, uint64_t size,
                       const char *prefix);

#endif /* LIBTASKMAN_CPIO_H */

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
 * Copyright (c) 2020, 2026 Yuri Zaporozhets <r_tty@yahoo.co.uk>
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

#endif /* LIBTASKMAN_CPIO_H */

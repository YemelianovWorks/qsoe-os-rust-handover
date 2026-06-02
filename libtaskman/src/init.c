/*
 * libtaskman/src/init.c -- the one-time seam wire-up.
 *
 * Stores the per-kernel callback table in a file-scope pointer that
 * module code reads through libtaskman_seams().  Init is allowed
 * exactly once per process; a second call is a programming error.
 *
 * Copyright (c) 2026 Yuri Zaporozhets <r_tty@yahoo.co.uk>
 * SPDX-License-Identifier: Apache-2.0
 */

#include <tm_seams.h>

static const struct libtaskman_seams *g_seams;

int libtaskman_init(const struct libtaskman_seams *seams)
{
    if (!seams) return -1;
    if (g_seams) return -1;
    g_seams = seams;
    return 0;
}

const struct libtaskman_seams *libtaskman_seams(void)
{
    return g_seams;
}

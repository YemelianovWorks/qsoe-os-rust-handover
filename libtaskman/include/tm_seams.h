/*
 * <libtaskman/seams.h> -- the per-kernel callback table.
 *
 * libtaskman is the OS-independent body of every QSOE taskman.  Two
 * concrete taskmen consume it: nq/taskman (on the Skimmer microkernel)
 * and lq/taskman (on seL4).  Where the shared body needs to
 * touch a kernel-specific resource -- mint a notification, ask the
 * scheduler for ticks, walk the platform FDT -- it dispatches through
 * a function pointer in this table.  No #ifdefs, no compile-time
 * dispatch, no per-kernel build of libtaskman itself.
 *
 * The taskman main initialises libtaskman exactly once at boot:
 *
 *     static const struct libtaskman_seams seams = {
 *         .clock_now_ticks   = my_clock_now_ticks,
 *         .fdt_get_property  = my_fdt_get_property,
 *         .send_pulse        = my_send_pulse,
 *         ...
 *     };
 *     libtaskman_init(&seams);
 *
 * Subsequent calls into libtaskman read the table through a
 * file-scope pointer in libtaskman/src/init.c.  Seam routines MUST be
 * supplied for every field libtaskman actually uses; a NULL slot is
 * an init-time error (we'd rather crash at boot than at first call).
 *
 * The struct grows over time as more modules surface their seams.
 * Adding a field is source-compatible: existing taskmen leave the
 * new slot zero-init and libtaskman_init() flags the gap loudly.
 *
 * Copyright (c) 2026 Yuri Zaporozhets <r_tty@yahoo.co.uk>
 * SPDX-License-Identifier: Apache-2.0
 */
#ifndef LIBTASKMAN_SEAMS_H
#define LIBTASKMAN_SEAMS_H

#include <qsoe-system.h>     /* pid_t, basic types */

struct libtaskman_seams {
    /* ---- Clock (timer module needs this) ---- */
    /* Read the monotonic tick count.  Stage-A taskmen back this with
     * RISC-V `rdtime`; later builds may run a more elaborate clock. */
    uint64_t (*clock_now_ticks)(void);

    /* ---- FDT (syscfg module needs this) ---- */
    /* Look up a property on a named node and copy its raw value into
     * `out`.  Returns the property byte-length on success, -1 on miss.
     * The taskman backing this typically wraps fdt_get_property() from
     * the platform's flattened device tree blob. */
    int (*fdt_get_property)(const char *node_path,
                            const char *prop_name,
                            void *out, unsigned cap);

    /* ---- Pulse delivery (dispatch module needs this) ---- */
    /* Queue a pulse (code, value) for delivery to `pid`'s system
     * thread.  Returns 0 on success, -errno on failure. */
    int (*send_pulse)(pid_t pid, int code, int value);

    /* Future seams (multi-process spawn, cap minting, IRQ binding,
     * VSpace ops, etc.) get fields appended below as the corresponding
     * modules graduate from per-OS into shared libtaskman code. */
};

/* Initialise libtaskman.  Must be called once before any other
 * libtaskman_* / tm_* entry point.  Returns 0 on success; non-zero if
 * a required seam is NULL, in which case libtaskman is unusable. */
int libtaskman_init(const struct libtaskman_seams *seams);

/* Read-only accessor for module code.  Returns NULL before init. */
const struct libtaskman_seams *libtaskman_seams(void);

#endif /* LIBTASKMAN_SEAMS_H */

/*
 * <tm_limits.h> -- system-wide bookkeeping caps for every QSOE taskman.
 *
 * These bounds size the fixed-allocation tables that every taskman
 * variant carries (process registry, channel table, connection table,
 * thread registry, per-process mmap tracker).  They live here so
 * each per-kernel taskman picks them up without restating them.
 *
 * Sized for "QSOE on a developer's RISC-V board": ~12 system services
 * (taskman, devc-*, devb-*, fs-*, pci-server, slogger, pipe, net,
 * audio, sysmon, usb, ...) + qsh + transient shell-spawned helpers +
 * a working set of user programs.  Static BSS cost across all tables
 * is ~240 KiB total -- noise relative to taskman.elf size at multi-
 * megabyte range.
 *
 * Per-process bounds (TM_MAX_TID_PER_PROC, TM_MAX_MMAP_PER_PROC)
 * scale with what one process can do, not the system-wide count.
 *
 * Copyright (c) 2026 Yuri Zaporozhets <r_tty@yahoo.co.uk>
 * SPDX-License-Identifier: Apache-2.0
 */
#ifndef LIBTASKMAN_LIMITS_H
#define LIBTASKMAN_LIMITS_H

/* System-wide caps. */
#define TM_MAX_PROCESSES         128   /* concurrent processes */
#define TM_MAX_CHANNELS          256   /* registered channels system-wide */
#define TM_MAX_CONNECTIONS      1024   /* open connections system-wide */
#define TM_MAX_THREADS           512   /* live threads system-wide */

/* Per-process caps. */
#define TM_MAX_TID_PER_PROC       32   /* threads in one process */
/* TM_MAX_MMAP_PER_PROC bumped from 16 -> 64 on 2026-06-02: qsh chews
 * through mmaps fast under v0.8 -- every posix_spawn currently grabs a
 * fresh 2 MiB Mega_Page for the args side channel (no pool reuse yet),
 * stdio buffers each take a page, mallocng grabs more, and external
 * commands compound that.  64 buys headroom; a future args-page pool
 * (one slot reused across spawns) will let us walk this back. */
#define TM_MAX_MMAP_PER_PROC      64   /* mmap'd Mega_Page regions tracked */

#endif /* LIBTASKMAN_LIMITS_H */

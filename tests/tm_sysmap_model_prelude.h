/*
 * Host-fixture prelude for lq/taskman/sys/sysmap.c.
 *
 * sysmap.c only needs the tm_syscfg_* ABI and qsoe/syscfg.h constants, but
 * taskman's production syscfg.h also pulls generated seL4 headers.  Predefine
 * that header guard for the host fixture and provide the exact prototypes.
 */
#ifndef QSOE_TASKMAN_SYSCFG_H
#define QSOE_TASKMAN_SYSCFG_H

#include <stdint.h>

#include <qsoe/syscfg.h>

int tm_syscfg_get(const void **out_blob, unsigned *out_len);
int tm_syscfg_find_u64(unsigned tag_id, uint64_t *out);
int tm_syscfg_find_u32(unsigned tag_id, uint32_t *out);
int tm_syscfg_find(unsigned tag_id, const void **out_ptr, unsigned *out_len);

#endif /* QSOE_TASKMAN_SYSCFG_H */

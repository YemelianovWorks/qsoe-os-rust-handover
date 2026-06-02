/*
 * libtaskman/src/cred.c -- per-process cwd / umask / uid-gid operations.
 *
 * Plain-state operations on a tm_cred_state_t the per-OS taskman owns
 * and embeds in its process record.  No kernel calls, no IPC, no
 * libtaskman_seams().  Lifted from the equivalent fragments scattered
 * across LQ taskman's proc/process.c (tm_chdir, tm_getcwd, tm_umask,
 * tm_set_cred, tm_proc_self_info -- the parts that mutate the state
 * struct only, not the per-OS process-table lookups around them).
 *
 * Copyright (c) 2026 Yuri Zaporozhets <r_tty@yahoo.co.uk>
 * SPDX-License-Identifier: Apache-2.0
 */

#include <tm_cred.h>

void tm_cred_init(tm_cred_state_t *s)
{
    if (!s) return;
    s->cwd[0] = '/';
    s->cwd[1] = 0;
    s->umask  = 0022u;
    s->cred.ruid = s->cred.euid = s->cred.suid = 0;
    s->cred.rgid = s->cred.egid = s->cred.sgid = 0;
    s->cred.ngroups = 0;
}

int tm_cred_chdir(tm_cred_state_t *s, const char *path, unsigned path_len)
{
    if (!s || !path) return -EINVAL;
    if (path_len == 0 || path_len >= TM_CWD_MAX) return -ENAMETOOLONG;
    /* Stage-A accepts absolute paths only.  Relative paths need full
     * cwd-relative resolution which lands when a real fs server can
     * verify the destination exists. */
    if (path[0] != '/') return -EINVAL;
    for (unsigned i = 0; i < path_len; ++i) s->cwd[i] = path[i];
    s->cwd[path_len] = 0;
    return 0;
}

int tm_cred_getcwd(const tm_cred_state_t *s,
                   char *dst_buf, unsigned cap, unsigned *out_len)
{
    if (!s || !dst_buf || cap == 0 || !out_len) return -EINVAL;
    unsigned len = 0;
    while (s->cwd[len] != 0 && len < TM_CWD_MAX) ++len;
    if (len > cap) return -ERANGE;
    for (unsigned i = 0; i < len; ++i) dst_buf[i] = s->cwd[i];
    *out_len = len;
    return 0;
}

int tm_cred_umask(tm_cred_state_t *s, int set, unsigned *out_old)
{
    if (!s || !out_old) return -EINVAL;
    *out_old = s->umask;
    if (set >= 0) s->umask = (unsigned)set & 0777u;
    return 0;
}

int tm_cred_set(tm_cred_state_t *s,
                unsigned ruid_new, unsigned euid_new, unsigned suid_new,
                unsigned rgid_new, unsigned egid_new, unsigned sgid_new)
{
    if (!s) return -EINVAL;
    /* 0xFFFFFFFF = "leave alone".  Stage-A is single-user-root so
     * permission checks are deferred to the caller; the per-OS
     * taskman gates this with whatever policy it has. */
    if (ruid_new != 0xFFFFFFFFu) s->cred.ruid = (uid_t)ruid_new;
    if (euid_new != 0xFFFFFFFFu) s->cred.euid = (uid_t)euid_new;
    if (suid_new != 0xFFFFFFFFu) s->cred.suid = (uid_t)suid_new;
    if (rgid_new != 0xFFFFFFFFu) s->cred.rgid = (gid_t)rgid_new;
    if (egid_new != 0xFFFFFFFFu) s->cred.egid = (gid_t)egid_new;
    if (sgid_new != 0xFFFFFFFFu) s->cred.sgid = (gid_t)sgid_new;
    return 0;
}

void tm_cred_self_info(const tm_cred_state_t *s,
                       struct _cred_info *out_cred)
{
    if (!s || !out_cred) return;
    *out_cred = s->cred;
}

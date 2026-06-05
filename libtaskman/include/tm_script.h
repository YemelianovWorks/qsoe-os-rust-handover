/*
 * <libtaskman/tm_script.h> -- POSIX shebang line parsing.
 *
 * Pure byte arithmetic; no allocations, no syscalls.  Used by each
 * QSOE taskman's spawn path to decide whether a cpio entry starts
 * with a #!/path/to/interp line and, if so, dispatch the load through
 * that interpreter instead of treating the bytes as ELF.
 *
 * Copyright (c) 2026 Yuri Zaporozhets <yuriz@qsoe.net>
 * SPDX-License-Identifier: Apache-2.0
 */
#ifndef LIBTASKMAN_SCRIPT_H
#define LIBTASKMAN_SCRIPT_H

#include <stdint.h>

/* Parse the POSIX shebang line at the start of `data`.  On success:
 *   - return 0
 *   - `interp` is filled with the interpreter path (NUL-terminated)
 *   - `arg`    is filled with the optional single argument (empty if
 *              the shebang line carried only the interpreter)
 * POSIX reserves the rest of the first line, after the interpreter
 * and any whitespace, as ONE arg -- no further word splitting.
 *
 * Returns -1 if `data` doesn't start with "#!" or any malformed-line
 * condition is detected (empty interp, oversized buffers, etc.). */
int tm_script_parse_shebang(const uint8_t *data, unsigned size,
                            char *interp, unsigned interp_cap,
                            char *arg, unsigned arg_cap);

#endif /* LIBTASKMAN_SCRIPT_H */

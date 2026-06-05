/*
 * libtaskman/src/script.c -- POSIX shebang parser.
 *
 * Single function: tm_script_parse_shebang.  Pure byte arithmetic,
 * no allocations.  See <tm_script.h> for the contract.
 *
 * Copyright (c) 2026 Yuri Zaporozhets <yuriz@qsoe.net>
 * SPDX-License-Identifier: Apache-2.0
 */

#include <tm_script.h>

int tm_script_parse_shebang(const uint8_t *data, unsigned size,
                            char *interp, unsigned interp_cap,
                            char *arg, unsigned arg_cap)
{
    if (interp_cap == 0 || arg_cap == 0) return -1;
    interp[0] = '\0';
    arg[0]    = '\0';
    if (size < 2 || data[0] != '#' || data[1] != '!') return -1;

    unsigned i = 2;
    while (i < size && (data[i] == ' ' || data[i] == '\t')) i++;

    /* Interpreter path -- bytes until whitespace or newline. */
    unsigned j = 0;
    while (i < size && data[i] != ' ' && data[i] != '\t' &&
           data[i] != '\n' && data[i] != '\r' && j + 1 < interp_cap) {
        interp[j++] = (char) data[i++];
    }
    interp[j] = '\0';
    if (j == 0) return -1;

    while (i < size && (data[i] == ' ' || data[i] == '\t')) i++;

    /* Optional single arg -- the entire rest of the line is one arg
     * per POSIX (no further word splitting). */
    j = 0;
    while (i < size && data[i] != '\n' && data[i] != '\r' &&
           j + 1 < arg_cap) {
        arg[j++] = (char) data[i++];
    }
    while (j > 0 && (arg[j - 1] == ' ' || arg[j - 1] == '\t')) j--;
    arg[j] = '\0';
    return 0;
}

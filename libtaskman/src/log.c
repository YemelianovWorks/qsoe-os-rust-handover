/*
 * libtaskman/src/log.c -- leveled, printf-style logging for taskman.
 *
 * Format + filter are OS-independent; the per-kernel taskman supplies
 * only the sink callback (tm_log_init).  See <tm_log.h> for the
 * contract and the --debug[=N] verbosity mapping.
 *
 * The formatter is a deliberate printf SUBSET (no float, no %n, no
 * positional args): taskman is statically linked without stdio, and
 * every consumer of a richer format spec so far turned out to be a
 * log line that didn't need it.
 *
 * Copyright (c) 2026 Yuri Zaporozhets <yuriz@qsoe.net>
 * SPDX-License-Identifier: Apache-2.0
 */

#include <stdarg.h>
#include <tm_log.h>

static tm_log_sink_t tm_sink;                       /* NULL until tm_log_init */
static int tm_level = TM_LOG_LEVEL_DEFAULT;

void tm_log_init(tm_log_sink_t sink)
{
    tm_sink = sink;
}

void tm_log_set_level(int level)
{
    if (level < TM_LOG_ERR)
        level = TM_LOG_ERR;
    if (level > TM_LOG_TRACE)
        level = TM_LOG_TRACE;
    tm_level = level;
}

int tm_log_get_level(void)
{
    return tm_level;
}

int tm_log_enabled(int level)
{
    return level <= tm_level && tm_sink != 0;
}

void tm_log_emit(int level, const char *buf, unsigned len)
{
    if (!tm_log_enabled(level) || len == 0)
        return;

    tm_sink(buf, len);
}

/* ---- --debug[=N] command-line scan ---------------------------------- */

/* The option token.  Matched per-token (delimited by blanks), so a
 * hypothetical "--debugfs" does not false-positive. */
#define TM_DEBUG_OPT        "--debug"
#define TM_DEBUG_OPT_LEN    (sizeof TM_DEBUG_OPT - 1)

static int is_blank(char c)
{
    return c == ' ' || c == '\t';
}

int tm_log_apply_cmdline(const char *cmdline)
{
    const char *p = cmdline;

    if (p == 0)
        return tm_level;

    while (*p != '\0') {
        while (is_blank(*p))
            p++;
        if (*p == '\0')
            break;

        /* [p .. token end) is one token. */
        unsigned i = 0;
        while (i < TM_DEBUG_OPT_LEN && p[i] == TM_DEBUG_OPT[i])
            i++;
        if (i == TM_DEBUG_OPT_LEN) {
            if (p[i] == '\0' || is_blank(p[i])) {
                /* bare --debug == --debug=1 */
                tm_log_set_level(TM_LOG_DBG);
            } else if (p[i] == '=') {
                unsigned long n = 0;
                const char *d = p + i + 1;
                int any = 0;
                while (*d >= '0' && *d <= '9') {
                    n = n * 10 + (unsigned long) (*d - '0');
                    d++;
                    any = 1;
                }
                if (any && (*d == '\0' || is_blank(*d))) {
                    /* --debug=0 keeps the default; 1 adds DBG; 2 and
                     * anything higher clamps to TRACE. */
                    if (n >= 1)
                        tm_log_set_level(n >= 2 ? TM_LOG_TRACE : TM_LOG_DBG);
                }
                /* malformed value ("--debug=x"): ignore the token */
            }
        }

        while (*p != '\0' && !is_blank(*p))
            p++;
    }
    return tm_level;
}

/* ---- printf-subset formatter ----------------------------------------- */

/* Emission cursor over the fixed line buffer.  Excess output is
 * silently truncated; `pos` keeps counting so the final flush just
 * clamps once. */
struct tm_out {
    char     *buf;
    unsigned  cap;
    unsigned  pos;
};

static void out_ch(struct tm_out *o, char c)
{
    if (o->pos < o->cap)
        o->buf[o->pos] = c;
    o->pos++;
}

static void out_str(struct tm_out *o, const char *s)
{
    while (*s != '\0')
        out_ch(o, *s++);
}

/* Digits for %x.  Lowercase only -- the QSOE log idiom ("entry=0x15166"). */
static const char tm_hex_digits[] = "0123456789abcdef";

/* Worst case: 64-bit value in decimal = 20 digits (+ sign). */
#define TM_NUM_BUF      24

static void out_num(struct tm_out *o, unsigned long long v, unsigned base,
                    int negative, unsigned width, char pad)
{
    char tmp[TM_NUM_BUF];
    unsigned n = 0;

    do {
        tmp[n++] = tm_hex_digits[v % base];
        v /= base;
    } while (v != 0 && n < sizeof tmp);

    if (negative)
        tmp[n++] = '-';

    while (n < width && n < sizeof tmp)
        tmp[n++] = pad;

    while (n > 0)
        out_ch(o, tmp[--n]);
}

/* Length-modifier states for the %-spec parser below. */
enum tm_len_mod { LEN_INT, LEN_LONG, LEN_LLONG, LEN_SIZE };

static const char *parse_spec(const char *fmt, enum tm_len_mod *len,
                              char *conv, unsigned *width, char *pad)
{
    *pad = ' ';
    *width = 0;
    *len = LEN_INT;

    if (*fmt == '0') {
        *pad = '0';
        fmt++;
    }
    while (*fmt >= '0' && *fmt <= '9')
        *width = *width * 10 + (unsigned) (*fmt++ - '0');

    if (*fmt == 'l') {
        *len = LEN_LONG;
        fmt++;
        if (*fmt == 'l') {
            *len = LEN_LLONG;
            fmt++;
        }
    } else if (*fmt == 'z') {
        *len = LEN_SIZE;
        fmt++;
    }

    *conv = *fmt;
    if (*conv == '\0')
        return fmt;
    return fmt + 1;
}

static void set_arg_str(tm_log_arg_t *arg, const char *s)
{
    arg->kind = TM_LOG_ARG_STR;
    arg->signed_value = 0;
    arg->unsigned_value = 0;
    arg->str_value = s;
    arg->ptr_value = 0;
}

static void set_arg_char(tm_log_arg_t *arg, int c)
{
    arg->kind = TM_LOG_ARG_CHAR;
    arg->signed_value = c;
    arg->unsigned_value = 0;
    arg->str_value = 0;
    arg->ptr_value = 0;
}

static void set_arg_signed(tm_log_arg_t *arg, long long v)
{
    arg->kind = TM_LOG_ARG_SIGNED;
    arg->signed_value = v;
    arg->unsigned_value = 0;
    arg->str_value = 0;
    arg->ptr_value = 0;
}

static void set_arg_unsigned(tm_log_arg_t *arg, unsigned long long v)
{
    arg->kind = TM_LOG_ARG_UNSIGNED;
    arg->signed_value = 0;
    arg->unsigned_value = v;
    arg->str_value = 0;
    arg->ptr_value = 0;
}

static void set_arg_ptr(tm_log_arg_t *arg, const void *p)
{
    arg->kind = TM_LOG_ARG_PTR;
    arg->signed_value = 0;
    arg->unsigned_value = 0;
    arg->str_value = 0;
    arg->ptr_value = p;
}

static tm_log_arg_t *next_store(tm_log_arg_t *args, unsigned cap,
                                unsigned *n, int *overflow)
{
    if (*n < cap)
        return &args[(*n)++];
    *overflow = 1;
    return 0;
}

static unsigned tm_marshal_args(const char *fmt, tm_log_va_list ap,
                                tm_log_arg_t *args, unsigned cap,
                                int *overflow)
{
    unsigned n = 0;
    *overflow = 0;

    while (*fmt != '\0') {
        char c = *fmt++;
        if (c != '%')
            continue;

        char pad;
        unsigned width;
        enum tm_len_mod len;
        char conv;
        fmt = parse_spec(fmt, &len, &conv, &width, &pad);
        (void) width;
        (void) pad;
        if (conv == '\0')
            break;

        tm_log_arg_t *arg;
        switch (conv) {
        case 's': {
            const char *s = va_arg(ap, const char *);
            arg = next_store(args, cap, &n, overflow);
            if (arg != 0)
                set_arg_str(arg, s);
            break;
        }
        case 'c': {
            int ch = va_arg(ap, int);
            arg = next_store(args, cap, &n, overflow);
            if (arg != 0)
                set_arg_char(arg, ch);
            break;
        }
        case 'd':
        case 'i': {
            long long v;
            if (len == LEN_LLONG)
                v = va_arg(ap, long long);
            else if (len == LEN_LONG)
                v = va_arg(ap, long);
            else if (len == LEN_SIZE)
                v = (long long) va_arg(ap, unsigned long);
            else
                v = va_arg(ap, int);
            arg = next_store(args, cap, &n, overflow);
            if (arg != 0)
                set_arg_signed(arg, v);
            break;
        }
        case 'u':
        case 'x': {
            unsigned long long v;
            if (len == LEN_LLONG)
                v = va_arg(ap, unsigned long long);
            else if (len == LEN_LONG || len == LEN_SIZE)
                v = va_arg(ap, unsigned long);
            else
                v = va_arg(ap, unsigned int);
            arg = next_store(args, cap, &n, overflow);
            if (arg != 0)
                set_arg_unsigned(arg, v);
            break;
        }
        case 'p': {
            const void *p = va_arg(ap, void *);
            arg = next_store(args, cap, &n, overflow);
            if (arg != 0)
                set_arg_ptr(arg, p);
            break;
        }
        case '%':
        default:
            break;
        }
    }

    return n;
}

static const tm_log_arg_t *take_arg(const tm_log_arg_t *args,
                                    unsigned nargs, unsigned *idx)
{
    if (*idx >= nargs)
        return 0;
    return &args[(*idx)++];
}

static void out_bad_arg(struct tm_out *o)
{
    out_str(o, "(badarg)");
}

static void tm_fmt_args(struct tm_out *o, const char *fmt,
                        const tm_log_arg_t *args, unsigned nargs)
{
    unsigned idx = 0;

    while (*fmt != '\0') {
        char c = *fmt++;
        if (c != '%') {
            out_ch(o, c);
            continue;
        }

        char pad;
        unsigned width;
        enum tm_len_mod len;
        char conv;
        fmt = parse_spec(fmt, &len, &conv, &width, &pad);
        (void) len;
        if (conv == '\0')
            break;                  /* dangling '%' at end of format */

        switch (conv) {
        case 's': {
            const tm_log_arg_t *arg = take_arg(args, nargs, &idx);
            if (arg == 0 || arg->kind != TM_LOG_ARG_STR) {
                out_bad_arg(o);
                break;
            }
            out_str(o, arg->str_value != 0 ? arg->str_value : "(null)");
            break;
        }
        case 'c': {
            const tm_log_arg_t *arg = take_arg(args, nargs, &idx);
            if (arg == 0 || arg->kind != TM_LOG_ARG_CHAR) {
                out_bad_arg(o);
                break;
            }
            out_ch(o, (char) arg->signed_value);
            break;
        }
        case 'd':
        case 'i': {
            const tm_log_arg_t *arg = take_arg(args, nargs, &idx);
            if (arg == 0 || arg->kind != TM_LOG_ARG_SIGNED) {
                out_bad_arg(o);
                break;
            }
            long long v = arg->signed_value;
            /* Negate in unsigned space: -LLONG_MIN is UB in signed. */
            if (v < 0)
                out_num(o, 0ULL - (unsigned long long) v, 10, 1, width, pad);
            else
                out_num(o, (unsigned long long) v, 10, 0, width, pad);
            break;
        }
        case 'u':
        case 'x': {
            const tm_log_arg_t *arg = take_arg(args, nargs, &idx);
            if (arg == 0 || arg->kind != TM_LOG_ARG_UNSIGNED) {
                out_bad_arg(o);
                break;
            }
            unsigned long long v = arg->unsigned_value;
            out_num(o, v, conv == 'x' ? 16 : 10, 0, width, pad);
            break;
        }
        case 'p': {
            const tm_log_arg_t *arg = take_arg(args, nargs, &idx);
            if (arg == 0 || arg->kind != TM_LOG_ARG_PTR) {
                out_bad_arg(o);
                break;
            }
            out_str(o, "0x");
            out_num(o, (unsigned long long) (unsigned long) arg->ptr_value,
                    16, 0, 0, ' ');
            break;
        }
        case '%':
            out_ch(o, '%');
            break;
        default:
            /* Unknown conversion: echo it visibly rather than
             * desynchronize the va_list any further. */
            out_ch(o, '%');
            out_ch(o, conv);
            break;
        }
    }
}

void tm_log_emit_args(int level, const char *fmt,
                      const tm_log_arg_t *args, unsigned nargs)
{
    if (!tm_log_enabled(level))
        return;

    char line[TM_LOG_LINE_MAX];
    struct tm_out o = { line, sizeof line, 0 };

    tm_fmt_args(&o, fmt, args, nargs);

    unsigned n = o.pos < o.cap ? o.pos : o.cap;
    tm_log_emit(level, line, n);
}

void tm_vlog(int level, const char *fmt, tm_log_va_list ap)
{
    if (!tm_log_enabled(level))
        return;

    tm_log_arg_t args[TM_LOG_ARGS_MAX];
    int overflow;
    unsigned nargs = tm_marshal_args(fmt, ap, args, TM_LOG_ARGS_MAX, &overflow);
    if (overflow) {
        static const char msg[] = "tm_log: too many format arguments";
        tm_log_emit(level, msg, sizeof msg - 1);
        return;
    }

    tm_log_emit_args(level, fmt, args, nargs);
}

void tm_log(int level, const char *fmt, ...)
{
    if (!tm_log_enabled(level))
        return;

    tm_log_va_list ap;
    va_start(ap, fmt);
    tm_vlog(level, fmt, ap);
    va_end(ap);
}

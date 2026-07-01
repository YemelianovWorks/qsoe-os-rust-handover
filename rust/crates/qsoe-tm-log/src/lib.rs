#![cfg_attr(not(any(test, feature = "host-tests")), no_std)]

use core::ffi::{c_char, c_int, c_uint};
use core::slice;

const TM_LOG_LINE_MAX: usize = 256;
const TM_LOG_ARG_STR: c_int = 1;
const TM_LOG_ARG_CHAR: c_int = 2;
const TM_LOG_ARG_SIGNED: c_int = 3;
const TM_LOG_ARG_UNSIGNED: c_int = 4;
const TM_LOG_ARG_PTR: c_int = 5;
const TM_NUM_BUF: usize = 24;

#[repr(C)]
#[derive(Clone, Copy)]
pub struct TmLogArg {
    pub kind: c_int,
    pub signed_value: i64,
    pub unsigned_value: u64,
    pub str_value: *const c_char,
    pub ptr_value: *const core::ffi::c_void,
}

#[cfg(not(any(test, feature = "host-tests")))]
extern "C" {
    fn tm_log_enabled(level: c_int) -> c_int;
    fn tm_log_emit(level: c_int, buf: *const c_char, len: c_uint);
}

#[cfg(any(test, feature = "host-tests"))]
unsafe fn tm_log_enabled(_level: c_int) -> c_int {
    1
}

#[cfg(any(test, feature = "host-tests"))]
unsafe fn tm_log_emit(_level: c_int, _buf: *const c_char, _len: c_uint) {}

#[no_mangle]
pub extern "C" fn qsoe_tm_log_provider_anchor() {}

/// Format typed taskman log arguments and emit the finished line.
///
/// # Safety
///
/// `fmt` must point to a valid NUL-terminated C string. When `nargs` is
/// non-zero, `args` must point to at least `nargs` valid `TmLogArg` entries.
/// Any `TM_LOG_ARG_STR` entry must either be null or point to a valid
/// NUL-terminated C string for the duration of the call.
#[no_mangle]
pub unsafe extern "C" fn tm_log_emit_args(
    level: c_int,
    fmt: *const c_char,
    args: *const TmLogArg,
    nargs: c_uint,
) {
    if tm_log_enabled(level) == 0 || fmt.is_null() {
        return;
    }

    let fmt_len = cstr_len(fmt);
    let fmt = slice::from_raw_parts(fmt.cast::<u8>(), fmt_len);
    let args = if args.is_null() {
        &[]
    } else {
        slice::from_raw_parts(args, nargs as usize)
    };

    let mut line = [0u8; TM_LOG_LINE_MAX];
    let n = format_args_into(fmt, args, &mut line);
    tm_log_emit(level, line.as_ptr().cast::<c_char>(), n as c_uint);
}

unsafe fn cstr_len(s: *const c_char) -> usize {
    let mut n = 0usize;
    while *s.cast::<u8>().add(n) != 0 {
        n += 1;
    }
    n
}

struct Out<'a> {
    buf: &'a mut [u8],
    pos: usize,
}

impl<'a> Out<'a> {
    fn ch(&mut self, c: u8) {
        if self.pos < self.buf.len() {
            self.buf[self.pos] = c;
        }
        self.pos += 1;
    }

    fn bytes(&mut self, s: &[u8]) {
        for &c in s {
            self.ch(c);
        }
    }

    unsafe fn cstr(&mut self, s: *const c_char) {
        if s.is_null() {
            self.bytes(b"(null)");
            return;
        }

        let mut i = 0usize;
        loop {
            let c = *s.cast::<u8>().add(i);
            if c == 0 {
                break;
            }
            self.ch(c);
            i += 1;
        }
    }

    fn len(&self) -> usize {
        if self.pos < self.buf.len() {
            self.pos
        } else {
            self.buf.len()
        }
    }
}

#[derive(Clone, Copy)]
struct Spec {
    conv: u8,
    width: usize,
    pad: u8,
}

fn parse_spec(fmt: &[u8], i: &mut usize) -> Spec {
    let mut pad = b' ';
    let mut width = 0usize;

    if *i < fmt.len() && fmt[*i] == b'0' {
        pad = b'0';
        *i += 1;
    }

    while *i < fmt.len() && fmt[*i].is_ascii_digit() {
        width = width * 10 + usize::from(fmt[*i] - b'0');
        *i += 1;
    }

    if *i < fmt.len() && fmt[*i] == b'l' {
        *i += 1;
        if *i < fmt.len() && fmt[*i] == b'l' {
            *i += 1;
        }
    } else if *i < fmt.len() && fmt[*i] == b'z' {
        *i += 1;
    }

    if *i >= fmt.len() {
        return Spec {
            conv: 0,
            width,
            pad,
        };
    }

    let conv = fmt[*i];
    *i += 1;
    Spec { conv, width, pad }
}

fn take_arg<'a>(args: &'a [TmLogArg], idx: &mut usize) -> Option<&'a TmLogArg> {
    if *idx >= args.len() {
        None
    } else {
        let arg = &args[*idx];
        *idx += 1;
        Some(arg)
    }
}

fn out_bad_arg(o: &mut Out<'_>) {
    o.bytes(b"(badarg)");
}

fn out_num(o: &mut Out<'_>, mut v: u64, base: u64, negative: bool, width: usize, pad: u8) {
    let mut tmp = [0u8; TM_NUM_BUF];
    let mut n = 0usize;

    loop {
        tmp[n] = b"0123456789abcdef"[(v % base) as usize];
        n += 1;
        v /= base;
        if v == 0 || n >= tmp.len() {
            break;
        }
    }

    if negative && n < tmp.len() {
        tmp[n] = b'-';
        n += 1;
    }

    while n < width && n < tmp.len() {
        tmp[n] = pad;
        n += 1;
    }

    while n > 0 {
        n -= 1;
        o.ch(tmp[n]);
    }
}

fn format_args_into(fmt: &[u8], args: &[TmLogArg], out: &mut [u8]) -> usize {
    let mut o = Out { buf: out, pos: 0 };
    let mut i = 0usize;
    let mut idx = 0usize;

    while i < fmt.len() {
        let c = fmt[i];
        i += 1;
        if c != b'%' {
            o.ch(c);
            continue;
        }

        let spec = parse_spec(fmt, &mut i);
        if spec.conv == 0 {
            break;
        }

        match spec.conv {
            b's' => match take_arg(args, &mut idx) {
                Some(arg) if arg.kind == TM_LOG_ARG_STR => unsafe { o.cstr(arg.str_value) },
                _ => out_bad_arg(&mut o),
            },
            b'c' => match take_arg(args, &mut idx) {
                Some(arg) if arg.kind == TM_LOG_ARG_CHAR => o.ch(arg.signed_value as u8),
                _ => out_bad_arg(&mut o),
            },
            b'd' | b'i' => match take_arg(args, &mut idx) {
                Some(arg) if arg.kind == TM_LOG_ARG_SIGNED => {
                    let v = arg.signed_value;
                    if v < 0 {
                        out_num(
                            &mut o,
                            0u64.wrapping_sub(v as u64),
                            10,
                            true,
                            spec.width,
                            spec.pad,
                        );
                    } else {
                        out_num(&mut o, v as u64, 10, false, spec.width, spec.pad);
                    }
                }
                _ => out_bad_arg(&mut o),
            },
            b'u' | b'x' => match take_arg(args, &mut idx) {
                Some(arg) if arg.kind == TM_LOG_ARG_UNSIGNED => out_num(
                    &mut o,
                    arg.unsigned_value,
                    if spec.conv == b'x' { 16 } else { 10 },
                    false,
                    spec.width,
                    spec.pad,
                ),
                _ => out_bad_arg(&mut o),
            },
            b'p' => match take_arg(args, &mut idx) {
                Some(arg) if arg.kind == TM_LOG_ARG_PTR => {
                    o.bytes(b"0x");
                    out_num(&mut o, arg.ptr_value as usize as u64, 16, false, 0, b' ');
                }
                _ => out_bad_arg(&mut o),
            },
            b'%' => o.ch(b'%'),
            conv => {
                o.ch(b'%');
                o.ch(conv);
            }
        }
    }

    o.len()
}

#[cfg(test)]
mod tests {
    use super::*;
    use std::ffi::CString;

    fn arg_str(s: *const c_char) -> TmLogArg {
        TmLogArg {
            kind: TM_LOG_ARG_STR,
            signed_value: 0,
            unsigned_value: 0,
            str_value: s,
            ptr_value: core::ptr::null(),
        }
    }

    fn arg_signed(v: i64) -> TmLogArg {
        TmLogArg {
            kind: TM_LOG_ARG_SIGNED,
            signed_value: v,
            unsigned_value: 0,
            str_value: core::ptr::null(),
            ptr_value: core::ptr::null(),
        }
    }

    fn arg_unsigned(v: u64) -> TmLogArg {
        TmLogArg {
            kind: TM_LOG_ARG_UNSIGNED,
            signed_value: 0,
            unsigned_value: v,
            str_value: core::ptr::null(),
            ptr_value: core::ptr::null(),
        }
    }

    fn arg_ptr(v: usize) -> TmLogArg {
        TmLogArg {
            kind: TM_LOG_ARG_PTR,
            signed_value: 0,
            unsigned_value: 0,
            str_value: core::ptr::null(),
            ptr_value: v as *const core::ffi::c_void,
        }
    }

    fn render(fmt: &str, args: &[TmLogArg]) -> String {
        let fmt = CString::new(fmt).unwrap();
        let mut out = [0u8; TM_LOG_LINE_MAX];
        let n = format_args_into(fmt.as_bytes(), args, &mut out);
        String::from_utf8(out[..n].to_vec()).unwrap()
    }

    #[test]
    fn formats_supported_subset() {
        let path = CString::new("/dev/ser0").unwrap();
        let out = render(
            "entry=%08lx path=%s p=%p %% %q end%",
            &[
                arg_unsigned(0x15166),
                arg_str(path.as_ptr()),
                arg_ptr(0x1234),
            ],
        );
        assert_eq!(out, "entry=00015166 path=/dev/ser0 p=0x1234 % %q end");
    }

    #[test]
    fn preserves_c_fallback_edge_cases() {
        let out = render(
            "%s %05d %d %x",
            &[arg_str(core::ptr::null()), arg_signed(-42), arg_unsigned(7)],
        );
        assert_eq!(out, "(null) 00-42 (badarg) (badarg)");
    }

    #[test]
    fn truncates_to_line_limit() {
        let fmt = "x".repeat(TM_LOG_LINE_MAX + 16);
        let out = render(&fmt, &[]);
        assert_eq!(out.len(), TM_LOG_LINE_MAX);
        assert!(out.bytes().all(|b| b == b'x'));
    }
}

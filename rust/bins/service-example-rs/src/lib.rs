#![no_std]

use core::ffi::{c_char, CStr};
use core::panic::PanicInfo;

use qsoe_ressrv::{
    DirectRequestHandler, DirectServer, IoReply, IoRequest, ReceivedMessage, ReplyStatus, ENOSYS,
    IO_CONNECT, IO_DUP, TM_IO_MAX, TM_REQ_CLOSE, TM_REQ_IO_READ, TM_REQ_IO_WRITE,
};

const EXAMPLE_PATH: &[u8] = b"/dev/rust-example\0";
const EXAMPLE_REPLY: &[u8] = b"qsoe-rs\n";

struct ExampleService {
    writes: u64,
}

impl DirectRequestHandler for ExampleService {
    fn handle_message(&mut self, message: ReceivedMessage, req: &IoRequest) {
        match req.opcode() {
            IO_CONNECT | IO_DUP | TM_REQ_CLOSE => {
                let _ = message.reply_empty(ReplyStatus::OK);
            }
            TM_REQ_IO_WRITE => {
                let written = req.payload_prefix().len() as u64;
                self.writes = self.writes.wrapping_add(written);
                let _ = message.reply_word(ReplyStatus::OK, written);
            }
            TM_REQ_IO_READ => reply_to_read(message, req),
            _ => {
                let _ = message.reply_empty(ReplyStatus::from_errno(ENOSYS));
            }
        }
    }
}

#[panic_handler]
fn panic(_info: &PanicInfo<'_>) -> ! {
    debug_write(b"[service-example-rs] panic\n");
    loop {
        core::hint::spin_loop();
    }
}

#[no_mangle]
pub extern "C" fn qsoe_service_example_rust_marker() -> u64 {
    0x5153_4f45_4558_414d
}

#[no_mangle]
pub extern "C" fn main(_argc: isize, _argv: *const *const u8, _envp: *const *const u8) -> i32 {
    debug_write(b"[service-example-rs] alive\n");

    let mut server = match DirectServer::register(example_path(), ExampleService { writes: 0 }) {
        Ok(server) => server,
        Err(_) => {
            debug_write(b"[service-example-rs] register failed\n");
            return 1;
        }
    };

    debug_write(b"[service-example-rs] /dev/rust-example registered\n");
    if server.detach_ready(0).is_err() {
        debug_write(b"[service-example-rs] procmgr_detach failed\n");
    }

    debug_write(b"[service-example-rs] entering MsgReceive loop\n");
    server.run()
}

fn example_path() -> &'static CStr {
    // SAFETY: `EXAMPLE_PATH` is a static byte string with exactly one trailing
    // NUL and no interior NUL bytes.
    unsafe { CStr::from_bytes_with_nul_unchecked(EXAMPLE_PATH) }
}

fn reply_to_read(message: ReceivedMessage, req: &IoRequest) {
    let want = if req.requested_count() == 0 || req.requested_count() > TM_IO_MAX as u64 {
        TM_IO_MAX
    } else {
        req.requested_count() as usize
    };
    let n = core::cmp::min(want, EXAMPLE_REPLY.len());
    let mut reply = IoReply::zeroed();
    reply.data_mut()[..n].copy_from_slice(&EXAMPLE_REPLY[..n]);
    reply.count = n as u64;

    match reply.bytes_with_payload(n) {
        Ok(bytes) => {
            let _ = message.reply_bytes(ReplyStatus::OK, bytes);
        }
        Err(_) => {
            let _ = message.reply_empty(ReplyStatus::from_errno(ENOSYS));
        }
    }
}

fn debug_write(bytes: &[u8]) {
    // SAFETY: `bytes` points to readable memory for this synchronous debug
    // write call; QSOE does not retain the pointer.
    unsafe { qsoe_ffi::dbg_write(bytes.as_ptr().cast::<c_char>(), bytes.len()) };
}

#![no_std]

use core::ffi::c_void;

pub type PidT = i32;
pub type ModeT = u32;
pub type UidT = u32;
pub type GidT = u32;
pub type OffT = i64;
pub type SizeT = usize;
pub type SsizeT = isize;
pub type QsoeTimeT = u64;

pub const TASKMAN_PID: i32 = 1;
pub const QSOE_SIDE_CHANNEL: u32 = 0x4000_0000;
pub const TASKMAN_COID: i32 = QSOE_SIDE_CHANNEL as i32;
pub const TASKMAN_CHID: u32 = (1 << 16) | 1;
pub const QSOE_MSG_MAX_LENGTH: usize = 120;
pub const QSOE_MSG_MAX_EXTRA_CAPS: usize = 3;
pub const ND_LOCAL_NODE: u32 = 0;
pub const QSOE_MI_PULSE: u32 = 0x0000_0010;
pub const QSOE_RCVID_SAVED: u32 = 0x8000_0000;
pub const PULSE_TYPE: u16 = 0;

pub const QSOE_RUST_SPIKE_MARKER: u64 = 0x5153_4f45_5255_5354;

#[repr(C)]
#[derive(Clone, Copy, Debug, Eq, PartialEq)]
pub struct QsoeMsgInfo {
    pub nd: u32,
    pub pid: PidT,
    pub chid: i32,
    pub scoid: i32,
    pub coid: i32,
    pub msglen: i32,
    pub srcmsglen: i32,
    pub dstmsglen: i32,
    pub priority: i32,
    pub flags: i32,
    pub label: u32,
}

#[repr(C)]
#[derive(Clone, Copy, Debug, Eq, PartialEq)]
pub struct QsoeCredInfo {
    pub ruid: UidT,
    pub euid: UidT,
    pub suid: UidT,
    pub rgid: GidT,
    pub egid: GidT,
    pub sgid: GidT,
    pub ngroups: u32,
}

#[repr(C)]
#[derive(Clone, Copy, Debug, Eq, PartialEq)]
pub struct QsoeClientInfo {
    pub nd: u32,
    pub pid: PidT,
    pub sid: PidT,
    pub flags: u32,
    pub cred: QsoeCredInfo,
}

#[repr(C)]
#[derive(Clone, Copy)]
pub union QsoePulseValue {
    pub sival_int: i32,
    pub sival_ptr: *mut c_void,
}

#[repr(C)]
#[derive(Clone, Copy)]
pub struct QsoePulse {
    pub type_: u16,
    pub subtype: u16,
    pub code: i8,
    pub reserved: [u8; 3],
    pub value: QsoePulseValue,
    pub scoid: i32,
}

#[cfg(test)]
extern crate std;

#[cfg(test)]
mod tests {
    use super::*;
    use core::mem::{align_of, size_of};

    #[test]
    fn qsoe_message_layouts_match_rv64_c_abi() {
        assert_eq!(size_of::<QsoeMsgInfo>(), 44);
        assert_eq!(align_of::<QsoeMsgInfo>(), 4);
        assert_eq!(size_of::<QsoeCredInfo>(), 28);
        assert_eq!(align_of::<QsoeCredInfo>(), 4);
        assert_eq!(size_of::<QsoeClientInfo>(), 44);
        assert_eq!(align_of::<QsoeClientInfo>(), 4);
        assert_eq!(size_of::<QsoePulse>(), 24);
        assert_eq!(align_of::<QsoePulse>(), 8);
    }
}

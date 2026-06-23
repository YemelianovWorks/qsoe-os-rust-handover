#![no_std]

use core::ffi::{c_char, c_int, c_uint, c_void};

pub use qsoe_abi::{GidT, ModeT, OffT, QsoeTimeT, SizeT, SsizeT, UidT};

pub const QSOE_ATTR_MODE: c_uint = 0x01;
pub const QSOE_ATTR_UID: c_uint = 0x02;
pub const QSOE_ATTR_GID: c_uint = 0x04;
pub const QSOE_ATTR_SIZE: c_uint = 0x08;
pub const QSOE_ATTR_TIME: c_uint = 0x10;

pub const QSOE_POLLIN: c_uint = 0x01;
pub const QSOE_POLLOUT: c_uint = 0x02;
pub const QSOE_POLLERR: c_uint = 0x04;
pub const QSOE_POLLHUP: c_uint = 0x08;

pub const QSOE_PROV_PARALLEL: c_uint = 0x01;
pub const QSOE_PROV_NOATIME: c_uint = 0x02;

pub const QSOE_ERRNO_MAX: c_int = 1023;
pub const QSOE_DEFER: c_int = -(QSOE_ERRNO_MAX + 1);
pub const QSOE_MAX_PROVIDERS: usize = 32;

#[repr(C)]
#[derive(Clone, Copy, Debug, Eq, PartialEq)]
pub struct Attr {
    pub mode: ModeT,
    pub uid: UidT,
    pub gid: GidT,
    pub size: OffT,
    pub ino: u64,
    pub rdev: u64,
    pub nlink: c_uint,
    pub atime: QsoeTimeT,
    pub mtime: QsoeTimeT,
    pub ctime: QsoeTimeT,
}

#[repr(C)]
#[derive(Clone, Copy, Debug, Eq, PartialEq)]
pub struct Open {
    pub subpath: *const c_char,
    pub oflags: c_uint,
    pub mode: ModeT,
    pub uid: UidT,
    pub gid: GidT,
}

#[repr(C)]
pub struct Handle {
    prov: *mut Provider,
    pos: OffT,
    oflags: c_uint,
    udata: *mut c_void,
    fw: *mut c_void,
}

impl Handle {
    pub fn provider(&self) -> *mut Provider {
        self.prov
    }

    pub fn pos(&self) -> OffT {
        self.pos
    }

    pub fn oflags(&self) -> c_uint {
        self.oflags
    }

    pub fn udata(&self) -> *mut c_void {
        self.udata
    }

    pub fn set_udata(&mut self, value: *mut c_void) {
        self.udata = value;
    }
}

pub type AcquireFn = unsafe extern "C" fn(*mut Provider, *mut Handle, *const Open) -> c_int;
pub type ReleaseFn = unsafe extern "C" fn(*mut Provider, *mut Handle);
pub type PullFn =
    unsafe extern "C" fn(*mut Provider, *mut Handle, *mut c_void, SizeT, OffT) -> SsizeT;
pub type PushFn =
    unsafe extern "C" fn(*mut Provider, *mut Handle, *const c_void, SizeT, OffT) -> SsizeT;
pub type SeekFn = unsafe extern "C" fn(*mut Provider, *mut Handle, OffT, c_int) -> OffT;
pub type QueryFn = unsafe extern "C" fn(*mut Provider, *mut Handle, *mut Attr) -> c_int;
pub type AdjustFn = unsafe extern "C" fn(*mut Provider, *mut Handle, *const Attr, c_uint) -> c_int;
pub type ControlFn =
    unsafe extern "C" fn(*mut Provider, *mut Handle, c_uint, *mut c_void, SizeT, SizeT) -> SsizeT;
pub type ReadyFn = unsafe extern "C" fn(*mut Provider, *mut Handle, c_uint) -> c_int;
pub type ServiceFn = unsafe extern "C" fn(*mut Provider);
pub type CancelFn = unsafe extern "C" fn(*mut Provider, *mut Handle);
pub type LookupFn = unsafe extern "C" fn(*mut Provider, *const c_char, *mut *mut Provider) -> c_int;
pub type ListFn =
    unsafe extern "C" fn(*mut Provider, *mut Handle, *mut c_void, SizeT, OffT) -> SsizeT;
pub type MakeFn =
    unsafe extern "C" fn(*mut Provider, *const c_char, ModeT, *mut *mut Provider) -> c_int;
pub type UnlinkFn = unsafe extern "C" fn(*mut Provider, *const c_char) -> c_int;
pub type DupFn = unsafe extern "C" fn(*mut Provider, *mut Handle, *const Handle) -> c_int;

#[repr(C)]
#[derive(Clone, Copy)]
pub struct ProviderVtable {
    pub acquire: Option<AcquireFn>,
    pub release: Option<ReleaseFn>,
    pub pull: Option<PullFn>,
    pub push: Option<PushFn>,
    pub seek: Option<SeekFn>,
    pub query: Option<QueryFn>,
    pub adjust: Option<AdjustFn>,
    pub control: Option<ControlFn>,
    pub ready: Option<ReadyFn>,
    pub service: Option<ServiceFn>,
    pub cancel: Option<CancelFn>,
    pub lookup: Option<LookupFn>,
    pub list: Option<ListFn>,
    pub make: Option<MakeFn>,
    pub unlink: Option<UnlinkFn>,
    pub dup: Option<DupFn>,
}

#[repr(C)]
pub struct Provider {
    pub cls: *const ProviderVtable,
    pub attr: Attr,
    pub path: *const c_char,
    pub flags: c_uint,
    fw_chid: c_int,
    fw_refs: c_uint,
}

impl Provider {
    pub const fn zeroed() -> Self {
        Self {
            cls: core::ptr::null(),
            attr: Attr {
                mode: 0,
                uid: 0,
                gid: 0,
                size: 0,
                ino: 0,
                rdev: 0,
                nlink: 0,
                atime: 0,
                mtime: 0,
                ctime: 0,
            },
            path: core::ptr::null(),
            flags: 0,
            fw_chid: -1,
            fw_refs: 0,
        }
    }

    /// Initialize provider storage through the C framework.
    ///
    /// # Safety
    ///
    /// `vtbl` and `path` must remain valid for at least as long as the
    /// provider can be reached from the QSOE path manager or dispatch loop.
    pub unsafe fn init(&mut self, vtbl: *const ProviderVtable, path: *const c_char, mode: ModeT) {
        // SAFETY: the caller promises that `vtbl` and `path` satisfy the C
        // framework lifetime contract described above.
        unsafe { qsoe_provider_init(self, vtbl, path, mode) };
    }

    pub fn listen(&mut self) -> Result<(), c_int> {
        // SAFETY: `self` is valid provider storage; the C framework validates
        // whether it was initialized enough to publish.
        let rc = unsafe { qsoe_provider_listen(self) };
        if rc == 0 {
            Ok(())
        } else {
            Err(-rc)
        }
    }

    pub fn dispatch_run(&mut self) -> c_int {
        // SAFETY: The C dispatcher owns the receive loop after this call.
        unsafe { qsoe_dispatch_run(self) }
    }
}

#[repr(C)]
pub struct Server {
    pub chid: c_int,
    pub nprovs: c_uint,
    pub provs: [*mut Provider; QSOE_MAX_PROVIDERS],
}

impl Server {
    pub const fn zeroed() -> Self {
        Self {
            chid: -1,
            nprovs: 0,
            provs: [core::ptr::null_mut(); QSOE_MAX_PROVIDERS],
        }
    }
}

#[repr(C)]
pub struct Call {
    _private: [u8; 0],
}

unsafe extern "C" {
    pub fn qsoe_default_acquire(self_: *mut Provider, h: *mut Handle, req: *const Open) -> c_int;
    pub fn qsoe_default_release(self_: *mut Provider, h: *mut Handle);
    pub fn qsoe_default_seek(
        self_: *mut Provider,
        h: *mut Handle,
        off: OffT,
        whence: c_int,
    ) -> OffT;
    pub fn qsoe_default_query(self_: *mut Provider, h: *mut Handle, out: *mut Attr) -> c_int;
    pub fn qsoe_default_adjust(
        self_: *mut Provider,
        h: *mut Handle,
        set: *const Attr,
        which: c_uint,
    ) -> c_int;

    pub fn qsoe_provider_init(
        p: *mut Provider,
        vtbl: *const ProviderVtable,
        path: *const c_char,
        mode: ModeT,
    );
    pub fn qsoe_provider_listen(p: *mut Provider) -> c_int;
    pub fn qsoe_provider_retire(p: *mut Provider) -> c_int;
    pub fn qsoe_dispatch_run(p: *mut Provider) -> c_int;

    pub fn qsoe_server_init(s: *mut Server) -> c_int;
    pub fn qsoe_server_publish(s: *mut Server, p: *mut Provider) -> c_int;
    pub fn qsoe_server_run(s: *mut Server) -> c_int;

    pub fn qsoe_defer() -> *mut Call;
    pub fn qsoe_done(call: *mut Call, status: SsizeT);
    pub fn qsoe_reply(call: *mut Call, buf: *const c_void, n: SizeT);
    pub fn qsoe_fail(call: *mut Call, err: c_int);
}

#[cfg(test)]
extern crate std;

#[cfg(test)]
mod tests {
    use super::*;
    use core::mem::{align_of, size_of};

    #[test]
    fn resource_server_layouts_match_rv64_c_abi() {
        assert_eq!(size_of::<Attr>(), 72);
        assert_eq!(align_of::<Attr>(), 8);
        assert_eq!(size_of::<Open>(), 24);
        assert_eq!(align_of::<Open>(), 8);
        assert_eq!(size_of::<Handle>(), 40);
        assert_eq!(align_of::<Handle>(), 8);
        assert_eq!(size_of::<ProviderVtable>(), 128);
        assert_eq!(align_of::<ProviderVtable>(), 8);
        assert_eq!(size_of::<Provider>(), 104);
        assert_eq!(align_of::<Provider>(), 8);
        assert_eq!(size_of::<Server>(), 264);
        assert_eq!(align_of::<Server>(), 8);
    }
}

use crate::util::CborOwnedExt;
use cbor_data::CborOwned;
use redis_module::{
    native_types::RedisType, raw, RedisModuleIO, RedisModuleString, RedisModuleTypeMethods,
};
use std::{
    ffi::{c_int, c_void},
    ptr::null_mut,
};

pub const MODULE_TYPE_NAME: &str = "ReCBORTyp"; // MUST be 9 characters long
pub const REDIS_CBOR_TYPE_VERSION: i32 = 1;

pub static REDIS_CBOR_TYPE: RedisType = RedisType::new(
    MODULE_TYPE_NAME,
    REDIS_CBOR_TYPE_VERSION,
    RedisModuleTypeMethods {
        version: redis_module::TYPE_METHOD_VERSION,
        rdb_load: Some(rdb_load),
        rdb_save: Some(rdb_save),
        aof_rewrite: Some(aof_rewrite),
        mem_usage: Some(mem_usage),
        digest: None,
        free: Some(free),
        aux_load: None,
        aux_save: None,
        aux_save_triggers: 0,
        free_effort: None,
        unlink: None,
        copy: Some(copy),
        defrag: None,
        copy2: None,
        free_effort2: None,
        mem_usage2: None,
        unlink2: None,
    },
);

pub extern "C" fn rdb_load(rdb: *mut raw::RedisModuleIO, _encver: c_int) -> *mut c_void {
    let Ok(buffer) = raw::load_string_buffer(rdb) else {
        return null_mut();
    };

    let bytes = buffer.as_ref();
    let cbor = CborOwned::unchecked(bytes);
    Box::into_raw(Box::new(cbor)).cast::<libc::c_void>()
}

pub unsafe extern "C" fn rdb_save(rdb: *mut raw::RedisModuleIO, value: *mut c_void) {
    let cbor = unsafe { &*(value as *mut CborOwned) };
    let bytes = cbor.as_slice();
    let str = std::str::from_utf8_unchecked(bytes); // no save_string_buffer available in redis-module :(
    raw::save_string(rdb, str);
}

unsafe extern "C" fn aof_rewrite(
    _aof: *mut RedisModuleIO,
    _key: *mut RedisModuleString,
    _value: *mut c_void,
) {
    todo!();
}

unsafe extern "C" fn mem_usage(value: *const c_void) -> usize {
    let cbor = unsafe { &*(value as *mut CborOwned) };
    cbor.mem_usage()
}

unsafe extern "C" fn free(value: *mut c_void) {
    if value.is_null() {
        // on Redis 6.0 we might get a NULL value here, so we need to handle it.
        return;
    }

    let cbor = value.cast::<CborOwned>();
    std::mem::drop(Box::from_raw(cbor));
}

unsafe extern "C" fn copy(
    _fromkey: *mut RedisModuleString,
    _tokey: *mut RedisModuleString,
    value: *const c_void,
) -> *mut c_void {
    let cbor = unsafe { &*(value as *mut CborOwned) };
    let cbor_cloned = cbor.clone();
    Box::into_raw(Box::new(cbor_cloned)).cast::<c_void>()
}

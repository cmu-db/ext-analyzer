use crate::redis_cbor_type::REDIS_CBOR_TYPE;

mod redis_cbor_type;
mod commands;
mod util;

pub const MODULE_NAME: &str = "ReCBOR";

#[macro_use]
extern crate redis_module;

redis_module! {
    name: MODULE_NAME,
    version: 1,
    data_types: [REDIS_CBOR_TYPE],
    commands: [
        ["cbor.arrappend", commands::cbor_arr_append, "write deny-oom", 1, 1, 1],
        ["cbor.arrindex", commands::cbor_arr_index, "readonly", 1, 1, 1],
        ["cbor.arrinsert", commands::cbor_arr_insert, "write deny-oom", 1, 1, 1],
        ["cbor.arrlen", commands::cbor_arr_len, "readonly", 1, 1, 1],
        ["cbor.arrpop", commands::cbor_arr_pop, "write deny-oom", 1, 1, 1],
        ["cbor.arrtrim", commands::cbor_arr_trim, "write deny-oom", 1, 1, 1],
        ["cbor.clear", commands::cbor_clear, "write deny-oom", 1, 1, 1],
        ["cbor.debug", commands::cbor_debug, "readonly", 2, 2, 1],
        ["cbor.del", commands::cbor_del, "write deny-oom", 1, 1, 1],
        ["cbor.get", commands::cbor_get, "readonly", 1, 1, 1],
        ["cbor.mapappend", commands::cbor_map_append, "write deny-oom", 1, 1, 1],
        ["cbor.mapkeys", commands::cbor_mapkeys, "readonly", 1, 1, 1],
        ["cbor.maplen", commands::cbor_map_len, "readonly", 1, 1, 1],
        ["cbor.mget", commands::cbor_mget, "readonly", 1,1,1],
        ["cbor.numincrby", commands::cbor_num_incr_by, "write deny-oom", 1,1,1],
        ["cbor.nummultby", commands::cbor_num_mult_by, "write deny-oom", 1,1,1],
        ["cbor.resp", commands::cbor_resp, "readonly", 1,1,1],
        ["cbor.set", commands::cbor_set, "write deny-oom", 1, 1, 1],
        ["cbor.strappend", commands::cbor_str_append, "write deny-oom", 1, 1, 1],
        ["cbor.strlen", commands::cbor_str_len, "readonly", 1,1,1],
        ["cbor.toggle", commands::cbor_toggle, "write deny-oom", 1, 1, 1],
        ["cbor.type", commands::cbor_type, "readonly", 1,1,1],
    ],
}

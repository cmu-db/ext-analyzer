use crate::util::{CborKey, CborOwnedExt, NextArgExt};
use redis_module::{Context, RedisError, RedisResult, RedisString, RedisValue};

///
/// CBOT.DEBUG <subcommand & arguments>
///
/// subcommands:
/// MEMORY key
/// HELP
///
pub fn cbor_debug(ctx: &Context, args: Vec<RedisString>) -> RedisResult {
    let mut args = args.iter().skip(1);

    match args.next_str()?.to_uppercase().as_str() {
        "DIAG" => {
            let key = args.next_arg()?;
            let key = ctx.open_key(key);

            Ok(key
                .get_cbor_value()?
                .map(|v| RedisValue::BulkString(format!("{v}")))
                .unwrap_or(RedisValue::Null))
        }
        "MEMORY" => {
            let key = args.next_arg()?;
            let key = ctx.open_key(key);

            Ok(key
                .get_cbor_value()?
                .map(|v| v.mem_usage())
                .unwrap_or(0)
                .into())
        },
        "HELP" => {
            let results = vec![
                "DIAG <key> - display key in CBOR diagnostic notation",
                "MEMORY <key> - reports memory usage",
                "HELP                - this message",
            ];
            Ok(results.into())
        }
        _ => Err(RedisError::Str(
            "ERR unknown subcommand - try `CBOR.DEBUG HELP`",
        )),
    }
}

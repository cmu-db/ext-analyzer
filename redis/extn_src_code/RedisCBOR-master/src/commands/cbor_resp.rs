use crate::util::{CborKey, CborPathExt, NextArgExt};
use cbor_data::{Cbor, CborOwned, ItemKind};
use cborpath::CborPath;
use redis_module::{Context, RedisError, RedisResult, RedisString, RedisValue};

///
/// CBOR.RESP key [path]
///
/// Return the CBOR document in `key` in [Redis serialization protocol specification](https://redis.io/docs/reference/protocol-spec) form
pub fn cbor_resp(ctx: &Context, args: Vec<RedisString>) -> RedisResult {
    let mut args = args.iter().skip(1);

    let key = args.next_arg()?;
    let cbor_path = match args.next_arg() {
        Ok(cbor_path) => CborPath::from_arg(cbor_path)?,
        Err(_) => CborPath::root(),
    };

    let key = ctx.open_key(key);
    let Some(existing) = key.get_cbor_value()? else {
        return Err(RedisError::nonexistent_key());
    };

    Ok(resp(existing, &cbor_path).into())
}

fn resp(existing: &CborOwned, cbor_path: &CborPath) -> Vec<RedisValue> {
    let results = cbor_path.read(existing);
    results.into_iter().map(resp_from_cbor).collect()
}

fn resp_from_cbor(value: &Cbor) -> RedisValue {
    match value.kind() {
        ItemKind::Pos(v) => RedisValue::Integer(v as i64),
        ItemKind::Neg(v) => RedisValue::Integer(-1 - (v as i64)),
        ItemKind::Float(v) => RedisValue::Float(v),
        ItemKind::Str(v) => RedisValue::BulkString(v.as_cow().into_owned()),
        ItemKind::Bytes(v) => RedisValue::StringBuffer(v.as_cow().into_owned()),
        ItemKind::Bool(v) => RedisValue::Boolean(v),
        ItemKind::Null => RedisValue::Null,
        ItemKind::Undefined => RedisValue::Null,
        ItemKind::Simple(v) => RedisValue::Integer(v as i64),
        ItemKind::Array(a) => RedisValue::Array(a.map(resp_from_cbor).collect()),
        ItemKind::Dict(d) => RedisValue::Map(
            d.map(|(k, v)| (resp_from_cbor(k), resp_from_cbor(v)))
                .collect(),
        ),
    }
}

#[cfg(test)]
mod tests {
    use super::resp;
    use crate::util::diag_to_cbor;
    use cborpath::CborPath;
    use redis_module::RedisValue;

    #[test]
    fn complex() {
        let cbor = diag_to_cbor(
            r#"[{"a":1,"b":2},[1,2,3],"foo",h'0123456789ABCDEF',12,-12,12.12,true,null,undefined]"#,
        );
        let cbor_path = CborPath::root();

        let values = resp(&cbor, &cbor_path);
        assert_eq!(
            values,
            vec![RedisValue::Array(vec![
                RedisValue::Map(vec![
                    (
                        RedisValue::BulkString("a".to_string()),
                        RedisValue::Integer(1)
                    ),
                    (
                        RedisValue::BulkString("b".to_string()),
                        RedisValue::Integer(2)
                    )
                ]),
                RedisValue::Array(vec![
                    RedisValue::Integer(1),
                    RedisValue::Integer(2),
                    RedisValue::Integer(3)
                ]),
                RedisValue::BulkString("foo".to_string()),
                RedisValue::StringBuffer(b"\x01\x23\x45\x67\x89\xAB\xCD\xEF".to_vec()),
                RedisValue::Integer(12),
                RedisValue::Integer(-12),
                RedisValue::Float(12.12),
                RedisValue::Boolean(true),
                RedisValue::Null,
                RedisValue::Null,
            ])]
        );
    }
}

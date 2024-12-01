use crate::util::{CborKey, CborPathExt, NextArgExt};
use cbor_data::{CborOwned, ItemKind};
use cborpath::CborPath;
use redis_module::{Context, RedisError, RedisResult, RedisString, RedisValue};

///
/// CBOR.MAPKEYS key [path]
///
/// Return the keys in the map that's referenced by path
pub fn cbor_mapkeys(ctx: &Context, args: Vec<RedisString>) -> RedisResult {
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

    Ok(map_keys(existing, &cbor_path).into())
}

fn map_keys(existing: &CborOwned, cbor_path: &CborPath) -> Vec<RedisValue> {
    cbor_path
        .read(existing)
        .into_iter()
        .map(|value| match value.kind() {
            ItemKind::Dict(d) => RedisValue::Array(
                d.map(|(k, _v)| RedisValue::StringBuffer(k.as_slice().to_vec()))
                    .collect(),
            ),
            _ => RedisValue::Null,
        })
        .collect()
}

#[cfg(test)]
mod map_keys {
    use super::map_keys;
    use crate::util::{diag_to_bytes, diag_to_cbor};
    use cborpath::{builder::segment, CborPath};
    use redis_module::RedisValue;

    #[test]
    fn multiple() {
        let cbor = diag_to_cbor(r#"{"a":[3], "nested": {"a": {"b":2, "c": 1}}}"#);
        let cbor_path = CborPath::builder().descendant(segment().key("a")).build();

        let result = map_keys(&cbor, &cbor_path);
        assert_eq!(
            vec![
                RedisValue::Null,
                RedisValue::Array(vec![
                    RedisValue::StringBuffer(diag_to_bytes(r#""b""#)),
                    RedisValue::StringBuffer(diag_to_bytes(r#""c""#))
                ]),
            ],
            result
        );
    }
}

use crate::util::{CborKey, CborPathExt, NextArgExt};
use cbor_data::{CborOwned, ItemKind};
use cborpath::CborPath;
use redis_module::{Context, RedisError, RedisResult, RedisString, RedisValue};

///
/// CBOR.MAPLEN key [path]
///
/// Return the keys in the map that's referenced by path
pub fn cbor_map_len(ctx: &Context, args: Vec<RedisString>) -> RedisResult {
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

    Ok(map_len(existing, &cbor_path).into())
}

fn map_len(existing: &CborOwned, cbor_path: &CborPath) -> Vec<RedisValue> {
    let results = cbor_path.read(existing);
    results
        .into_iter()
        .map(|v| {
            if let ItemKind::Dict(d) = v.kind() {
                if let Some(len) = d.size() {
                    RedisValue::Integer(len as i64)
                } else {
                    RedisValue::Integer(d.count() as i64)
                }
            } else {
                RedisValue::Null
            }
        })
        .collect()
}

#[cfg(test)]
mod tests {
    use super::map_len;
    use crate::util::diag_to_cbor;
    use cborpath::{builder::segment, CborPath};
    use redis_module::RedisValue;

    #[test]
    fn simple_array() {
        let cbor = diag_to_cbor(r#"{"a":[3], "nested": {"a": {"b":2, "c": 1}}}"#);
        let cbor_path = CborPath::builder().descendant(segment().key("a")).build();

        let results = map_len(&cbor, &cbor_path);
        assert_eq!(vec![RedisValue::Null, RedisValue::Integer(2)], results);
    }
}

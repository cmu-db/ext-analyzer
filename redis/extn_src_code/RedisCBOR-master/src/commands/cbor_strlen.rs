use crate::util::{CborKey, CborPathExt, NextArgExt};
use cbor_data::{CborOwned, ItemKind};
use cborpath::CborPath;
use redis_module::{Context, RedisError, RedisResult, RedisString, RedisValue};

///
/// CBOR.STRLEN key [path]
///
/// Report the length of the CBOR String at path in key
pub fn cbor_str_len(ctx: &Context, args: Vec<RedisString>) -> RedisResult {
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

    Ok(str_len(existing, &cbor_path).into())
}

fn str_len(existing: &CborOwned, cbor_path: &CborPath) -> Vec<RedisValue> {
    let results = cbor_path.read(existing);
    results
        .into_iter()
        .map(|v| {
            if let ItemKind::Str(s) = v.kind() {
                RedisValue::Integer(s.len() as i64)
            } else {
                RedisValue::Null
            }
        })
        .collect()
}

#[cfg(test)]
mod tests {
    use super::str_len;
    use crate::util::diag_to_cbor;
    use cborpath::{builder::segment, CborPath};
    use redis_module::RedisValue;

    #[test]
    fn test() {
        let cbor = diag_to_cbor(r#"{"a":"foo", "nested": {"a": "hello"}, "nested2": {"a": 31}}"#);
        let cbor_path = CborPath::builder().descendant(segment().key("a")).build();

        let str_lengths = str_len(&cbor, &cbor_path);
        assert_eq!(
            vec![
                RedisValue::Integer(3),
                RedisValue::Integer(5),
                RedisValue::Null
            ],
            str_lengths
        );
    }
}

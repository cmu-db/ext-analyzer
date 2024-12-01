use crate::util::{CborKey, CborPathExt, NextArgExt};
use cbor_data::{CborOwned, ItemKind};
use cborpath::CborPath;
use redis_module::{Context, RedisError, RedisResult, RedisString, RedisValue};

///
/// CBOR.ARRLEN key [path]
///
/// Report the length of the CBOR array at path in key
pub fn cbor_arr_len(ctx: &Context, args: Vec<RedisString>) -> RedisResult {
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

    Ok(array_len(existing, &cbor_path).into())
}

fn array_len(existing: &CborOwned, cbor_path: &CborPath) -> Vec<RedisValue> {
    let results = cbor_path.read(existing);
    results
        .into_iter()
        .map(|v| {
            if let ItemKind::Array(array) = v.kind() {
                if let Some(len) = array.size() {
                    RedisValue::Integer(len as i64)
                } else {
                    RedisValue::Integer(array.count() as i64)
                }
            } else {
                RedisValue::Null
            }
        })
        .collect()
}

#[cfg(test)]
mod tests {
    use super::array_len;
    use crate::util::diag_to_cbor;
    use cborpath::CborPath;
    use redis_module::RedisValue;

    #[test]
    fn simple_array() {
        let cbor = diag_to_cbor(r#"["a","b","c","d"]"#);

        // ["$"]
        let cbor_path = CborPath::root();

        let results = array_len(&cbor, &cbor_path);
        assert_eq!(vec![RedisValue::Integer(4)], results);
    }

    #[test]
    fn deep_array() {
        let cbor = diag_to_cbor(r#"{"foo":["a","b","c"]}"#);

        // ["$", "foo"]
        let cbor_path = CborPath::builder().key("foo").build();
        let results = array_len(&cbor, &cbor_path);

        assert_eq!(vec![RedisValue::Integer(3)], results);
    }

    #[test]
    fn multiple_arrays() {
        let cbor = diag_to_cbor(r#"{"foo":["a","b","c"],"bar":[1,2,3,4]}"#);

        // ["$", {"*":1}]
        let cbor_path = CborPath::builder().wildcard().build();
        let results = array_len(&cbor, &cbor_path);

        assert_eq!(vec![RedisValue::Integer(3), RedisValue::Integer(4)], results);
    }

    #[test]
    fn not_an_array() {
        let cbor = diag_to_cbor(r#"{"foo":12,"bar":["a","b","c"]}"#);

        // ["$", {"*":1}]
        let cbor_path = CborPath::builder().wildcard().build();
        let results = array_len(&cbor, &cbor_path);

        assert_eq!(vec![RedisValue::Null, RedisValue::Integer(3)], results);
    }
}

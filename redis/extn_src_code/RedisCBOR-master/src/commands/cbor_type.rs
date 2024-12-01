use crate::util::{CborKey, CborPathExt, NextArgExt};
use cbor_data::{CborOwned, ItemKind};
use cborpath::CborPath;
use redis_module::{Context, RedisError, RedisResult, RedisString, RedisValue};

///
/// CBOR.TYPE key [path]
///
/// Report the type of CBOR value at path
pub fn cbor_type(ctx: &Context, args: Vec<RedisString>) -> RedisResult {
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

    Ok(_type(existing, &cbor_path).into())
}

pub fn _type(existing: &CborOwned, cbor_path: &CborPath) -> Vec<RedisValue> {
    let results = cbor_path.read(existing);
    results
        .into_iter()
        .map(|v| {
            RedisValue::BulkString(
                match v.kind() {
                    ItemKind::Pos(_) => "unsigned",
                    ItemKind::Neg(_) => "negative",
                    ItemKind::Float(_) => "float",
                    ItemKind::Str(_) => "string",
                    ItemKind::Bytes(_) => "bytestring",
                    ItemKind::Bool(_) => "boolean",
                    ItemKind::Null => "null",
                    ItemKind::Undefined => "undefined",
                    ItemKind::Simple(_) => "simple",
                    ItemKind::Array(_) => "array",
                    ItemKind::Dict(_) => "map",
                }
                .to_string(),
            )
        })
        .collect()
}

#[cfg(test)]
mod tests {
    use super::_type;
    use crate::util::diag_to_cbor;
    use cborpath::CborPath;
    use redis_module::RedisValue;

    #[test]
    fn test() {
        let cbor = diag_to_cbor(
            r#"[12,-12,12.12,"foo",h'0123456789ABCDEF',true,null,undefined,simple(32),[1,2,3],{"a":1,"b":2}]"#,
        );
        let cbor_path = CborPath::builder().wildcard().build();

        let _types = _type(&cbor, &cbor_path);
        assert_eq!(
            vec![
                RedisValue::BulkString("unsigned".to_string()),
                RedisValue::BulkString("negative".to_string()),
                RedisValue::BulkString("float".to_string()),
                RedisValue::BulkString("string".to_string()),
                RedisValue::BulkString("bytestring".to_string()),
                RedisValue::BulkString("boolean".to_string()),
                RedisValue::BulkString("null".to_string()),
                RedisValue::BulkString("undefined".to_string()),
                RedisValue::BulkString("simple".to_string()),
                RedisValue::BulkString("array".to_string()),
                RedisValue::BulkString("map".to_string()),
            ],
            _types
        );
    }
}

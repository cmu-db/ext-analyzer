use crate::util::{CborKey, CborPathExt};
use cbor_data::{CborBuilder, CborOwned, Writer};
use cborpath::CborPath;
use redis_module::{Context, RedisError, RedisResult, RedisString, RedisValue};

///
/// CBOR.MGET key [key ...] path
///
/// Return the values at path from multiple key arguments
pub fn cbor_mget(ctx: &Context, args: Vec<RedisString>) -> RedisResult {
    if args.len() < 3 {
        return Err(RedisError::WrongArity);
    }

    let keys = &args[1..args.len() - 1];
    let path = &args[args.len() - 1];

    let cbor_path = CborPath::from_arg(path)?;

    let keys = keys.iter().map(|key| ctx.open_key(key)).collect::<Vec<_>>();

    let existing_values = keys.iter().map(|key| key.get_cbor_value());

    multiple_get(existing_values, &cbor_path)
}

fn multiple_get<'a, I>(existing_values: I, cbor_path: &CborPath) -> RedisResult
where
    I: Iterator<Item = Result<Option<&'a CborOwned>, RedisError>>,
{
    Ok(existing_values
        .map(|existing| {
            Ok(existing?.map_or(RedisValue::Null, |existing| {
                let results = cbor_path.read(existing);
                let new_value = CborBuilder::new().write_array(None, |builder| {
                    for result in results {
                        builder.write_item(result);
                    }
                });
                RedisValue::StringBuffer(new_value.into_vec())
            }))
        })
        .collect::<Result<Vec<RedisValue>, RedisError>>()?
        .into())
}

#[cfg(test)]
mod tests {
    use super::multiple_get;
    use crate::util::{diag_to_bytes, diag_to_cbor};
    use cborpath::{builder::segment, CborPath};
    use redis_module::{RedisError, RedisValue};

    #[test]
    fn success() -> Result<(), RedisError> {
        let value1 = diag_to_cbor(r#"{"a":1, "b": 2, "nested": {"a": 3}, "c": null}"#);
        let value2 = diag_to_cbor(r#"{"a":4, "b": 5, "nested": {"a": 6}, "c": null}"#);
        let values = vec![Ok(Some(&value1)), Ok(Some(&value2)), Ok(None)];
        let path = CborPath::builder().descendant(segment().key("a")).build();
        let result = multiple_get(values.into_iter(), &path)?;
        assert_eq!(
            RedisValue::Array(vec![
                RedisValue::StringBuffer(diag_to_bytes("[1,3]")),
                RedisValue::StringBuffer(diag_to_bytes("[4,6]")),
                RedisValue::Null
            ]),
            result
        );

        Ok(())
    }

    #[test]
    fn error() -> Result<(), RedisError> {
        let value1 = diag_to_cbor(r#"{"a":1, "b": 2, "nested": {"a": 3}, "c": null}"#);
        let value2 = diag_to_cbor(r#"{"a":4, "b": 5, "nested": {"a": 6}, "c": null}"#);
        let values = vec![Ok(Some(&value1)), Ok(Some(&value2)), Err(RedisError::Str("MyError"))];
        let path = CborPath::builder().descendant(segment().key("a")).build();
        let result = multiple_get(values.into_iter(), &path);
        assert!(matches!(result, Err(RedisError::Str(e)) if e == "MyError"));

        Ok(())
    }
}

use crate::util::{apply_changes, CborKeyWritable, CborPathExt, NextArgExt};
use cbor_data::{Cbor, CborBuilder, CborOwned, ItemKind, Writer};
use cborpath::CborPath;
use redis_module::{Context, RedisError, RedisResult, RedisString, RedisValue};
use std::borrow::Cow;

///
/// CBOR.STRAPPEND key path value
///
/// Append the CBOR-string value to the string at path
pub fn cbor_str_append(ctx: &Context, args: Vec<RedisString>) -> RedisResult {
    let mut args = args.iter().skip(1).peekable();

    let key_name = args.next_arg()?;
    let path = args.next_arg()?;
    let value = args.next_arg()?;

    let key = ctx.open_key_writable(key_name);
    let cbor_path = CborPath::from_arg(path)?;
    let value = value.try_as_str()?;

    let Some(existing) = key.get_cbor_value()? else {
        return Err(RedisError::nonexistent_key());
    };

    let (new_value, str_lengths) = str_append(existing, &cbor_path, value);

    if let Some(new_value) = new_value {
        key.set_cbor_value(new_value)?;
        apply_changes(ctx, "cbor.strappend", key_name)?;
    }

    Ok(str_lengths.into())
}

fn str_append<'a>(
    existing: &'a Cbor,
    cbor_path: &CborPath,
    value: &'a str,
) -> (Option<CborOwned>, Vec<RedisValue>) {
    let mut str_lengths = Vec::<RedisValue>::new();

    let new_value = cbor_path
        .write(existing, |old_value| {
            if let ItemKind::Str(s) = old_value.kind() {
                let result = s.as_cow() + value;
                str_lengths.push(RedisValue::Integer(result.len() as i64));
                Ok(Some(Cow::Owned(
                    CborBuilder::new().write_str(&result, None),
                )))
            } else {
                str_lengths.push(RedisValue::Null);
                Ok(Some(Cow::Borrowed(old_value)))
            }
        })
        .unwrap();

    (new_value, str_lengths)
}

#[cfg(test)]
mod tests {
    use super::str_append;
    use crate::util::{cbor_to_diag, diag_to_cbor};
    use cborpath::{builder::segment, CborPath};
    use redis_module::RedisValue;

    #[test]
    fn test() {
        let cbor = diag_to_cbor(r#"{"a":"foo", "nested": {"a": "hello"}, "nested2": {"a": 12}}"#);
        let cbor_path = CborPath::builder().descendant(segment().key("a")).build();
        let value = "baz";

        let (new_value, str_lengths) = str_append(&cbor, &cbor_path, value);
        assert_eq!(
            r#"{"a":"foobaz","nested":{"a":"hellobaz"},"nested2":{"a":12}}"#,
            cbor_to_diag(&new_value.unwrap())
        );
        assert_eq!(
            vec![
                RedisValue::Integer(6),
                RedisValue::Integer(8),
                RedisValue::Null
            ],
            str_lengths
        );
    }
}

use crate::util::{apply_changes, CborKeyWritable, CborPathExt, NextArgExt};
use cbor_data::{Cbor, CborBuilder, CborOwned, ItemKind, Writer};
use cborpath::CborPath;
use redis_module::{Context, RedisError, RedisResult, RedisString, RedisValue};
use std::borrow::Cow;

///
/// CBOR.TOGGLE key [path]
///
/// Toggle a Boolean value stored at path
pub fn cbor_toggle(ctx: &Context, args: Vec<RedisString>) -> RedisResult {
    let mut args = args.iter().skip(1).peekable();

    let key_name = args.next_arg()?;
    let path = args.next_arg()?;

    let key = ctx.open_key_writable(key_name);
    let cbor_path = CborPath::from_arg(path)?;

    let Some(existing) = key.get_cbor_value()? else {
        return Err(RedisError::nonexistent_key());
    };

    let (new_value, new_booleans) = toggle(existing, &cbor_path);

    if let Some(new_value) = new_value {
        key.set_cbor_value(new_value)?;
        apply_changes(ctx, "cbor.toggle", key_name)?;
    }

    Ok(new_booleans.into())
}

fn toggle(existing: &Cbor, cbor_path: &CborPath) -> (Option<CborOwned>, Vec<RedisValue>) {
    let mut new_booleans = Vec::<RedisValue>::new();

    let new_value = cbor_path
        .write(existing, |old_value| {
            if let ItemKind::Bool(b) = old_value.kind() {
                new_booleans.push(RedisValue::Boolean(!b));
                Ok(Some(Cow::Owned(CborBuilder::new().write_bool(!b, None))))
            } else {
                new_booleans.push(RedisValue::Null);
                Ok(Some(Cow::Borrowed(old_value)))
            }
        })
        .unwrap();

    (new_value, new_booleans)
}


#[cfg(test)]
mod tests {
    use super::toggle;
    use crate::util::{cbor_to_diag, diag_to_cbor};
    use cborpath::{builder::segment, CborPath};
    use redis_module::RedisValue;

    #[test]
    fn test() {
        let cbor = diag_to_cbor(r#"{"a": true, "nested": {"a": "hello"}}"#);
        let cbor_path = CborPath::builder().descendant(segment().key("a")).build();

        let (new_value, new_booleans) = toggle(&cbor, &cbor_path);
        let new_value = new_value.unwrap();
        assert_eq!(
            r#"{"a":false,"nested":{"a":"hello"}}"#,
            cbor_to_diag(&new_value)
        );
        assert_eq!(
            vec![
                RedisValue::Boolean(false),
                RedisValue::Null
            ],
            new_booleans
        );

        let (new_value, new_booleans) = toggle(&new_value, &cbor_path);
        assert_eq!(
            r#"{"a":true,"nested":{"a":"hello"}}"#,
            cbor_to_diag(&new_value.unwrap())
        );
        assert_eq!(
            vec![
                RedisValue::Boolean(true),
                RedisValue::Null
            ],
            new_booleans
        );
    }
}

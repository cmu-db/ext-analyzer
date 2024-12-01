use crate::util::{apply_changes, CborExt, CborKeyWritable, CborPathExt, NextArgExt};
use cbor_data::{Cbor, CborBuilder, CborOwned, ItemKind, Writer};
use cborpath::CborPath;
use redis_module::{Context, RedisError, RedisResult, RedisString, RedisValue};
use std::borrow::Cow;

///
/// CBOR.ARRAPPEND key path value [value ...]
///
pub fn cbor_arr_append(ctx: &Context, args: Vec<RedisString>) -> RedisResult {
    let mut args = args.iter().skip(1).peekable();

    let key_name = args.next_arg()?;
    let path = args.next_arg()?;

    // We require at least one CBOR value to append
    args.peek().ok_or(RedisError::WrongArity)?;

    let values = args.try_fold(Vec::with_capacity(args.len()), |mut acc, arg| {
        let value = Cbor::from_arg(arg)?;
        acc.push(value);
        Result::<Vec<&Cbor>, RedisError>::Ok(acc)
    })?;

    let key = ctx.open_key_writable(key_name);
    let cbor_path = CborPath::from_arg(path)?;

    let Some(existing) = key.get_cbor_value()? else {
        return Err(RedisError::nonexistent_key());
    };

    let (new_value, array_sizes) = array_append(existing, &cbor_path, values);

    if let Some(new_value) = new_value {
        key.set_cbor_value(new_value)?;
        apply_changes(ctx, "cbor.arrappend", key_name)?;
    }

    Ok(array_sizes.into())
}

fn array_append<'a>(
    existing: &'a Cbor,
    cbor_path: &CborPath,
    values: Vec<&'a Cbor>,
) -> (Option<CborOwned>, Vec<RedisValue>) {
    let mut array_sizes = Vec::<RedisValue>::new();

    let new_value = cbor_path
        .write(existing, |old_value| {
            if let ItemKind::Array(array) = old_value.kind() {
                Ok(Some(Cow::Owned(CborBuilder::new().write_array(
                    None,
                    |builder| {
                        let mut size = 0i64;
                        for item in array {
                            size += 1;
                            builder.write_item(item);
                        }
                        for value in &values {
                            builder.write_item(value);
                            size += 1;
                        }

                        array_sizes.push(RedisValue::Integer(size));
                    },
                ))))
            } else {
                array_sizes.push(RedisValue::Null);
                Ok(Some(Cow::Borrowed(old_value)))
            }
        })
        .unwrap();

    (new_value, array_sizes)
}

#[cfg(test)]
mod tests {
    use super::array_append;
    use crate::util::{cbor_to_diag, diag_to_cbor};
    use cborpath::CborPath;
    use redis_module::RedisValue;

    #[test]
    fn simple_array() {
        let cbor = diag_to_cbor(r#"["a","b","c"]"#);
        let item1 = diag_to_cbor(r#""d""#);
        let item2 = diag_to_cbor(r#""e""#);

        // ["$"]
        let cbor_path = CborPath::root();
        let (new_value, array_sizes) = array_append(&cbor, &cbor_path, vec![&item1, &item2]);

        assert_eq!(
            r#"["a","b","c","d","e"]"#,
            cbor_to_diag(&new_value.unwrap())
        );
        assert_eq!(vec![RedisValue::Integer(5)], array_sizes);
    }

    #[test]
    fn deep_array() {
        let cbor = diag_to_cbor(r#"{"foo":["a","b","c"]}"#);
        let item1 = diag_to_cbor(r#""d""#);
        let item2 = diag_to_cbor(r#""e""#);

        // ["$", "foo"]
        let cbor_path = CborPath::builder().key("foo").build();
        let (new_value, array_sizes) = array_append(&cbor, &cbor_path, vec![&item1, &item2]);

        assert_eq!(
            r#"{"foo":["a","b","c","d","e"]}"#,
            cbor_to_diag(&new_value.unwrap())
        );
        assert_eq!(vec![RedisValue::Integer(5)], array_sizes);
    }

    #[test]
    fn multiple_arrays() {
        let cbor = diag_to_cbor(r#"{"foo":["a","b","c"],"bar":[1,2,3,4]}"#);
        let item1 = diag_to_cbor(r#""d""#);
        let item2 = diag_to_cbor(r#""e""#);

        // ["$", {"*":1}]
        let cbor_path = CborPath::builder().wildcard().build();
        let (new_value, array_sizes) = array_append(&cbor, &cbor_path, vec![&item1, &item2]);

        assert_eq!(
            r#"{"foo":["a","b","c","d","e"],"bar":[1,2,3,4,"d","e"]}"#,
            cbor_to_diag(&new_value.unwrap())
        );
        assert_eq!(
            vec![RedisValue::Integer(5), RedisValue::Integer(6)],
            array_sizes
        );
    }

    #[test]
    fn not_an_array() {
        let cbor = diag_to_cbor(r#"{"foo":12,"bar":[1,2,3]}"#);
        let item1 = diag_to_cbor(r#""d""#);
        let item2 = diag_to_cbor(r#""e""#);

        // ["$", {"*":1}]
        let cbor_path = CborPath::builder().wildcard().build();
        let (new_value, array_sizes) = array_append(&cbor, &cbor_path, vec![&item1, &item2]);

        assert_eq!(
            r#"{"foo":12,"bar":[1,2,3,"d","e"]}"#,
            cbor_to_diag(&new_value.unwrap())
        );
        assert_eq!(vec![RedisValue::Null, RedisValue::Integer(5)], array_sizes);
    }
}

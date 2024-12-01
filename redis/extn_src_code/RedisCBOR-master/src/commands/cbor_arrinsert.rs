use crate::util::{
    apply_changes, normalize_index, CborExt, CborKeyWritable, CborPathExt, NextArgExt,
};
use cbor_data::{Cbor, CborBuilder, CborOwned, ItemKind, Writer};
use cborpath::CborPath;
use redis_module::{Context, RedisError, RedisResult, RedisString, RedisValue};
use std::borrow::Cow;

///
/// CBOR.ARRINSERT key path index value [value ...]
///
pub fn cbor_arr_insert(ctx: &Context, args: Vec<RedisString>) -> RedisResult {
    let mut args = args.iter().skip(1).peekable();

    let key_name = args.next_arg()?;
    let path = args.next_arg()?;
    let index = args.next_i64()? as isize;

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

    let (new_value, array_sizes) = array_insert(existing, &cbor_path, index, values);

    if let Some(new_value) = new_value {
        key.set_cbor_value(new_value)?;
        apply_changes(ctx, "cbor.arrappend", key_name)?;
    }

    Ok(array_sizes.into())
}

fn array_insert<'a>(
    existing: &'a Cbor,
    cbor_path: &CborPath,
    index: isize,
    values: Vec<&'a Cbor>,
) -> (Option<CborOwned>, Vec<RedisValue>) {
    let mut array_sizes = Vec::<RedisValue>::new();

    let new_value = cbor_path
        .write(existing, |old_value| {
            if let ItemKind::Array(array) = old_value.kind() {
                let new_value = match (index, array.size()) {
                    (index, _) if index >= 0 => {
                        write_array(array, index as usize, &values, &mut array_sizes)
                    }
                    (index, Some(len)) => {
                        let index = normalize_index(index, len as usize);
                        write_array(array, index, &values, &mut array_sizes)
                    }
                    (index, None) => {
                        let array = array.collect::<Vec<_>>();
                        let index = normalize_index(index, array.len());
                        write_array(array.into_iter(), index, &values, &mut array_sizes)
                    }
                };

                Ok(Some(Cow::Owned(new_value)))
            } else {
                array_sizes.push(RedisValue::Null);
                Ok(Some(Cow::Borrowed(old_value)))
            }
        })
        .unwrap();

    (new_value, array_sizes)
}

fn write_array<'a, I>(
    mut existing_items: I,
    index: usize,
    values: &Vec<&Cbor>,
    array_sizes: &mut Vec<RedisValue>,
) -> CborOwned
where
    I: Iterator<Item = &'a Cbor>,
{
    CborBuilder::new().write_array(None, |builder| {
        let mut size = 0usize;
        while size < index {
            if let Some(item) = existing_items.next() {
                size += 1;
                builder.write_item(item);
            } else {
                break;
            }
        }
        for value in values {
            builder.write_item(value);
            size += 1;
        }
        for item in existing_items {
            size += 1;
            builder.write_item(item);
        }

        array_sizes.push(RedisValue::Integer(size as i64));
    })
}

#[cfg(test)]
mod tests {
    use super::array_insert;
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

        let (new_value, array_sizes) = array_insert(&cbor, &cbor_path, 3, vec![&item1, &item2]);
        assert_eq!(
            r#"["a","b","c","d","e"]"#,
            cbor_to_diag(&new_value.unwrap())
        );
        assert_eq!(vec![RedisValue::Integer(5)], array_sizes);

        let (new_value, array_sizes) = array_insert(&cbor, &cbor_path, 2, vec![&item1, &item2]);
        assert_eq!(
            r#"["a","b","d","e","c"]"#,
            cbor_to_diag(&new_value.unwrap())
        );
        assert_eq!(vec![RedisValue::Integer(5)], array_sizes);

        let (new_value, array_sizes) = array_insert(&cbor, &cbor_path, -1, vec![&item1, &item2]);
        assert_eq!(
            r#"["a","b","d","e","c"]"#,
            cbor_to_diag(&new_value.unwrap())
        );
        assert_eq!(vec![RedisValue::Integer(5)], array_sizes);

        let (new_value, array_sizes) = array_insert(&cbor, &cbor_path, -3, vec![&item1, &item2]);
        assert_eq!(
            r#"["d","e","a","b","c"]"#,
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
        let (new_value, array_sizes) = array_insert(&cbor, &cbor_path, 2, vec![&item1, &item2]);

        assert_eq!(
            r#"{"foo":["a","b","d","e","c"]}"#,
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
        let (new_value, array_sizes) = array_insert(&cbor, &cbor_path, 2, vec![&item1, &item2]);

        assert_eq!(
            r#"{"foo":["a","b","d","e","c"],"bar":[1,2,"d","e",3,4]}"#,
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
        let (new_value, array_sizes) = array_insert(&cbor, &cbor_path, 2, vec![&item1, &item2]);

        assert_eq!(
            r#"{"foo":12,"bar":[1,2,"d","e",3]}"#,
            cbor_to_diag(&new_value.unwrap())
        );
        assert_eq!(vec![RedisValue::Null, RedisValue::Integer(5)], array_sizes);
    }
}

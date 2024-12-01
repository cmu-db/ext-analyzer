use crate::util::{apply_changes, normalize_index, CborKeyWritable, CborPathExt, NextArgExt};
use cbor_data::{Cbor, CborBuilder, CborOwned, ItemKind, Writer};
use cborpath::CborPath;
use redis_module::{Context, RedisError, RedisResult, RedisString, RedisValue};
use std::borrow::Cow;

///
/// CBOR.ARRTRIM key [path [start [stop]]]
///
pub fn cbor_arr_trim(ctx: &Context, args: Vec<RedisString>) -> RedisResult {
    let mut args = args.iter().skip(1);

    let key_name = args.next_arg()?;
    let cbor_path = match args.next_arg() {
        Ok(cbor_path) => CborPath::from_arg(cbor_path)?,
        Err(_) => CborPath::root(),
    };
    let start = args.next().map_or(Ok(0), |v| v.parse_integer())? as isize;
    let stop = args.next().map_or(Ok(-1), |v| v.parse_integer())? as isize;

    let key = ctx.open_key_writable(key_name);

    let Some(existing) = key.get_cbor_value()? else {
        return Err(RedisError::nonexistent_key());
    };

    let (new_value, array_sizes) = array_trim(existing, &cbor_path, start, stop);

    if let Some(new_value) = new_value {
        key.set_cbor_value(new_value)?;
        apply_changes(ctx, "cbor.arrpop", key_name)?;
    }

    Ok(array_sizes.into())
}

fn array_trim(
    existing: &cbor_data::CborOwned,
    cbor_path: &CborPath,
    start: isize,
    stop: isize,
) -> (Option<CborOwned>, Vec<RedisValue>) {
    let mut array_sizes = Vec::<RedisValue>::new();

    let new_value = cbor_path
        .write(existing, |old_value| {
            if let ItemKind::Array(array) = old_value.kind() {
                let new_value = match (start, stop, array.size()) {
                    (start, stop, _) if start >= 0 && stop >= 0 && stop > start => {
                        write_array(array, start as usize, stop as usize, &mut array_sizes)
                    }
                    (start, stop, Some(len)) => {
                        let start = normalize_index(start, len as usize);
                        let stop = normalize_index(stop, len as usize);
                        write_array(array, start, stop, &mut array_sizes)
                    }
                    (start, stop, None) => {
                        let array = array.collect::<Vec<_>>();
                        let start = normalize_index(start, array.len());
                        let stop = normalize_index(stop, array.len());
                        write_array(array.into_iter(), start, stop, &mut array_sizes)
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
    existing_items: I,
    start: usize,
    stop: usize,
    array_sizes: &mut Vec<RedisValue>,
) -> CborOwned
where
    I: Iterator<Item = &'a Cbor>,
{
    CborBuilder::new().write_array(None, |builder| {
        if start > stop {
            array_sizes.push(RedisValue::Integer(0));
        } else {
            for item in existing_items
                .into_iter()
                .skip(start)
                .take(stop - start + 1)
            {
                builder.write_item(item);
            }

            array_sizes.push(RedisValue::Integer((stop - start + 1) as i64));
        }
    })
}

#[cfg(test)]
mod tests {
    use super::array_trim;
    use crate::util::{cbor_to_diag, diag_to_cbor};
    use cborpath::CborPath;
    use redis_module::RedisValue;

    #[test]
    fn simple_array() {
        let cbor = diag_to_cbor(r#"["a","b","c","d","e"]"#);

        // ["$"]
        let cbor_path = CborPath::root();

        let (new_value, array_sizes) = array_trim(&cbor, &cbor_path, 1, 3);
        assert_eq!(r#"["b","c","d"]"#, cbor_to_diag(&new_value.unwrap()));
        assert_eq!(vec![RedisValue::Integer(3)], array_sizes);

        let (new_value, array_sizes) = array_trim(&cbor, &cbor_path, -4, -2);
        assert_eq!(r#"["b","c","d"]"#, cbor_to_diag(&new_value.unwrap()));
        assert_eq!(vec![RedisValue::Integer(3)], array_sizes);

        let (new_value, array_sizes) = array_trim(&cbor, &cbor_path, 2, 1);
        assert_eq!(r#"[]"#, cbor_to_diag(&new_value.unwrap()));
        assert_eq!(vec![RedisValue::Integer(0)], array_sizes);

        let (new_value, array_sizes) = array_trim(&cbor, &cbor_path, 4, 4);
        assert_eq!(r#"["e"]"#, cbor_to_diag(&new_value.unwrap()));
        assert_eq!(vec![RedisValue::Integer(1)], array_sizes);
    }

    #[test]
    fn deep_array() {
        let cbor = diag_to_cbor(r#"{"foo":["a","b","c","d","e"]}"#);

        // ["$", "foo"]
        let cbor_path = CborPath::builder().key("foo").build();
        let (new_value, array_sizes) = array_trim(&cbor, &cbor_path, 1, 3);

        assert_eq!(
            r#"{"foo":["b","c","d"]}"#,
            cbor_to_diag(&new_value.unwrap())
        );
        assert_eq!(vec![RedisValue::Integer(3)], array_sizes);
    }

    #[test]
    fn multiple_arrays() {
        let cbor = diag_to_cbor(r#"{"foo":["a","b","c","d","e"],"bar":[1,2,3,4,5]}"#);

        // ["$", {"*":1}]
        let cbor_path = CborPath::builder().wildcard().build();
        let (new_value, array_sizes) = array_trim(&cbor, &cbor_path, 1, 3);

        assert_eq!(
            r#"{"foo":["b","c","d"],"bar":[2,3,4]}"#,
            cbor_to_diag(&new_value.unwrap())
        );
        assert_eq!(
            vec![RedisValue::Integer(3), RedisValue::Integer(3)],
            array_sizes
        );
    }

    #[test]
    fn not_an_array() {
        let cbor = diag_to_cbor(r#"{"foo":12,"bar":[1,2,3,4,5]}"#);

        // ["$", {"*":1}]
        let cbor_path = CborPath::builder().wildcard().build();
        let (new_value, array_sizes) = array_trim(&cbor, &cbor_path, 1, 3);

        assert_eq!(
            r#"{"foo":12,"bar":[2,3,4]}"#,
            cbor_to_diag(&new_value.unwrap())
        );
        assert_eq!(vec![RedisValue::Null, RedisValue::Integer(3)], array_sizes);
    }
}

use std::borrow::Cow;

use crate::util::{apply_changes, normalize_index, CborKeyWritable, CborPathExt, NextArgExt};
use cbor_data::{Cbor, CborBuilder, CborOwned, ItemKind, Writer};
use cborpath::CborPath;
use redis_module::{Context, RedisError, RedisResult, RedisString, RedisValue};

///
/// CBOR.ARRPOP key [path [index]]
///
/// Remove and return an element from the index in the array at path in key
pub fn cbor_arr_pop(ctx: &Context, args: Vec<RedisString>) -> RedisResult {
    let mut args = args.iter().skip(1);

    let key_name = args.next_arg()?;
    let cbor_path = match args.next_arg() {
        Ok(cbor_path) => CborPath::from_arg(cbor_path)?,
        Err(_) => CborPath::root(),
    };
    let index = args.next().map_or(Ok(-1), |v| v.parse_integer())? as isize;

    let key = ctx.open_key_writable(key_name);

    let Some(existing) = key.get_cbor_value()? else {
        return Err(RedisError::nonexistent_key());
    };

    let (new_value, popped_items) = array_pop(existing, &cbor_path, index);

    if let Some(new_value) = new_value {
        key.set_cbor_value(new_value)?;
        apply_changes(ctx, "cbor.arrpop", key_name)?;
    }

    Ok(popped_items.into())
}

fn array_pop(
    existing: &CborOwned,
    cbor_path: &CborPath,
    index: isize,
) -> (Option<CborOwned>, Vec<RedisValue>) {
    let mut popped_items = Vec::<RedisValue>::new();

    let new_value = cbor_path
        .write(existing, |old_value| {
            if let ItemKind::Array(array) = old_value.kind() {
                let new_value = match (index, array.size()) {
                    (index, _) if index >= 0 => {
                        write_array(array, index as usize, &mut popped_items)
                    }
                    (index, Some(len)) => {
                        let index = normalize_index(index, len as usize);
                        write_array(array, index, &mut popped_items)
                    }
                    (index, None) => {
                        let array = array.collect::<Vec<_>>();
                        let index = normalize_index(index, array.len());
                        write_array(array.into_iter(), index, &mut popped_items)
                    }
                };

                Ok(Some(Cow::Owned(new_value)))
            } else {
                popped_items.push(RedisValue::Null);
                Ok(Some(Cow::Borrowed(old_value)))
            }
        })
        .unwrap();

    (new_value, popped_items)
}

fn write_array<'a, I>(
    mut existing_items: I,
    index: usize,
    popped_items: &mut Vec<RedisValue>,
) -> CborOwned
where
    I: Iterator<Item = &'a Cbor>,
{
    CborBuilder::new().write_array(None, |builder| {
        let mut i = 0usize;
        while i < index {
            if let Some(item) = existing_items.next() {
                i += 1;
                builder.write_item(item);
            } else {
                break;
            }
        }

        if let Some(item) = existing_items.next() {
            popped_items.push(RedisValue::StringBuffer(item.as_slice().to_vec()));
        }

        for item in existing_items {
            builder.write_item(item);
        }
    })
}

#[cfg(test)]
mod tests {
    use super::array_pop;
    use crate::util::{cbor_to_diag, diag_to_bytes, diag_to_cbor};
    use cborpath::CborPath;
    use redis_module::RedisValue;

    #[test]
    fn simple_array() {
        let cbor = diag_to_cbor(r#"["a","b","c"]"#);

        // ["$"]
        let cbor_path = CborPath::root();

        let (new_value, popped_items) = array_pop(&cbor, &cbor_path, 0);
        assert_eq!(r#"["b","c"]"#, cbor_to_diag(&new_value.unwrap()));
        assert_eq!(
            vec![RedisValue::StringBuffer(diag_to_bytes(r#""a""#))],
            popped_items
        );

        let (new_value, popped_items) = array_pop(&cbor, &cbor_path, 2);
        assert_eq!(r#"["a","b"]"#, cbor_to_diag(&new_value.unwrap()));
        assert_eq!(
            vec![RedisValue::StringBuffer(diag_to_bytes(r#""c""#))],
            popped_items
        );

        let (new_value, popped_items) = array_pop(&cbor, &cbor_path, -1);
        assert_eq!(r#"["a","b"]"#, cbor_to_diag(&new_value.unwrap()));
        assert_eq!(
            vec![RedisValue::StringBuffer(diag_to_bytes(r#""c""#))],
            popped_items
        );
    }

    #[test]
    fn deep_array() {
        let cbor = diag_to_cbor(r#"{"foo":["a","b","c"]}"#);

        // ["$", "foo"]
        let cbor_path = CborPath::builder().key("foo").build();

        let (new_value, popped_items) = array_pop(&cbor, &cbor_path, 0);
        assert_eq!(r#"{"foo":["b","c"]}"#, cbor_to_diag(&new_value.unwrap()));
        assert_eq!(
            vec![RedisValue::StringBuffer(diag_to_bytes(r#""a""#))],
            popped_items
        );

        let (new_value, popped_items) = array_pop(&cbor, &cbor_path, 2);
        assert_eq!(r#"{"foo":["a","b"]}"#, cbor_to_diag(&new_value.unwrap()));
        assert_eq!(
            vec![RedisValue::StringBuffer(diag_to_bytes(r#""c""#))],
            popped_items
        );

        let (new_value, popped_items) = array_pop(&cbor, &cbor_path, -1);
        assert_eq!(r#"{"foo":["a","b"]}"#, cbor_to_diag(&new_value.unwrap()));
        assert_eq!(
            vec![RedisValue::StringBuffer(diag_to_bytes(r#""c""#))],
            popped_items
        );
    }

    #[test]
    fn multiple_arrays() {
        let cbor = diag_to_cbor(r#"{"foo":["a","b","c"],"bar":[1,2,3,4]}"#);

        // ["$", {"*":1}]
        let cbor_path = CborPath::builder().wildcard().build();

        let (new_value, popped_items) = array_pop(&cbor, &cbor_path, 2);
        assert_eq!(
            r#"{"foo":["a","b"],"bar":[1,2,4]}"#,
            cbor_to_diag(&new_value.unwrap())
        );
        assert_eq!(
            vec![
                RedisValue::StringBuffer(diag_to_bytes(r#""c""#)),
                RedisValue::StringBuffer(diag_to_bytes("3"))
            ],
            popped_items
        );
    }

    #[test]
    fn not_an_array() {
        let cbor = diag_to_cbor(r#"{"foo":12,"bar":[1,2,3]}"#);

        // ["$", {"*":1}]
        let cbor_path = CborPath::builder().wildcard().build();

        let (new_value, popped_items) = array_pop(&cbor, &cbor_path, 2);
        assert_eq!(r#"{"foo":12,"bar":[1,2]}"#, cbor_to_diag(&new_value.unwrap()));
        assert_eq!(vec![RedisValue::Null, RedisValue::StringBuffer(diag_to_bytes("3"))], popped_items);
    }
}

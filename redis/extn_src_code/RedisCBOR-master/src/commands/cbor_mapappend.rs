use crate::util::{apply_changes, CborExt, CborKeyWritable, CborPathExt, NextArgExt};
use cbor_data::{Cbor, CborBuilder, CborOwned, ItemKind, Writer};
use cborpath::CborPath;
use redis_module::{Context, RedisError, RedisResult, RedisString, RedisValue};
use std::borrow::Cow;

///
/// CBOR.MAPAPPEND key path [map_key map_value] [map_key map_value ...]
///
pub fn cbor_map_append(ctx: &Context, args: Vec<RedisString>) -> RedisResult {
    if args.len() < 5 || args.len() % 2 == 0 {
        return Err(RedisError::WrongArity);
    }

    let mut args = args.iter().skip(1);

    let key_name = args.next_arg()?;
    let path = args.next_arg()?;

    let mut key_value_pairs = Vec::<(&Cbor, &Cbor)>::with_capacity(args.len());
    while let (Some(key), Some(value)) = (args.next(), args.next()) {
        key_value_pairs.push((Cbor::from_arg(key)?, Cbor::from_arg(value)?));
    }

    let key = ctx.open_key_writable(key_name);
    let cbor_path = CborPath::from_arg(path)?;

    let Some(existing) = key.get_cbor_value()? else {
        return Err(RedisError::nonexistent_key());
    };

    let (new_value, map_sizes) = map_append(existing, &cbor_path, key_value_pairs);

    if let Some(new_value) = new_value {
        key.set_cbor_value(new_value)?;
        apply_changes(ctx, "cbor.mapappend", key_name)?;
    }

    Ok(map_sizes.into())
}

fn map_append(
    existing: &Cbor,
    cbor_path: &CborPath,
    key_value_pairs: Vec::<(&Cbor, &Cbor)>,
) -> (Option<CborOwned>, Vec<RedisValue>) {
    let mut array_sizes = Vec::<RedisValue>::new();

    let new_value = cbor_path
        .write(existing, |old_value| {
            if let ItemKind::Dict(dict) = old_value.kind() {
                Ok(Some(Cow::Owned(CborBuilder::new().write_dict(
                    None,
                    |builder| {
                        let mut size = 0i64;
                        for (key, value) in dict {
                            size += 1;
                            builder.with_cbor_key(|b| b.write_item(key), |b| b.write_item(value));
                        }
                        for (key, value) in &key_value_pairs {
                            size += 1;
                            builder.with_cbor_key(|b| b.write_item(key), |b| b.write_item(value));
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
    use super::map_append;
    use crate::util::{cbor_to_diag, diag_to_cbor};
    use cborpath::CborPath;
    use redis_module::RedisValue;

    #[test]
    fn simple() {
        let cbor = diag_to_cbor(r#"{"a":1,"b":2}"#);
        let key1 = diag_to_cbor(r#""c""#);
        let item1 = diag_to_cbor("3");
        let key2 = diag_to_cbor(r#""d""#);
        let item2 = diag_to_cbor("4");

        // ["$"]
        let cbor_path = CborPath::root();
        let (new_value, array_sizes) = map_append(&cbor, &cbor_path, vec![(&key1, &item1), (&key2, &item2)]);

        assert_eq!(
            r#"{"a":1,"b":2,"c":3,"d":4}"#,
            cbor_to_diag(&new_value.unwrap())
        );
        assert_eq!(vec![RedisValue::Integer(4)], array_sizes);
    }

    #[test]
    fn deep() {
        let cbor = diag_to_cbor(r#"{"foo":{"a":1,"b":2}}"#);
        let key1 = diag_to_cbor(r#""c""#);
        let item1 = diag_to_cbor("3");
        let key2 = diag_to_cbor(r#""d""#);
        let item2 = diag_to_cbor("4");

        // ["$", "foo"]
        let cbor_path = CborPath::builder().key("foo").build();
        let (new_value, array_sizes) = map_append(&cbor, &cbor_path, vec![(&key1, &item1), (&key2, &item2)]);

        assert_eq!(
            r#"{"foo":{"a":1,"b":2,"c":3,"d":4}}"#,
            cbor_to_diag(&new_value.unwrap())
        );
        assert_eq!(vec![RedisValue::Integer(4)], array_sizes);
    }

    #[test]
    fn multiple() {
        let cbor = diag_to_cbor(r#"{"foo":{"a":1,"b":2},"bar":{"f":1,"g":2,"h":3}}"#);
        let key1 = diag_to_cbor(r#""c""#);
        let item1 = diag_to_cbor("3");
        let key2 = diag_to_cbor(r#""d""#);
        let item2 = diag_to_cbor("4");

        // ["$", {"*":1}]
        let cbor_path = CborPath::builder().wildcard().build();
        let (new_value, array_sizes) = map_append(&cbor, &cbor_path, vec![(&key1, &item1), (&key2, &item2)]);

        assert_eq!(
            r#"{"foo":{"a":1,"b":2,"c":3,"d":4},"bar":{"f":1,"g":2,"h":3,"c":3,"d":4}}"#,
            cbor_to_diag(&new_value.unwrap())
        );
        assert_eq!(
            vec![RedisValue::Integer(4), RedisValue::Integer(5)],
            array_sizes
        );
    }

    #[test]
    fn not_a_map() {
        let cbor = diag_to_cbor(r#"{"foo":12,"bar":{"a":1,"b":2}}"#);
        let key1 = diag_to_cbor(r#""c""#);
        let item1 = diag_to_cbor("3");
        let key2 = diag_to_cbor(r#""d""#);
        let item2 = diag_to_cbor("4");


        // ["$", {"*":1}]
        let cbor_path = CborPath::builder().wildcard().build();
        let (new_value, array_sizes) = map_append(&cbor, &cbor_path, vec![(&key1, &item1), (&key2, &item2)]);

        assert_eq!(
            r#"{"foo":12,"bar":{"a":1,"b":2,"c":3,"d":4}}"#,
            cbor_to_diag(&new_value.unwrap())
        );
        assert_eq!(vec![RedisValue::Null, RedisValue::Integer(4)], array_sizes);
    }
}
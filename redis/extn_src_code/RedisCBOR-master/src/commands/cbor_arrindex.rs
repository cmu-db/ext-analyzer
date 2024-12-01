use crate::util::{normalize_index, CborExt, CborKey, CborPathExt, NextArgExt};
use cbor_data::{Cbor, CborOwned, ItemKind};
use cborpath::CborPath;
use redis_module::{Context, RedisError, RedisResult, RedisString, RedisValue};

///
/// CBOR.ARRINDEX key path value [start [stop]]
///
pub fn cbor_arr_index(ctx: &Context, args: Vec<RedisString>) -> RedisResult {
    let mut args = args.iter().skip(1);

    let key_name = args.next_arg()?;
    let path = args.next_arg()?;
    let value = args.next_arg()?;
    let start = args.next().map_or(Ok(0), |v| v.parse_integer())? as isize;
    let stop = args.next().map_or(Ok(-1), |v| v.parse_integer())? as isize;

    let cbor_path = CborPath::from_arg(path)?;
    let value = Cbor::from_arg(value)?;

    let key = ctx.open_key(key_name);
    let Some(existing) = key.get_cbor_value()? else {
        return Err(RedisError::nonexistent_key());
    };

    Ok(array_index(existing, &cbor_path, value, start, stop).into())
}

fn array_index(
    existing: &CborOwned,
    cbor_path: &CborPath,
    value: &Cbor,
    start: isize,
    stop: isize,
) -> Vec<RedisValue> {
    let results = cbor_path.read(existing);
    let mut matches = Vec::<RedisValue>::new();

    for result in results {
        if let ItemKind::Array(array) = result.kind() {
            let index: isize = match (start, stop) {
                (start, stop) if start >= 0 && stop >= 0 && stop > start => array
                    .skip(start as usize)
                    .take((stop - start) as usize)
                    .position(|item| item == value)
                    .map(|idx| (idx + start as usize) as isize)
                    .unwrap_or_else(|| -1),
                (start, stop) if start >= 0 && stop == -1 => array
                    .skip(start as usize)
                    .position(|item| item == value)
                    .map(|idx| (idx + start as usize) as isize)
                    .unwrap_or_else(|| -1),
                (start, stop)
                    if (start >= 0 && stop >= 0 || start < 0 && stop < 0) && stop <= start =>
                {
                    -1
                }
                _ => match array.size() {
                    Some(len) => {
                        let start = normalize_index(start, len as usize);
                        let stop = normalize_index(stop, len as usize);
                        array
                            .skip(start)
                            .take(stop - start)
                            .position(|item| item == value)
                            .map(|idx| (idx + start) as isize)
                            .unwrap_or_else(|| -1)
                    }
                    None => {
                        let array = array.collect::<Vec<_>>();
                        let start = normalize_index(start, array.len());
                        let stop = normalize_index(stop, array.len());
                        array
                            .into_iter()
                            .skip(start)
                            .take(stop - start)
                            .position(|item| item == value)
                            .map(|idx| (idx + start) as isize)
                            .unwrap_or_else(|| -1)
                    }
                },
            };

            matches.push(RedisValue::Integer(index as i64))
        } else {
            matches.push(RedisValue::Null);
        }
    }

    matches
}

#[cfg(test)]
mod tests {
    use super::array_index;
    use crate::util::diag_to_cbor;
    use cborpath::CborPath;
    use redis_module::RedisValue;

    #[test]
    fn simple_array() {
        let cbor = diag_to_cbor(r#"["a","b","c","d"]"#);
        let value = diag_to_cbor(r#""b""#);

        // ["$"]
        let cbor_path = CborPath::root();

        let results = array_index(&cbor, &cbor_path, &value, 0, 4);
        assert_eq!(vec![RedisValue::Integer(1)], results);

        let results = array_index(&cbor, &cbor_path, &value, 0, -2);
        assert_eq!(vec![RedisValue::Integer(1)], results);

        let results = array_index(&cbor, &cbor_path, &value, 3, 0);
        assert_eq!(vec![RedisValue::Integer(-1)], results);

        let results = array_index(&cbor, &cbor_path, &value, 4, 5);
        assert_eq!(vec![RedisValue::Integer(-1)], results);

        let results = array_index(&cbor, &cbor_path, &value, -3, -1);
        assert_eq!(vec![RedisValue::Integer(1)], results);
    }

    #[test]
    fn deep_array() {
        let cbor = diag_to_cbor(r#"{"foo":["a","b","c"]}"#);
        let value = diag_to_cbor(r#""c""#);

        // ["$", "foo"]
        let cbor_path = CborPath::builder().key("foo").build();
        let results = array_index(&cbor, &cbor_path, &value, 0, 3);

        assert_eq!(vec![RedisValue::Integer(2)], results);
    }

    #[test]
    fn multiple_arrays() {
        let cbor = diag_to_cbor(r#"{"foo":["a","b","c"],"bar":["c","b","a"]}"#);
        let value = diag_to_cbor(r#""c""#);

        // ["$", {"*":1}]
        let cbor_path = CborPath::builder().wildcard().build();
        let results = array_index(&cbor, &cbor_path, &value, 0, 4);

        assert_eq!(
            vec![RedisValue::Integer(2), RedisValue::Integer(0)],
            results
        );
    }

    #[test]
    fn not_an_array_not_found() {
        let cbor = diag_to_cbor(r#"{"foo":12,"bar":["a","b","c"]}"#);
        let value = diag_to_cbor(r#""d""#);

        // ["$", {"*":1}]
        let cbor_path = CborPath::builder().wildcard().build();
        let results = array_index(&cbor, &cbor_path, &value, 0, 4);

        assert_eq!(vec![RedisValue::Null, RedisValue::Integer(-1)], results);
    }
}

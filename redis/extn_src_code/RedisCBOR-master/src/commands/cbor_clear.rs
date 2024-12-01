use std::borrow::Cow;

use crate::util::{apply_changes, CborKeyWritable, CborPathExt, NextArgExt};
use cbor_data::{CborBuilder, CborOwned, ItemKind, Writer};
use cborpath::CborPath;
use redis_module::{Context, RedisError, RedisResult, RedisString, RedisValue};

///
/// CBOR.CLEAR key [path]
///
/// Clear container values (arrays/objects) and set numeric values to 0 (integers/floats)
pub fn cbor_clear(ctx: &Context, args: Vec<RedisString>) -> RedisResult {
    let mut args = args.iter().skip(1);

    let key_name = args.next_arg()?;
    let cbor_path = match args.next_arg() {
        Ok(cbor_path) => CborPath::from_arg(cbor_path)?,
        Err(_) => CborPath::root(),
    };

    let key = ctx.open_key_writable(key_name);

    let Some(existing) = key.get_cbor_value()? else {
        return Err(RedisError::nonexistent_key());
    };

    let (new_value, num_cleared) = clear(existing, &cbor_path);

    if let Some(new_value) = new_value {
        key.set_cbor_value(new_value)?;
        apply_changes(ctx, "cbor.arrpop", key_name)?;
    }

    Ok(RedisValue::Integer(num_cleared as i64))
}

fn clear(existing: &CborOwned, cbor_path: &CborPath) -> (Option<CborOwned>, usize) {
    let mut num_cleared = 0;
    let new_value = cbor_path
        .write(existing, |old_value| {
            let new_value = match old_value.kind() {
                ItemKind::Pos(_) | ItemKind::Neg(_) => {
                    num_cleared += 1;
                    CborBuilder::new().write_pos(0, None)
                }
                ItemKind::Float(_) => {
                    num_cleared += 1;
                    CborBuilder::new().write_lit(cbor_data::Literal::L2(0), None)
                }
                ItemKind::Str(_)
                | ItemKind::Bytes(_)
                | ItemKind::Bool(_)
                | ItemKind::Null
                | ItemKind::Undefined
                | ItemKind::Simple(_) => CborBuilder::new().write_item(old_value),
                ItemKind::Array(_) => {
                    num_cleared += 1;
                    CborBuilder::new().write_array(None, |_| ())
                }
                ItemKind::Dict(_) => {
                    num_cleared += 1;
                    CborBuilder::new().write_dict(None, |_| ())
                }
            };

            Ok(Some(Cow::Owned(new_value)))
        })
        .unwrap();

    (new_value, num_cleared)
}

#[cfg(test)]
mod tests {
    use super::clear;
    use crate::util::{cbor_to_diag, diag_to_cbor};
    use cborpath::CborPath;

    #[test]
    fn clear_values() {
        let cbor = diag_to_cbor(
            r#"{"obj":{"a":1, "b":2}, "arr":[1,2,3], "str": "foo", "bool": true, "int": 42, "float": 3.14}"#,
        );

        let cbor_path = CborPath::builder().wildcard().build();
        let (new_value, num_cleared_values) = clear(&cbor, &cbor_path);
        let new_value = new_value.unwrap();

        assert_eq!(
            r#"{"obj":{},"arr":[],"str":"foo","bool":true,"int":0,"float":0.0_1}"#,
            cbor_to_diag(&new_value)
        );
        assert_eq!(4, num_cleared_values);
    }
}

use super::num_operation::{num_operation, Number};
use crate::util::{apply_changes, CborExt, CborKeyWritable, CborPathExt, NextArgExt};
use cbor_data::{Cbor, CborOwned};
use cborpath::CborPath;
use redis_module::{Context, RedisError, RedisResult, RedisString, RedisValue};

///
/// CBOR.NUMINCRBY key path value
///
/// Increment the number value stored at path by number
pub fn cbor_num_incr_by(ctx: &Context, args: Vec<RedisString>) -> RedisResult {
    let mut args = args.iter().skip(1);

    let key_name = args.next_arg()?;
    let path = args.next_arg()?;
    let value = args.next_arg()?;

    let key = ctx.open_key_writable(key_name);
    let cbor_path = CborPath::from_arg(path)?;
    let value = Cbor::from_arg(value)?;

    let Some(existing) = key.get_cbor_value()? else {
        return Err(RedisError::nonexistent_key());
    };

    let (new_value, new_nums) = num_incr_by(existing, &cbor_path, value);

    if let Some(new_value) = new_value {
        key.set_cbor_value(new_value)?;
        apply_changes(ctx, "cbor.numincrby", key_name)?;
    }

    Ok(new_nums.into())
}

fn num_incr_by(
    existing: &CborOwned,
    cbor_path: &CborPath,
    value: &Cbor,
) -> (Option<CborOwned>, Vec<RedisValue>) {
    num_operation(existing, cbor_path, value, |v1, v2| match (v1, v2) {
        (Number::Signed(v1), Number::Signed(v2)) => Some(Number::Signed(v1 + v2)),
        (Number::Signed(v1), Number::Unsigned(v2)) => Some(Number::Signed(v1 + v2 as i64)),
        (Number::Signed(v1), Number::Float(v2)) => Some(Number::Float(v1 as f64 + v2)),
        (Number::Unsigned(v1), Number::Signed(v2)) => Some(Number::Signed(v1 as i64 + v2)),
        (Number::Unsigned(v1), Number::Unsigned(v2)) => Some(Number::Unsigned(v1 + v2)),
        (Number::Unsigned(v1), Number::Float(v2)) => Some(Number::Float(v1 as f64 + v2)),
        (Number::Float(v1), Number::Signed(v2)) => Some(Number::Float(v1 + v2 as f64)),
        (Number::Float(v1), Number::Unsigned(v2)) => Some(Number::Float(v1 + v2 as f64)),
        (Number::Float(v1), Number::Float(v2)) => Some(Number::Float(v1 + v2)),
    })
}

#[cfg(test)]
mod tests {
    use super::num_incr_by;
    use crate::util::{cbor_to_diag, diag_to_bytes, diag_to_cbor};
    use cborpath::{builder::segment, CborPath};
    use redis_module::RedisValue;

    #[test]
    fn integer() {
        let cbor = diag_to_cbor("[2,-2]");
        let cbor_path = CborPath::builder().wildcard().build();

        let value = diag_to_cbor("1");
        let (new_value, nem_nums) = num_incr_by(&cbor, &cbor_path, &value);
        assert_eq!("[3,-1]", cbor_to_diag(&new_value.unwrap()));
        assert_eq!(
            vec![
                RedisValue::StringBuffer(diag_to_bytes("3")),
                RedisValue::StringBuffer(diag_to_bytes("-1")),
            ],
            nem_nums
        );

        let value = diag_to_cbor("-1");
        let (new_value, nem_nums) = num_incr_by(&cbor, &cbor_path, &value);
        assert_eq!("[1,-3]", cbor_to_diag(&new_value.unwrap()));
        assert_eq!(
            vec![
                RedisValue::StringBuffer(diag_to_bytes("1")),
                RedisValue::StringBuffer(diag_to_bytes("-3")),
            ],
            nem_nums
        );
    }

    #[test]
    fn float() {
        let cbor = diag_to_cbor("[12.0,12]");
        let cbor_path = CborPath::builder().wildcard().build();

        let value = diag_to_cbor("2.0");
        let (new_value, nem_nums) = num_incr_by(&cbor, &cbor_path, &value);
        assert_eq!(diag_to_cbor("[14.0,14.0]"), new_value.unwrap());
        assert_eq!(
            vec![
                RedisValue::StringBuffer(diag_to_bytes("14.0")),
                RedisValue::StringBuffer(diag_to_bytes("14.0")),
            ],
            nem_nums
        );

        let value = diag_to_cbor("2");
        let (new_value, nem_nums) = num_incr_by(&cbor, &cbor_path, &value);
        assert_eq!(diag_to_cbor("[14.0,14]"), new_value.unwrap());
        assert_eq!(
            vec![
                RedisValue::StringBuffer(diag_to_bytes("14.0")),
                RedisValue::StringBuffer(diag_to_bytes("14")),
            ],
            nem_nums
        );

        let value = diag_to_cbor("-2");
        let (new_value, nem_nums) = num_incr_by(&cbor, &cbor_path, &value);
        assert_eq!(diag_to_cbor("[10.0,10]"), new_value.unwrap());
        assert_eq!(
            vec![
                RedisValue::StringBuffer(diag_to_bytes("10.0")),
                RedisValue::StringBuffer(diag_to_bytes("10")),
            ],
            nem_nums
        );
    }

    #[test]
    fn multiple() {
        let cbor = diag_to_cbor(r#"{"a":"b","b":[{"a":2}, {"a":5}, {"a":"c"}]}"#);
        let cbor_path = CborPath::builder().descendant(segment().key("a")).build();
        let value = diag_to_cbor("2");

        let (new_value, nem_nums) = num_incr_by(&cbor, &cbor_path, &value);
        assert_eq!(
            r#"{"a":"b","b":[{"a":4},{"a":7},{"a":"c"}]}"#,
            cbor_to_diag(&new_value.unwrap())
        );
        assert_eq!(
            vec![
                RedisValue::Null,
                RedisValue::StringBuffer(diag_to_bytes("4")),
                RedisValue::StringBuffer(diag_to_bytes("7")),
                RedisValue::Null
            ],
            nem_nums
        );
    }
}

use crate::util::{CborKey, CborPathExt, NextArgExt};
use cbor_data::{CborBuilder, CborOwned, Writer};
use cborpath::CborPath;
use redis_module::{Context, RedisResult, RedisString, RedisValue};

///
/// CBOR.GET key [path]
///
/// Return the value at path in CBOR serialized form
pub fn cbor_get(ctx: &Context, args: Vec<RedisString>) -> RedisResult {
    let mut args = args.iter().skip(1);

    let key = args.next_arg()?;
    let cbor_path = match args.next_arg() {
        Ok(cbor_path) => CborPath::from_arg(cbor_path)?,
        Err(_) => CborPath::root(),
    };

    let key = ctx.open_key(key);
    let existing = key.get_cbor_value()?;

    match get(existing, &cbor_path) {
        Some(value) => Ok(RedisValue::StringBuffer(value.into_vec())),
        None => Ok(RedisValue::Null),
    }
}

fn get(existing: Option<&CborOwned>, cbor_path: &CborPath) -> Option<CborOwned> {
    existing.map(|value| {
        let results = cbor_path.read(value);
        CborBuilder::new().write_array(None, |builder| {
            for result in results {
                builder.write_item(result);
            }
        })
    })
}

#[cfg(test)]
mod tests {
    use super::get;
    use crate::util::{cbor_to_diag, diag_to_cbor};
    use cborpath::{builder::segment, CborPath};

    #[test]
    fn root() {
        let result = get(Some(&diag_to_cbor("[1,2,3]")), &CborPath::root());
        assert!(result.is_some());
        assert_eq!("[[1,2,3]]", cbor_to_diag(&result.unwrap()));
    }

    #[test]
    fn multiple_matches() {
        let result = get(
            Some(&diag_to_cbor("[1,2,3]")),
            &CborPath::builder()
                .child(segment().index(0).index(2))
                .build(),
        );
        assert!(result.is_some());
        assert_eq!("[1,3]", cbor_to_diag(&result.unwrap()));
    }

    #[test]
    fn no_match() {
        let result = get(
            Some(&diag_to_cbor("[1,2,3]")),
            &CborPath::builder().child(segment().index(3)).build(),
        );
        assert!(result.is_some());
        assert_eq!("[]", cbor_to_diag(&result.unwrap()));
    }

    #[test]
    fn not_found() {
        let result = get(None, &CborPath::root());
        assert!(result.is_none());
    }
}

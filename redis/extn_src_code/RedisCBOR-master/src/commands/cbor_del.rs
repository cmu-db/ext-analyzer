use crate::util::{apply_changes, CborKeyWritable, CborPathExt, NextArgExt};
use cbor_data::CborOwned;
use cborpath::CborPath;
use redis_module::{Context, RedisError, RedisResult, RedisString, RedisValue};

///
/// CBOR.DEL key [path]
///
/// Delete values at path in key
pub fn cbor_del(ctx: &Context, args: Vec<RedisString>) -> RedisResult {
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

    let (new_value, num_deleted) = del(existing, &cbor_path);

    if let Some(new_value) = new_value {
        key.set_cbor_value(new_value)?;
        apply_changes(ctx, "cbor.arrpop", key_name)?;
    }

    Ok(RedisValue::Integer(num_deleted as i64))
}

fn del(existing: &cbor_data::CborOwned, cbor_path: &CborPath) -> (Option<CborOwned>, usize) {
    let mut num_deleted = 0;
    let new_value = cbor_path
        .write(existing, |_| {
            num_deleted += 1;
            Ok(None)
        })
        .unwrap();

    (new_value, num_deleted)
}

#[cfg(test)]
mod tests {
    use super::del;
    use crate::util::{cbor_to_diag, diag_to_cbor};
    use cborpath::CborPath;

    #[test]
    fn simple_array() {
        let cbor = diag_to_cbor(r#"["a","b","c"]"#);
    
        let cbor_path = CborPath::builder().index(1).build();
        let (new_value, num_deleted) = del(&cbor, &cbor_path);
        let new_value = new_value.unwrap();
    
        assert_eq!(r#"["a","c"]"#, cbor_to_diag(&new_value));
        assert_eq!(1, num_deleted);
    }

    #[test]
    fn deep_array() {
        let cbor = diag_to_cbor(r#"{"foo":["a","b","c"]}"#);
    
        let cbor_path = CborPath::builder().key("foo").index(1).build();
        let (new_value, num_deleted) = del(&cbor, &cbor_path);
        let new_value = new_value.unwrap();
    
        assert_eq!(r#"{"foo":["a","c"]}"#, cbor_to_diag(&new_value));
        assert_eq!(1, num_deleted);
    }

    #[test]
    fn simple_map() {
        let cbor = diag_to_cbor(r#"{"a":1,"b":2}"#);
    
        let cbor_path = CborPath::builder().key("b").build();
        let (new_value, num_deleted) = del(&cbor, &cbor_path);
        let new_value = new_value.unwrap();
    
        assert_eq!(r#"{"a":1}"#, cbor_to_diag(&new_value));
        assert_eq!(1, num_deleted);
    }

    #[test]
    fn deep_map() {
        let cbor = diag_to_cbor(r#"{"foo":{"a":1,"b":2}}"#);
    
        let cbor_path = CborPath::builder().key("foo").key("b").build();
        let (new_value, num_deleted) = del(&cbor, &cbor_path);
        let new_value = new_value.unwrap();
    
        assert_eq!(r#"{"foo":{"a":1}}"#, cbor_to_diag(&new_value));
        assert_eq!(1, num_deleted);
    }

    #[test]
    fn map_as_value() {
        let cbor = diag_to_cbor(r#"{"foo":{"a":{"b":1},"c":2}}"#);
    
        let cbor_path = CborPath::builder().key("foo").key("a").build();
        let (new_value, num_deleted) = del(&cbor, &cbor_path);
        let new_value = new_value.unwrap();
    
        assert_eq!(r#"{"foo":{"c":2}}"#, cbor_to_diag(&new_value));
        assert_eq!(1, num_deleted);
    }

    #[test]
    fn array_as_value() {
        let cbor = diag_to_cbor(r#"{"foo":{"a":[1,2,3],"c":2}}"#);
    
        let cbor_path = CborPath::builder().key("foo").key("a").build();
        let (new_value, num_deleted) = del(&cbor, &cbor_path);
        let new_value = new_value.unwrap();
    
        assert_eq!(r#"{"foo":{"c":2}}"#, cbor_to_diag(&new_value));
        assert_eq!(1, num_deleted);
    }

    #[test]
    fn no_match() {
        let cbor = diag_to_cbor(r#"["a","b","c"]"#);
    
        let cbor_path = CborPath::builder().index(3).build();
        let (new_value, num_deleted) = del(&cbor, &cbor_path);
    
        assert!(new_value.is_none());
        assert_eq!(0, num_deleted);
    }
}
    
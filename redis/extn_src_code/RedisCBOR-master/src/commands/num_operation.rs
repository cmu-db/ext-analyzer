use cbor_data::{Cbor, CborOwned, ItemKind};
use cborpath::CborPath;
use redis_module::RedisValue;
use std::borrow::Cow;

#[derive(Clone, Copy)]
pub enum Number {
    Signed(i64),
    Unsigned(u64),
    Float(f64),
}

impl TryFrom<&Cbor> for Number {
    type Error = ();

    #[inline]
    fn try_from(value: &Cbor) -> Result<Self, Self::Error> {
        match value.kind() {
            ItemKind::Pos(value) => Ok(Number::Unsigned(value)),
            ItemKind::Neg(value) => Ok(Number::Signed(-1 - (value as i64))),
            ItemKind::Float(value) => Ok(Number::Float(value)),
            _ => Err(()),
        }
    }
}

impl From<Number> for CborOwned {
    #[inline]
    fn from(num: Number) -> Self {
        match num {
            Number::Signed(value) => cborpath::builder::IntoCborOwned::into(value),
            Number::Unsigned(value) => cborpath::builder::IntoCborOwned::into(value),
            Number::Float(value) => cborpath::builder::IntoCborOwned::into(value),
        }
    }
}

pub fn num_operation<F>(
    existing: &CborOwned,
    cbor_path: &CborPath,
    value: &Cbor,
    mut operation: F,
) -> (Option<CborOwned>, Vec<RedisValue>)
where
    F: FnMut(Number, Number) -> Option<Number>,
{
    let mut new_nums = Vec::<RedisValue>::new();

    let new_value = cbor_path
        .write(existing, |old_value| {
            if let (Ok(v1), Ok(v2)) = (old_value.try_into(), value.try_into()) {
                if let Some(result) = operation(v1, v2) {
                    let result: CborOwned = result.into();
                    new_nums.push(RedisValue::StringBuffer(result.clone().into_vec()));
                    return Ok(Some(Cow::Owned(result)));
                }
            }
            new_nums.push(RedisValue::Null);
            Ok(Some(Cow::Borrowed(old_value)))
        })
        .unwrap();

    (new_value, new_nums)
}

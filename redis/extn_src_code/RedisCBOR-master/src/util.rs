use crate::redis_cbor_type::REDIS_CBOR_TYPE;
use cbor_data::Cbor;
use cbor_data::CborOwned;
#[cfg(test)]
use cbor_diag::{parse_bytes, parse_diag};
use cborpath::CborPath;
use redis_module::{
    key::{RedisKey, RedisKeyWritable},
    Context, NotifyEvent, RedisError, RedisString, Status,
};

pub fn apply_changes(
    ctx: &Context,
    command: &str,
    key_name: &RedisString,
) -> Result<(), RedisError> {
    if ctx.notify_keyspace_event(NotifyEvent::MODULE, command, key_name) != Status::Ok {
        Err(RedisError::Str("failed notify key space event"))
    } else {
        ctx.replicate_verbatim();
        Ok(())
    }
}

pub trait CborKey {
    fn get_cbor_value(&self) -> Result<Option<&CborOwned>, RedisError>;
}

pub trait CborKeyWritable {
    fn get_cbor_value(&self) -> Result<Option<&CborOwned>, RedisError>;
    fn set_cbor_value(&self, value: CborOwned) -> Result<(), RedisError>;
}

impl CborKey for RedisKey {
    #[inline]
    fn get_cbor_value(&self) -> Result<Option<&CborOwned>, RedisError> {
        self.get_value::<CborOwned>(&REDIS_CBOR_TYPE)
    }
}

impl CborKeyWritable for RedisKeyWritable {
    #[inline]
    fn get_cbor_value(&self) -> Result<Option<&CborOwned>, RedisError> {
        match self.get_value::<CborOwned>(&REDIS_CBOR_TYPE) {
            Ok(Some(result)) => Ok(Some(result)),
            Ok(None) => Ok(None),
            Err(e) => Err(e),
        }
    }

    #[inline]
    fn set_cbor_value(&self, value: CborOwned) -> Result<(), RedisError> {
        self.set_value(&REDIS_CBOR_TYPE, value)
    }
}

pub trait CborPathExt: Sized {
    fn from_arg(arg: &RedisString) -> Result<Self, RedisError>;
}

impl CborPathExt for CborPath {
    #[inline]
    fn from_arg(arg: &RedisString) -> Result<Self, RedisError> {
        CborPath::from_bytes(arg.as_slice()).map_err(|_| RedisError::Str("ERR Invalid CBORPath"))
    }
}

pub trait CborExt {
    fn from_arg(arg: &RedisString) -> Result<&Cbor, RedisError>;
}

impl CborExt for Cbor {
    #[inline]
    fn from_arg(arg: &RedisString) -> Result<&Cbor, RedisError> {
        Cbor::checked(arg.as_slice()).map_err(|_| RedisError::Str("ERR Invalid CBOR value"))
    }
}

pub trait CborOwnedExt {
    fn mem_usage(&self) -> usize;
}

impl CborOwnedExt for CborOwned {
    #[inline]
    fn mem_usage(&self) -> usize {
        // CborOwned definition: pub struct CborOwned(SmallVec<[u8; 16]>);
        let len = self.as_slice().len();
        let size_of_struct = std::mem::size_of::<CborOwned>();
        if len > 16 {
            // allocated buffer
            size_of_struct + len
        } else {
            // in-place buffer
            size_of_struct
        }
    }
}

pub trait NextArgExt<'a> {
    fn next_arg(&mut self) -> Result<&'a RedisString, RedisError>;
    fn next_i64(&mut self) -> Result<i64, RedisError>;
    fn next_str(&mut self) -> Result<&'a str, RedisError>;
}

impl<'a, T> NextArgExt<'a> for T
where
    T: Iterator<Item = &'a RedisString>,
{
    #[inline]
    fn next_arg(&mut self) -> Result<&'a RedisString, RedisError> {
        self.next().ok_or(RedisError::WrongArity)
    }

    #[inline]
    fn next_i64(&mut self) -> Result<i64, RedisError> {
        self.next()
            .map_or(Err(RedisError::WrongArity), |v| v.parse_integer())
    }

    #[inline]
    fn next_str(&mut self) -> Result<&'a str, RedisError> {
        self.next()
            .map_or(Err(RedisError::WrongArity), |v| v.try_as_str())
    }
}

#[inline]
pub fn normalize_index(i: isize, len: usize) -> usize {
    if i >= 0 {
        (i as usize).min(len - 1)
    } else {
        0.max((len as isize + i) as usize)
    }
}

#[cfg(test)]
pub fn diag_to_cbor(cbor_diag_str: &str) -> CborOwned {
    let buf = diag_to_bytes(cbor_diag_str);
    CborOwned::canonical(buf).unwrap()
}

#[cfg(test)]
pub fn diag_to_bytes(cbor_diag_str: &str) -> Vec<u8> {
    parse_diag(cbor_diag_str).unwrap().to_bytes()
}

#[cfg(test)]
pub fn cbor_to_diag(cbor: &Cbor) -> String {
    bytes_to_diag(cbor.as_ref())
}

#[cfg(test)]
pub fn bytes_to_diag(cbor: &[u8]) -> String {
    parse_bytes(cbor).unwrap().to_diag()
}

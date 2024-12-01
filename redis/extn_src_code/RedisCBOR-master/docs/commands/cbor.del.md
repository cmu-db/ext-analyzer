# CBOR.DEL

### Syntax
```bash
CBOR.DEL key [path]
```

Delete a value at `path` in `key`

## Required arguments

### key
the key to modify.

## Optional arguments

### value
the CBORPath to specify. 

Default is root `"\x81\x61$"` (`["$"]`). Nonexisting paths are ignored.

Deleting an object's root is equivalent to deleting the key from Redis.

## Return

CBOR.DEL returns an integer reply specified as the number of paths deleted (0 or more).
For more information about replies, see [Redis serialization protocol specification](/docs/reference/protocol-spec).

## Examples

Create a CBOR document.
```bash
# path: ["$"]
# value: {"a": 1, "nested": {"a": 2, "b": 3}}
redis> CBOR.SET key "\x81\x61$" "\xa2\x61a\x01\x66nested\xa2\x61a\x02\x61b\x03"
OK
```

Delete specified values.
```bash
# path: ["$", {"..": "a"}]
redis> CBOR.DEL key "\x82\x61$\xa1\x62..\x61a"
(integer) 2
```

Get the updated document.
```bash
# path: ["$"]
# result: [{\"nested\":{\"b\":3}}]
redis> CBOR.GET key "\x81\x61$"
"\x81\xa1fnested\xa1ab\x03"
```

## See also

[`CBOR.GET`](cbor.get.md) | [`CBOR.MGET`](cbor.mget.md) | [`CBOR.SET`](cbor.set.md)




# CBOR.MGET

### Syntax
```bash
CBOR.MGET key [key ...] path
```

Return the values at `path` from multiple `key` arguments

## Required arguments

### key
the key(s) to parse. Returns `null` for nonexistent keys.

### path
is CBORPath to specify. Returns `null` for nonexistent paths.

## Return

CBOR.MGET returns an array of bulk string replies specified as the CBOR serialization of the value at each key's path.
For more information about replies, see [Redis serialization protocol specification](/docs/reference/protocol-spec).

## Examples

Create two CBOR documents.
```bash
# path: ["$"]
# value: {"a":1, "b": 2, "nested": {"a": 3}, "c": null}
redis> CBOR.SET key1 "\x81\x61$" "\xa4\x61a\x01\x61b\x02\x66nested\xa1\x61a\x03\x61c\xf6"
OK
# path: ["$"]
# value: {"a":4, "b": 5, "nested": {"a": 6}, "c": null}
redis> CBOR.SET key2 "\x81\x61$" "\xa4\x61a\x04\x61b\x05\x66nested\xa1\x61a\x06\x61c\xf6"
OK
```

Get values from all arguments in the documents.
```bash
redis> CBOR.MGET key1 key2 "\x82\x61$\xa1\x62..\x61a"
# path: ["$", {"..": "a"}]
# results: [1,3] and [4,6]
1) "\x82\x01\x03"
2) "\x82\x04\x06"
```

## See also

[`CBOR.DEL`](cbor.del.md) | [`CBOR.GET`](cbor.get.md) | [`CBOR.MGET`](cbor.mget.md) | [`CBOR.SET`](cbor.set.md)
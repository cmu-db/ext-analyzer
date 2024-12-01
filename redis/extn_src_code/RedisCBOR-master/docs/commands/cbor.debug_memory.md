# CBOR.DEBUG MEMORY

### Syntax
```bash
CBOR.DEBUG MEMORY key
```

Report a value's memory usage in bytes in `key`

## Required arguments

### key
the key to parse.

## Return

CBOR.DEBUG MEMORY returns an integer reply specified as the value size in bytes.
For more information about replies, see [Redis serialization protocol specification](/docs/reference/protocol-spec).

## Examples

Create a CBOR document.
```bash
# path: ["$"]
# value: {"obj":{"a":1, "b":2}, "arr":[1,2,3], "str": "foo", "bool": true, "int": 42, "float": 3.14}
redis> CBOR.SET key "\x81\x61$" "\xa6\x63obj\xa2\x61a\x01\x61b\x02\x63arr\x83\x01\x02\x03\x63str\x63foo\x64bool\xf5\x63int\x18\x2a\x65float\xfb\x40\x09\x1e\xb8\x51\xeb\x85\x1f"
OK
```

Get the values' memory usage in bytes.
```bash
redis> CBOR.DEBUG MEMORY key
(integer) 87
```

## See also

[`CBOR.SET`](cbor.set.md)

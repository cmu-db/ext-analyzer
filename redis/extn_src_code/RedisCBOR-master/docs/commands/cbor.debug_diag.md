# CBOR.DEBUG DIAG

### Syntax
```bash
CBOR.DEBUG DIAG key
```

Display CBOR document in `key` in [CBOR Diagnostic Notation](https://datatracker.ietf.org/doc/html/rfc8949#section-8).

## Required arguments

### key
the key to parse.

## Return

CBOR.DEBUG DIAG returns a bulk string containing the 
[CBOR Diagnostic Notation](https://datatracker.ietf.org/doc/html/rfc8949#section-8).
For more information about replies, see [Redis serialization protocol specification](/docs/reference/protocol-spec).

## Examples

Create a CBOR document.
```bash
# path: ["$"]
# value: {"obj":{"a":1, "b":2}, "arr":[1,2,3], "str": "foo", "bool": true, "int": 42, "float": 3.14}
redis> CBOR.SET key "\x81\x61$" "\xa6\x63obj\xa2\x61a\x01\x61b\x02\x63arr\x83\x01\x02\x03\x63str\x63foo\x64bool\xf5\x63int\x18\x2a\x65float\xfb\x40\x09\x1e\xb8\x51\xeb\x85\x1f"
OK
```

Get the CBOR document diagnostic notation
```bash
redis> CBOR.DEBUG DIAG key
"{\"obj\": {\"a\": 1, \"b\": 2}, \"arr\": [1, 2, 3], \"str\": \"foo\", \"bool\": true, \"int\": 42, \"float\": 3.14}"
```

## See also

[`CBOR.SET`](cbor.set.md)

# CBOR.CLEAR

### Syntax
```bash
CBOR.CLEAR key [path]
```

Clear container values (arrays/objects) and set numeric values to `0`

## Required arguments

### key
the key to parse.

## Optional arguments

### path
the CBORPath to specify. 

Default is root `"\x81\x61$"` (`["$"]`). Nonexisting paths are ignored.

## Return

CBOR.CLEAR returns an integer reply specified as the number of values cleared. 
For more information about replies, see [Redis serialization protocol specification](/docs/reference/protocol-spec).

Already cleared values are ignored for empty containers and zero numbers.

## Examples

Create a CBOR document.
```bash
# path: ["$"]
# value: {"obj":{"a":1, "b":2}, "arr":[1,2,3], "str": "foo", "bool": true, "int": 42, "float": 3.14}
redis> CBOR.SET key "\x81\x61$" "\xa6\x63obj\xa2\x61a\x01\x61b\x02\x63arr\x83\x01\x02\x03\x63str\x63foo\x64bool\xf5\x63int\x18\x2a\x65float\xfb\x40\x09\x1e\xb8\x51\xeb\x85\x1f"
OK
```

Clear all container values. This returns the number of objects with cleared values.
```bash
# path: ["$", {"*":1}]
redis> CBOR.CLEAR key "\x82\x61$\xa1\x61*\x01"
(integer) 4
```

Get the updated document. Note that numeric values have been set to `0`.
```bash
# path: ["$"]
# result: "[{"obj":{},"arr":[],"str":"foo","bool":true,"int":0,"float":0}]"
redis> CBOR.GET key "\x81\x61$"
"\x81\xa6cobj\xa0carr\x80cstrcfoodbool\xf5cint\x00efloat\xf9\x00\x00"
```

## See also

[`CBOR.DEL`](cbor.del.md)

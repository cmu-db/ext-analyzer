# CBOR.MAPKEYS

### Syntax
```bash
CBOR.MAPKEYS key [path]
```

Return the keys in the CBOR map that's referenced by `path`

## Required arguments

### key
the key to parse. Returns `null` for nonexistent keys.

## Optional arguments

### path
the CBORPath to specify. 

Default is root `"\x81\x61$"` (`["$"]`). Returns `null` for nonexistant path.

## Return

CBOR.MAPKEYS returns an array of array replies for each path, an array of the keys in the map as a bulk string reply, or `nil` if the matching CBOR value is not a map. 
For more information about replies, see [Redis serialization protocol specification](/docs/reference/protocol-spec).

## Examples

Create a document with a map.
```bash
# path: ["$"]
# value: '{"a":[3], "nested": {"a": {"b":2, "c": 1}}}'
redis> CBOR.SET key "\x81\x61$" "\xa2\x61a\x81\x03\x66nested\xa1\x61a\xa2\x61b\x02\x61c\x01"
OK
```

Get map keys
```bash
# path: ["$", {"..": "a"}]
# result: "b" and "c"
redis> CBOR.MAPKEYS key "\x82\x61$\xa1\x62..\x61a"
1) (nil)
2) 1) "ab"
   2) "ac"
```

## See also

[`CBOR.MAPAPPEND`](cbor.mapappend.md) | [`CBOR.MAPLEN`](cbor.maplen.md)

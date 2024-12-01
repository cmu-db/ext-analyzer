# CBOR.MAPLEN

### Syntax
```bash
CBOR.MAPLEN key [path]
```

Report the number of keys in the CBOR map at `path` in `key`

## Required arguments

### keys
the key to parse. Returns `null` for nonexistent keys.

## Optional arguments

### path
is CBORPath to specify. 

Default is root `"\x81\x61$"` (`["$"]`). Returns `null` for nonexistant path.

## Return

CBOR.MAPLEN returns an array of integer replies for each path specified as the number of keys in the map or `nil`, if the matching CBOR value is not a map.
For more information about replies, see [Redis serialization protocol specification](/docs/reference/protocol-spec).

## Examples

Create a document with a map.
```bash
# path: ["$"]
# value: '{"a":[3], "nested": {"a": {"b":2, "c": 1}}}'
redis> CBOR.SET key "\x81\x61$" "\xa2\x61a\x81\x03\x66nested\xa1\x61a\xa2\x61b\x02\x61c\x01"
OK
```

Get map length
```bash
# path: ["$", {"..": "a"}]
redis> CBOR.MAPLEN key "\x82\x61$\xa1\x62..\x61a"
1) (nil)
2) (integer) 2
```

## See also

`JSON.ARRINDEX` | `JSON.ARRINSERT` 

## Related topics

* [RedisJSON](/docs/stack/json)
* [Index and search JSON documents](/docs/stack/search/indexing_json)
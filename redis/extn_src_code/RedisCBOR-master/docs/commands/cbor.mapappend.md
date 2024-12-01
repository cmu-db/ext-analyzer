# CBOR.MAPAPPEND

### Syntax
```bash
CBOR.MAPAPPEND key path [map_key map_value] [map_key map_value ...]
```

Append the `CBOR` key/value pairs into the map at `path` after the last element in it, in `key`.

## Required arguments

### key
the key to modify.

### path
the CBORPath to specify.

### map_key map_value
one or more key/value pairs to append to one or more maps. 

## Return value 

`CBOR.MAPAPPEND` returns an [array](/docs/reference/protocol-spec/#resp-arrays) of integer replies for each path, the map's new size, or `nil`, if the matching CBOR value is not a map. 
For more information about replies, see [Redis serialization protocol specification](/docs/reference/protocol-spec). 

## Examples

Create a document with a map.
```bash
# path: ["$"] 
# value: {"a":1,"b":2}
redis> CBOR.SET key "\x81\x61$" "\xa2\x61a\x01\x61b\x02"
OK
```

Add `{"c":3,"d":4}` to the end of the map. `CBOR.MAPAPEND` returns the map's new size.
```bash
# path: ["$", "foo"] 
# key1: "c"
# value1: 3
# key2: "d"
# value2: 4
redis> CBOR.MAPAPPEND key "\x81\x61$" "\x61c" "\x03" "\x61d" "\x04"
1) (integer) 4
```

Get the updated document.
```bash
# result: {"a":1,"b":2,"c":3,"d":4}
redis> CBOR.GET key
"\x81\xa4aa\x01ab\x02ac\x03ad\x04"
```

## See also

[`CBOR.MAPKEYS`](cbor.mapkeys.md) | [`CBOR.MAPLEN`](cbor.maplen.md)

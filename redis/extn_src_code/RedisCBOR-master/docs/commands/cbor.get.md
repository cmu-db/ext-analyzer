# CBOR.GET

### Syntax
```bash
CBOR.GET key [path ...]
```

Return the value at `path` in CBOR serialized form

## Required arguments

### key
the key to parse.

## Optional arguments

### path
the CBORPath to specify. 

Default is root `"\x81\x61$"` (`["$"]`). 

the root of the matching values is a CBOR document with a top-level **array** of serialized CBOR value. 

## Return

CBOR.GET returns a bulk string representing a CBOR array of string replies. 
Each string is the CBOR serialization of each CBOR value that matches a path. 
For more information about replies, see [Redis serialization protocol specification](/docs/reference/protocol-spec).

## Examples

Create a CBOR document.
```bash
# path: ["$"] 
# value: {"a":2, "b": 3, "nested": {"a": 4, "b": null}}
redis> CBOR.SET key "\x81\x61$" "\xa3\x61a\x02\x61b\x03\x66nested\xa2\x61a\x04\x61b\xf6"
OK
```

Apply a CBORPath on the document
```bash
# path: ["$",{"..":"b"}] 
# result: [3,null]
redis> CBOR.GET key "\x82\x61$\xa1\x62..\x61b"
"\x82\x03\xf6"
```

## See also

[`CBOR.DEL`](cbor.del.md) | [`CBOR.MGET`](cbor.mget.md) | [`CBOR.SET`](cbor.set.md)
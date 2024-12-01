# CBOR.SET

### Syntax
```bash
CBOR.SET key path value [NX | XX]
```

Set the CBOR value at `path` in `key`.

[Examples](#examples)

## Required arguments

### key
the key to modify.

### value
the value to set at the specified path

### path
the CBORPath to specify. 

For new Redis keys the `path` must be the root. For existing keys, when the entire `path` exists, the value that it contains is replaced with the CBOR value. 

## Optional arguments

### NX
sets the key only if it does not already exist.

### XX
sets the key only if it already exists.

## Return value 

CBOR.SET returns a simple string reply: `OK` if executed correctly or `nil` if the specified `NX` or `XX` conditions were not met.
For more information about replies, see [Redis serialization protocol specification](/docs/reference/protocol-spec).

## Examples

Replace an existing value.
```bash
# path: ["$"] 
# value: {"a":2}
redis> CBOR.SET key "\x81\x61$" "\xa1\x61a\x02"
OK
# path: ["$","a"] 
# value: 3
redis> CBOR.SET key "\x82\x61$\x61a" "\x03"
OK
redis> CBOR.GET key
# value: {"a":3}
"'\xa1\x61a\x03'"
```

Update multi-paths
```bash
# path: ["$"] 
# value: {"f1": {"a":1}, "f2":{"a":2}}
redis> CBOR.SET key "\x81\x61$" "\xa2\x62f1\xa1\x61a\x01\x62f2\xa1\x61a\x02"
OK
# path: ["$",{"..":"a"}] 
# value: 3
redis> CBOR.SET key "\x82\x61$\xa1\x62..\x61a" "\x03"
OK
# result: {"f1": {"a":1}, "f2":{"a":3}}
redis> CBOR.GET key
"{\"f1\":{\"a\":3},\"f2\":{\"a\":3}}"
```

## See also

[`CBOR.DEL`](cbor.del.md) | [`CBOR.GET`](cbor.get.md) | [`CBOR.MGET`](cbor.mget.md)

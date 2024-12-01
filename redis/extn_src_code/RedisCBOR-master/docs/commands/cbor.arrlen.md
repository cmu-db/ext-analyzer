# CBOR.ARRLEN

### Syntax
```bash
CBOR.ARRLEN key [path]
```

Report the length of the CBOR array at `path` in `key`.

## Required arguments

### key
the key to parse.

## Optional arguments

### path
the CBORPath to specify. Default is root `"\x81\x61$"` (`["$"]`), if not provided. 

Returns null if the `key` or `path` do not exist.

## Return

`CBOR.ARRLEN` returns an [array](/docs/reference/protocol-spec/#resp-arrays) of integer replies, an integer for each matching value, each is the array's length, or `nil`, if the matching value is not an array.
For more information about replies, see [Redis serialization protocol specification](/docs/reference/protocol-spec). 

## Examples

Create a document.
```bash
# path: ["$"] 
# value: {"foo":12,"bar":["a","b","c"]}
redis> CBOR.SET key "\x81\x61$" "\xa2\x63foo\x0c\x63bar\x83\x61a\x61b\x61c"
OK
```

Find lengths of arrays in all objects of the document.
```bash
# path: ["$", {"*":1}] 
redis> CBOR.ARRLEN key "\x82\x61$\xa1\x61*\x01"
1) (nil)
2) (integer) 3
```

## See also

[`CBOR.ARRAPPEND`](cbor.arrappend.md) | [`CBOR.ARRINDEX`](cbor.arrindex.md) | [`CBOR.ARRINSERT`](cbor.arrinsert.md) | [`CBOR.ARRPOP`](cbor.arrpop.md) | [`CBOR.ARRTRIM`](cbor.arrtrim.md)
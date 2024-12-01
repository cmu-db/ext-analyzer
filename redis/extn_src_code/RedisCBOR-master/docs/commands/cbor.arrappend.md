# CBOR.ARRAPPEND

### Syntax
```bash
CBOR.ARRAPPEND key path value [value ...]
```

Append the `CBOR` values into the array at `path` after the last element in it, in `key`.

## Required arguments

### key
the key to modify.

### path
the CBORPath to specify.

### value
one or more values to append to one or more arrays. 

## Return value 

`CBOR.ARRAPEND` returns an [array](/docs/reference/protocol-spec/#resp-arrays) of integer replies for each path, the array's new size, or `nil`, if the matching CBOR value is not an array. 
For more information about replies, see [Redis serialization protocol specification](/docs/reference/protocol-spec). 

## Examples

Create a map with a sub-array.
```bash
# path: ["$"] 
# value: {"foo":["a","b","c"]}
redis> CBOR.SET key "\x81\x61$" "\xa1\x63foo\x83\x61a\x61b\x61c"
OK
```

Add `"d"` to the end of the sub-array. `CBOR.ARRAPEND` returns the array's new size.
```bash
# path: ["$", "foo"] 
# value: "d"
redis> CBOR.ARRAPPEND key "\x82\x61$\x63foo" "\x61d"
1) (integer) 4
```

Get the updated document.
```bash
# result: {"foo":["a","b","c","d"]}
redis> CBOR.GET key
"\x81\xa1cfoo\x84aaabacad"
```

## See also

[`CBOR.ARRINDEX`](cbor.arrindex.md) | [`CBOR.ARRINSERT`](cbor.arrinsert.md) | [`CBOR.ARRLEN`](cbor.arrlen.md) | [`CBOR.ARRPOP`](cbor.arrpop.md) | [`CBOR.ARRTRIM`](cbor.arrtrim.md)

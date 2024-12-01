# CBOR.ARRINSERT

### Syntax
```bash
CBOR.ARRINSERT key path index value [value ...]
```

Insert the CBOR values into the array at `path` before the index (shifts to the right),  in `key`.

## Required arguments

### key
the key to modify.

### path
the CBORPath to specify.

### value
one or more values to insert in one or more arrays. 

### index
the position in the array where you want to insert a value. 

The index must be in the array's range. Inserting at `index` 0 prepends to the array. Negative index values start from the end of the array.

## Return value 

`CBOR.ARRINSERT` returns an [array](/docs/reference/protocol-spec/#resp-arrays) of integer replies for each path, the array's new size, or `nil`, if the matching CBOR value is not an array. 
For more information about replies, see [Redis serialization protocol specification](/docs/reference/protocol-spec). 

## Examples

Create a map with a sub-array.
```bash
# path: ["$"] 
# value: {"foo":["a","b","c"]}
redis> CBOR.SET key "\x81\x61$" "\xa1\x63foo\x83\x61a\x61b\x61c"
OK
```

Insert `"d"` and `"e"` into the sub-array, before `"c"`. `CBOR.ARRINSERT` returns the array's new size.
```bash
# path: ["$", "foo"] 
# values: "d" and "e"
redis> CBOR.ARRINSERT key "\x82\x61$\x63foo" 2 "\x61d" "\x61e"
1) (integer) 5
```

Get the updated document.
```bash
# result: {"foo":["a","b","d","e","c"]}
redis> CBOR.GET key
"\x81\xa1cfoo\x85aaabadaeac"
```

## See also

[`CBOR.ARRAPPEND`](cbor.arrappend.md) | [`CBOR.ARRINDEX`](cbor.arrindex.md) | [`CBOR.ARRLEN`](cbor.arrlen.md) | [`CBOR.ARRPOP`](cbor.arrpop.md) | [`CBOR.ARRTRIM`](cbor.arrtrim.md)

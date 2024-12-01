# CBOR.ARRPOP

### Syntax
```bash
CBOR.ARRPOP key [path [index]]
```

Remove and return an element from the `index` in the array at `path` in `key`.

[Examples](#examples)

## Required arguments

### key
the key to modify.

## Optional arguments

### path
the CBORPath to specify. 

Default is root `"\x81\x61$"` (`["$"]`).

### index
the position in the array to start popping from. 

Default is `-1`, meaning the last element. 

Out-of-range indexes round to their respective array ends. Popping an empty array returns null.

## Return

`CBOR.ARRPOP` returns an [array](/docs/reference/protocol-spec/#resp-arrays) of bulk string replies for each path, each reply is the popped CBOR value, or `nil`, if the matching CBOR value is not an array.
For more information about replies, see [Redis serialization protocol specification](/docs/reference/protocol-spec). 

## Examples

Create a document.
```bash
# path: ["$"] 
# value: {"foo":12,"bar":["a","b","c"]}
redis> CBOR.SET key "\x81\x61$" "\xa2\x63foo\x0c\x63bar\x83\x61a\x61b\x61c"
OK
```

Pop the last element.
```bash
# path: ["$", {"*":1}] 
redis> CBOR.ARRPOP key "\x82\x61$\xa1\x61*\x01"
1) (nil)
2) "ac" # "c"
```

Get the updated document.
```bash
# result: {"foo":12,"bar":["a","b"]}
redis> CBOR.GET key
"\x81\xa2cfoo\x0ccbar\x82aaab"
```

## See also

[`CBOR.ARRAPPEND`](cbor.arrappend.md) | [`CBOR.ARRINDEX`](cbor.arrindex.md) | [`CBOR.ARRINSERT`](cbor.arrinsert.md) | [`CBOR.ARRLEN`](cbor.arrlen.md) | [`CBOR.ARRTRIM`](cbor.arrtrim.md)
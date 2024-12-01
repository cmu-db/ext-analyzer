# CBOR.ARRTRIM

### Syntax
```bash
CBOR.ARRTRIM key [path [start [stop]]]
```

Trim an array so that it contains only the specified inclusive range of elements at `path` in `key`.

## Required arguments

### key
the key to modify.

## Optional arguments

### path
the CBORPath to specify.

Default is root `"\x81\x61$"` (`["$"]`).

### start

the index of the first element to keep (previous elements are trimmed). 

Default is 0. 

### open

the index of the last element to keep (following elements are trimmed), including the last element. 

Default is 0. Negative values are interpreted as starting from the end.

CBOR.ARRTRIM is extremely forgiving, and using it with out-of-range indexes does not produce an error:
* If `start` is larger than the array's size or `start` > `stop`, returns 0 and an empty array. 
* If `start` is < 0, then start from the end of the array.
* If `stop` is larger than the end of the array, it is treated like the last element.

## Return

CBOR.ARRTRIM returns an array of integer replies for each path, the array's new size, or `nil`, if the matching CBOR value is not an array.
For more information about replies, see [Redis serialization protocol specification](/docs/reference/protocol-spec). 

## Examples

Create a document.
```bash
# path: ["$"] 
# value: {"foo":12,"bar":["a","b","c","d"]}
redis> CBOR.SET key "\x81\x61$" "\xa2\x63foo\x0c\x63bar\x84\x61a\x61b\x61c\x61d" 
OK
```

Trim arrays
```bash
# path: ["$", {"*":1}] 
redis> CBOR.ARRTRIM key "\x82\x61$\xa1\x61*\x01" 1 2
1) (nil)
2) (integer) 2
```

Get the updated document.
```bash
# result: {"foo":12,"bar":["b","c"]}
redis> CBOR.GET key
"\x81\xa2cfoo\x0ccbar\x82abac"
```

## See also

[`CBOR.ARRAPPEND`](cbor.arrappend.md) | [`CBOR.ARRINDEX`](cbor.arrindex.md) | [`CBOR.ARRINSERT`](cbor.arrinsert.md) | [`CBOR.ARRLEN`](cbor.arrlen.md) | [`CBOR.ARRPOP`](cbor.arrpop.md)

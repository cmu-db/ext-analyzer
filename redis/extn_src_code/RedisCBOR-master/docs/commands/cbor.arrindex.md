# CBOR.ARRINDEX

### Syntax
```bash
CBOR.ARRINDEX key path value [start [stop]]
```

Search for the first occurrence of a CBOR `value` in an array,  in `key`.

## Required arguments

### key
the key to parse.

### path
the CBORPath to specify.

### value
the value to find its index in one or more arrays. 

## Optional arguments

### start
the inclusive start value to specify in a slice of the array to search. Default is `0`. 

### stop
the exclusive stop value to specify in a slice of the array to search, including the last element. Default is `-1`. 

Negative values are interpreted as starting from the end.

Out-of-range indexes round to the array's start and end. An inverse index range (such as the range from 1 to 0) returns unfound or `-1`.

## Return value 

`CBOR.ARRINDEX` returns an [array](/docs/reference/protocol-spec/#resp-arrays) of integer replies for each path, the first position in the array of each CBOR value that matches the path, `-1` if unfound in the array, or `nil`, if the matching CBOR value is not an array.
For more information about replies, see [Redis serialization protocol specification](/docs/reference/protocol-spec). 

## Examples

Create a map with 3 items.
```bash
# path: ["$"] 
# value: {"foo":12,"bar":["a","b","c"],"baz":["d","e","f"]}
redis> CBOR.SET key "\x81\x61$" "\xa3\x63foo\x0c\x63\bar\x83\x61a\x61b\x61c\x63baz\x83\x61d\x61e\x61f"
OK
```

Find the position of `"e"` in each value of the map:
* The first value is not an array (nil)
* The second value is an array but does not contain `"e"` (-1)
* The third value is an array and contains `"e"` at position (1)
```bash
# path: ["$", {"*":1}] 
# value: "e"
redis> CBOR.ARRINDEX key "\x82\x61$\xa1\x61*\x01" "\x61e"
1) (nil)
2) (integer) -1
3) (integer) 1
```

## See also

[`CBOR.ARRAPPEND`](cbor.arrappend.md) | [`CBOR.ARRINSERT`](cbor.arrinsert.md) | [`CBOR.ARRLEN`](cbor.arrlen.md) | [`CBOR.ARRPOP`](cbor.arrpop.md) | [`CBOR.ARRTRIM`](cbor.arrtrim.md)
 
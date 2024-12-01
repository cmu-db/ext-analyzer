# CBOR.STRLEN

### Syntax
```bash
CBOR.STRLEN key [path]
```

Report the length of the CBOR String at `path` in `key`

## Required arguments

### key
the key to parse.

## Optional arguments

### path
the CBORPath to specify. 

Default is root `"\x81\x61$"` (`["$"]`), if not provided. Returns null if the `key` or `path` do not exist.

## Return

CBOR.STRLEN returns by recursive descent an array of integer replies for each path, the string's length, or `nil`, if the matching CBOR value is not a string.
For more information about replies, see [Redis serialization protocol specification](/docs/reference/protocol-spec). 

## Examples

Create a CBOR document.
```bash
# path: ["$"]
# value: {"a":"foo", "nested": {"a": "hello"}, "nested2": {"a": 31}}
redis> CBOR.SET key "\x81\x61$" "\xa3\x61a\x63foo\x66nested\xa1\x61a\x65hello\x67nested2\xa1\x61a\x18\x1f"
OK
```

Recursively find and get length of all `a` string children.
```bash
# path: ["$", {"..: "a"}]
redis> CBOR.STRLEN key "\x82\x61$\xa1\x62..\x61a"
1) (integer) 3
2) (integer) 5
3) (nil)
```

## See also

[`CBOR.STRAPPEND`](cbor.strappend.md)
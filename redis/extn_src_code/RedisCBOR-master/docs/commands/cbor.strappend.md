# CBOR.STRAPPEND

### Syntax
```bash
CBOR.STRAPPEND key path value
```

Append the CBOR-string value to the string at `path` in `key`

## Required arguments

### key
the key to modify.

### path
the CBORPath to specify.

### value
the value in bulk string format to append to one or more strings.

## Return value 

CBOR.STRAPPEND returns an array of integer replies for each path, the string's new length, or `nil`, if the matching CBOR value is not a string.
For more information about replies, see [Redis serialization protocol specification](/docs/reference/protocol-spec). 

## Examples

Create a CBOR document.
```bash
# path: ["$"]
# value: {"a":"foo", "nested": {"a": "hello"}, "nested2": {"a": 31}}
redis> CBOR.SET key "\x81\x61$" "\xa3\x61a\x63foo\x66nested\xa1\x61a\x65hello\x67nested2\xa1\x61a\x18\x1f"
OK
```

Recursively find and append `baz` to all `a` string children.
```bash
# path: ["$", {"..: "a"}]
# results: [6, 8, nil]
redis> CBOR.STRAPPEND key "\x82\x61$\xa1\x62..\x61a" "baz"
1) (integer) 6
2) (integer) 8
3) (nil)
```

Get the updated document.
```bash
# result: {"a":"foobaz", "nested": {"a": "hellobaz"}, "nested2": {"a": 31}}
redis> CBOR.GET key
"[{\"a\":\"foobaz\",\"nested\":{\"a\":\"hellobaz\"},\"nested2\":{\"a\":31}}]"
```

## See also

[`CBOR.STRLEN`](cbor.strlen.md) | [`CBOR.ARRAPPEND`](cbor.arrappend.md)

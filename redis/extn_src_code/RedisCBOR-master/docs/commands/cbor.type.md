# CBOR.TYPE

### Syntax
```bash
CBOR.TYPE key [path]
```

Report the type of CBOR value at `path` in `key`

## Required arguments

### key
the key to parse.

## Optional arguments

### path
the CBORPath to specify. 

Default is root `"\x81\x61$"` (`["$"]`), if not provided. Returns null if the `key` or `path` do not exist.

## Return

CBOR.TYPE returns an array of string replies for each path, specified as the value's type.

Possible values are:
* `unsigned`
* `negative`
* `float`
* `string`
* `bytestring`
* `boolean`
* `null`
* `undefined`
* `simple`
* `array`
* `map`

For more information about replies, see [Redis serialization protocol specification](/docs/reference/protocol-spec).

## Examples

Create a CBOR document.
```bash
# path: ["$"]
# value: {"a":2, "nested": {"a": true}, "foo": "bar"}
redis> CBOR.SET key "\x81\x61$" "\xa3\x61a\x02\x66nested\xa1\x61a\xf5\x63foo\x63bar"
OK
```

Get type(s) for path.
```bash
# path: ["$", {"..": "foo"}]
redis> CBOR.TYPE key "\x82\x61$\xa1\x62..\x63foo"
1) "string"
# path: ["$", {"..": "a"}]
redis> CBOR.TYPE key "\x82\x61$\xa1\x62..\x61a"
1) "unsigned"
2) "boolean"
# path: ["$", {"..": "dummy"}]
redis> CBOR.TYPE key "\x82\x61$\xa1\x62..\x65dummy"
(empty array)
```

## See also

[`CBOR.GET`](cbor.get.md) | [`CBOR.SET`](cbor.set.md)


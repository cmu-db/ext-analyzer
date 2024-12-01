# CBOR.NUMMULTBY

### Syntax
```bash
CBOR.NUMMULTBY key path value
```

Multiply the number value stored at `path` by `number` in `key`

## Required arguments

### key
the key to modify.

### path
the CBORPath to specify.

### value
the number value to multiply. 

## Return

CBOR.NUMMULTBY returns a bulk string reply specified as a  new value for each path, or `nil` element if the matching CBOR value is not a number.
For more information about replies, see [Redis serialization protocol specification](/docs/reference/protocol-spec).

## Examples

Create a document.
```bash
# path: ["$"]
# value: {"a":"b","b":[{"a":2}, {"a":5}, {"a":"c"}]}
redis> CBOR.SET key "\x81\x61$" "\xa2\x61a\x61b\x61b\x83\xa1\x61a\x02\xa1\x61a\x05\xa1\x61a\x61c"
OK
```

Multiply a value of `a` map by 2. The command fails to find a number and returns `nil`.
```bash
# path: ["$", "a"]
# value: 2
# results: [nil]
redis> CBOR.NUMMULTBY key "\x82\x61$\x61a" "\x02"
1) (nil)
```

Recursively find and multiply a value of all `a` objects. The command multiply numbers it finds and returns `nil` for nonnumber values.
```bash
# path: ["$", {"..: "a"}]
# value: 2
# results: [nil, 4, 10, nil]
redis> CBOR.NUMMULTBY key "\x82\x61$\xa1\x62..\x61a" "\x02"
1) (nil)
2) "\x04"
3) "\n"
4) (nil)
```

## See also

[`CBOR.NUMINCRBY`](cbor.numincrby.md)
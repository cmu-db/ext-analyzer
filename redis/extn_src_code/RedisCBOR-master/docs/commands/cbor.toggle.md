# CBOR.TOGGLE

### Syntax
```bash
CBOR.TOGGLE key [path]
```

Toggle a Boolean value stored at `path` in `key`

## Required arguments

### key
the key to modify.

## Optional arguments

### path
the CBORPath to specify. 

Default is root `"\x81\x61$"` (`["$"]`), if not provided.

## Return

CBOR.TOGGLE returns an array of integer replies for each path, the new value, or `nil` for CBOR values matching the path that are not Boolean.
For more information about replies, see [Redis serialization protocol specification](/docs/reference/protocol-spec).

## Examples

Create a CBOR document.
```bash
# path: ["$"]
# value: {"bool": true}
redis> CBOR.SET key "\x81\x61$" "\xa1\x64bool\xf5"
OK
```

Toggle the Boolean value.
```bash
# path: ["$", "bool"]
redis> CBOR.TOGGLE key "\x82\x61$\x64bool"
1) (integer) 0
1) (false)
```

Get the updated document.
```bash
# result: {"bool": true}
redis> CBOR.GET key
"\x81\xa1dbool\xf5"
```

Switch to RESP3
```bash
redis> HELLO 3
1# "server" => "redis"
2# "version" => "7.0.8"
3# "proto" => (integer) 3
4# "id" => (integer) 4
5# "mode" => "standalone"
6# "role" => "master"
7# "modules" => 1) 1# "name" => "ReCBOR"
      2# "ver" => (integer) 1
      3# "path" => "/librecbor.so"
      4# "args" => (empty array)
```

Toggle the Boolean value.
```bash
# path: ["$", "bool"]
redis> CBOR.TOGGLE key "\x82\x61$\x64bool"
1) (true)
```

Get the updated document.
```bash
# result: {"bool": false}
redis> CBOR.GET key
"\x81\xa1dbool\xf4"
```

## See also

[`CBOR.GET`](cbor.get.md) | [`CBOR.SET`](cbor.set.md)

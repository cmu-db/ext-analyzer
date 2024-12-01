# CBOR.RESP

### Syntax
```bash
CBOR.RESP key [path]
```

Return the CBOR in `key` in [Redis serialization protocol specification](/docs/reference/protocol-spec) form 

## Required arguments

### key
the key to parse.

## Optional arguments

### path
the CBORPath to specify. 

Default is root `"\x81\x61$"` (`["$"]`).
This command uses the following mapping from CBOR to RESP:
*   CBOR `null` and `undefined` map to the `nil` reply.
*   CBOR `false` and `true` values map to the boolean reply (RESP3) or integer reply (RESP2)
*   CBOR number maps to the integer reply or double reply, depending on type.
*   CBOR text string and byte string map to the bulk string reply.
*   CBOR array is represented as an array reply
*   CBOR map is represented as a map reply (RESP3) or array reply (RESP2)

For more information about replies, see [RESP2](/docs/reference/protocol-spec) or [RESP3](https://github.com/redis/redis-specifications/blob/master/protocol/RESP3.md)

## Return

CBOR.RESP returns an array reply specified as the CBOR RESP form detailed in [RESP2](/docs/reference/protocol-spec) or [RESP3](https://github.com/redis/redis-specifications/blob/master/protocol/RESP3.md).

## Examples

Create a CBOR document.
```bash
# path: ["$"]
# value: [{"a":1,"b":2},[1,2,3],"foo",h'0123456789ABCDEF',12,-12,12.12,true,null,undefined]
redis> CBOR.SET key "\x81\x61$" "\x8a\xa2\x61a\x01\x61b\x02\x83\x01\x02\x03\x63foo\x48\x01\x23\x45\x67\x89\xab\xcd\xef\x0c\x2b\xfb\x40\x28\x3d\x70\xa3\xd7\x0a\x3d\xf5\xf6\xf7"
OK
```

Get all RESP2 details about the document.
```bash
redis> CBOR.RESP key
1)  1) 1) "a"
       2) (integer) 1
       3) "b"
       4) (integer) 2
    2) 1) (integer) 1
       2) (integer) 2
       3) (integer) 3
    3) "foo"
    4) "\x01#Eg\x89\xab\xcd\xef"
    5) (integer) 12
    6) (integer) -12
    7) "12.119999999999999"
    8) (integer) 1
    9) (nil)
   10) (nil)
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

Get all RESP3 details about the document.
```bash
redis> CBOR.RESP key
1)  1) 1# "a" => (integer) 1
       2# "b" => (integer) 2
    2) 1) (integer) 1
       2) (integer) 2
       3) (integer) 3
    3) "foo"
    4) "\x01#Eg\x89\xab\xcd\xef"
    5) (integer) 12
    6) (integer) -12
    7) (double) 12.119999999999999
    8) (true)
    9) (nil)
   10) (nil)
```

## See also

[`CBOR.SET`](cbor.set.md)

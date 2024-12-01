# RedisCBOR

RedisCBOR is a [Redis](https://redis.io/) module that implements [CBOR](https://cbor.io/) as a native data type. 
It allows storing, updating and fetching CBOR documents from Redis keys (documents).

It is based on [RedisJson](https://redis.io/docs/stack/json/) for its concepts, its commands and their syntax.

Benefits of CBOR over JSON:
* ~25% smaller memory footprint / bandwidth, depending on which type are used (text will have the same footprint in JSON or CBOR)
* integer, floats, boolean and null types should be faster to deserialize because they are encoded in binary, consequently there is no parsing from text to process.

## Primary features:

* Full support of the CBOR standard
* [CBORPath](https://github.com/dahomey-technologies/cborpath-rs) syntax for selecting elements inside documents
* Documents are stored as raw CBOR binary data, allowing reduced memory footprint
* Typed atomic operations for all CBOR types
 
## Build
### With Docker

Run the following on the main directory:
```bash
docker build -t redis-cbor .
```

### From Source

Make sure you have Rust installed: https://www.rust-lang.org/tools/install

Run the following on the main directory:
```bash
cargo build --release
```

When running the tests, you need to explicitly specify the test feature to disable use of the Redis memory allocator when testing:
```bash
cargo test --features test
```

## Run
### With Docker

run the built image:
```bash
docker run --name redis-cbor -d -p 6379:6379 redis-cbor
```

### From Source
Run Redis pointing to the newly built module:
```bash
redis-server --loadmodule ./target/release/librecbor.so
```

Alternatively add the following to a redis.conf file:
```bash
loadmodule /path/to/modules/librecbor.so
```

## Documentation
Read the docs [here](docs/docs/commands.md)

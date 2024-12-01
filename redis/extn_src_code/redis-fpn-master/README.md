
# Overview

**FPN** module - it is a **F**ixed **P**oint **N**umber module for Redis 4, which adds decimal data type and different commands for such type.

> **IMPORTANT:** This module works only from 4 Redis version...because modules API become available only in 4 version.

## Contents

- [How to load](#how-to-load)
- [How to use](#how-to-use)
- [Restrictions](#restrictions)
- [Development](#development)
  - [Prerequisites](#prerequisites)
  - [Building](#building)
- [Built With](#built-with)
- [Changelog](#changelog)
- [Contributing](#contributing)
- [Versioning](#versioning)
- [Authors](#authors)
- [License](#license)

## How to load

Just run the Redis server like this:

```bash
$> redis-server --loadmodule /<path_to_module>/fpn_module.so
```

You also can load the module using the following `redis.conf` configuration directive:

```redis
loadmodule /<path_to_module>/fpn_module.so
```

It is also possible to load a module at runtime using the following command:

```redis
MODULE LOAD /<path_to_module>/fpn_module.so
```

In order to list all loaded modules, use:

```redis
MODULE LIST
```

Finally, you can unload (and later reload if you wish) a module using the following command:

```redis
MODULE UNLOAD fpn_module
```

## How to use

Available commands:

* `FPN.SET` **key** **value** [**scale**] - sets a new **value** for **key** with specified **scale** (optional, default is 2). It returns previous value or **0**.

* `FPN.GET` **key** [**scale**] - gets a value by its **key** with specified **scale** (optional). If now value - it returns `nil` value.

* `FPN.ADD` **key** **value** [**scale**] - adds a **value** to the specified **key**, the result of the command could be converted to specific **scale** (optional) or it will be the max scale (ex: 12.34 + 5.678 = 18.018)

* `FPN.SUBTRACT` **key** **value** [**scale**] - subtracts a **value** from the specified **key**, the result of the command could be converted to specific **scale** (optional) or it will be the max scale (ex: 1.234 - 5.6 = -4.366)

* `FPN.MULTIPLY` **key** **value** [**scale**] - multiply the specified **key** by a **value**, the result of the command may be converted to specific **scale** (optional) or it will be the max scale

Example:

```bash
$> redis-cli FPN.GET popa.key
""
$> redis-cli FPN.SET popa.key 1.23
"0.00"
$> redis-cli FPN.ADD popa.key 3.567
"4.797"
$> redis-cli FPN.GET popa.key
"4.797"
$> redis-cli FPN.GET popa.key 2
"4.80"
$> redis-cli FPN.GET popa.key 0
"5"
$> redis-cli FPN.SUBTRACT popa.key 3
"1.797"
$> redis-cli FPN.MULTIPLY popa.key 2.36
"4.24092"
$> redis-cli FPN.MULTIPLY popa.key 2 1
"8.5"
```

## Restrictions

The value in this lib is present as a structure:

```c
/**
 * The structure represents decimal number.
 */
typedef struct {
  int128_t value; // 128bit integer for storing a value
  uint8_t scale;  // number of digits in mantissa
} Decimal;
```

`int128_t` has 39 digits at all (plus a sign), but we use only **38** digits, so the maximum value, which we could store in that structure, is **99_999_999_999_999_999_999_999_999_999_999_999_999** and the lowest is **-99_999_999_999_999_999_999_999_999_999_999_999_999**. The lib also validates your input values, like this:

```bash
$> redis-cli FPN.SET popa.key 1234567891234567890.0123456789123456789
"0.00"
$> redis-cli FPN.GET popa.key
"1234567891234567890.0123456789123456789"
$> redis-cli FPN.GET popa.key 19
"1234567891234567890.0123456789123456789"
$> redis-cli FPN.GET popa.key 20
(error) value must be greater or equal than -99_999_999_999_999_999_999_999_999_999_999_999_999 and lower or equal than 99_999_999_999_999_999_999_999_999_999_999_999_999
```

The error above shows us, what it couldn't conver value `1234567891234567890.0123456789123456789` to `1234567891234567890.01234567891234567890` (we have set **20** as the scale parameter), because `12_345_678_912_345_678_900_123_456_789_123_456_789` is greater than `99_999_999_999_999_999_999_999_999_999_999_999_999` (we store the whole value as integer).

## Development

These instructions will get you a copy of the project up and running on your local machine for development and testing purposes.

### Prerequisites

For building the project you need only a [GCC](https://gcc.gnu.org).

```bash
$> git https://github.com/infobip/redis-fpn
$> cd redis-fpn
```

### Building

The following `make` commands are useful:

* **make build** - just for building the module for your machine.

* **make container_build** - to build the module for `Debian` OS. It uses a Docker image with GCC inside it. After building, it compiles the module and puts the result into the target folder.

* **make start_redis** - it builds `Debian` module and starts a Docker container with Redis 4 and loads this module.

* **make integration_tests** - runs `Redis` with `FPN` module and executes integration tests against it.

> **IMPORTANT:** The last three commands required the Docker.

## Built With

* [C] - is a general-purpose, imperative computer programming language
* [make] - is a build automation tool that automatically builds executable programs and libraries from source code

## Changelog

To see what has changed in recent versions of Popout, see the [changelog](./CHANGELOG.md) file.

## Contributing

Please read [contributing](./CONTRIBUTING.md) file for details on my code of conduct, and the process for submitting pull requests to me.

## Versioning

We use [SemVer](http://semver.org/) for versioning. For the versions available, see the [tags on this repository](https://github.com/infobip/redis-fpn/tags).

## Authors

* **[Artem Labazin](https://github.com/xxlabaza)** - creator and the main developer

## License

This project is licensed under the Apache License 2.0 License - see the [license](./LICENSE) file for details

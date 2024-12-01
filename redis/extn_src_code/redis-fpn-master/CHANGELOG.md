
# Changelog

All notable changes to this project will be documented in this file.

The format is based on [Keep a Changelog](http://keepachangelog.com/en/1.0.0/)
and this project adheres to [Semantic Versioning](http://semver.org/spec/v2.0.0.html).

[Tags on this repository](https://github.com/infobip/redis-fpn/tags)

## [Unreleased]

- Implement `FPN.DIVIDE` functions;
- Add unit tests durign build.

## **1.1.8** - 2018-08-28

### Added

- The module prints its own version in logs.

### Changed

- `FPN.GET` returns Redis' `NIL` value, if it doesn't find a key's value;
- Rewrite save/load functions for `FPN` data type.

## **1.1.7** - 2018-08-27

Different bugfixes and improvments.

### Added

- Integration tests and `Makefile` command for it.

### Changed

- Max value is `128` bits right now;
- Add several checks for input values;
- `container_build` and `start_redis` work on `Debian` container version.

## **1.1.4** - 2018-08-08

Scale function now rounding the value.

### Changed

- Scale operations rounds the result value like in the math.

## **1.1.3** - 2018-08-08

Minor fixes and improvements.

### Added

- `scale` and `value` validation;
- Strings terminating sign.

### Changed

- Method descriptions.

## **1.1.0** - 2018-08-06

### Added

- Add `FPN.MULTIPLY` command implementation.

## **1.0.0** - 2018-08-06

Initial release.

### Added

- Decimal data type;
- Basic set of commands.

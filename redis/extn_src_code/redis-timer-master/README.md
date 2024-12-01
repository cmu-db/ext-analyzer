# Timer Redis Module

This module allows the delayed execution of LUA scripts, both periodic and one-time.


## Quick start

1. Obtain a redis server version 4.0 or above (with module support)
2. Download and compile redis-timer (`make`)
3. Start the redis server with the timer module:
    - By adding this line to redis.conf: `loadmodule /path/to/timer.so`
    - By using a command-line argument: `redis-server --load-module /path/to/timer.so`
    - By running the command: `MODULE LOAD /path/to/timer.so`


## Commands

### `TIMER.NEW milliseconds sha1 [LOOP]`

Create and activate a new timer.

The LUA script referenced by `sha1` will be
executed after `milliseconds`. If `LOOP` is specified, after the execution a
new timer will be setup with the same time.

**Notes:**

- if the script is unloaded, the timer will be deleted
- no info is provided regarding the execution of the script
- for simplicity, in periodic timers the execution interval will start counting at the end of the previous execution, and not at the beginning. After some time, the exact time of the triggering may be difficult to deduce, particularly if the the script takes a long time to execute or if different executions require different ammounts of time.

**Reply:** the id of the timer.


### `TIMER.KILL id`

Removes a timer.

**Reply:** OK if succesfull, error otherwise.


### `TIMER.INFO id`

Provides info of a timer.

**Reply**: an array with the following items:
1. the id of the timer
2. the sha1 of the script
3. the remaining time (milliseconds to the next execution)
4. the time interval if it is a periodic timer, or 0 otherwise


### `TIMER.LIST`

Provides info for all the active timers.

**Reply**: an array with one subarray per timer, with the same format of the `TIMER.INFO` command.


## Contributing

Issue reports, pull and feature requests are welcome.


## License

MIT - see [LICENSE](LICENSE)

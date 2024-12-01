# Laravel Queue Redis Module

This is a Redis module that implements a laravel compatible queue driver for redis with the following improvements over
the original Laravel 5.7 PHP-Lua driver:

1. Blocking pop is now more reliable than before.
2. Blocking pop now works on delayed and reserved jobs as well.
3. Timer for delayed and reserved jobs is server side, with milliseconds precision. This means
you don't need to worry about syncing your php and redis servers, in case your projects are 
distributed accross different servers. Moreover, this makes `retry_after` and `block_for`
configurations independent of each other.
4. Laravel queue can now be available for other programming languages and frameworks as well.
Feel free to port it to your favorite ones.

## Commands

1. laravel.push \<queue-name\> \<job\>
2. laravel.later \<queue-name\>:delayed \<delay-ms\> \<job\>
3. laravel.pop \<queue-name\> \<queue-name\>:delayed \<queue-name\>:reserved \<reply-after-ms\> \<block-for-ms\>
4. laravel.delete \<queue-name\>:reserved \<job\>
5. laravel.release \<queue-name\>:delayed \<queue-name\>:reserved \<job\> \<delay-ms\>

## Requirements
1. Redis version 5.0 or higher.
2. cmake > 3.1.

## Build
Running this command will create a `build/liblaravelq.so` file.
`mkdir build && cd build && cmake .. && make`
Place this file in a directory to which your redis-server have access.

## Installation
To load the module to an already running redis server run the following redis command:

    module load <path/to/liblaravelq.so>

To always run redis server with the module, you can add the following to redis.conf configuration file (usually in /etc/redis directory):

    loadmodule </path/to/liblaravelq.so>

After loading modules, laravel.* commands will be available.

## Drivers

To use this module with laravel, use [halaei/lqrm](https://github.com/halaei/lqrm-php) composer package:

    composer require halaei/lqrm

Please note that this module is not language dependent. Other languages and frameworks can hopefully benefit from this module.

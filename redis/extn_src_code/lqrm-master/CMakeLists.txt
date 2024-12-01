CMAKE_MINIMUM_REQUIRED(VERSION 3.1)
project(laravel_queue_redis_module C)

set(CMAKE_C_STANDARD 99)

add_library(laravelq SHARED
        src/blocking-pop.c
        src/containers.c
        src/laravel-queue-module.c
        src/laravel-pop.c
        src/laravel-push.c
        src/laravel-later.c
        src/laravel-delete-reserved.c
        src/laravel-release-reserved.c
        vendor/cJSON.c
)
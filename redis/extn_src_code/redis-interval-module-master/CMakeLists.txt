cmake_minimum_required(VERSION 3.17)
project(redis_interval_module C)

set(CMAKE_C_STANDARD 99)

add_library(
        redis-interval-module
        SHARED
        src/interval-set.h
        src/interval-set.c
        src/redis-interval-module.c
        src/interval.h
        src/interval.c
        src/io.h
        src/io.c
        src/treap-tree.c
        src/hashmap.h
        src/hashmap.c
        src/util.c
        src/util.h
        src/linked-list.h
        src/linked-list.c
        src/item.h
        src/item.c
        src/output.h
        src/output.c
)

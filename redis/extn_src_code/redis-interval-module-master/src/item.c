#include "item.h"
#include "redismodule.h"

Item *newItem(char *member, Interval *interval) {
    Item *create = RedisModule_Alloc(sizeof(Item));
    create->interval = interval;
    create->member = member;
    return create;
}

void freeItem(Item *item) {
    freeInterval(item->interval);
    RedisModule_Free(item->member);
    RedisModule_Free(item);
}
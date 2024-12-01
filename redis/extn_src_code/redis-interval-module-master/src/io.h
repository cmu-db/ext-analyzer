#ifndef IO_H
#define IO_H

#include "redismodule.h"

void *IntervalSetTypeRdbLoad(RedisModuleIO *rdb, int encver);
void IntervalSetTypeRdbSave(RedisModuleIO *rdb, void *value);
void IntervalSetTypeAofRewrite(RedisModuleIO *aof, RedisModuleString *key, void *value);
void IntervalSetTypeFree(void *value);

#endif
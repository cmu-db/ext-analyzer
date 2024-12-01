#ifndef OUTPUT_H
#define OUTPUT_H

#include "redismodule.h"
#include "item.h"

typedef struct OutputContext {
    RedisModuleCtx *ctx;
    RedisModuleString *destinationKeyName;
    RedisModuleType *type;
    RedisModuleKey *destinationKey;
    int len;
    int (*beforeOutput)(struct OutputContext *outputContext);
    int (*output)(struct OutputContext *outputContext, Item *item);
    int (*afterOutput)(struct OutputContext *outputContext);
} OutputContext;

OutputContext *buildOutputContextFor(RedisModuleCtx *ctx, RedisModuleString *outputKey, RedisModuleType *type);
void freeOutputContext(OutputContext *outputContext);

#endif
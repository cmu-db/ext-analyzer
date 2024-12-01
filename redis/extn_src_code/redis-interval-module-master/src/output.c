#include "output.h"
#include "float.h"
#include "interval-set.h"

void outputInfBoundOrValue(RedisModuleCtx *ctx, double value) {
    if (value == -DBL_MAX) {
        RedisModule_ReplyWithCString(ctx, "-inf");
    } else if (value == DBL_MAX) {
        RedisModule_ReplyWithCString(ctx, "+inf");
    } else {
        RedisModule_ReplyWithDouble(ctx, value);
    }
}

int beforeOutputReply(OutputContext *outputContext) {
    RedisModule_ReplyWithArray(outputContext->ctx, REDISMODULE_POSTPONED_ARRAY_LEN);
    return REDISMODULE_OK;
}

int afterOutputReply(OutputContext *outputContext) {
    RedisModule_ReplySetArrayLength(outputContext->ctx, outputContext->len);
    return REDISMODULE_OK;
}

int replyInterval(OutputContext *outputContext, Item *item) {
    RedisModule_ReplyWithArray(outputContext->ctx, 5);
    RedisModule_ReplyWithCString(outputContext->ctx, item->member);
    RedisModule_ReplyWithDouble(outputContext->ctx, item->interval->includeLowerBound);
    outputInfBoundOrValue(outputContext->ctx, item->interval->lowerBound);
    RedisModule_ReplyWithDouble(outputContext->ctx, item->interval->includeUpperBound);
    outputInfBoundOrValue(outputContext->ctx, item->interval->upperBound);
    outputContext->len++;
    return REDISMODULE_OK;
}

int beforeOutputStore(OutputContext *outputContext) {
    RedisModuleKey *key = RedisModule_OpenKey(outputContext->ctx, outputContext->destinationKeyName, REDISMODULE_WRITE);
    int type = RedisModule_KeyType(key);
    if (type != REDISMODULE_KEYTYPE_EMPTY && RedisModule_ModuleTypeGetType(key) != outputContext->type) {
        return RedisModule_ReplyWithError(outputContext->ctx, "destination key type is not an interval set");
    }
    outputContext->destinationKey = key;
    return REDISMODULE_OK;
}

int afterOutputStore(OutputContext *outputContext) {
    RedisModule_ReplyWithLongLong(outputContext->ctx, outputContext->len);
    return REDISMODULE_OK;
}

int storeInterval(OutputContext *outputContext, Item *item) {
    IntervalSet *intervalSet;
    int type = RedisModule_KeyType(outputContext->destinationKey);
    if (type == REDISMODULE_KEYTYPE_EMPTY) {
        intervalSet = createIntervalSet();
        RedisModule_ModuleTypeSetValue(outputContext->destinationKey, outputContext->type, intervalSet);
    } else {
        intervalSet = RedisModule_ModuleTypeGetValue(outputContext->destinationKey);
    }
    outputContext->len += add(intervalSet, item->member, item->interval);
    return REDISMODULE_OK;
}

OutputContext *buildOutputContextFor(RedisModuleCtx *ctx, RedisModuleString *outputKey, RedisModuleType *type) {
    OutputContext *context = RedisModule_Alloc(sizeof (OutputContext));
    if (outputKey != NULL) {
        context->beforeOutput = beforeOutputStore;
        context->output = storeInterval;
        context->afterOutput = afterOutputStore;
    } else {
        context->beforeOutput = beforeOutputReply;
        context->output = replyInterval;
        context->afterOutput = afterOutputReply;
    }
    context->ctx = ctx;
    context->destinationKeyName = outputKey;
    context->destinationKey = NULL;
    context->len = 0;
    context->type = type;
    return context;
}

void freeOutputContext(OutputContext *outputContext) {
    RedisModule_Free(outputContext);
}
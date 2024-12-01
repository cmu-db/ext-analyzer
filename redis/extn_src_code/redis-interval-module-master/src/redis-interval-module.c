#include "redismodule.h"
#include "interval-set.h"
#include "io.h"
#include <string.h>
#include "util.h"
#include "float.h"
#include "output.h"
#include "limits.h"

static RedisModuleType *IntervalSetType;

int iAddCommand(RedisModuleCtx *ctx, RedisModuleString **argv, int argc) {
    RedisModule_AutoMemory(ctx);
    if (argc < 4 || argc % 2 != 0) {
        return RedisModule_WrongArity(ctx);
    }
    RedisModuleString *keyName = argv[1];
    RedisModuleKey *key = RedisModule_OpenKey(ctx, keyName, REDISMODULE_WRITE);
    int type = RedisModule_KeyType(key);
    if (type != REDISMODULE_KEYTYPE_EMPTY && RedisModule_ModuleTypeGetType(key) != IntervalSetType) {
        return RedisModule_ReplyWithError(ctx, REDISMODULE_ERRORMSG_WRONGTYPE);
    } else {
        IntervalSet *intervalSet;
        if (type == REDISMODULE_KEYTYPE_EMPTY) {
            intervalSet = createIntervalSet();
            RedisModule_ModuleTypeSetValue(key, IntervalSetType, intervalSet);
        } else {
            intervalSet = RedisModule_ModuleTypeGetValue(key);
        }
        for (int i = 2; i < argc; i += 2) {
            Interval *interval = parseInterval(argv[i]);
            if (interval == NULL) {
                return RedisModule_ReplyWithError(ctx, "incorrect interval");
            }
            RedisModule_Free(interval);
        }
        int reply = 0;
        for (int i = 2; i < argc; i += 2) {
            Interval *interval = parseInterval(argv[i]);
            char *member = RedisModule_Strdup(RedisModule_StringPtrLen(argv[i+1], NULL));
            reply += add(intervalSet, member, interval);
        }
        return RedisModule_ReplyWithLongLong(ctx, reply);
    }
}

int iCardCommand(RedisModuleCtx *ctx, RedisModuleString **argv, int argc) {
    RedisModule_AutoMemory(ctx);
    if (argc != 2) {
        return RedisModule_WrongArity(ctx);
    }
    RedisModuleString *keyName = argv[1];
    RedisModuleKey *key = RedisModule_OpenKey(ctx, keyName, REDISMODULE_READ);
    int type = RedisModule_KeyType(key);
    if (type != REDISMODULE_KEYTYPE_EMPTY && RedisModule_ModuleTypeGetType(key) != IntervalSetType) {
        return RedisModule_ReplyWithError(ctx, REDISMODULE_ERRORMSG_WRONGTYPE);
    } else {
        IntervalSet *intervalSet;
        if (type == REDISMODULE_KEYTYPE_EMPTY) {
            return RedisModule_ReplyWithLongLong(ctx, 0);
        } else {
            intervalSet = RedisModule_ModuleTypeGetValue(key);
            int len = intervalSet->hash->len;
            return RedisModule_ReplyWithLongLong(ctx, len);
        }
    }
}

int iContainsCommand(RedisModuleCtx *ctx, RedisModuleString **argv, int argc) {
    RedisModule_AutoMemory(ctx);
    if (argc < 3) {
        return RedisModule_WrongArity(ctx);
    }
    RedisModuleString *keyName = argv[1];
    RedisModuleKey *key = RedisModule_OpenKey(ctx, keyName, REDISMODULE_READ);
    double valueToSearch;
    if (!redisModuleStringToDouble(argv[2], &valueToSearch)) {
        return RedisModule_ReplyWithError(ctx, "incorrect value");
    }
    long long count = LLONG_MAX;
    if (getLongLongParameter(argv, argc, "count", &count) == REDISMODULE_ERR) {
        return RedisModule_ReplyWithError(ctx, "invalid count");
    }
    if (count < 0) {
        return RedisModule_ReplyWithError(ctx, "invalid count");
    }
    RedisModuleString *destinationKey = NULL;
    if (getRedisModuleStringParameter(argv, argc, "store", &destinationKey) == REDISMODULE_ERR) {
        return RedisModule_ReplyWithError(ctx, "invalid store");
    }
    int type = RedisModule_KeyType(key);
    if (type != REDISMODULE_KEYTYPE_EMPTY && RedisModule_ModuleTypeGetType(key) != IntervalSetType) {
        return RedisModule_ReplyWithError(ctx, REDISMODULE_ERRORMSG_WRONGTYPE);
    } else {
        IntervalSet *intervalSet;
        if (type == REDISMODULE_KEYTYPE_EMPTY) {
            return RedisModule_ReplyWithEmptyArray(ctx);
        } else {
            intervalSet = RedisModule_ModuleTypeGetValue(key);
            OutputContext *outputContext = buildOutputContextFor(ctx, destinationKey, IntervalSetType);
            searchValue(ctx, intervalSet, valueToSearch, count, outputContext);
            freeOutputContext(outputContext);
            return REDISMODULE_OK;
        }
    }
}

int iOverlapsCommand(RedisModuleCtx *ctx, RedisModuleString **argv, int argc) {
    RedisModule_AutoMemory(ctx);
    if (argc < 3) {
        return RedisModule_WrongArity(ctx);
    }
    RedisModuleString *keyName = argv[1];
    RedisModuleKey *key = RedisModule_OpenKey(ctx, keyName, REDISMODULE_READ);
    Interval *intervalToSearch = parseInterval(argv[2]);
    if (intervalToSearch == NULL) {
        return RedisModule_ReplyWithError(ctx, "incorrect interval");
    }
    long long count = LLONG_MAX;
    if (getLongLongParameter(argv, argc, "count", &count) == REDISMODULE_ERR) {
        return RedisModule_ReplyWithError(ctx, "invalid count");
    }
    if (count < 0) {
        return RedisModule_ReplyWithError(ctx, "invalid count");
    }
    RedisModuleString *destinationKey = NULL;
    if (getRedisModuleStringParameter(argv, argc, "store", &destinationKey) == REDISMODULE_ERR) {
        return RedisModule_ReplyWithError(ctx, "invalid store");
    }
    int type = RedisModule_KeyType(key);
    if (type != REDISMODULE_KEYTYPE_EMPTY && RedisModule_ModuleTypeGetType(key) != IntervalSetType) {
        return RedisModule_ReplyWithError(ctx, REDISMODULE_ERRORMSG_WRONGTYPE);
    } else {
        IntervalSet *intervalSet;
        if (type == REDISMODULE_KEYTYPE_EMPTY) {
            return RedisModule_ReplyWithEmptyArray(ctx);
        } else {
            intervalSet = RedisModule_ModuleTypeGetValue(key);
            OutputContext *outputContext = buildOutputContextFor(ctx, destinationKey, IntervalSetType);
            searchInterval(ctx, intervalSet, intervalToSearch, count, outputContext);
            freeOutputContext(outputContext);
            return REDISMODULE_OK;
        }
    }
}

int iRemCommand(RedisModuleCtx *ctx, RedisModuleString **argv, int argc) {
    RedisModule_AutoMemory(ctx);
    if (argc < 3) {
        return RedisModule_WrongArity(ctx);
    }
    RedisModuleString *keyName = argv[1];
    RedisModuleKey *key = RedisModule_OpenKey(ctx, keyName, REDISMODULE_READ);
    int type = RedisModule_KeyType(key);
    if (type != REDISMODULE_KEYTYPE_EMPTY && RedisModule_ModuleTypeGetType(key) != IntervalSetType) {
        return RedisModule_ReplyWithError(ctx, REDISMODULE_ERRORMSG_WRONGTYPE);
    } else {
        IntervalSet *intervalSet;
        if (type == REDISMODULE_KEYTYPE_EMPTY) {
            return RedisModule_ReplyWithLongLong(ctx, 0);
        } else {
            intervalSet = RedisModule_ModuleTypeGetValue(key);
            int reply = 0;
            for (int i = 2; i < argc; i++) {
                char *member = RedisModule_Strdup(RedisModule_StringPtrLen(argv[i], NULL));
                reply += removeInterval(intervalSet, member);
            }
            return RedisModule_ReplyWithLongLong(ctx, reply);
        }
    }
}

int iScanCommand(RedisModuleCtx *ctx, RedisModuleString **argv, int argc) {
    RedisModule_AutoMemory(ctx);
    if (argc != 5 && argc != 7) {
        return RedisModule_WrongArity(ctx);
    }
    RedisModuleString *keyName = argv[1];
    RedisModuleKey *key = RedisModule_OpenKey(ctx, keyName, REDISMODULE_READ);
    int type = RedisModule_KeyType(key);
    long long cursor;
    RedisModule_StringToLongLong(argv[2], &cursor);
    const char *match;
    long long count = 10;
    if (strcasecmp(RedisModule_StringPtrLen(argv[3], NULL), "match") == 0) {
        match = RedisModule_StringPtrLen(argv[4], NULL);
    } else {
        return RedisModule_ReplyWithError(ctx, "missing match");
    }
    if (argc > 5 && strcasecmp(RedisModule_StringPtrLen(argv[5], NULL), "count") == 0 && argv[6] != NULL) {
        RedisModule_StringToLongLong(argv[6], &count);
    }
    if (cursor < 0) {
        return RedisModule_ReplyWithError(ctx, "invalid cursor");
    }
    if (count < 0) {
        return RedisModule_ReplyWithError(ctx, "invalid count");
    }
    if (type != REDISMODULE_KEYTYPE_EMPTY && RedisModule_ModuleTypeGetType(key) != IntervalSetType) {
        return RedisModule_ReplyWithError(ctx, REDISMODULE_ERRORMSG_WRONGTYPE);
    } else {
        IntervalSet *intervalSet;
        if (type == REDISMODULE_KEYTYPE_EMPTY) {
            RedisModule_ReplyWithArray(ctx, REDISMODULE_POSTPONED_ARRAY_LEN);
            RedisModule_ReplyWithLongLong(ctx, 0);
            RedisModule_ReplyWithEmptyArray(ctx);
            RedisModule_ReplySetArrayLength(ctx, 2);
        } else {
            intervalSet = RedisModule_ModuleTypeGetValue(key);
            OutputContext *outputContext = buildOutputContextFor(ctx, NULL, IntervalSetType);
            scanIntervalSet(ctx, intervalSet, cursor, match, count, outputContext);
            freeOutputContext(outputContext);
        }
        return REDISMODULE_OK;
    }
}

int iGetCommand(RedisModuleCtx *ctx, RedisModuleString **argv, int argc) {
    RedisModule_AutoMemory(ctx);
    if (argc < 3) {
        return RedisModule_WrongArity(ctx);
    }
    RedisModuleString *keyName = argv[1];
    RedisModuleKey *key = RedisModule_OpenKey(ctx, keyName, REDISMODULE_READ);
    int type = RedisModule_KeyType(key);
    if (type != REDISMODULE_KEYTYPE_EMPTY && RedisModule_ModuleTypeGetType(key) != IntervalSetType) {
        return RedisModule_ReplyWithError(ctx, REDISMODULE_ERRORMSG_WRONGTYPE);
    } else {
        IntervalSet *intervalSet;
        if (type == REDISMODULE_KEYTYPE_EMPTY) {
            RedisModule_ReplyWithEmptyArray(ctx);
        } else {
            intervalSet = RedisModule_ModuleTypeGetValue(key);
            OutputContext *outputContext = buildOutputContextFor(ctx, NULL, IntervalSetType);
            if (outputContext->beforeOutput(outputContext) == REDISMODULE_ERR) {
                return REDISMODULE_ERR;
            }
            for (int i = 2; i < argc; i++) {
                const char *member = RedisModule_StringPtrLen(argv[i], NULL);
                Item *item =  get(intervalSet->hash, member);
                if (item != NULL) {
                    outputContext->output(outputContext, item);
                }
            }
            outputContext->afterOutput(outputContext);
            freeOutputContext(outputContext);
        }
        return REDISMODULE_OK;
    }
}

int RedisModule_OnLoad(RedisModuleCtx *ctx) {
    if (RedisModule_Init(ctx, "interval", 1, REDISMODULE_APIVER_1) == REDISMODULE_ERR) {
        return REDISMODULE_ERR;
    }

    RedisModuleTypeMethods tm = {
            .version = REDISMODULE_TYPE_METHOD_VERSION,
            .rdb_load = IntervalSetTypeRdbLoad,
            .rdb_save = IntervalSetTypeRdbSave,
            .aof_rewrite = IntervalSetTypeAofRewrite,
            .free = IntervalSetTypeFree
    };

    IntervalSetType = RedisModule_CreateDataType(ctx, "intertype", 0, &tm);
    if (IntervalSetType == NULL) {
        return REDISMODULE_ERR;
    }
    if (RedisModule_CreateCommand(ctx, "iadd", iAddCommand, "write", 1, 1, 1) == REDISMODULE_ERR) {
        return REDISMODULE_ERR;
    }
    if (RedisModule_CreateCommand(ctx, "irem", iRemCommand, "write", 1, 1, 1) == REDISMODULE_ERR) {
        return REDISMODULE_ERR;
    }
    if (RedisModule_CreateCommand(ctx, "icard", iCardCommand, "readonly", 1, 1, 1) == REDISMODULE_ERR) {
        return REDISMODULE_ERR;
    }
    if (RedisModule_CreateCommand(ctx, "icontains", iContainsCommand, "readonly", 1, 1, 1) == REDISMODULE_ERR) {
        return REDISMODULE_ERR;
    }
    if (RedisModule_CreateCommand(ctx, "ioverlaps", iOverlapsCommand, "readonly", 1, 1, 1) == REDISMODULE_ERR) {
        return REDISMODULE_ERR;
    }
    if (RedisModule_CreateCommand(ctx, "iscan", iScanCommand, "readonly", 1, 1, 1) == REDISMODULE_ERR) {
        return REDISMODULE_ERR;
    }
    if (RedisModule_CreateCommand(ctx, "iget", iGetCommand, "readonly", 1, 1, 1) == REDISMODULE_ERR) {
        return REDISMODULE_ERR;
    }
    return REDISMODULE_OK;
}
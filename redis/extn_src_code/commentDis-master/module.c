#include "redismodule.h"


#define RMUtil_RegisterReadCmd(ctx, cmd, f) \
    if (RedisModule_CreateCommand(ctx, cmd, f, "readonly fast allow-loading allow-stale", \
        1, 1, 1) == REDISMODULE_ERR) return REDISMODULE_ERR;

int CommentCommand(RedisModuleCtx *ctx, RedisModuleString **argv, int argc)
{
    RedisModule_ReplyWithSimpleString(ctx, "");
    return REDISMODULE_OK;    
}

int RedisModule_OnLoad(RedisModuleCtx *ctx)
{
    // Register the module itself
    if (RedisModule_Init(ctx, "COMMENTDIS", 1, REDISMODULE_APIVER_1) ==
      REDISMODULE_ERR) {
        return REDISMODULE_ERR;
    }

    // register CommentCommand - using the shortened utility registration macro
    RMUtil_RegisterReadCmd(ctx, "#",        CommentCommand);
    RMUtil_RegisterReadCmd(ctx, ";;",       CommentCommand);
    RMUtil_RegisterReadCmd(ctx, "//",       CommentCommand);
    RMUtil_RegisterReadCmd(ctx, "/*",       CommentCommand);
    RMUtil_RegisterReadCmd(ctx, "/**",      CommentCommand);
    RMUtil_RegisterReadCmd(ctx, "comment",  CommentCommand);
    RMUtil_RegisterReadCmd(ctx, "comment:", CommentCommand);

    return REDISMODULE_OK;
}

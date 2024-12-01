/*
 * Copyright (c) 2018, Hamid Alaei Varnosfaderani
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions are met:
 *
 * * Redistributions of source code must retain the above copyright notice, this
 *   list of conditions and the following disclaimer.
 *
 * * Redistributions in binary form must reproduce the above copyright notice,
 *   this list of conditions and the following disclaimer in the documentation
 *   and/or other materials provided with the distribution.
 *
 * * Neither the name of the copyright holder nor the names of its
 *   contributors may be used to endorse or promote products derived from
 *   this software without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
 * AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
 * DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE LIABLE
 * FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
 * DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR
 * SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER
 * CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY,
 * OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
 * OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */

#include "laravel-later.h"
#include <string.h>
#include "blocking-pop.h"

typedef struct LaravelLaterArguments {
    RedisModuleKey *queue;
    RedisModuleString *strQueue;
    long long delayMs;
    double availableAt;
    RedisModuleString *strAvailableAt;
    RedisModuleString *payload;
} LaravelLaterArguments;

void releaseLaravelLaterArguments(RedisModuleCtx *ctx, LaravelLaterArguments *arguments)
{
    if (arguments->queue) {
        RedisModule_CloseKey(arguments->queue);
        arguments->queue = NULL;
    }
    if (arguments->strAvailableAt) {
        RedisModule_FreeString(ctx, arguments->strAvailableAt);
        arguments->strAvailableAt = NULL;
    }
}

LaravelLaterArguments * getLaravelLaterArguments(RedisModuleCtx *ctx, RedisModuleString **argv, int argc, LaravelLaterArguments *arguments)
{
    if (argc != 4) {
        RedisModule_WrongArity(ctx);
        return NULL;
    }

    memset(arguments, 0, sizeof(LaravelLaterArguments));

    arguments->queue = RedisModule_OpenKey(ctx, argv[1], REDISMODULE_WRITE);
    arguments->strQueue = argv[1];
    switch (RedisModule_KeyType(arguments->queue)) {
        case REDISMODULE_KEYTYPE_EMPTY:
            break;
        case REDISMODULE_KEYTYPE_ZSET:
            break;
        default:
            releaseLaravelLaterArguments(ctx, arguments);
            RedisModule_ReplyWithError(ctx, "ERR WRONG KEY TYPE FOR KEYS[1] (sorted set expected for delayed queue)");
            return NULL;
    }

    if (RedisModule_StringToLongLong(argv[2], &arguments->delayMs) != REDISMODULE_OK) {
        releaseLaravelLaterArguments(ctx, arguments);
        RedisModule_ReplyWithError(ctx, "ERR ARGV[1] IS NOT A VALID INTEGER (delay in milliseconds)");
        return NULL;
    }
    arguments->availableAt = msdelayToTime(arguments->delayMs);
    arguments->strAvailableAt = RedisModule_CreateStringPrintf(ctx, "%f", arguments->availableAt);

    arguments->payload = argv[3];

    return arguments;
}

int Laravel_Later_Command(RedisModuleCtx *ctx, RedisModuleString **argv, int argc)
{
    LaravelLaterArguments arguments;
    if (! getLaravelLaterArguments(ctx, argv, argc, &arguments)) {
        return REDISMODULE_ERR;
    }

    int flags = 0;
    if (RedisModule_ZsetAdd(arguments.queue, arguments.availableAt, arguments.payload, &flags) != REDISMODULE_OK) {
        RedisModule_ReplyWithError(ctx, "ERR Unknown error in zadd");
    } else {
        RedisModule_Replicate(ctx, "zadd", "sss", arguments.strQueue, arguments.strAvailableAt, arguments.payload);
        RedisModule_ReplyWithLongLong(ctx, (flags & REDISMODULE_ZADD_ADDED) ? 1 : 0);
        updateTimerFor(ctx, arguments.strQueue, ":delayed");
    }

    releaseLaravelLaterArguments(ctx, &arguments);

    return REDISMODULE_OK;
}

int Create_Laravel_Later_Command(RedisModuleCtx *ctx)
{
    if (RedisModule_CreateCommand(ctx, "laravel.later", Laravel_Later_Command, "write deny-oom fast", 1, 1, 1)
        == REDISMODULE_ERR) {
        return REDISMODULE_ERR;
    }

    return REDISMODULE_OK;
}

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

#include <string.h>
#include "laravel-delete-reserved.h"
#include "blocking-pop.h"

typedef struct LaravelDeleteArguments {
    RedisModuleKey *reserved;
    RedisModuleString *strReserved;
    RedisModuleString *payload;
} LaravelDeleteArguments;

void releaseLaravelDeleteArguments(LaravelDeleteArguments *arguments)
{
    if (arguments->reserved) {
        RedisModule_CloseKey(arguments->reserved);
        arguments->reserved = NULL;
    }
}

LaravelDeleteArguments *getLaravelDeleteArguments(RedisModuleCtx *ctx, RedisModuleString **argv, int argc, LaravelDeleteArguments *arguments)
{
    if (argc != 3) {
        RedisModule_WrongArity(ctx);
        return NULL;
    }

    memset(arguments, 0, sizeof(LaravelDeleteArguments));

    arguments->reserved = RedisModule_OpenKey(ctx, argv[1], REDISMODULE_WRITE);
    switch (RedisModule_KeyType(arguments->reserved)) {
        case REDISMODULE_KEYTYPE_EMPTY:
            break;
        case REDISMODULE_KEYTYPE_ZSET:
            break;
        default:
            RedisModule_ReplyWithError(ctx, "ERR WRONG KEY TYPE FOR KEYS[1] (zset expected for reserved queue)");
            return NULL;
    }
    arguments->strReserved = argv[1];

    arguments->payload = argv[2];

    return arguments;
}

int Laravel_Delete_Command(RedisModuleCtx *ctx, RedisModuleString **argv, int argc)
{
    LaravelDeleteArguments arguments;
    if (! getLaravelDeleteArguments(ctx, argv, argc, &arguments)) {
        releaseLaravelDeleteArguments(&arguments);
        return REDISMODULE_ERR;
    }

    RedisModuleCallReply *deleted = RedisModule_Call(ctx, "zrem", "ss!", arguments.strReserved, arguments.payload);
    RedisModule_FreeCallReply(deleted);

    RedisModule_ReplyWithSimpleString(ctx, "OK");
    updateTimerFor(ctx, arguments.strReserved, ":reserved");

    releaseLaravelDeleteArguments(&arguments);
    return REDISMODULE_OK;
}

int Create_Laravel_Delete_Reserved_Command(RedisModuleCtx *ctx)
{
    if (RedisModule_CreateCommand(ctx, "laravel.delete", Laravel_Delete_Command, "write deny-oom fast", 1, 1, 1)
        == REDISMODULE_ERR) {
        return REDISMODULE_ERR;
    }

    return REDISMODULE_OK;
}

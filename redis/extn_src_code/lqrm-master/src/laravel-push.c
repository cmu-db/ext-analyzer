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
#include "laravel-push.h"
#include "blocking-pop.h"

typedef struct LaravelPushArguments {
    RedisModuleKey *queue;
    RedisModuleString *strQueue;
    RedisModuleString *job;
} LaravelPushArguments;


void releaseLaravelPushArguments(LaravelPushArguments *arguments)
{
    if (arguments->queue) {
        RedisModule_CloseKey(arguments->queue);
        arguments->queue = NULL;
    }
}

LaravelPushArguments * getLaravelPushArguments(RedisModuleCtx *ctx, RedisModuleString **argv, int argc, LaravelPushArguments *arguments)
{
    if (argc != 3) {
        RedisModule_WrongArity(ctx);
        return NULL;
    }

    memset(arguments, 0, sizeof(LaravelPushArguments));

    arguments->queue = RedisModule_OpenKey(ctx, argv[1], REDISMODULE_WRITE);
    arguments->strQueue = argv[1];
    switch (RedisModule_KeyType(arguments->queue)) {
        case REDISMODULE_KEYTYPE_EMPTY:
            break;
        case REDISMODULE_KEYTYPE_LIST:
            break;
        default:
            releaseLaravelPushArguments(arguments);
            RedisModule_ReplyWithError(ctx, "ERR WRONG KEY TYPE FOR KEYS[1] (list expected for the main queue)");
            return NULL;
    }

    arguments->job = argv[2];

    return arguments;
}

int Laravel_Push_Command(RedisModuleCtx *ctx, RedisModuleString **argv, int argc)
{
    LaravelPushArguments arguments;
    if (! getLaravelPushArguments(ctx, argv, argc, &arguments)) {
        return REDISMODULE_ERR;
    }

    if (RedisModule_ListPush(arguments.queue, REDISMODULE_LIST_TAIL, arguments.job) != REDISMODULE_OK) {
        RedisModule_ReplyWithError(ctx, "ERR Unknown error in rpush");
    } else {
        RedisModule_Replicate(ctx, "rpush", "ss", arguments.strQueue, arguments.job);
        RedisModuleCallReply *reply = RedisModule_Call(ctx, "llen", "s", arguments.strQueue);
        RedisModule_ReplyWithLongLong(ctx, RedisModule_CallReplyInteger(reply));
        RedisModule_FreeCallReply(reply);
        jobsWasPushed(RedisModule_GetSelectedDb(ctx), arguments.strQueue, 1);
    }

    releaseLaravelPushArguments(&arguments);

    return REDISMODULE_OK;
}

int Create_Laravel_Push_Command(RedisModuleCtx *ctx)
{
    if (RedisModule_CreateCommand(ctx, "laravel.push", Laravel_Push_Command, "write deny-oom fast", 1, 1, 1)
        == REDISMODULE_ERR) {
        return REDISMODULE_ERR;
    }

    return REDISMODULE_OK;
}
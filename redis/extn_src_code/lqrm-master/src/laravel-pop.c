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

#include "laravel-pop.h"

#include <string.h>

#include "../vendor/cJSON.h"
#include "blocking-pop.h"

int openLaravelPopKeys(RedisModuleCtx *ctx, LaravelPopArguments *arguments)
{
    if (! arguments->list) {
        arguments->list = RedisModule_OpenKey(ctx, arguments->strList, REDISMODULE_WRITE);
        int type = RedisModule_KeyType(arguments->list);
        if (type != REDISMODULE_KEYTYPE_EMPTY && type != REDISMODULE_KEYTYPE_LIST) {
            return REDISMODULE_ERR;
        }
    }
    if (! arguments->delayed) {
        arguments->delayed = RedisModule_OpenKey(ctx, arguments->strDelayed, REDISMODULE_WRITE);
        int type = RedisModule_KeyType(arguments->delayed);
        if (type != REDISMODULE_KEYTYPE_EMPTY && type != REDISMODULE_KEYTYPE_ZSET) {
            return REDISMODULE_ERR;
        }
    }
    if (! arguments->reserved) {
        arguments->reserved = RedisModule_OpenKey(ctx, arguments->strReserved, REDISMODULE_WRITE);
        int type = RedisModule_KeyType(arguments->reserved);
        if (type != REDISMODULE_KEYTYPE_EMPTY && type != REDISMODULE_KEYTYPE_ZSET) {
            return REDISMODULE_ERR;
        }
    }
    return REDISMODULE_OK;
}

void closeLaravelPopKeys(LaravelPopArguments *arguments)
{
    if (arguments->list) {
        RedisModule_CloseKey(arguments->list);
        arguments->list = NULL;
    }
    if (arguments->delayed) {
        RedisModule_CloseKey(arguments->delayed);
        arguments->delayed = NULL;
    }
    if (arguments->reserved) {
        RedisModule_CloseKey(arguments->reserved);
        arguments->reserved = NULL;
    }
}

void releaseLaravelPopArguments(RedisModuleCtx *ctx, LaravelPopArguments *arguments)
{
    if (! arguments) {
        return;
    }
    closeLaravelPopKeys(arguments);

    RedisModule_Free(arguments);
}

void prepareArgumentsForBlockingPop(RedisModuleCtx *ctx, LaravelPopArguments *arguments)
{
    arguments->strList = RedisModule_CreateStringFromString(ctx, arguments->strList);
    arguments->strDelayed = RedisModule_CreateStringFromString(ctx, arguments->strDelayed);
    arguments->strReserved = RedisModule_CreateStringFromString(ctx, arguments->strReserved);
}

LaravelPopArguments * getLaravelPopArguments(RedisModuleCtx *ctx, RedisModuleString **argv, int argc)
{
    if (argc != 6) {
        RedisModule_WrongArity(ctx);
        return NULL;
    }
    LaravelPopArguments *arguments = RedisModule_Alloc(sizeof(LaravelPopArguments));
    memset(arguments, 0, sizeof(LaravelPopArguments));

    arguments->list = RedisModule_OpenKey(ctx, argv[1], REDISMODULE_WRITE);
    arguments->strList = argv[1];
    switch (RedisModule_KeyType(arguments->list)) {
        case REDISMODULE_KEYTYPE_EMPTY:
            break;
        case REDISMODULE_KEYTYPE_LIST:
            break;
        default:
            releaseLaravelPopArguments(ctx, arguments);
            RedisModule_ReplyWithError(ctx, "ERR WRONG KEY TYPE FOR KEYS[1] (list expected for main queue)");
            return NULL;
    }
    arguments->delayed = RedisModule_OpenKey(ctx, argv[2], REDISMODULE_WRITE);
    arguments->strDelayed = argv[2];
    switch (RedisModule_KeyType(arguments->delayed)) {
        case REDISMODULE_KEYTYPE_EMPTY:
            break;
        case REDISMODULE_KEYTYPE_ZSET:
            break;
        default:
            releaseLaravelPopArguments(ctx, arguments);
            RedisModule_ReplyWithError(ctx, "ERR WRONG KEY TYPE FOR KEYS[2] (zset expected for delayed queue)");
            return NULL;
    }
    arguments->reserved = RedisModule_OpenKey(ctx, argv[3], REDISMODULE_WRITE);
    arguments->strReserved = argv[3];
    switch (RedisModule_KeyType(arguments->reserved)) {
        case REDISMODULE_KEYTYPE_EMPTY:
            break;
        case REDISMODULE_KEYTYPE_ZSET:
            break;
        default:
            releaseLaravelPopArguments(ctx, arguments);
            RedisModule_ReplyWithError(ctx, "ERR WRONG KEY TYPE FOR KEYS[3] (zset expected for reserved queue)");
            return NULL;
    }
    if (RedisModule_StringToLongLong(argv[4], &arguments->retryAfterMs) != REDISMODULE_OK) {
        releaseLaravelPopArguments(ctx, arguments);
        RedisModule_ReplyWithError(ctx, "ERR ARGV[1] IS NOT A VALID INTEGER (retry after in milliseconds)");
    }

    if (RedisModule_StringToLongLong(argv[5], &arguments->blockFor) != REDISMODULE_OK) {
        releaseLaravelPopArguments(ctx, arguments);
        RedisModule_ReplyWithError(ctx, "ERR ARGV[2] IS NOT A VALID INTEGER (blockFor in milliseconds)");
        return NULL;
    }
    arguments->jobWasAssigned = 0;
    arguments->jobWasDelivered = 0;
    return arguments;
}

int reply_blocking_pop(RedisModuleCtx *ctx, RedisModuleString **argv, int argc);

int timeout_blocking_pop(RedisModuleCtx *ctx, RedisModuleString **argv, int argc)
{
    removeFromWaitingList(RedisModule_GetSelectedDb(ctx), RedisModule_GetBlockedClientHandle(ctx));
    return RedisModule_ReplyWithNull(ctx);
}

void disconnect_blocking_pop(RedisModuleCtx *ctx, RedisModuleBlockedClient *bc)
{
    removeFromWaitingList(RedisModule_GetSelectedDb(ctx), bc);
}

void free_blocking_pop_data(RedisModuleCtx *ctx, void *data)
{
    LaravelPopArguments *arguments = data;
    if (arguments->jobWasAssigned && ! arguments->jobWasDelivered) {
        // unblock another client because this one timed-out/disconnected after job was assigned but before delivered.
        jobsWasPushed(RedisModule_GetSelectedDb(ctx), arguments->strList, 1);
    }
    RedisModule_FreeString(ctx, arguments->strList);
    RedisModule_FreeString(ctx, arguments->strDelayed);
    RedisModule_FreeString(ctx, arguments->strReserved);
    releaseLaravelPopArguments(ctx, data);
}

RedisModuleString * reserveJob(RedisModuleCtx *ctx, LaravelPopArguments *arguments, RedisModuleString *job)
{
    size_t len;
    const char *str = RedisModule_StringPtrLen(job, &len);
    // Validate string does not have 0
    for (size_t i = 0; i < len; ++i) {
        if (! str[i]) {
            return NULL;
        }
    }
    // Convert the string to zero-terminated c-string
    char *cstr = RedisModule_PoolAlloc(ctx, len + 1);
    memcpy(cstr, str, len);
    cstr[len] = 0;
    // Parse the json
    cJSON *json = cJSON_Parse(cstr); // Pool Memory Management
    if (json == NULL) {
        return NULL;
    }
    // Validate json is object
    if (cJSON_IsObject(json)) {
        cJSON * attempts = cJSON_GetObjectItemCaseSensitive(json, "attempts");
        // Validate json has attempts
        if (cJSON_IsNumber(attempts)) {
            // Increment attempts
            cJSON *newAttempts = cJSON_CreateNumber(attempts->valueint + 1); // Pool Memory Management
            cJSON_ReplaceItemInObjectCaseSensitive(json, "attempts", newAttempts);
            // Print json
            char * rJob = cJSON_PrintUnformatted(json); // Pool Memory Management
            RedisModuleString *rStrJob = RedisModule_CreateString(ctx, rJob, strlen(rJob));
            double availableAt = msdelayToTime(arguments->retryAfterMs);
            RedisModule_ZsetAdd(arguments->reserved, availableAt, rStrJob, NULL);
            RedisModuleString *strAvailableAt = RedisModule_CreateStringPrintf(ctx, "%f", availableAt);
            RedisModule_Replicate(ctx, "zadd", "sss", arguments->strReserved, strAvailableAt, rStrJob);
            RedisModule_FreeString(ctx, strAvailableAt);
            return rStrJob;
        }
    }
    return NULL;
}


#define JOB_RETRIEVAL_DONE 0
#define JOB_RETRIEVAL_NEEDS_BLOCKING 1

int retrieveNextJob(RedisModuleCtx *ctx, LaravelPopArguments *arguments)
{
    RedisModuleString *job = RedisModule_ListPop(arguments->list, REDISMODULE_LIST_HEAD);
    if (job) {
        RedisModule_Replicate(ctx, "lpop", "s", arguments->strList);
        RedisModuleString *reservedJob = reserveJob(ctx, arguments, job);
        if (reservedJob) {
            RedisModule_ReplyWithArray(ctx, 2);
            RedisModule_ReplyWithString(ctx, job);
            RedisModule_ReplyWithString(ctx, reservedJob);
            RedisModule_FreeString(ctx, reservedJob);
        } else {
            RedisModule_ReplyWithError(ctx, "ERR AN INVALID JOB DROPPED FROM THE QUEUE");
        }
        RedisModule_FreeString(ctx, job);
        return JOB_RETRIEVAL_DONE;
    } else {
        if (arguments->blockFor < 1) {
            RedisModule_ReplyWithNull(ctx);
            return JOB_RETRIEVAL_DONE;
        } else {
            closeLaravelPopKeys(arguments);
            RedisModuleBlockedClient *bc = RedisModule_BlockClient(
                    ctx, reply_blocking_pop, timeout_blocking_pop, free_blocking_pop_data, arguments->blockFor);
            RedisModule_SetDisconnectCallback(bc,disconnect_blocking_pop);
            prepareArgumentsForBlockingPop(ctx, arguments);
            addToWaitingList(RedisModule_GetSelectedDb(ctx), bc, arguments);
            createTimerFor(ctx, arguments->strDelayed, ":delayed");
            createTimerFor(ctx, arguments->strReserved, ":reserved");
        }
        return JOB_RETRIEVAL_NEEDS_BLOCKING;
    }
}

int reply_blocking_pop(RedisModuleCtx *ctx, RedisModuleString **argv, int argc)
{
    LaravelPopArguments *arguments = RedisModule_GetBlockedClientPrivateData(ctx);
    if (openLaravelPopKeys(ctx, arguments) != REDISMODULE_OK) {
        RedisModule_ReplyWithError(ctx, "ERR Wrong key type detected after unblock");
    }
    arguments->blockFor = 0;
    retrieveNextJob(ctx, arguments);
    arguments->jobWasDelivered = 1;
}

int Laravel_Pop_Command(RedisModuleCtx *ctx, RedisModuleString **argv, int argc)
{
    LaravelPopArguments *arguments = getLaravelPopArguments(ctx, argv, argc);
    if (! arguments) {
        return REDISMODULE_OK;
    }
    double currentTime = (double)ustime()/1000000;
    long long migrated =
            migrateExpiredJobs(ctx, arguments->list, arguments->strList, currentTime,
                                            arguments->delayed, arguments->strDelayed, ":delayed") +
            migrateExpiredJobs(ctx, arguments->list, arguments->strList, currentTime,
                                   arguments->reserved, arguments->strReserved, ":reserved");
    if (retrieveNextJob(ctx, arguments) == JOB_RETRIEVAL_DONE) {
        releaseLaravelPopArguments(ctx, arguments);
        if (migrated > 1) {
            // If there is any blocked client, one migrated job is just retrieved.
            // So we signal other migrated jobs to the blocked cliens.
            jobsWasPushed(RedisModule_GetSelectedDb(ctx), arguments->strList, migrated - 1);
        }
    } // else: migrated must be 0
    return REDISMODULE_OK;
}

int Create_Laravel_Pop_Command(RedisModuleCtx *ctx) {
    if (RedisModule_CreateCommand(ctx, "laravel.pop", Laravel_Pop_Command, "write deny-oom fast", 1, 3, 3)
        == REDISMODULE_ERR) {
        return REDISMODULE_ERR;
    }

    return REDISMODULE_OK;
}

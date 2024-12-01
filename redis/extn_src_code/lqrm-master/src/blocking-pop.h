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

#ifndef LARAVEL_QUEUE_BLOCKING_POP_H
#define LARAVEL_QUEUE_BLOCKING_POP_H

#include "redismodule.h"

typedef struct LaravelPopArguments
{
    RedisModuleKey *list;
    RedisModuleKey *delayed;
    RedisModuleKey *reserved;
    RedisModuleString *strList;
    RedisModuleString *strDelayed;
    RedisModuleString *strReserved;
    long long retryAfterMs;
    long long blockFor;
    char jobWasAssigned;
    char jobWasDelivered;
} LaravelPopArguments;

/* Return the UNIX time in microseconds */
long long ustime(void);

#define msdelayToMstime(delay) (ustime()/1000+(delay))
#define msdelayToTime(delay) (double)msdelayToMstime(delay)/1000

int initWaitingList();
void addToWaitingList(int db, RedisModuleBlockedClient *bc, LaravelPopArguments *arguments);
void removeFromWaitingList(int db, RedisModuleBlockedClient *bc);
void jobsWasPushed(int db, RedisModuleString *strList, long long n);
void updateTimerFor(RedisModuleCtx *ctx, RedisModuleString *strZset, const char *suffix);
void createTimerFor(RedisModuleCtx *ctx, RedisModuleString *strZset, const char *suffix);

/**
 * Migrate Expired Jobs
 *
 * @return number of migrated jobs.
 */
long long migrateExpiredJobs(RedisModuleCtx *ctx, RedisModuleKey *list, RedisModuleString *strList, double currentTime,
                             RedisModuleKey *zset, RedisModuleString *strZset, const char *suffix);

#endif //LARAVEL_QUEUE_BLOCKING_POP_H

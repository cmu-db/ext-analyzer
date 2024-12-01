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
#include <sys/time.h>
#include "redismodule.h"
#include "blocking-pop.h"
#include "containers.h"

typedef struct BlockingPopDS
{
    /**
     * Dictionary [blocked client => list string]
     */
    RedisModuleDict *blockedClients;

    /**
     * Dictionary[list strings => waiting list DLDictionary[list string => ClientArgumentPair]]
     * Each WorkerList is a DLDictionary (sting list => ClientArgumentPair)
     */
    RedisModuleDict *waitingClients;

    /**
     * Dictionary[delayed/reserved strings => timers]
     */
    RedisModuleDict *timers;
} BlockingPopDS;

/**
 * Dictionary [database => BlockingPopDS]
 */
RedisModuleDict *dbs;

typedef struct ClientArgumentsPair
{
    RedisModuleBlockedClient *bc;
    LaravelPopArguments *arguments;
} ClientArgumentsPair;

ClientArgumentsPair *createClientArgumentsPair(RedisModuleBlockedClient *bc, LaravelPopArguments *arguments)
{
    ClientArgumentsPair *pair = RedisModule_Alloc(sizeof(ClientArgumentsPair));
    pair->arguments = arguments;
    pair->bc = bc;
    return pair;
}

int initWaitingList()
{
    dbs = RedisModule_CreateDict(NULL);

    return REDISMODULE_OK;
}

BlockingPopDS * getBlockingPopDS(int db)
{
    BlockingPopDS *ds = RedisModule_DictGetC(dbs, &db, sizeof(int), NULL);
    if (ds) {
        return ds;
    }
    ds = RedisModule_Alloc(sizeof(BlockingPopDS));
    ds->blockedClients = RedisModule_CreateDict(NULL);
    ds->waitingClients = RedisModule_CreateDict(NULL);
    ds->timers = RedisModule_CreateDict(NULL);
    RedisModule_DictSetC(dbs, &db, sizeof(int), ds);

    return ds;
}

void addToWaitingList(int db, RedisModuleBlockedClient *bc, LaravelPopArguments *arguments)
{
    BlockingPopDS *ds = getBlockingPopDS(db);
    RedisModule_DictSetC(ds->blockedClients, bc, sizeof(RedisModuleBlockedClient *), arguments->strList);

    DLDictionary * waitingList = RedisModule_DictGet(ds->waitingClients, arguments->strList, NULL);
    if (! waitingList) {
        waitingList = DLDictionary_Create();
        RedisModule_DictSet(ds->waitingClients, arguments->strList, waitingList);
    }
    DLDictionary_Set_Back(waitingList, arguments->strList, createClientArgumentsPair(bc, arguments));
}

void removeFromWaitingList(int db, RedisModuleBlockedClient *bc)
{
    BlockingPopDS *ds = getBlockingPopDS(db);
    RedisModuleString *strList = RedisModule_DictGetC(ds->blockedClients, bc, sizeof(RedisModuleBlockedClient *), NULL);
    if (! strList) {
        return;
    }
    DLDictionary * waitingList = RedisModule_DictGet(ds->waitingClients, strList, NULL);
    if (! waitingList) {
        return;
    }
    ClientArgumentsPair *pair = DLDictionary_Delete(waitingList, strList);
    if (! pair) {
        return;
    }
    RedisModule_Free(pair);
    // free_blocking_pop_data() in laravel-pop.c will free pair->arguments
    if (! waitingList->list->size) {
        DLDictionary_Drop(waitingList, NULL, NULL);
        RedisModule_DictDel(ds->waitingClients, strList, NULL);
    }
    RedisModule_DictDelC(ds->blockedClients, bc, sizeof(RedisModuleBlockedClient *), NULL);
}

/**
 * Deliver jobs to the workers by unblocking the blocked clients.
 *
 * @param strList
 * @param n
 */
void jobsWasPushed(int db, RedisModuleString *strList, long long n)
{
    BlockingPopDS *ds = getBlockingPopDS(db);
    DLDictionary *waitingList = RedisModule_DictGet(ds->waitingClients, strList, NULL);
    if (waitingList == NULL) {
        return;
    }
    DLNode *node;
    uint64_t size = waitingList->list->size;
    while (size > 0 && n > 0) {
        node = waitingList->list->front;
        DLDKeyValue *keyValue = node->data;
        ClientArgumentsPair *pair = keyValue->value;
        pair->arguments->jobWasAssigned = 1;
        RedisModule_UnblockClient(pair->bc, pair->arguments);
        n--;
        removeFromWaitingList(db, pair->bc);
        size--;
    }
}

typedef struct TimerData
{
    RedisModuleString *strList;
    RedisModuleString *strZset;
    char suffix[10];
} TimerData;

TimerData * createTimerData(RedisModuleCtx *ctx, RedisModuleString *strZset, const char *suffix)
{
    TimerData *td = RedisModule_Alloc(sizeof(TimerData));
    td->strZset = strZset;
    size_t zlen;
    const char *zstr = RedisModule_StringPtrLen(strZset, &zlen);
    td->strList = RedisModule_CreateString(ctx, zstr, zlen - strlen(suffix));
    strcpy(td->suffix, suffix);
    return td;
}

void freeTimerData(RedisModuleCtx *ctx, TimerData *td)
{
    if (td) {
        if (td->strList) {
            RedisModule_FreeString(ctx, td->strList);
            td->strList = NULL;
        }
        RedisModule_Free(td);
    }
}

/* Return the UNIX time in microseconds */
long long ustime(void) {
    struct timeval tv;
    long long ust;

    gettimeofday(&tv, NULL);
    ust = ((long long)tv.tv_sec)*1000000;
    ust += tv.tv_usec;
    return ust;
}

long long availableAtToMsPeriod(double availableAt)
{
    return (availableAt * 1000000 - ustime()) / 1000;
}

void timerCallback(RedisModuleCtx *ctx, void *data)
{
    int db = RedisModule_GetSelectedDb(ctx);
    BlockingPopDS *ds = getBlockingPopDS(db);
    TimerData *td = data;
    RedisModuleTimerID *timerId;
    RedisModule_DictDel(ds->timers, td->strZset, &timerId);
    RedisModule_Free(timerId);
    RedisModuleKey *list = RedisModule_OpenKey(ctx, td->strList, REDISMODULE_WRITE);
    RedisModuleKey *zset = RedisModule_OpenKey(ctx, td->strZset, REDISMODULE_WRITE);
    // validate key types
    int ltype = RedisModule_KeyType(list);
    int ztype = RedisModule_KeyType(zset);
    if ((ltype == REDISMODULE_KEYTYPE_EMPTY || ltype == REDISMODULE_KEYTYPE_LIST) &&
            (ztype == REDISMODULE_KEYTYPE_EMPTY || ztype == REDISMODULE_KEYTYPE_ZSET)) {
        long long n = migrateExpiredJobs(ctx, list, td->strList, (double)ustime()/1000000, zset, td->strZset, td->suffix);
        jobsWasPushed(db, td->strList, n);
        updateTimerFor(ctx, td->strZset, td->suffix);
    }
    RedisModule_CloseKey(list);
    RedisModule_CloseKey(zset);
    freeTimerData(ctx, td);
}

void jobWillBeAvailable(RedisModuleCtx *ctx, RedisModuleString *strZSet, double availableAt, const char *suffix)
{
    int db = RedisModule_GetSelectedDb(ctx);
    BlockingPopDS *ds = getBlockingPopDS(db);
    size_t slen = strlen(suffix);
    size_t len;
    const char * zset = RedisModule_StringPtrLen(strZSet, &len);
    if (len < slen || memcmp(suffix, zset + len - slen, slen)) {
        // suffix does not match!
        return;
    }
    if (RedisModule_DictGetC(ds->waitingClients, zset, len - slen, NULL) == NULL) {
        // No client is waiting!
        return;
    }
    // Stop the previous timer
    RedisModuleTimerID *timer = RedisModule_DictGet(ds->timers, strZSet, NULL);
    if (timer) {

        void *data;
        RedisModule_StopTimer(ctx, *timer, &data);
        RedisModule_Free(data);
    } else {
        timer = RedisModule_Alloc(sizeof(RedisModuleTimerID));
        RedisModule_DictSet(ds->timers, strZSet, timer);
    }
    // Start a new timer
    TimerData *td = createTimerData(ctx, strZSet, suffix);
    *timer = RedisModule_CreateTimer(ctx, availableAtToMsPeriod(availableAt), timerCallback, td);
}

void jobWontBeAvailable(RedisModuleCtx *ctx, RedisModuleString *strZSet)
{
    int db = RedisModule_GetSelectedDb(ctx);
    BlockingPopDS *ds = getBlockingPopDS(db);
    RedisModuleTimerID *timer = RedisModule_DictGet(ds->timers, strZSet, NULL);
    if (timer) {
        void *data;
        RedisModule_StopTimer(ctx, *timer, &data);
        RedisModule_Free(timer);
        freeTimerData(ctx, data);
        RedisModule_DictDel(ds->timers, strZSet, NULL);
    }
}

int minScore(RedisModuleCtx *ctx, RedisModuleString *key, double *score)
{
    RedisModuleCallReply *reply = RedisModule_Call(ctx, "ZRANGE", "sllc", key, 0ll, 0ll, "WITHSCORES");
    if ( RedisModule_CallReplyType(reply) != REDISMODULE_REPLY_ARRAY || RedisModule_CallReplyLength(reply) != 2) {
        RedisModule_FreeCallReply(reply);
        return 0;
    }
    RedisModuleCallReply *rscore = RedisModule_CallReplyArrayElement(reply, 1);
    RedisModuleString *sscore = RedisModule_CreateStringFromCallReply(rscore);
    RedisModule_StringToDouble(sscore, score);
    RedisModule_FreeString(ctx, sscore);
    RedisModule_FreeCallReply(rscore);
    RedisModule_FreeCallReply(reply);
    return 1;
}

void updateTimerFor(RedisModuleCtx *ctx, RedisModuleString *strZset, const char *suffix)
{
    double score;
    if (minScore(ctx, strZset, &score)) {
        jobWillBeAvailable(ctx, strZset, score, suffix);
    } else {
        jobWontBeAvailable(ctx, strZset);
    }
}

void createTimerFor(RedisModuleCtx *ctx, RedisModuleString *strZset, const char *suffix)
{
    int db = RedisModule_GetSelectedDb(ctx);
    BlockingPopDS *ds = getBlockingPopDS(db);
    double score;
    if (RedisModule_DictGet(ds->timers, strZset, NULL) == NULL && minScore(ctx, strZset, &score)) {
        jobWillBeAvailable(ctx, strZset, score, suffix);
    }
}

#define LARAVEL_MAX_KEY_TO_MIGRATE 100

/**
 * Migrate Expired Jobs
 *
 * @return number of migrated jobs.
 */
long long migrateExpiredJobs(RedisModuleCtx *ctx, RedisModuleKey *list, RedisModuleString *strList, double currentTime,
                             RedisModuleKey *zset, RedisModuleString *strZset, const char *suffix)
{
    // If the key is empty, iteration fails
    if (RedisModule_ZsetFirstInScoreRange(zset,  REDISMODULE_NEGATIVE_INFINITE, currentTime, 0, 0) == REDISMODULE_ERR) {
        return 0;
    }

    double score;
    long long n;
    // Migrate a constant number of jobs to maintain a logarithmic time complexity.
    for (n = 0; n < LARAVEL_MAX_KEY_TO_MIGRATE && !RedisModule_ZsetRangeEndReached(zset); ++n, RedisModule_ZsetRangeNext(zset)) {
        // Get the job
        RedisModuleString *cur = RedisModule_ZsetRangeCurrentElement(zset, &score);

        // Migrate it to the list
        RedisModule_ListPush(list, REDISMODULE_LIST_TAIL, cur);
        RedisModule_Replicate(ctx, "rpush", "ss", strList, cur);
        RedisModule_FreeString(ctx, cur);
    }

    // Remove migrated jobs from zset
    if (n) {
        RedisModuleCallReply *reply = RedisModule_Call(ctx, "zremrangebyrank", "sll", strZset, 0ll, n - 1);
        RedisModule_FreeCallReply(reply);
        RedisModule_Replicate(ctx, "zremrangebyrank", "sll", strZset, 0ll, n - 1);
    }
    RedisModule_ZsetRangeStop(zset);
    updateTimerFor(ctx, strZset, suffix);
    return n;
}

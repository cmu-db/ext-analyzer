
#include <string.h>
#include <stdbool.h>
#include <pthread.h>

#define REDISMODULE_EXPERIMENTAL_API
#include "redismodule.h"

const int TimerIDLen = 10;

/* structure with timer information */
typedef struct TimerData {
    char id[TimerIDLen];        /* id, used for further referencing */
    RedisModuleTimerID tid;     /* internal id for the timer API */
    RedisModuleString *sha1;    /* sha1 for the script to execute */
    mstime_t interval;          /* looping interval. 0 if it is only once */
} TimerData;


void TimerCallback(RedisModuleCtx *ctx, void *data);
void DeleteTimerData(RedisModuleCtx *ctx, TimerData *td);
bool ScriptExists(RedisModuleCtx *ctx, RedisModuleString *sha1);


/* internal structure for storing timers */
static RedisModuleDict *timers;


/* release all the memory used in timer structure */
void DeleteTimerData(RedisModuleCtx *ctx, TimerData *td) {
    RedisModule_FreeString(ctx, td->sha1);
    RedisModule_Free(td);
}

/* callback called by the Timer API. Data contains a TimerData structure */
void TimerCallback(RedisModuleCtx *ctx, void *data) {
    RedisModuleCallReply *rep;
    TimerData *td;

    td = (TimerData*)data;

    /* remove the timer to avoid raise conditions */
    RedisModule_DictDelC(timers, td->id, TimerIDLen, NULL);

    /* remove the timer if the script is not active */
    if (!ScriptExists(ctx, td->sha1)) {
        DeleteTimerData(ctx, td);
        return;
    }

    /* execute the script */
    rep = RedisModule_Call(ctx, "EVALSHA", "sl", td->sha1, 0);
    RedisModule_FreeCallReply(rep);

    /* if loop, create a new timer and reinsert
     * if not, delete the timer data
     */
    if (td->interval) {
        td->tid = RedisModule_CreateTimer(ctx, td->interval, TimerCallback, td);
        RedisModule_DictSetC(timers, td->id, TimerIDLen, td);
    } else {
        DeleteTimerData(ctx, td);
    }
}


/* Return true if a script exists. On error, return false */
bool ScriptExists(RedisModuleCtx *ctx, RedisModuleString *sha1) {
    RedisModuleCallReply *rep = NULL, *irep = NULL;
    bool found = false;

    rep = RedisModule_Call(ctx, "SCRIPT", "cs", "EXISTS", sha1);
    if (rep == NULL || RedisModule_CallReplyType(rep) == REDISMODULE_REPLY_ERROR) {
        goto exit;
    }

    irep = RedisModule_CallReplyArrayElement(rep, 0);
    if (irep == NULL || RedisModule_CallReplyType(irep) == REDISMODULE_REPLY_ERROR) {
        goto exit;
    }

    found = RedisModule_CallReplyInteger(irep) != 0;

exit:
    if (rep)
        RedisModule_FreeCallReply(rep);

    return found;
}

/* Fill a response with a particulat timer data */
int ReplyWithTimerData(RedisModuleCtx *ctx, TimerData *td) {
    uint64_t rem = 0;

    RedisModule_GetTimerInfo(ctx, td->tid, &rem, NULL);

    RedisModule_ReplyWithArray(ctx,4);
    RedisModule_ReplyWithStringBuffer(ctx, td->id, TimerIDLen);
    RedisModule_ReplyWithString(ctx,td->sha1);
    RedisModule_ReplyWithLongLong(ctx, (long long)rem);
    RedisModule_ReplyWithLongLong(ctx,td->interval);

    return REDISMODULE_OK;
}

/* Entrypoint for TIMER.NEW command.
 * This command creates a new timer.
 * Syntax: TIMER.NEW milliseconds sha1 [LOOP]
 * If LOOP is specified, after executing a new timer is created
 */
int TimerNewCommand(RedisModuleCtx *ctx, RedisModuleString **argv, int argc) {
    long long interval;
    bool loop = false;
    RedisModuleString *sha1;
    TimerData *td = NULL;
    const char *s;

    // check arguments
    if (argc < 3 || argc > 4) {
        return RedisModule_WrongArity(ctx);
    }

    if (RedisModule_StringToLongLong(argv[1], &interval) != REDISMODULE_OK) {
        return RedisModule_ReplyWithError(ctx, "ERR invalid interval");
    }

    sha1 = argv[2];
    if (!ScriptExists(ctx, sha1)) {
        return RedisModule_ReplyWithError(ctx, "ERR invalid script");
    }

    if (argc == 4) {
        s = RedisModule_StringPtrLen(argv[3], NULL);
        if (strcasecmp(s, "LOOP")) {
            return RedisModule_ReplyWithError(ctx, "ERR invalid argument");
        }
        loop = true;
    }

    /* allocate structure and generate a unique id */
    td = (TimerData*)RedisModule_Alloc(sizeof(*td));
    while (1) {
        RedisModule_GetRandomHexChars(td->id, TimerIDLen);
        if (!RedisModule_DictGetC(timers, td->id, TimerIDLen, NULL)) {
            break;
        }
    }
    td->sha1 = RedisModule_CreateStringFromString(ctx, sha1);
    td->interval = loop ? interval : 0;

    /* create the timer through the Timer API */
    td->tid = RedisModule_CreateTimer(ctx, interval, TimerCallback, td);
    if (RedisModule_GetTimerInfo(ctx, td->tid, NULL, NULL) != REDISMODULE_OK) {
        DeleteTimerData(ctx, td);
        return RedisModule_ReplyWithError(ctx, "ERR cannot create timer");
    }

    /* add the timer to the list of timers */
    RedisModule_DictSetC(timers, (void*)td->id, TimerIDLen, td);

    /* respond with the id */
    RedisModule_ReplyWithStringBuffer(ctx, td->id, TimerIDLen);
    return REDISMODULE_OK;
}

/* Entrypoint for TIMER.KILL command.
 * This command terminates an existing timer.
 * Syntax: TIMER.KILL id
 */
int TimerKillCommand(RedisModuleCtx *ctx, RedisModuleString **argv, int argc) {
    const char *id;
    TimerData *td;

    /* check arguments */
    if (argc != 2) {
        return RedisModule_WrongArity(ctx);
    }

    /* get timer data */
    id = RedisModule_StringPtrLen(argv[1], NULL);

    td = RedisModule_DictGetC(timers, (void*)id, TimerIDLen, NULL);
    if (!td) {
        return RedisModule_ReplyWithError(ctx, "ERR invalid id");
    }

    /* stop timer */
    if (RedisModule_StopTimer(ctx, td->tid, NULL) != REDISMODULE_OK) {
        return RedisModule_ReplyWithError(ctx, "ERR cannot stop timer");
    }

    /* remove from dict and free */
    RedisModule_DictDelC(timers, (void*)id, TimerIDLen, NULL);
    DeleteTimerData(ctx, td);

    /* return OK */
    RedisModule_ReplyWithSimpleString(ctx, "OK");
    return REDISMODULE_OK;
}

/* Entrypoint for TIMER.INFO command.
 * This command returns information for a particular timer.
 * Syntax: TIMER.INFO id
 * The response contains:
 *   - the timer ID
 *   - the sha1 of the script to be executed
 *   - time (in milliseconds) for the next trigger
 *   - interval if the timer is a loop, or 0 if it is only-once
 */
int TimerInfoCommand(RedisModuleCtx *ctx, RedisModuleString **argv, int argc) {
    TimerData *td;
    const char *id;

    /* check arguments */
    if (argc != 2) {
        return RedisModule_WrongArity(ctx);
    }

    id = RedisModule_StringPtrLen(argv[1], NULL);
    td = RedisModule_DictGetC(timers, (void*)id, TimerIDLen, NULL);
    if (!td) {
        return RedisModule_ReplyWithError(ctx, "ERR invalid id");
    }

    ReplyWithTimerData(ctx, td);
    return REDISMODULE_OK;
}

/* Entrypoint for TIMER.LIST command.
 * This command returns information for all existing timers.
 * Syntax: TIMER.LIST
 * Each line of the response contains:
 *   - the timer ID
 *   - the sha1 of the script to be executed
 *   - time (in milliseconds) for the next trigger
 *   - interval if the timer is a loop, or 0 if it is only-once
 */
int TimerListCommand(RedisModuleCtx *ctx, RedisModuleString **argv, int argc) {
    TimerData *td;
    RedisModuleDictIter *iter;
    long long cnt = 0;

    /* check arguments */
    if (argc != 1) {
        return RedisModule_WrongArity(ctx);
    }

    RedisModule_ReplyWithArray(ctx,REDISMODULE_POSTPONED_ARRAY_LEN);
    iter = RedisModule_DictIteratorStartC(timers, "^", NULL, 0);
    while ((RedisModule_DictNext(ctx, iter,(void**)&td)) != NULL) {
        ReplyWithTimerData(ctx, td);
        cnt++;
    }
    RedisModule_DictIteratorStop(iter);
    RedisModule_ReplySetArrayLength(ctx,cnt);
    return REDISMODULE_OK;
}


/* Module entrypoint */
int RedisModule_OnLoad(RedisModuleCtx *ctx) {

    /* Register the module itself */
    if (RedisModule_Init(ctx, "timer", 1, REDISMODULE_APIVER_1) == REDISMODULE_ERR) {
        return REDISMODULE_ERR;
    }

    /* register commands */
    if (RedisModule_CreateCommand(ctx, "timer.new", TimerNewCommand, "write deny-oom", 0, 0, 0) == REDISMODULE_ERR) {
        return REDISMODULE_ERR;
    }

    if (RedisModule_CreateCommand(ctx, "timer.kill", TimerKillCommand, "write", 0, 0, 0) == REDISMODULE_ERR) {
        return REDISMODULE_ERR;
    }

    if (RedisModule_CreateCommand(ctx, "timer.info", TimerInfoCommand, "readonly", 0, 0, 0) == REDISMODULE_ERR) {
        return REDISMODULE_ERR;
    }

    if (RedisModule_CreateCommand(ctx, "timer.list", TimerListCommand, "readonly", 0, 0, 0) == REDISMODULE_ERR) {
        return REDISMODULE_ERR;
    }

    /* initialize map */
    timers = RedisModule_CreateDict(NULL);

    return REDISMODULE_OK;
}

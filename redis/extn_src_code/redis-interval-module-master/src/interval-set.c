#include "interval-set.h"
#include "string.h"
#include "treap-tree.h"
#include "linked-list.h"
#include "output.h"

IntervalSet *createIntervalSet() {
    IntervalSet *intervalSet = RedisModule_Alloc(sizeof(IntervalSet));
    HashMap *hash = createHashMap();
    intervalSet->hash = hash;
    intervalSet->tree = NULL;
    return intervalSet;
}

int add(IntervalSet *intervalSet, char *member, Interval *interval) {
    Item *insertedNode = put(intervalSet->hash, member, interval);
    if (insertedNode != NULL) {
        insertNode(&intervalSet->tree, insertedNode);
    }
    return insertedNode != NULL;
}

int removeInterval(IntervalSet *intervalSet, char *member) {
    Item *existingItem = get(intervalSet->hash, member);
    if (existingItem != NULL) {
        deleteNode(&intervalSet->tree, existingItem);
        delete(intervalSet->hash, member);
        return 1;
    }
    return 0;
}

void searchValue(RedisModuleCtx *ctx, IntervalSet *intervalSet, double valueToSearch, long long count, OutputContext *outputContext) {
    if (outputContext->beforeOutput(outputContext) == REDISMODULE_ERR) {
        return;
    }
    int len = 0;
    findContains(intervalSet->tree, valueToSearch, ctx, &len, count, outputContext);
    outputContext->afterOutput(outputContext);
}

void searchInterval(RedisModuleCtx *ctx, IntervalSet *intervalSet, Interval *intervalToSearch, long long count, OutputContext *outputContext) {
    if (outputContext->beforeOutput(outputContext) == REDISMODULE_ERR) {
        return;
    }
    int len = 0;
    findOverlaps(intervalSet->tree, intervalToSearch, ctx, &len, count, outputContext);
    outputContext->afterOutput(outputContext);
}

void scanIntervalSet(RedisModuleCtx *ctx, IntervalSet *intervalSet, long long cursor, const char *match, long long count, OutputContext *outputContext) {
    char *matchPattern = strdup(match);
    LinkedList *list = scanHash(intervalSet->hash, &cursor, matchPattern, count);
    RedisModule_ReplyWithArray(ctx, 2);
    RedisModule_ReplyWithLongLong(ctx, cursor);
    if (outputContext->beforeOutput(outputContext) == REDISMODULE_ERR) {
        return;
    }
    LinkedListNode *listNode = list->head;
    while (listNode != NULL) {
        outputContext->output(outputContext, listNode->item);
        listNode = listNode->next;
    }
    outputContext->afterOutput(outputContext);
}
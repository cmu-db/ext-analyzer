#include "io.h"
#include "redismodule.h"
#include "interval-set.h"
#include "treap-tree.h"
#include "string.h"
#include "hashmap.h"

void *IntervalSetTypeRdbLoad(RedisModuleIO *rdb, int encver) {
    if (encver != 0) {
        RedisModule_LogIOError(rdb, "warning","Can't load data with version %d", encver);
        return NULL;
    }
    size_t len = RedisModule_LoadUnsigned(rdb);
    IntervalSet *intervalSet = createIntervalSet();
    while (len--) {
        char *member = RedisModule_LoadStringBuffer(rdb, NULL);
        int includeLowerBound = RedisModule_LoadUnsigned(rdb);
        double lowerBound = RedisModule_LoadDouble(rdb);
        double upperBound = RedisModule_LoadDouble(rdb);
        int includeUpperBound = RedisModule_LoadUnsigned(rdb);
        Interval *interval = createInterval(includeLowerBound, lowerBound, upperBound, includeUpperBound);
        add(intervalSet, member, interval);
    }
    return intervalSet;
}

void saveNode(RedisModuleIO *rdb, TreeNode *node) {
    if (node != NULL) {
        saveNode(rdb, node->left);
        saveNode(rdb, node->right);
        RedisModule_SaveStringBuffer(rdb, node->item->member, strlen(node->item->member));
        RedisModule_SaveUnsigned(rdb, node->item->interval->includeLowerBound);
        RedisModule_SaveDouble(rdb, node->item->interval->lowerBound);
        RedisModule_SaveDouble(rdb, node->item->interval->upperBound);
        RedisModule_SaveUnsigned(rdb, node->item->interval->includeUpperBound);
    }
}

void IntervalSetTypeRdbSave(RedisModuleIO *rdb, void *value) {
    IntervalSet *intervalSet = value;
    RedisModule_SaveUnsigned(rdb, intervalSet->hash->len);
    saveNode(rdb, intervalSet->tree);
}

void rewriteNode(RedisModuleIO *aof, RedisModuleString *key, TreeNode *node) {
    if (node != NULL) {
        rewriteNode(aof, key, node->left);
        rewriteNode(aof, key, node->right);
        char intervalStr[203];
        char includeLowerChar = '[';
        char includeUpperChar = ']';
        if (node->item->interval->includeLowerBound == 0) {
            includeLowerChar = ']';
        }
        if (node->item->interval->includeUpperBound == 0) {
            includeUpperChar = ']';
        }
        snprintf(intervalStr, 203, "%c%f,%f%c",
                 includeLowerChar,
                 node->item->interval->lowerBound,
                 node->item->interval->upperBound,
                 includeUpperChar);
        RedisModule_EmitAOF(aof, "IADD", "scc", key, intervalStr, node->item);
    }
}

void IntervalSetTypeAofRewrite(RedisModuleIO *aof, RedisModuleString *key, void *value) {
    IntervalSet *intervalSet = value;
    rewriteNode(aof, key, intervalSet->tree);
}

void freeIntervalSet(IntervalSet *intervalSet) {
    freeHashMap(intervalSet->hash);
    RedisModule_Free(intervalSet->hash);
    freeTreeNode(intervalSet->tree);
    RedisModule_Free(intervalSet);
}

void IntervalSetTypeFree(void *value) {
    freeIntervalSet(value);
}

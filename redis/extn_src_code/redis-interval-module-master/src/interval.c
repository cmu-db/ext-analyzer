#include "redismodule.h"
#include "interval.h"
#include "string.h"
#include <float.h>
#include "util.h"
#include "math.h"

#define doubleBufferSize 512

Interval *createInterval(int includeLowerBound, double lowerBound, double upperBound, int includeUpperBound) {
    Interval *interval = RedisModule_Alloc(sizeof (Interval));
    interval->includeLowerBound = includeLowerBound;
    interval->includeUpperBound = includeUpperBound;
    interval->lowerBound = lowerBound;
    interval->upperBound = upperBound;
    return interval;
}

void freeInterval(Interval *interval) {
    RedisModule_Free(interval);
}

Interval *parseInterval(RedisModuleString *intervalString) {
    size_t len;
    const char *str = RedisModule_StringPtrLen(intervalString, &len);
    char bounds[2][doubleBufferSize] = {
            { 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0 },
            { 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0 }
    };
    int i;
    int partIndex = 0;
    int partIterator = 0;
    int missingBound[2] = { 1, 1 };

    int includeLowerBound = 1;
    int includeUpperBound = 1;
    double lowerBound;
    double upperBound;

    for (i = 0; i < len; i++){
        if (i == 0 && str[i] == ']') {
            includeLowerBound = 0;
        } else if (i == len - 1 && str[i] == '[') {
            includeUpperBound = 0;
        } else if (str[i] == ',') {
            partIndex = 1;
            partIterator = 0;
        } else if (str[i] != '[' && str[i] != ']' && partIterator < doubleBufferSize) {
            bounds[partIndex][partIterator] = str[i];
            missingBound[partIndex] = 0;
            partIterator++;
        }
    }
    if (missingBound[0] || missingBound[1]) {
        return NULL;
    }
    if (!stringToDouble(bounds[0], &lowerBound)) {
        return NULL;
    }
    if (!stringToDouble(bounds[1], &upperBound)) {
        return NULL;
    }
    if (lowerBound > upperBound) {
        return NULL;
    }
    if (lowerBound == upperBound && (includeUpperBound == 0 || includeLowerBound == 0)) {
        return NULL;
    }
    return createInterval(includeLowerBound, lowerBound, upperBound, includeUpperBound);
}

int containsValue(Interval *interval, double value, int includeValue) {
    int lowerBoundMatch = 0;
    int upperBoundMatch = 0;
    if (includeValue) {
        if ((interval->includeLowerBound == 1 && interval->lowerBound <= value) ||
            (interval->includeLowerBound == 0 && interval->lowerBound < value)) {
            lowerBoundMatch = 1;
        }
        if ((interval->includeUpperBound == 1 && interval->upperBound >= value) ||
            (interval->includeUpperBound == 0 && interval->upperBound > value)) {
            upperBoundMatch = 1;
        }
    } else {
        if (interval->lowerBound < value) {
            lowerBoundMatch = 1;
        }
        if (interval->upperBound > value) {
            upperBoundMatch = 1;
        }
    }
    if (lowerBoundMatch == 1 && upperBoundMatch == 1) {
        return 1;
    } else {
        return 0;
    }
}

int overlaps(Interval *interval1, Interval *interval2) {
    return containsValue(interval1, interval2->lowerBound, interval2->includeLowerBound) ||
            containsValue(interval1, interval2->upperBound, interval2->includeUpperBound) ||
            containsValue(interval2, interval1->upperBound, interval1->includeUpperBound) ||
            containsValue(interval2, interval1->upperBound, interval1->includeUpperBound);
}

#ifndef INTERVAL_H
#define INTERVAL_H

#include "redismodule.h"

typedef struct Interval {
    double lowerBound;
    double upperBound;
    int includeLowerBound:2;
    int includeUpperBound:2;
} Interval;

Interval *createInterval(int includeLowerBound, double lowerBound, double upperBound, int includeUpperBound);
void freeInterval(Interval *interval);
Interval *parseInterval(RedisModuleString *intervalString);
int containsValue(Interval *interval, double value, int includeValue);
int overlaps(Interval *interval1, Interval *interval2);

#endif
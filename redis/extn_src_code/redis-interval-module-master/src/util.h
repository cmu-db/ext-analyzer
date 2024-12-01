#include "redismodule.h"

#ifndef UTIL_H
#define UTIL_H

int stringMatchLen(const char *pattern, int patternLen,
                   const char *string, int stringLen, int nocase);
int stringToDouble(const char *str, double *value);
int redisModuleStringToDouble(RedisModuleString *str, double *value);

int getRedisModuleStringParameter(RedisModuleString **argv, int argc, const char *parameterName, RedisModuleString **parameterValue);
int getLongLongParameter(RedisModuleString **argv, int argc, const char *parameterName, long long *parameterValue);
#endif

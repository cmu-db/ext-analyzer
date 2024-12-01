// copied from redis sources util.c

#include <ctype.h>
#include "util.h"
#include "string.h"
#include <float.h>
#include <stdlib.h>
#include "math.h"

int stringMatchLen(const char *pattern, int patternLen,
                   const char *string, int stringLen, int nocase)
{
    while(patternLen && stringLen) {
        switch(pattern[0]) {
            case '*':
                while (patternLen && pattern[1] == '*') {
                    pattern++;
                    patternLen--;
                }
                if (patternLen == 1)
                    return 1; /* match */
                while(stringLen) {
                    if (stringMatchLen(pattern+1, patternLen-1,
                                       string, stringLen, nocase))
                        return 1; /* match */
                    string++;
                    stringLen--;
                }
                return 0; /* no match */
                break;
            case '?':
                string++;
                stringLen--;
                break;
            case '[':
            {
                int not, match;

                pattern++;
                patternLen--;
                not = pattern[0] == '^';
                if (not) {
                    pattern++;
                    patternLen--;
                }
                match = 0;
                while(1) {
                    if (pattern[0] == '\\' && patternLen >= 2) {
                        pattern++;
                        patternLen--;
                        if (pattern[0] == string[0])
                            match = 1;
                    } else if (pattern[0] == ']') {
                        break;
                    } else if (patternLen == 0) {
                        pattern--;
                        patternLen++;
                        break;
                    } else if (patternLen >= 3 && pattern[1] == '-') {
                        int start = pattern[0];
                        int end = pattern[2];
                        int c = string[0];
                        if (start > end) {
                            int t = start;
                            start = end;
                            end = t;
                        }
                        if (nocase) {
                            start = tolower(start);
                            end = tolower(end);
                            c = tolower(c);
                        }
                        pattern += 2;
                        patternLen -= 2;
                        if (c >= start && c <= end)
                            match = 1;
                    } else {
                        if (!nocase) {
                            if (pattern[0] == string[0])
                                match = 1;
                        } else {
                            if (tolower((int)pattern[0]) == tolower((int)string[0]))
                                match = 1;
                        }
                    }
                    pattern++;
                    patternLen--;
                }
                if (not)
                    match = !match;
                if (!match)
                    return 0; /* no match */
                string++;
                stringLen--;
                break;
            }
            case '\\':
                if (patternLen >= 2) {
                    pattern++;
                    patternLen--;
                }
                /* fall through */
            default:
                if (!nocase) {
                    if (pattern[0] != string[0])
                        return 0; /* no match */
                } else {
                    if (tolower((int)pattern[0]) != tolower((int)string[0]))
                        return 0; /* no match */
                }
                string++;
                stringLen--;
                break;
        }
        pattern++;
        patternLen--;
        if (stringLen == 0) {
            while(*pattern == '*') {
                pattern++;
                patternLen--;
            }
            break;
        }
    }
    if (patternLen == 0 && stringLen == 0)
        return 1;
    return 0;
}

int stringToDouble(const char *str, double *value) {
    if (strcmp(str, "-inf") == 0) {
        *value = -DBL_MAX;
        return 1;
    } else if (strcmp(str, "inf") == 0 || strcmp(str, "+inf") == 0) {
        *value = DBL_MAX;
        return 1;
    } else {
        char *endPtr;
        *value = strtod(str, &endPtr);
        if (isinf(*value) && *value > 0) {
            *value = DBL_MAX;
        } else if (isinf(*value) && *value < 0) {
            *value = -DBL_MAX;
        }
        return strlen(endPtr) == 0;
    }
}
int redisModuleStringToDouble(RedisModuleString *str, double *value) {
    char *strValue = RedisModule_StringPtrLen(str, NULL);
    return stringToDouble(strValue, value);
}

int getLongLongParameter(RedisModuleString **argv, int argc, const char *parameterName, long long *parameterValue) {
    RedisModuleString *value = NULL;
    if (getRedisModuleStringParameter(argv, argc, parameterName, &value) == REDISMODULE_ERR) {
        return REDISMODULE_ERR;
    }
    if (value != NULL) {
        if (RedisModule_StringToLongLong(argv[4], parameterValue) == REDISMODULE_ERR) {
            return REDISMODULE_ERR;
        }
    }
    return REDISMODULE_OK;
}

int getRedisModuleStringParameter(RedisModuleString **argv, int argc, const char *parameterName, RedisModuleString **parameterValue) {
    int i = 0;
    int found = 0;
    while (i < argc) {
        const char *parameter = RedisModule_StringPtrLen(argv[i], NULL);
        if (strcasecmp(parameterName, parameter) == 0) {
            i++;
            found = 1;
            break;
        }
        i++;
    }
    if (found) {
        if (i >= argc || argv[i] == NULL) {
            return REDISMODULE_ERR;
        }
        *parameterValue = argv[i];
    }
    return REDISMODULE_OK;
}
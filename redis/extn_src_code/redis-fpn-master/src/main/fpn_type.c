
/*
 * Copyright 2018 Infobip Ltd.
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *     http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */


#include "redismodule.h"
#include "fpn_type.h"
#include "utils.h"
#include <ctype.h>
#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdint.h>


#define ZEROS_19 10000000000000000000ULL
#define NINES    9999999999999999999ULL
#define UNUSED(V) ((void) V)


static int128_t MAX_VALUE = ((int128_t) ZEROS_19 * NINES) + NINES;
static int128_t MIN_VALUE = -(((int128_t) ZEROS_19 * NINES) + NINES);
static int8_t MAX_SCALE = 38;


ValueResult parse_string_to_decimal (const char *source_string, const size_t source_string_length) {
  ValueResult value_result;

  size_t source_string_index = 0;

  char *value_string = (char*) RedisModule_Alloc(source_string_length);
  size_t value_string_length = 0;

  bool started_scale_count = false;
  uint8_t scale_count = 0;

  while (source_string_index < source_string_length) {
    char character = source_string[source_string_index];
    if (character == '\0' || character == '\n') {
      break;
    } else if (isdigit(character) || (source_string_index == 0 && (character == '-' || character == '+'))) {
      value_string[value_string_length] = character;
      value_string_length++;
      if (started_scale_count) {
        scale_count++;
      }
    } else if (!started_scale_count && character == DEFAULT_DECIMAL_SEPARATOR) {
      started_scale_count = true;
    } else {
      value_result.ok = false;
      value_result.error = "Unknown character in the parsing value";
      return value_result;
    }
    source_string_index++;
  }
  value_string[value_string_length] = '\0';

  Decimal *result = new_decimal();
  ErrorOrNothing set_result = set_decimal_fields(result, parse_to_int128(value_string), scale_count);
  if (!set_result.ok) {
    value_result.ok = false;
    value_result.error = set_result.error;
  } else {
    value_result.ok = true;
    value_result.value = result;
  }
  return value_result;
}

ValueResult parse_redis_string_to_decimal (RedisModuleString *string) {
  const char *source_string = RedisModule_StringPtrLen(string, NULL);
  const size_t source_string_length = strlen(source_string);
  return parse_string_to_decimal(source_string, source_string_length);
}

Decimal *new_decimal () {
  Decimal *result;
  result = RedisModule_Alloc(sizeof(*result));
  result->value = 0;
  result->scale = DEFAULT_DECIMAL_SCALE;
  return result;
}

ErrorOrNothing set_decimal_fields (Decimal *decimal, int128_t value, uint8_t scale) {
  ErrorOrNothing result;
  if (value < MIN_VALUE || value > MAX_VALUE) {
    result.ok = false;
    result.error = "value must be greater or equal than -99_999_999_999_999_999_999_999_999_999_999_999_999 and lower or equal than 99_999_999_999_999_999_999_999_999_999_999_999_999";
    return result;
  }
  if (scale > MAX_SCALE) {
    result.ok = false;
    result.error = "scale must lower than 38";
    return result;
  }
  decimal->value = value;
  decimal->scale = scale;
  result.ok = true;
  return result;
}

ErrorOrNothing set_decimal (Decimal *to, Decimal *from) {
  ErrorOrNothing result;
  if (to == NULL || from == NULL) {
    result.ok = false;
    result.error = "couldn't copy to/from NULL";
  } else {
    result = set_decimal_fields(to, from->value, from->scale);
  }
  return result;
}

int128_t scale_shift (int128_t value, uint8_t old_scale, uint8_t new_scale) {
  if (old_scale == new_scale) {
    return value;
  }
  uint128_t scale_factor = 1;
  int limit = abs(old_scale - new_scale);
  for (int count = 0; count < limit; count++) {
    scale_factor *= 10;
  }

  if (old_scale > new_scale) {
    int128_t result = value / scale_factor;
    if ((value % scale_factor) > (scale_factor / 2)) {
      result++;
    }
    return result;
  } else {
    return value * scale_factor;
  }
}

ValueResult scale_to (Decimal *decimal, uint8_t scale) {
  ValueResult value_result;
  Decimal *result = new_decimal();
  ErrorOrNothing set_result = set_decimal_fields(result,
                                                 scale_shift(decimal->value, decimal->scale, scale),
                                                 scale);
  if (!set_result.ok) {
    value_result.ok = false;
    value_result.error = set_result.error;
  } else {
    value_result.ok = true;
    value_result.value = result;
  }
  return value_result;
}

char *to_string (Decimal *deciaml) {
  const int128_t value = deciaml->value;
  const uint8_t scale = deciaml->scale;

  bool has_scale = scale > 0;
  if (value == 0 && !has_scale) {
    return "0";
  }

  const bool has_sign = value < 0;
  const char *value_as_string = int128_to_string(value);
  const size_t value_length = strlen(value_as_string);

  size_t result_length = 1; // string terminating sign - \0
  if ((value_length - has_sign) > scale) {
    result_length += value_length + has_scale; // .
  } else {
    result_length += scale + has_sign + 2; // '0.'
  }
  char *result = (char*) RedisModule_Alloc(result_length);

  int decimal_separator_index = has_scale
                                ? result_length - 2 - scale
                                : -1;

  int value_reverse_index = value_length - 1;
  for (int index = result_length - 2; index >= 0; index--) {
    if (index == decimal_separator_index) {
      result[index] = DEFAULT_DECIMAL_SEPARATOR;
    } else if (value_reverse_index >= has_sign) {
      result[index] = value_as_string[value_reverse_index--];
    } else {
      result[index] = '0';
    }
  }

  if (has_sign) {
    result[0] = value_as_string[0];
  }
  result[result_length - 1] = '\0';
  return result;
}

void fpnt_save (RedisModuleIO *rdb, void *value) {
  Decimal *decimal = (Decimal*) value;
  char *string = to_string(decimal);
  size_t length = strlen(string);
  RedisModule_SaveStringBuffer(rdb, string, length);
  RedisModule_Free(string);
}

void *fpnt_load (RedisModuleIO *rdb, int encver) {
  if (encver != FIXED_POINT_NUMBER_TYPE_VERSION) {
    RedisModule_LogIOError(rdb, "warning", "Sorry the Fixed Point Number module only supports RDB files written with the encoding version %d. This file has encoding version %d, and was likely written by a previous version of this module that is now deprecated. Once the module will be stable we'll start supporting older versions of the encodings, in case we switch to newer encodings.", FIXED_POINT_NUMBER_TYPE_VERSION, encver);
    return NULL;
  }
  size_t length = 0;
  char *string = RedisModule_LoadStringBuffer(rdb, &length);
  ValueResult result = parse_string_to_decimal(string, length);
  RedisModule_Free(string);
  if (!result.ok) {
    RedisModule_LogIOError(rdb, "error", "Error during Fixed Point Number load: %s", result.error);
    return NULL;
  }
  Decimal *decimal = (Decimal*) result.value;
  return decimal;
}

void fpnt_aof_rewrite (RedisModuleIO *aof, RedisModuleString *key, void *value) {
  UNUSED(aof);
  UNUSED(key);
  UNUSED(value);
#if 0
  Decimal *decimal = (Decimal*) value;
  const char *string = to_string(decimal);
  RedisModule_EmitAOF(aof, "fpn.set", "sl", key, string);
  RedisModule_Free(string);
#endif
}

size_t fpnt_memory_usage (const void *value) {
  const Decimal *decimal = (Decimal*) value;
  return sizeof(*decimal);
}

void fpnt_free (void *value) {
  Decimal *decimal = (Decimal*) value;
  RedisModule_Free(decimal);
}

void fpnt_digest (RedisModuleDigest *md, void *value) {
  Decimal *decimal = (Decimal*) value;
  char *string = to_string(decimal);
  size_t length = strlen(string);

  RedisModule_DigestAddStringBuffer(md, string, length);
  RedisModule_DigestEndSequence(md);
  RedisModule_Free(string);
}

int register_type (RedisModuleCtx *context) {
  RedisModuleTypeMethods typeMethods = {
    .version = REDISMODULE_TYPE_METHOD_VERSION,
    .rdb_save = fpnt_save,
    .rdb_load = fpnt_load,
    .aof_rewrite = fpnt_aof_rewrite,
    .mem_usage = fpnt_memory_usage,
    .free = fpnt_free,
    .digest = fpnt_digest
  };
  FPNT = RedisModule_CreateDataType(context, TYPE_NAME, FIXED_POINT_NUMBER_TYPE_VERSION, &typeMethods);
  return FPNT == NULL
         ? REDISMODULE_ERR
         : REDISMODULE_OK;
}

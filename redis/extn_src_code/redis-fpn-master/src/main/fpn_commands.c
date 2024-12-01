
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
#include "fpn_commands.h"
#include <string.h>
#include <stdbool.h>



ValueResult get_value (RedisModuleCtx *context, RedisModuleString *keyName) {
  RedisModuleKey *key = RedisModule_OpenKey(context, keyName, REDISMODULE_READ|REDISMODULE_WRITE);
  int type = RedisModule_KeyType(key);
  ValueResult result = {false, NULL, NULL};
  if (type != REDISMODULE_KEYTYPE_EMPTY && RedisModule_ModuleTypeGetType(key) != FPNT) {
    result.ok = false;
    result.error = REDISMODULE_ERRORMSG_WRONGTYPE;
  } else if (type != REDISMODULE_KEYTYPE_EMPTY) {
    result.ok = true;
    result.value = (Decimal*) RedisModule_ModuleTypeGetValue(key);
  } else {
    // no value by key
    result.ok = true;
  }
  return result;
}

ValueResult get_or_create_value (RedisModuleCtx *context, RedisModuleString *keyName) {
  ValueResult value_result = get_value(context, keyName);
  if (!value_result.ok || value_result.value != NULL) {
    return value_result;
  }

  RedisModuleKey *key = RedisModule_OpenKey(context, keyName, REDISMODULE_READ|REDISMODULE_WRITE);
  Decimal *value = new_decimal();
  RedisModule_ModuleTypeSetValue(key, FPNT, value);
  value_result.value = value;
  return value_result;
}

ValueResult scale_it (Decimal *decimal, RedisModuleString *scaleString) {
  int64_t scale;
  if (RedisModule_StringToLongLong(scaleString, &scale) != REDISMODULE_OK) {
    ValueResult result = {false, NULL, NULL};
    result.ok = false;
    result.error = "ERR invalid value: must be a signed 128 bit integer";
    return result;
  }
  return scale_to(decimal, scale);
}

void reply_with_value (RedisModuleCtx *context, Decimal *decimal) {
  const char *string = to_string(decimal);
  RedisModuleString *response = RedisModule_CreateString(context, string, strlen(string));
  RedisModule_ReplyWithString(context, response);
}


/* COMMANDS */

/**
 * FPN.SET key value [scale]
 */
int fpnt_set (RedisModuleCtx *context, RedisModuleString **argv, int argc) {
  RedisModule_AutoMemory(context);
  if (argc < 3) {
    return RedisModule_WrongArity(context);
  }

  ValueResult value_result = get_or_create_value(context, argv[1]);
  if (!value_result.ok) {
    return RedisModule_ReplyWithError(context, value_result.error);
  }
  Decimal *value = value_result.value;

  reply_with_value(context, value);

  ValueResult parse_result = parse_redis_string_to_decimal(argv[2]);
  if (!parse_result.ok) {
    return RedisModule_ReplyWithError(context, parse_result.error);
  }
  Decimal *another_value = parse_result.value;

  ErrorOrNothing set_result = set_decimal(value, another_value);
  if (!set_result.ok) {
    return RedisModule_ReplyWithError(context, set_result.error);
  }

  if (argc > 3) {
    ValueResult scale_result = scale_it(value, argv[3]);
    if (!scale_result.ok) {
      return RedisModule_ReplyWithError(context, scale_result.error);
    }
    ErrorOrNothing set_result = set_decimal(value, scale_result.value);
    if (!set_result.ok) {
      return RedisModule_ReplyWithError(context, set_result.error);
    }
  }

  RedisModule_ReplicateVerbatim(context);
  return REDISMODULE_OK;
}

/**
 * FPN.GET key [scale]
 */
int fpnt_get (RedisModuleCtx *context, RedisModuleString **argv, int argc) {
  RedisModule_AutoMemory(context);
  if (argc < 2) {
    return RedisModule_WrongArity(context);
  }

  ValueResult value_result = get_value(context, argv[1]);
  if (!value_result.ok) {
    return RedisModule_ReplyWithError(context, value_result.error);
  } else if (value_result.value == NULL) {
    return RedisModule_ReplyWithNull(context);
  }
  Decimal *value = value_result.value;

  if (argc > 2) {
    ValueResult scale_result = scale_it(value, argv[2]);
    if (!scale_result.ok) {
      return RedisModule_ReplyWithError(context, scale_result.error);
    }
    reply_with_value(context, scale_result.value);
  } else {
    reply_with_value(context, value);
  }

  RedisModule_ReplicateVerbatim(context);
  return REDISMODULE_OK;
}

/**
 * FPN.ADD key value [scale]
 */
int fpnt_add (RedisModuleCtx *context, RedisModuleString **argv, int argc) {
  if (argc < 3) {
    return RedisModule_WrongArity(context);
  }

  ValueResult value_result = get_or_create_value(context, argv[1]);
  if (!value_result.ok) {
    return RedisModule_ReplyWithError(context, value_result.error);
  }
  Decimal *decimal_1 = value_result.value;
  ValueResult parse_result = parse_redis_string_to_decimal(argv[2]);
  if (!parse_result.ok) {
    return RedisModule_ReplyWithError(context, parse_result.error);
  }
  Decimal *decimal_2 = parse_result.value;

  uint8_t max_scale = decimal_1->scale > decimal_2->scale
                      ? decimal_1->scale
                      : decimal_2->scale;

  int128_t value1 = scale_shift(decimal_1->value, decimal_1->scale, max_scale);
  int128_t value2 = scale_shift(decimal_2->value, decimal_2->scale, max_scale);
  ErrorOrNothing set_result = set_decimal_fields(decimal_1, value1 + value2, max_scale);
  if (!set_result.ok) {
    return RedisModule_ReplyWithError(context, set_result.error);
  }

  if (argc > 3) {
    ValueResult scale_result = scale_it(decimal_1, argv[3]);
    if (!scale_result.ok) {
      return RedisModule_ReplyWithError(context, scale_result.error);
    }
    ErrorOrNothing set_result = set_decimal(decimal_1, scale_result.value);
    if (!set_result.ok) {
      return RedisModule_ReplyWithError(context, set_result.error);
    }
  }

  reply_with_value(context, decimal_1);

  RedisModule_ReplicateVerbatim(context);
  return REDISMODULE_OK;
}

/**
 * FPN.SUBTRACT key value [scale]
 */
int fpnt_subtract (RedisModuleCtx *context, RedisModuleString **argv, int argc) {
  if (argc < 3) {
    return RedisModule_WrongArity(context);
  }

  ValueResult value_result = get_or_create_value(context, argv[1]);
  if (!value_result.ok) {
    return RedisModule_ReplyWithError(context, value_result.error);
  }
  Decimal *decimal_1 = value_result.value;
  ValueResult parse_result = parse_redis_string_to_decimal(argv[2]);
  if (!parse_result.ok) {
    return RedisModule_ReplyWithError(context, parse_result.error);
  }
  Decimal *decimal_2 = parse_result.value;

  uint8_t max_scale = decimal_1->scale > decimal_2->scale
                      ? decimal_1->scale
                      : decimal_2->scale;

  int128_t value1 = scale_shift(decimal_1->value, decimal_1->scale, max_scale);
  int128_t value2 = scale_shift(decimal_2->value, decimal_2->scale, max_scale);
  ErrorOrNothing set_result = set_decimal_fields(decimal_1, value1 - value2, max_scale);
  if (!set_result.ok) {
    return RedisModule_ReplyWithError(context, set_result.error);
  }

  if (argc > 3) {
    ValueResult scale_result = scale_it(decimal_1, argv[3]);
    if (!scale_result.ok) {
      return RedisModule_ReplyWithError(context, scale_result.error);
    }
    ErrorOrNothing set_result = set_decimal(decimal_1, scale_result.value);
    if (!set_result.ok) {
      return RedisModule_ReplyWithError(context, set_result.error);
    }
  }

  reply_with_value(context, decimal_1);

  RedisModule_ReplicateVerbatim(context);
  return REDISMODULE_OK;
}

/**
 * FPN.DIVIDE key value [scale]
 */
int fpnt_divide (RedisModuleCtx *context, RedisModuleString **argv, int argc) {
  REDISMODULE_NOT_USED(context);
  REDISMODULE_NOT_USED(argv);
  REDISMODULE_NOT_USED(argc);
  return REDISMODULE_ERR;
}

/**
 * FPN.MULTIPLY key value [scale]
 */
int fpnt_multiply (RedisModuleCtx *context, RedisModuleString **argv, int argc) {
  if (argc < 3) {
    return RedisModule_WrongArity(context);
  }

  ValueResult value_result = get_or_create_value(context, argv[1]);
  if (!value_result.ok) {
    return RedisModule_ReplyWithError(context, value_result.error);
  }
  Decimal *decimal_1 = value_result.value;
  ValueResult parse_result = parse_redis_string_to_decimal(argv[2]);
  if (!parse_result.ok) {
    return RedisModule_ReplyWithError(context, parse_result.error);
  }
  Decimal *decimal_2 = parse_result.value;

  ErrorOrNothing set_result = set_decimal_fields(decimal_1,
                                                 decimal_1->value * decimal_2->value,
                                                 decimal_1->scale + decimal_2->scale);
  if (!set_result.ok) {
    return RedisModule_ReplyWithError(context, set_result.error);
  }

  if (argc > 3) {
    ValueResult scale_result = scale_it(decimal_1, argv[3]);
    if (!scale_result.ok) {
      return RedisModule_ReplyWithError(context, scale_result.error);
    }
    ErrorOrNothing set_result = set_decimal(decimal_1, scale_result.value);
    if (!set_result.ok) {
      return RedisModule_ReplyWithError(context, set_result.error);
    }
  }

  reply_with_value(context, decimal_1);

  RedisModule_ReplicateVerbatim(context);
  return REDISMODULE_OK;
}


int register_all_commands (RedisModuleCtx *context) {
  if (RedisModule_CreateCommand(context, FPNT_SET_COMMAND_NAME, fpnt_set, "write deny-oom", 1, 1, 1) == REDISMODULE_ERR) {
    return REDISMODULE_ERR;
  }
  if (RedisModule_CreateCommand(context, FPNT_GET_COMMAND_NAME, fpnt_get, "readonly deny-oom", 1, 1, 1) == REDISMODULE_ERR) {
    return REDISMODULE_ERR;
  }
  if (RedisModule_CreateCommand(context, FPNT_ADD_COMMAND_NAME, fpnt_add, "write deny-oom", 1, 1, 1) == REDISMODULE_ERR) {
    return REDISMODULE_ERR;
  }
  if (RedisModule_CreateCommand(context, FPNT_SUBTRACT_COMMAND_NAME, fpnt_subtract, "write deny-oom", 1, 1, 1) == REDISMODULE_ERR) {
    return REDISMODULE_ERR;
  }
  if (RedisModule_CreateCommand(context, FPNT_DIVIDE_COMMAND_NAME, fpnt_divide, "write deny-oom", 1, 1, 1) == REDISMODULE_ERR) {
    return REDISMODULE_ERR;
  }
  if (RedisModule_CreateCommand(context, FPNT_MULTIPLY_COMMAND_NAME, fpnt_multiply, "write deny-oom", 1, 1, 1) == REDISMODULE_ERR) {
    return REDISMODULE_ERR;
  }
  return REDISMODULE_OK;
}

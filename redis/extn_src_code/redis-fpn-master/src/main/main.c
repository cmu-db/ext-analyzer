
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
#include "utils.h"
#include <stdbool.h>
#include <string.h>


#define MOUDLE_NAME "fpn_module"

#ifndef MODULE_VERSION
#define MODULE_VERSION "development"
#endif


/**
 * This function must be present on each Redis module.
 * It is used in order to register the commands and types into the Redis server.
 */
int RedisModule_OnLoad (RedisModuleCtx *context, RedisModuleString **argv, int argc) {
  if (RedisModule_Init(context, MOUDLE_NAME, 1, REDISMODULE_APIVER_1) == REDISMODULE_ERR) {
    return REDISMODULE_ERR;
  }

  bool has_default_default_decimal_separator = false;
  bool has_default_decimal_scale = false;

  DEFAULT_DECIMAL_SCALE = 2;
  DEFAULT_DECIMAL_SEPARATOR = '.';

  for (int index = 0; index < argc; index++) {
    const char *argument = RedisModule_StringPtrLen(argv[index], NULL);
    if (has_default_default_decimal_separator) {
      if (strlen(argument) != 1) {
        return REDISMODULE_ERR;
      }
      DEFAULT_DECIMAL_SEPARATOR = argument[0];
      has_default_default_decimal_separator = false;
    } else if (has_default_decimal_scale) {
      DEFAULT_DECIMAL_SCALE = parse_to_uint8_t(argument);
      has_default_decimal_scale = false;
    } else if (strcmp(argument, "--default_decimal_separator") == 0) {
      has_default_default_decimal_separator = true;
    } else if (strcmp(argument, "--default_decimal_scale") == 0) {
      has_default_decimal_scale = true;
    } else {
      printf("module %s ERROR: unknown argument - '%s'\n", MOUDLE_NAME, argument);
      return REDISMODULE_ERR;
    }
  }
  printf("module %s INFO: module version is '%s'\n", MOUDLE_NAME, MODULE_VERSION);
  printf("module %s INFO: default scale is %d\n", MOUDLE_NAME, DEFAULT_DECIMAL_SCALE);
  printf("module %s INFO: default decimal separator is '%c'\n", MOUDLE_NAME, DEFAULT_DECIMAL_SEPARATOR);

  if (register_all_commands(context) == REDISMODULE_ERR) {
    return REDISMODULE_ERR;
  }
  if (register_type(context) == REDISMODULE_ERR) {
    return REDISMODULE_ERR;
  }
  return REDISMODULE_OK;
}

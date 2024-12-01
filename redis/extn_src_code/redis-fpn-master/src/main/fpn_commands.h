
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


#ifndef FPN_COMMANDS_H
#define FPN_COMMANDS_H

#define FPNT_SET_COMMAND_NAME "FPN.SET"
#define FPNT_GET_COMMAND_NAME "FPN.GET"
#define FPNT_ADD_COMMAND_NAME "FPN.ADD"
#define FPNT_SUBTRACT_COMMAND_NAME "FPN.SUBTRACT"
#define FPNT_MULTIPLY_COMMAND_NAME "FPN.MULTIPLY"
#define FPNT_DIVIDE_COMMAND_NAME "FPN.DIVIDE"


#include "redismodule.h"


/**
 * Registers all commands related to work with fixed point numbers in a module context.
 */
int register_all_commands (RedisModuleCtx *context);


#endif /* FPN_COMMANDS_H */

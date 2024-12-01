
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


#ifndef FPNT_TYPE_H
#define FPNT_TYPE_H

#define FIXED_POINT_NUMBER_TYPE_VERSION 101
#define TYPE_NAME "fpn-xxlab"


#include "redismodule.h"
#include "utils.h"
#include <stdint.h>


/**
 * Pointer to custom redis module's type.
 */
RedisModuleType *FPNT;

/**
 * Default decimal scale (used in new_decimal).
 * Can be overriden on module load.
 */
uint8_t DEFAULT_DECIMAL_SCALE;

/**
 * Default decimal separator (used in parse_string_to_decimal and to_string).
 * Can be overriden on module load.
 */
char DEFAULT_DECIMAL_SEPARATOR;


/**
 * The structure represents decimal number.
 */
typedef struct {
  int128_t value; // 128bit integer value
  uint8_t scale;  // number of digits in mantissa
} Decimal;


/**
 * Parses redis string to new Decimal structure.
 *
 * ValueResult.ok    - flag, indicates success/unsuccess of function
 * ValueResult.value - parsed result
 * ValueResult.error - error message, in case of ValueResult.ok == false
 */
ValueResult parse_redis_string_to_decimal (RedisModuleString *string);


/**
 * Parses string to new Decimal structure.
 *
 * ValueResult.ok    - flag, indicates success/unsuccess of function
 * ValueResult.value - parsed result
 * ValueResult.error - error message, in case of ValueResult.ok == false
 */
ValueResult parse_string_to_decimal (const char *source_string, const size_t source_string_length);


/**
 * Creates (allocates and fills with default values) a new decimal structure.
 */
Decimal *new_decimal ();

/**
 * Setter for decimal structure's fields with its validation.
 *
 * ErrorOrNothing.ok    - flag, indicates success/unsuccess of function
 * ErrorOrNothing.error - error message, in case of ErrorOrNothing.ok == false
 */
ErrorOrNothing set_decimal_fields (Decimal *decimal, int128_t value, uint8_t scale);

/**
 * Copies structure fields from one instance to another.
 *
 * ErrorOrNothing.ok    - flag, indicates success/unsuccess of function
 * ErrorOrNothing.error - error message, in case of ErrorOrNothing.ok == false
 */
ErrorOrNothing set_decimal (Decimal *to, Decimal *from);

/**
 * Converts a value depends on a new scale.
 */
int128_t scale_shift (int128_t value, uint8_t old_scale, uint8_t new_scale);

/**
 * Scales a decimal value to specific scale value.
 *
 * ValueResult.ok    - flag, indicates success/unsuccess of function
 * ValueResult.value - scaled instance, it could be the same instance (if old and new decimals are equal),
 *                     or a new one
 * ValueResult.error - error message, in case of ValueResult.ok == false
 *
 *
 * Example:
 *
 * decimal = 2.5
 * scale_to(decimal, 2) // decimal=2.50
 * scale_to(decimal, 0) // decimal=2
 */
ValueResult scale_to (Decimal *decimal, uint8_t scale);

/**
 * Converts a Decimal value to string representation;
 */
char *to_string (Decimal *deciaml);

/**
 * Registers the Decimal structure type in a module context.
 */
int register_type (RedisModuleCtx *context);


#endif /* FPNT_TYPE_H */

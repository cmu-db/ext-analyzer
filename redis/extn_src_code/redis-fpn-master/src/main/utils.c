
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
#include "utils.h"
#include <ctype.h>
#include <inttypes.h>
#include <stdbool.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>


static uint8_t get_digits_count (int128_t value) {
  uint8_t result = 0;
  while (value != 0) {
    value /= 10;
    result++;
  }
  return result;
}

char *int128_to_string (const int128_t value) {
  uint8_t digits = get_digits_count(value);
  bool is_negative = value < 0;
  size_t buffer_size = digits + is_negative + 1; // number_of_digits + sign + '\0'
  char *result = (char*) RedisModule_Alloc(buffer_size);

  int128_t abs_value = value;
  if (is_negative) {
    result[0] = '-';
    abs_value *= -1;
  }

  for (uint8_t i = buffer_size - 2; abs_value; i--) {
    char character = abs_value % 10 + '0';
    abs_value /= 10;
    result[i] = character;
  }

  result[buffer_size - 1] = '\0';
  return result;
}

int128_t parse_to_int128 (const char *string) {
  int128_t result = 0;
  uint8_t index = 0;
  bool has_sign = false;

  if (string[index] == '-') {
    index++;
    has_sign = true;
  } else if (string[index] == '+') {
    index++;
  }

  uint8_t length = strlen(string);
  for (; index < length; index++) {
    char character = string[index];
    if (!isdigit(character)) {
      // uh?
    }
    result *= 10;
    result += character - '0';
  }

  return has_sign
         ? -result
         : result;
}

uint8_t parse_to_uint8_t (const char *string) {
  bool has_sign = string[0] == '-';
  int offset = has_sign;
  uint8_t result = 0;
  for (int index = offset; string[index] != '\0'; index++) {
    result = result * 10 + string[index] - '0';
  }
  return has_sign ? -result : result;
}

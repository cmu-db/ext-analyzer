
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


#include <ctype.h>
#include <stdbool.h>
#include <inttypes.h>
#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <string.h>


typedef __int128_t int128_t;
typedef __uint128_t uint128_t;


#define ZEROS_19 10000000000000000000ULL
#define NINES    9999999999999999999ULL


static int128_t MAX_VALUE = ((int128_t) ZEROS_19 * NINES) + NINES;
static int128_t MIN_VALUE = -(((int128_t) ZEROS_19 * NINES) + NINES);


unsigned char DEFAULT_DECIMAL_SCALE = 2;
char DEFAULT_DECIMAL_SEPARATOR = '.';



static int128_t value (uint8_t digits, bool negative) {
  int128_t result = 9;
  for (uint8_t index = 1; index < digits; index++) {
    result *= 10;
    result += 9;
  }
  return negative
         ? -result
         : result;
}

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
  char *result = (char*) malloc(buffer_size);

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

  if (string[index] == '-') {
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

void copy (const char *from, size_t from_start, size_t from_end, char *to, size_t to_from) {
  for (size_t index = from_start; index < from_end; index++) {
    to[to_from++] = from[index];
  }
}

int test_1 (void) {
  uint128_t result = 1;
  uint8_t b = 3;

  for (int index = 0; index < b; index++) {
    result *= 10;
  }
  printf("%lld\n", (long long) result);

  return EXIT_SUCCESS;
}

int test_2 (void) {
  const int128_t value = 1000;
  const uint8_t scale = 0;

  bool has_scale = scale > 0;
  if (value == 0 && !has_scale) {
    printf("result 1: 0.0");
    return EXIT_SUCCESS;
  }

  const bool has_sign = value < 0;
  const char *value_as_string = int128_to_string(value);
  const size_t value_length = strlen(value_as_string);

  size_t result_length;
  if ((value_length - has_sign) > scale) {
    result_length = value_length + has_scale; // .
  } else {
    result_length = scale + has_sign + 2; // 0.
  }
  char *result = (char*) malloc(result_length);

  int DEFAULT_DECIMAL_SEPARATOR_index = has_scale
                                ? result_length - 1 - scale
                                : -1;

  int value_reverse_index = value_length - 1;
  for (int index = result_length - 1; index >= 0; index--) {
    if (index == DEFAULT_DECIMAL_SEPARATOR_index) {
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
  printf("result: %s", result);

  return EXIT_SUCCESS;
}

int test_3 (void) {
  int128_t value = -100500;
  int number_of_digits = snprintf(NULL, 0, "%lld", (long long) value) - (value < 0);
  printf("%d\n", number_of_digits);
  return EXIT_SUCCESS;
}

int test_4 (void) {
  const char *source_string = "+.50";
  size_t source_string_length = strlen(source_string);
  size_t source_string_index = 0;

  char *value_string = (char*) malloc(source_string_length);
  size_t value_string_length = 0;

  unsigned char scale_count = 0;

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
      printf("Wrong character '%c' (%d) at index %zu in string '%s'",
             character, character, source_string_index, source_string);
      return EXIT_FAILURE;
    }
    source_string_index++;
  }
  value_string[value_string_length] = '\0';

  int128_t value = atoll(value_string);

  printf("original: %s\n", source_string);
  printf("value: %lld\n", (long long) value);
  printf("scale: %d\n", scale_count);
  return EXIT_SUCCESS;
}

int test_5 () {
  const int128_t value = 1234567812345678;
  const uint8_t scale = 8;

  bool has_scale = scale > 0;
  if (value == 0 && !has_scale) {
    printf("result: 0");
    return EXIT_SUCCESS;
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
  char *result = (char*) malloc(result_length);

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

  printf("result: %s\n", result);
  return EXIT_SUCCESS;
}

int test_6 () {
  printf(" -  6, true  = %s\n", int128_to_string(value(6, true)));
  printf(" - 18, true  = %s\n", int128_to_string(value(18, true)));
  printf("MIN_VALUE: %s\n", int128_to_string(MIN_VALUE));
  return EXIT_SUCCESS;
}

int test_7 () {
  printf("%s\n", int128_to_string(parse_to_int128("1")));
  printf("%s\n", int128_to_string(parse_to_int128("999999")));
  printf("%s\n", int128_to_string(parse_to_int128("-999999")));
  printf("%s\n", int128_to_string(parse_to_int128("999999999999999999")));
  printf("%s\n", int128_to_string(parse_to_int128("-999999999999999999")));
  printf("%s\n", int128_to_string(parse_to_int128("9999999999999999999")));
  printf("%s\n", int128_to_string(parse_to_int128("-9999999999999999999")));
  printf("%s\n", int128_to_string(parse_to_int128("999999999999999999999999999999999999")));
  printf("%s\n", int128_to_string(parse_to_int128("-999999999999999999999999999999999999")));
  printf("%s\n", int128_to_string(parse_to_int128("99999999999999999999999999999999999999")));
  printf("%s\n", int128_to_string(parse_to_int128("-99999999999999999999999999999999999999")));
  return EXIT_SUCCESS;
}


int main (void) {
  if (test_1() != EXIT_SUCCESS) {
    return EXIT_FAILURE;
  }
  if (test_2() != EXIT_SUCCESS) {
    return EXIT_FAILURE;
  }
  if (test_3() != EXIT_SUCCESS) {
    return EXIT_FAILURE;
  }
  if (test_4() != EXIT_SUCCESS) {
    return EXIT_FAILURE;
  }
  if (test_5() != EXIT_SUCCESS) {
    return EXIT_FAILURE;
  }
  if (test_6() != EXIT_SUCCESS) {
    return EXIT_FAILURE;
  }
  if (test_7() != EXIT_SUCCESS) {
    return EXIT_FAILURE;
  }

  return EXIT_SUCCESS;
}

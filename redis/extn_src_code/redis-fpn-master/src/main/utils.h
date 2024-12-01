
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


#ifndef UTILS_H
#define UTILS_H


#include <stdbool.h>
#include <stdint.h>


typedef __int128_t int128_t;
typedef __uint128_t uint128_t;

typedef struct {

  bool ok;
  void *value;
  char *error;
} ValueResult;

typedef struct {

  bool ok;
  char *error;
} ErrorOrNothing;


/**
 * Converts 128 bits integer to string.
 */
char *int128_to_string (const int128_t);

/**
 * Parses string to signed 128 bits value.
 */
int128_t parse_to_int128 (const char *string);

/**
 * Parses string to unsigned byte value.
 */
uint8_t parse_to_uint8_t (const char *string);


#endif /* UTILS_H */

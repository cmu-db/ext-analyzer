// Copyright 2022 Fuchi Takeshi (fuchi@fuchi.com)

#pragma GCC diagnostic ignored "-Wtypedef-redefinition"
#pragma GCC diagnostic ignored "-Wunused-parameter"
#pragma GCC diagnostic ignored "-Wunused-function"
#pragma GCC diagnostic ignored "-Wunused-variable"

/**
 * @brief mmapしたファイルから値を読みだす
 */

#include <sys/mman.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <fcntl.h>
#include <unistd.h>
#include <stdbool.h>
#include <stdint.h>
#include <strings.h>
#include <string.h>
#include <float.h>

#include "redismodule.h"
#include "sds.h"
#include "zmalloc.h"

typedef struct _MMapObject
{
  sds file_path;
  int fd;
  void *mmap;
  size_t file_size;
  sds value_type;
  uint8_t value_size;
  bool writable;
} MMapObject;

static inline int mstringcmp(const RedisModuleString *rs1, const char *s2)
{
  return strcasecmp(RedisModule_StringPtrLen(rs1, NULL), s2);
}

RedisModuleType *MMapType = NULL;

MMapObject *MCreateObject(void)
{
  return (MMapObject *)zcalloc(sizeof(MMapObject));
}

void MFree(void *value)
{
  if (value == NULL) return;
  const MMapObject *obj_ptr = value;
  if (obj_ptr->mmap != NULL) munmap(obj_ptr->mmap, obj_ptr->file_size);
  if (obj_ptr->fd != -1) close(obj_ptr->fd);
  sdsfree(obj_ptr->file_path);
  sdsfree(obj_ptr->value_type);
  zfree(value);
}

// MMAP key file_path value_type [value_size] [writable]
int MMap_RedisCommand(RedisModuleCtx *ctx, RedisModuleString **argv, int argc)
{
  RedisModule_AutoMemory(ctx); /* Use automatic memory management. */
  if (argc < 4 || 6 < argc) return RedisModule_WrongArity(ctx);
  bool writable = false;
  uint8_t value_size = 0;
  long long tmp_size;
  for (int i = 4; i < argc; ++i) {
    if (mstringcmp(argv[i], "writable") == 0) writable = true;
    else if (RedisModule_StringToLongLong(argv[i], &tmp_size) == REDISMODULE_OK) {
      if (tmp_size <= 0) {
        return RedisModule_ReplyWithError(
            ctx, "value_size must be positive");
      }
      if (0x100 <= tmp_size) {
        return RedisModule_ReplyWithError(
            ctx, "value_size must be between 1 and 255");
      }
      value_size = (uint8_t)tmp_size;
    }
    else {
      return RedisModule_ReplyWithError(
          ctx, "Arguments must be \"writable\" or integer");
    }
  }

  if (mstringcmp(argv[3], "int8") == 0) {
    if (value_size == 0) value_size = 1;
  }
  else if (mstringcmp(argv[3], "uint8") == 0) {
    if (value_size == 0) value_size = 1;
    if (value_size != 1) {
      return RedisModule_ReplyWithError(ctx, "invalid value_size");
    }
  }
  else if (mstringcmp(argv[3], "int16") == 0) {
    if (value_size == 0) value_size = 2;
    if (value_size != 2) {
      return RedisModule_ReplyWithError(ctx, "invalid value_size");
    }
  }
  else if (mstringcmp(argv[3], "uint16") == 0) {
    if (value_size == 0) value_size = 2;
    if (value_size != 2) {
      return RedisModule_ReplyWithError(ctx, "invalid value_size");
    }
  }
  else if (mstringcmp(argv[3], "int32") == 0) {
    if (value_size == 0) value_size = 4;
    if (value_size != 4) {
      return RedisModule_ReplyWithError(ctx, "invalid value_size");
    }
  }
  else if (mstringcmp(argv[3], "uint32") == 0) {
    if (value_size == 0) value_size = 4;
    if (value_size != 4) {
      return RedisModule_ReplyWithError(ctx, "invalid value_size");
    }
  }
  else if (mstringcmp(argv[3], "int64") == 0) {
    if (value_size == 0) value_size = 8;
    if (value_size != 8) {
      return RedisModule_ReplyWithError(ctx, "invalid value_size");
    }
  }
  else if (mstringcmp(argv[3], "uint64") == 0) {
    if (value_size == 0) value_size = 8;
    if (value_size != 8) {
      return RedisModule_ReplyWithError(ctx, "invalid value_size");
    }
  }
  else if (mstringcmp(argv[3], "float") == 0) {
    if (value_size == 0) value_size = 4;
    if (value_size != 4) {
      return RedisModule_ReplyWithError(ctx, "invalid value_size");
    }
  }
  else if (mstringcmp(argv[3], "double") == 0) {
    if (value_size == 0) value_size = 8;
    if (value_size != 8) {
      return RedisModule_ReplyWithError(ctx, "invalid value_size");
    }
  }
  else if (mstringcmp(argv[3], "long_double") == 0) {
    if (value_size == 0) value_size = 16;
    if (value_size != 16) {
      return RedisModule_ReplyWithError(ctx, "invalid value_size");
    }
  }
  else if (mstringcmp(argv[3], "string") == 0) {
    if (value_size == 0) {
      return RedisModule_ReplyWithError(
        ctx, "string type must has value_size");
    }
  }
  else {
    return RedisModule_ReplyWithError(
      ctx, "value_type must be int8, uint8, int16, uint16, int32, uint32, int64, uint64, float, double, long_double or string");
  }

  RedisModuleKey *key = RedisModule_OpenKey(ctx, argv[1], REDISMODULE_READ | REDISMODULE_WRITE);
  int type = RedisModule_KeyType(key);
  if (type != REDISMODULE_KEYTYPE_EMPTY &&
      RedisModule_ModuleTypeGetType(key) != MMapType) {
    return RedisModule_ReplyWithError(ctx, REDISMODULE_ERRORMSG_WRONGTYPE);
  }

  /* Create an empty value object if the key is currently empty. */
  MMapObject *obj_ptr;
  if (type == REDISMODULE_KEYTYPE_EMPTY) {
    obj_ptr = MCreateObject();
    obj_ptr->file_path = sdsnew(RedisModule_StringPtrLen(argv[2], NULL));
    obj_ptr->value_type = sdsnew(RedisModule_StringPtrLen(argv[3], NULL));
    obj_ptr->value_size = value_size;
    obj_ptr->writable = writable;
    if (obj_ptr->writable) {
      obj_ptr->fd = open(obj_ptr->file_path, O_RDWR | O_CREAT, 0666);
      if (obj_ptr->fd == -1) {
        int ret = RedisModule_ReplyWithError(ctx, obj_ptr->file_path);
        MFree(obj_ptr);
        return ret;
      }
    }
    else {
      obj_ptr->fd = open(obj_ptr->file_path, O_RDONLY);
      if (obj_ptr->fd == -1) {
        int ret = RedisModule_ReplyWithError(ctx, obj_ptr->file_path);
        MFree(obj_ptr);
        return ret;
      }
    }
    struct stat sb;
    if (fstat(obj_ptr->fd, &sb) == -1) {
      int ret = RedisModule_ReplyWithError(ctx, obj_ptr->file_path);
      MFree(obj_ptr);
      return ret;
    }
    obj_ptr->file_size = sb.st_size;
    if (0 < obj_ptr->file_size) {
      if (obj_ptr->writable) {
        obj_ptr->mmap = mmap(NULL, sb.st_size, PROT_READ | PROT_WRITE, MAP_SHARED, obj_ptr->fd, 0);
      }
      else {
        obj_ptr->mmap = mmap(NULL, sb.st_size, PROT_READ, MAP_SHARED, obj_ptr->fd, 0);
      }
      if (obj_ptr->mmap == MAP_FAILED) {
        int ret = RedisModule_ReplyWithError(ctx, obj_ptr->file_path);
        MFree(obj_ptr);
        return ret;
      }
    }
    else obj_ptr->mmap = NULL;
    RedisModule_ModuleTypeSetValue(key, MMapType, obj_ptr);
  }
  else {
    obj_ptr = RedisModule_ModuleTypeGetValue(key);
    if (obj_ptr == NULL) {
      RedisModule_ReplyWithNull(ctx);
      return REDISMODULE_ERR;
    }
    if (strcmp(obj_ptr->file_path, RedisModule_StringPtrLen(argv[2], NULL)) != 0) {
      return RedisModule_ReplyWithError(ctx, "It is already mapped on another file");
    }
  }

  return RedisModule_ReplyWithLongLong(ctx, obj_ptr->file_size / obj_ptr->value_size);
}

// VGET key index
int VGet_RedisCommand(RedisModuleCtx *ctx, RedisModuleString **argv, int argc)
{
  RedisModule_AutoMemory(ctx); /* Use automatic memory management. */
  if (argc != 3) return RedisModule_WrongArity(ctx);

  RedisModuleKey *key =
      RedisModule_OpenKey(ctx, argv[1], REDISMODULE_READ | REDISMODULE_WRITE);
  int type = RedisModule_KeyType(key);
  if (type != REDISMODULE_KEYTYPE_EMPTY &&
      RedisModule_ModuleTypeGetType(key) != MMapType) {
    return RedisModule_ReplyWithError(ctx, REDISMODULE_ERRORMSG_WRONGTYPE);
  }

  if (type == REDISMODULE_KEYTYPE_EMPTY) {
    return RedisModule_ReplyWithError(ctx, "You must do MMAP first");
  }

  long long index;
  if (RedisModule_StringToLongLong(argv[2], &index) == REDISMODULE_ERR) {
    return RedisModule_ReplyWithError(ctx, "index argument must be integer");
  }

  MMapObject *obj_ptr = RedisModule_ModuleTypeGetValue(key);
  if (obj_ptr == NULL) {
    return REDISMODULE_ERR;
  }

  if (obj_ptr->file_size <= (size_t)index * obj_ptr->value_size || index < 0) {
    return RedisModule_ReplyWithError(ctx, "index exceeds size");
  }
  else {
    if (strcasecmp(obj_ptr->value_type, "int8") == 0) {
      RedisModule_ReplyWithLongLong(ctx, ((int8_t*)obj_ptr->mmap)[index]);
    }
    else if (strcasecmp(obj_ptr->value_type, "uint8") == 0) {
      RedisModule_ReplyWithLongLong(ctx, ((uint8_t*)obj_ptr->mmap)[index]);
    }
    else if (strcasecmp(obj_ptr->value_type, "int16") == 0) {
      RedisModule_ReplyWithLongLong(ctx, ((int16_t*)obj_ptr->mmap)[index]);
    }
    else if (strcasecmp(obj_ptr->value_type, "uint16") == 0) {
      RedisModule_ReplyWithLongLong(ctx, ((uint16_t*)obj_ptr->mmap)[index]);
    }
    else if (strcasecmp(obj_ptr->value_type, "int32") == 0) {
      RedisModule_ReplyWithLongLong(ctx, ((int32_t*)obj_ptr->mmap)[index]);
    }
    else if (strcasecmp(obj_ptr->value_type, "uint32") == 0) {
      RedisModule_ReplyWithLongLong(ctx, ((uint32_t*)obj_ptr->mmap)[index]);
    }
    else if (strcasecmp(obj_ptr->value_type, "int64") == 0) {
      RedisModule_ReplyWithLongLong(ctx, ((int64_t*)obj_ptr->mmap)[index]);
    }
    else if (strcasecmp(obj_ptr->value_type, "uint64") == 0) {
      RedisModule_ReplyWithLongLong(ctx, ((uint64_t*)obj_ptr->mmap)[index]);
    }
    else if (strcasecmp(obj_ptr->value_type, "float") == 0) {
      RedisModule_ReplyWithDouble(ctx, ((float*)obj_ptr->mmap)[index]);
    }
    else if (strcasecmp(obj_ptr->value_type, "double") == 0) {
      RedisModule_ReplyWithDouble(ctx, ((double*)obj_ptr->mmap)[index]);
    }
    else if (strcasecmp(obj_ptr->value_type, "long_double") == 0) {
      RedisModule_ReplyWithLongDouble(ctx, ((long double*)obj_ptr->mmap)[index]);
    }
    else if (strcasecmp(obj_ptr->value_type, "string") == 0) {
      char *buffer = zcalloc(obj_ptr->value_size + 1);
      snprintf(buffer, obj_ptr->value_size + 1, "%s", (char *)obj_ptr->mmap + index * obj_ptr->value_size);
      RedisModule_ReplyWithStringBuffer(ctx, buffer, strlen(buffer));
      zfree(buffer);
    }
    else return REDISMODULE_ERR;
  }
  return REDISMODULE_OK;
}

// VMGET key index [index ...]
int VMGet_RedisCommand(RedisModuleCtx *ctx, RedisModuleString **argv, int argc)
{
  RedisModule_AutoMemory(ctx); /* Use automatic memory management. */
  if (argc < 3) return RedisModule_WrongArity(ctx);

  RedisModuleKey *key =
      RedisModule_OpenKey(ctx, argv[1], REDISMODULE_READ | REDISMODULE_WRITE);
  int type = RedisModule_KeyType(key);
  if (type != REDISMODULE_KEYTYPE_EMPTY &&
      RedisModule_ModuleTypeGetType(key) != MMapType) {
    return RedisModule_ReplyWithError(ctx, REDISMODULE_ERRORMSG_WRONGTYPE);
  }

  if (type == REDISMODULE_KEYTYPE_EMPTY) {
    return RedisModule_ReplyWithError(ctx, "You must do MMAP first");
  }

  MMapObject *obj_ptr = RedisModule_ModuleTypeGetValue(key);
  if (obj_ptr == NULL) {
    return REDISMODULE_ERR;
  }

  long long index;
  for (int i = 2; i < argc; i += 2) {
    if (RedisModule_StringToLongLong(argv[i], &index) == REDISMODULE_ERR) {
      return RedisModule_ReplyWithError(ctx, "index argument must be integer");
    }
    if ((long long)obj_ptr->file_size <= index * obj_ptr->value_size || index < 0) {
      return RedisModule_ReplyWithError(ctx, "index exceeds size");
    }
  }

  RedisModule_ReplyWithArray(ctx, argc - 2);
  if (strcasecmp(obj_ptr->value_type, "int8") == 0) {
    for (int i = 2; i < argc; i++) {
      RedisModule_StringToLongLong(argv[i], &index);
      if (obj_ptr->file_size <= (size_t)index * obj_ptr->value_size || index < 0) {
        RedisModule_ReplyWithNull(ctx);
      }
      else {
        RedisModule_ReplyWithLongLong(ctx, ((int8_t*)obj_ptr->mmap)[index]);
      }
    }
  }
  else if (strcasecmp(obj_ptr->value_type, "uint8") == 0) {
    for (int i = 2; i < argc; i++) {
      RedisModule_StringToLongLong(argv[i], &index);
      if (obj_ptr->file_size <= (size_t)index * obj_ptr->value_size || index < 0) {
        RedisModule_ReplyWithNull(ctx);
      }
      else {
        RedisModule_ReplyWithLongLong(ctx, ((uint8_t*)obj_ptr->mmap)[index]);
      }
    }
  }
  else if (strcasecmp(obj_ptr->value_type, "int16") == 0) {
    for (int i = 2; i < argc; i++) {
      RedisModule_StringToLongLong(argv[i], &index);
      if (obj_ptr->file_size <= (size_t)index * obj_ptr->value_size || index < 0) {
        RedisModule_ReplyWithNull(ctx);
      }
      else {
        RedisModule_ReplyWithLongLong(ctx, ((int16_t*)obj_ptr->mmap)[index]);
      }
    }
  }
  else if (strcasecmp(obj_ptr->value_type, "uint16") == 0) {
    for (int i = 2; i < argc; i++) {
      RedisModule_StringToLongLong(argv[i], &index);
      if (obj_ptr->file_size <= (size_t)index * obj_ptr->value_size || index < 0) {
        RedisModule_ReplyWithNull(ctx);
      }
      else {
        RedisModule_ReplyWithLongLong(ctx, ((uint16_t*)obj_ptr->mmap)[index]);
      }
    }
  }
  else if (strcasecmp(obj_ptr->value_type, "int32") == 0) {
    for (int i = 2; i < argc; i++) {
      RedisModule_StringToLongLong(argv[i], &index);
      if (obj_ptr->file_size <= (size_t)index * obj_ptr->value_size || index < 0) {
        RedisModule_ReplyWithNull(ctx);
      }
      else {
        RedisModule_ReplyWithLongLong(ctx, ((int32_t*)obj_ptr->mmap)[index]);
      }
    }
  }
  else if (strcasecmp(obj_ptr->value_type, "uint32") == 0) {
    for (int i = 2; i < argc; i++) {
      RedisModule_StringToLongLong(argv[i], &index);
      if (obj_ptr->file_size <= (size_t)index * obj_ptr->value_size || index < 0) {
        RedisModule_ReplyWithNull(ctx);
      }
      else {
        RedisModule_ReplyWithLongLong(ctx, ((uint32_t*)obj_ptr->mmap)[index]);
      }
    }
  }
  else if (strcasecmp(obj_ptr->value_type, "int64") == 0) {
    for (int i = 2; i < argc; i++) {
      RedisModule_StringToLongLong(argv[i], &index);
      if (obj_ptr->file_size <= (size_t)index * obj_ptr->value_size || index < 0) {
        RedisModule_ReplyWithNull(ctx);
      }
      else {
        RedisModule_ReplyWithLongLong(ctx, ((int64_t*)obj_ptr->mmap)[index]);
      }
    }
  }
  else if (strcasecmp(obj_ptr->value_type, "uint64") == 0) {
    for (int i = 2; i < argc; i++) {
      RedisModule_StringToLongLong(argv[i], &index);
      if (obj_ptr->file_size <= (size_t)index * obj_ptr->value_size || index < 0) {
        RedisModule_ReplyWithNull(ctx);
      }
      else {
        RedisModule_ReplyWithLongLong(ctx, ((uint64_t*)obj_ptr->mmap)[index]);
      }
    }
  }
  else if (strcasecmp(obj_ptr->value_type, "float") == 0) {
    for (int i = 2; i < argc; i++) {
      RedisModule_StringToLongLong(argv[i], &index);
      if (obj_ptr->file_size <= (size_t)index * obj_ptr->value_size || index < 0) {
        RedisModule_ReplyWithNull(ctx);
      }
      else {
        RedisModule_ReplyWithDouble(ctx, ((float*)obj_ptr->mmap)[index]);
      }
    }
  }
  else if (strcasecmp(obj_ptr->value_type, "double") == 0) {
    for (int i = 2; i < argc; i++) {
      RedisModule_StringToLongLong(argv[i], &index);
      if (obj_ptr->file_size <= (size_t)index * obj_ptr->value_size || index < 0) {
        RedisModule_ReplyWithNull(ctx);
      }
      else {
        RedisModule_ReplyWithDouble(ctx, ((double*)obj_ptr->mmap)[index]);
      }
    }
  }
  else if (strcasecmp(obj_ptr->value_type, "long_double") == 0) {
    for (int i = 2; i < argc; i++) {
      RedisModule_StringToLongLong(argv[i], &index);
      if (obj_ptr->file_size <= (size_t)index * obj_ptr->value_size || index < 0) {
        RedisModule_ReplyWithNull(ctx);
      }
      else {
        RedisModule_ReplyWithLongDouble(ctx, ((long double*)obj_ptr->mmap)[index]);
      }
    }
  }
  else if (strcasecmp(obj_ptr->value_type, "string") == 0) {
    for (int i = 2; i < argc; i++) {
      RedisModule_StringToLongLong(argv[i], &index);
      if (obj_ptr->file_size <= (size_t)index * obj_ptr->value_size || index < 0) {
        RedisModule_ReplyWithNull(ctx);
      }
      else {
        char *buffer = zcalloc(obj_ptr->value_size + 1);
        snprintf(buffer, obj_ptr->value_size + 1, "%s", (char *)obj_ptr->mmap + index * obj_ptr->value_size);
        RedisModule_ReplyWithStringBuffer(ctx, buffer, strlen(buffer));
        zfree(buffer);
      }
    }
  }
  else return REDISMODULE_ERR;

  return REDISMODULE_OK;
}

// VALL key
int VAll_RedisCommand(RedisModuleCtx *ctx, RedisModuleString **argv, int argc)
{
  RedisModule_AutoMemory(ctx); /* Use automatic memory management. */
  if (argc != 2) return RedisModule_WrongArity(ctx);

  RedisModuleKey *key =
      RedisModule_OpenKey(ctx, argv[1], REDISMODULE_READ | REDISMODULE_WRITE);
  int type = RedisModule_KeyType(key);
  if (type != REDISMODULE_KEYTYPE_EMPTY &&
      RedisModule_ModuleTypeGetType(key) != MMapType) {
    return RedisModule_ReplyWithError(ctx, REDISMODULE_ERRORMSG_WRONGTYPE);
  }

  if (type == REDISMODULE_KEYTYPE_EMPTY) {
    return RedisModule_ReplyWithError(ctx, "You must do MMAP first");
  }

  MMapObject *obj_ptr = RedisModule_ModuleTypeGetValue(key);
  if (obj_ptr == NULL) {
    return REDISMODULE_ERR;
  }

  RedisModule_ReplyWithArray(ctx, obj_ptr->file_size / obj_ptr->value_size);

  if (strcasecmp(obj_ptr->value_type, "int8") == 0) {
    for (size_t index = 0; index < obj_ptr->file_size / obj_ptr->value_size; ++index) {
      RedisModule_ReplyWithLongLong(ctx, ((int8_t*)obj_ptr->mmap)[index]);
    }
  }
  else if (strcasecmp(obj_ptr->value_type, "uint8") == 0) {
    for (size_t index = 0; index < obj_ptr->file_size / obj_ptr->value_size; ++index) {
      RedisModule_ReplyWithLongLong(ctx, ((uint8_t*)obj_ptr->mmap)[index]);
    }
  }
  else if (strcasecmp(obj_ptr->value_type, "int16") == 0) {
    for (size_t index = 0; index < obj_ptr->file_size / obj_ptr->value_size; ++index) {
      RedisModule_ReplyWithLongLong(ctx, ((int16_t*)obj_ptr->mmap)[index]);
    }
  }
  else if (strcasecmp(obj_ptr->value_type, "uint16") == 0) {
    for (size_t index = 0; index < obj_ptr->file_size / obj_ptr->value_size; ++index) {
      RedisModule_ReplyWithLongLong(ctx, ((uint16_t*)obj_ptr->mmap)[index]);
    }
  }
  else if (strcasecmp(obj_ptr->value_type, "int32") == 0) {
    for (size_t index = 0; index < obj_ptr->file_size / obj_ptr->value_size; ++index) {
      RedisModule_ReplyWithLongLong(ctx, ((int32_t*)obj_ptr->mmap)[index]);
    }
  }
  else if (strcasecmp(obj_ptr->value_type, "uint32") == 0) {
    for (size_t index = 0; index < obj_ptr->file_size / obj_ptr->value_size; ++index) {
      RedisModule_ReplyWithLongLong(ctx, ((uint32_t*)obj_ptr->mmap)[index]);
    }
  }
  else if (strcasecmp(obj_ptr->value_type, "int64") == 0) {
    for (size_t index = 0; index < obj_ptr->file_size / obj_ptr->value_size; ++index) {
      RedisModule_ReplyWithLongLong(ctx, ((int64_t*)obj_ptr->mmap)[index]);
    }
  }
  else if (strcasecmp(obj_ptr->value_type, "uint64") == 0) {
    for (size_t index = 0; index < obj_ptr->file_size / obj_ptr->value_size; ++index) {
      RedisModule_ReplyWithLongLong(ctx, ((uint64_t*)obj_ptr->mmap)[index]);
    }
  }
  else if (strcasecmp(obj_ptr->value_type, "float") == 0) {
    for (size_t index = 0; index < obj_ptr->file_size / obj_ptr->value_size; ++index) {
      RedisModule_ReplyWithDouble(ctx, ((float*)obj_ptr->mmap)[index]);
    }
  }
  else if (strcasecmp(obj_ptr->value_type, "double") == 0) {
    for (size_t index = 0; index < obj_ptr->file_size / obj_ptr->value_size; ++index) {
      RedisModule_ReplyWithDouble(ctx, ((double*)obj_ptr->mmap)[index]);
    }
  }
  else if (strcasecmp(obj_ptr->value_type, "long_double") == 0) {
    for (size_t index = 0; index < obj_ptr->file_size / obj_ptr->value_size; ++index) {
      RedisModule_ReplyWithLongDouble(ctx, ((long double*)obj_ptr->mmap)[index]);
    }
  }
  else if (strcasecmp(obj_ptr->value_type, "string") == 0) {
    for (size_t index = 0; index < obj_ptr->file_size / obj_ptr->value_size; ++index) {
      char *buffer = zcalloc(obj_ptr->value_size + 1);
      snprintf(buffer, obj_ptr->value_size + 1, "%s", (char *)obj_ptr->mmap + index * obj_ptr->value_size);
      RedisModule_ReplyWithStringBuffer(ctx, buffer, strlen(buffer));
      zfree(buffer);
    }
  }
  else return REDISMODULE_ERR;

  return REDISMODULE_OK;
}

// VSET key index value [index value ...]
int VSet_RedisCommand(RedisModuleCtx *ctx, RedisModuleString **argv, int argc)
{
  RedisModule_AutoMemory(ctx); /* Use automatic memory management. */

  if (argc < 4) return RedisModule_WrongArity(ctx);
  if ((argc - 2) % 2 != 0) return RedisModule_WrongArity(ctx);

  RedisModuleKey *key =
      RedisModule_OpenKey(ctx, argv[1], REDISMODULE_READ | REDISMODULE_WRITE);
  int type = RedisModule_KeyType(key);
  if (type != REDISMODULE_KEYTYPE_EMPTY &&
      RedisModule_ModuleTypeGetType(key) != MMapType) {
    return RedisModule_ReplyWithError(ctx, REDISMODULE_ERRORMSG_WRONGTYPE);
  }
  if (type == REDISMODULE_KEYTYPE_EMPTY) {
    return RedisModule_ReplyWithError(ctx, "You must do MMAP first");
  }

  MMapObject *obj_ptr = RedisModule_ModuleTypeGetValue(key);
  if (obj_ptr == NULL) {
    RedisModule_ReplyWithNull(ctx);
    return REDISMODULE_ERR;
  }
  if (!obj_ptr->writable) {
    return RedisModule_ReplyWithError(ctx, "The file is not writable");
  }

  long long index;
  for (int i = 2; i < argc; i += 2) {
    if (RedisModule_StringToLongLong(argv[i], &index) == REDISMODULE_ERR) {
      return RedisModule_ReplyWithError(ctx, "index argument must be integer");
    }
    if ((long long)obj_ptr->file_size <= index * obj_ptr->value_size || index < 0) {
      return RedisModule_ReplyWithError(ctx, "index exceeds size");
    }
  }

  if (strcasecmp(obj_ptr->value_type, "int8") == 0) {
    long long value;
    for (int i = 3; i < argc; i += 2) {
      if (RedisModule_StringToLongLong(argv[i], &value) == REDISMODULE_ERR) {
        return RedisModule_ReplyWithError(ctx, "value must be integer");
      }
      if (value < INT8_MIN || INT8_MAX < value) {
        return RedisModule_ReplyWithError(ctx, "value must be between -128 and 127");
      }
    }
    for (int i = 2; i < argc; i += 2) {
      RedisModule_StringToLongLong(argv[i], &index);
      RedisModule_StringToLongLong(argv[i + 1], &value);
      *((int8_t*)obj_ptr->mmap + index) = (int8_t)value;
    }
  }
  else if (strcasecmp(obj_ptr->value_type, "uint8") == 0) {
    long long value;
    for (int i = 3; i < argc; i += 2) {
      if (RedisModule_StringToLongLong(argv[i], &value) == REDISMODULE_ERR) {
        return RedisModule_ReplyWithError(ctx, "value must be integer");
      }
      if (value < 0 || UINT8_MAX < value) {
        return RedisModule_ReplyWithError(ctx, "value must be between -128 and 127");
      }
    }
    for (int i = 2; i < argc; i += 2) {
      RedisModule_StringToLongLong(argv[i], &index);
      RedisModule_StringToLongLong(argv[i + 1], &value);
      *((uint8_t*)obj_ptr->mmap + index) = (uint8_t)value;
    }
  }
  else if (strcasecmp(obj_ptr->value_type, "int16") == 0) {
    long long value;
    for (int i = 3; i < argc; i += 2) {
      if (RedisModule_StringToLongLong(argv[i], &value) == REDISMODULE_ERR) {
        return RedisModule_ReplyWithError(ctx, "value must be integer");
      }
      if (value < INT16_MIN || INT16_MAX < value) {
        return RedisModule_ReplyWithError(ctx, "value must be int16");
      }
    }
    for (int i = 2; i < argc; i += 2) {
      RedisModule_StringToLongLong(argv[i], &index);
      RedisModule_StringToLongLong(argv[i + 1], &value);
      *((int16_t*)obj_ptr->mmap + index) = (int16_t)value;
    }
  }
  else if (strcasecmp(obj_ptr->value_type, "uint16") == 0) {
    long long value;
    for (int i = 3; i < argc; i += 2) {
      if (RedisModule_StringToLongLong(argv[i], &value) == REDISMODULE_ERR) {
        return RedisModule_ReplyWithError(ctx, "value must be integer");
      }
      if (value < 0 || UINT16_MAX < value) {
        return RedisModule_ReplyWithError(ctx, "value must be uint16");
      }
    }
    for (int i = 2; i < argc; i += 2) {
      RedisModule_StringToLongLong(argv[i], &index);
      RedisModule_StringToLongLong(argv[i + 1], &value);
      *((uint16_t*)obj_ptr->mmap + index) = (uint16_t)value;
    }
  }
  else if (strcasecmp(obj_ptr->value_type, "int32") == 0) {
    long long value;
    for (int i = 3; i < argc; i += 2) {
      if (RedisModule_StringToLongLong(argv[i], &value) == REDISMODULE_ERR) {
        return RedisModule_ReplyWithError(ctx, "value must be integer");
      }
      if ((value < INT32_MIN) || (INT32_MAX < value)) {
        return RedisModule_ReplyWithError(ctx, "value must be int32");
      }
    }
    for (int i = 2; i < argc; i += 2) {
      RedisModule_StringToLongLong(argv[i], &index);
      RedisModule_StringToLongLong(argv[i + 1], &value);
      *((int32_t*)obj_ptr->mmap + index) = (int32_t)value;
    }
  }
  else if (strcasecmp(obj_ptr->value_type, "uint32") == 0) {
    long long value;
    for (int i = 3; i < argc; i += 2) {
      if (RedisModule_StringToLongLong(argv[i], &value) == REDISMODULE_ERR) {
        return RedisModule_ReplyWithError(ctx, "value must be integer");
      }
      if (value < 0 || UINT32_MAX < value) {
        return RedisModule_ReplyWithError(ctx, "value must be uint32");
      }
    }
    for (int i = 2; i < argc; i += 2) {
      RedisModule_StringToLongLong(argv[i], &index);
      RedisModule_StringToLongLong(argv[i + 1], &value);
      *((uint32_t*)obj_ptr->mmap + index) = (uint32_t)value;
    }
  }
  else if (strcasecmp(obj_ptr->value_type, "int64") == 0) {
    long long value;
    for (int i = 3; i < argc; i += 2) {
      if (RedisModule_StringToLongLong(argv[i], &value) == REDISMODULE_ERR) {
        return RedisModule_ReplyWithError(ctx, "value must be integer");
      }
      if (value < INT64_MIN || INT64_MAX < value) {
        return RedisModule_ReplyWithError(ctx, "value must be int64");
      }
    }
    for (int i = 2; i < argc; i += 2) {
      RedisModule_StringToLongLong(argv[i], &index);
      RedisModule_StringToLongLong(argv[i + 1], &value);
      *((int64_t*)obj_ptr->mmap + index) = (int64_t)value;
    }
  }
  else if (strcasecmp(obj_ptr->value_type, "uint64") == 0) {
    long long value;
    for (int i = 3; i < argc; i += 2) {
      if (RedisModule_StringToLongLong(argv[i], &value) == REDISMODULE_ERR) {
        return RedisModule_ReplyWithError(ctx, "value must be integer");
      }
      if (value < 0) {
        return RedisModule_ReplyWithError(ctx, "value must be uint64");
      }
    }
    for (int i = 2; i < argc; i += 2) {
      RedisModule_StringToLongLong(argv[i], &index);
      RedisModule_StringToLongLong(argv[i + 1], &value);
      *((uint64_t*)obj_ptr->mmap + index) = (uint64_t)value;
    }
  }
  else if (strcasecmp(obj_ptr->value_type, "float") == 0) {
    double value;
    for (int i = 3; i < argc; i += 2) {
      if (RedisModule_StringToDouble(argv[i], &value) == REDISMODULE_ERR) {
        return RedisModule_ReplyWithError(ctx, "value must be float");
      }
    }
    for (int i = 2; i < argc; i += 2) {
      RedisModule_StringToLongLong(argv[i], &index);
      RedisModule_StringToDouble(argv[i + 1], &value);
      *((float*)obj_ptr->mmap + index) = (float)value;
    }
  }
  else if (strcasecmp(obj_ptr->value_type, "double") == 0) {
    double value;
    for (int i = 3; i < argc; i += 2) {
      if (RedisModule_StringToDouble(argv[i], &value) == REDISMODULE_ERR) {
        return RedisModule_ReplyWithError(ctx, "value must be double");
      }
    }
    for (int i = 2; i < argc; i += 2) {
      RedisModule_StringToLongLong(argv[i], &index);
      RedisModule_StringToDouble(argv[i + 1], &value);
      *((double*)obj_ptr->mmap + index) = (double)value;
    }
  }
  else if (strcasecmp(obj_ptr->value_type, "long_double") == 0) {
    long double value;
    for (int i = 3; i < argc; i += 2) {
      if (RedisModule_StringToLongDouble(argv[i], &value) == REDISMODULE_ERR) {
        return RedisModule_ReplyWithError(ctx, "value must be long double");
      }
    }
    for (int i = 2; i < argc; i += 2) {
      RedisModule_StringToLongLong(argv[i], &index);
      RedisModule_StringToLongDouble(argv[i + 1], &value);
      *((long double*)obj_ptr->mmap + index) = (long double)value;
    }
  }
  else if (strcasecmp(obj_ptr->value_type, "string") == 0) {
    size_t value_size;
    for (int i = 3; i < argc; i += 2) {
      RedisModule_StringPtrLen(argv[i], &value_size);
      if (obj_ptr->value_size < value_size) {
        return RedisModule_ReplyWithError(ctx, "value is too long");
      }
    }
    for (int i = 2; i < argc; i += 2) {
      RedisModule_StringToLongLong(argv[i], &index);
      char *value = zcalloc(obj_ptr->value_size + 1);
      memcpy(value, RedisModule_StringPtrLen(argv[i + 1], &value_size), value_size);
      memcpy((char*)obj_ptr->mmap + index * obj_ptr->value_size, value, obj_ptr->value_size);
      zfree(value);
    }
  }
  msync(obj_ptr->mmap, obj_ptr->file_size, MS_ASYNC);
  return RedisModule_ReplyWithLongLong(ctx, (argc - 2) / 2);
}


// VADD key value [value ...]
int VAdd_RedisCommand(RedisModuleCtx *ctx, RedisModuleString **argv, int argc)
{
  RedisModule_AutoMemory(ctx); /* Use automatic memory management. */

  if (argc < 3) return RedisModule_WrongArity(ctx);

  RedisModuleKey *key =
      RedisModule_OpenKey(ctx, argv[1], REDISMODULE_READ | REDISMODULE_WRITE);
  int type = RedisModule_KeyType(key);
  if (type != REDISMODULE_KEYTYPE_EMPTY &&
      RedisModule_ModuleTypeGetType(key) != MMapType) {
    return RedisModule_ReplyWithError(ctx, REDISMODULE_ERRORMSG_WRONGTYPE);
  }
  if (type == REDISMODULE_KEYTYPE_EMPTY) {
    return RedisModule_ReplyWithError(ctx, "You must do MMAP first");
  }

  MMapObject *obj_ptr = RedisModule_ModuleTypeGetValue(key);
  if (obj_ptr == NULL) {
    RedisModule_ReplyWithNull(ctx);
    return REDISMODULE_ERR;
  }
  if (!obj_ptr->writable) {
    return RedisModule_ReplyWithError(ctx, "The file is not writable");
  }

  if (strcasecmp(obj_ptr->value_type, "int8") == 0) {
    long long value;
    for (int i = 2; i < argc; ++i) {
      if (RedisModule_StringToLongLong(argv[i], &value) == REDISMODULE_ERR) {
        return RedisModule_ReplyWithError(ctx, "value must be integer");
      }
      if (value < INT8_MIN || INT8_MAX < value) {
        return RedisModule_ReplyWithError(ctx, "value must be int8");
      }
    }
    size_t new_size = obj_ptr->file_size + obj_ptr->value_size * (argc - 2);
    ftruncate(obj_ptr->fd, new_size);
    if (obj_ptr->mmap != NULL) munmap(obj_ptr->mmap, obj_ptr->file_size);
    obj_ptr->mmap = mmap(NULL, new_size, PROT_READ | PROT_WRITE, MAP_SHARED, obj_ptr->fd, 0);
    for (int i = 2; i < argc; ++i) {
      size_t index = obj_ptr->file_size / obj_ptr->value_size;
      RedisModule_StringToLongLong(argv[i], &value);
      obj_ptr->file_size += obj_ptr->value_size;
      *((int8_t*)obj_ptr->mmap + index) = (int8_t)value;
    }
  }
  else if (strcasecmp(obj_ptr->value_type, "uint8") == 0) {
    long long value;
    for (int i = 2; i < argc; ++i) {
      if (RedisModule_StringToLongLong(argv[i], &value) == REDISMODULE_ERR) {
        return RedisModule_ReplyWithError(ctx, "value must be integer");
      }
      if (value < 0 || UINT8_MAX < value) {
        return RedisModule_ReplyWithError(ctx, "value must be uint8");
      }
    }
    size_t new_size = obj_ptr->file_size + obj_ptr->value_size * (argc - 2);
    ftruncate(obj_ptr->fd, new_size);
    if (obj_ptr->mmap != NULL) munmap(obj_ptr->mmap, obj_ptr->file_size);
    obj_ptr->mmap = mmap(NULL, new_size, PROT_READ | PROT_WRITE, MAP_SHARED, obj_ptr->fd, 0);
    for (int i = 2; i < argc; ++i) {
      size_t index = obj_ptr->file_size / obj_ptr->value_size;
      RedisModule_StringToLongLong(argv[i], &value);
      obj_ptr->file_size += obj_ptr->value_size;
      *((uint8_t*)obj_ptr->mmap + index) = (uint8_t)value;
    }
  }
  else if (strcasecmp(obj_ptr->value_type, "int16") == 0) {
    long long value;
    for (int i = 2; i < argc; ++i) {
      if (RedisModule_StringToLongLong(argv[i], &value) == REDISMODULE_ERR) {
        return RedisModule_ReplyWithError(ctx, "value must be integer");
      }
      if (value < INT16_MIN || INT16_MAX < value) {
        return RedisModule_ReplyWithError(ctx, "value must be int16");
      }
    }
    size_t new_size = obj_ptr->file_size + obj_ptr->value_size * (argc - 2);
    ftruncate(obj_ptr->fd, new_size);
    if (obj_ptr->mmap != NULL) munmap(obj_ptr->mmap, obj_ptr->file_size);
    obj_ptr->mmap = mmap(NULL, new_size, PROT_READ | PROT_WRITE, MAP_SHARED, obj_ptr->fd, 0);
    for (int i = 2; i < argc; ++i) {
      size_t index = obj_ptr->file_size / obj_ptr->value_size;
      RedisModule_StringToLongLong(argv[i], &value);
      obj_ptr->file_size += obj_ptr->value_size;
      *((int16_t*)obj_ptr->mmap + index) = (int16_t)value;
    }
  }
  else if (strcasecmp(obj_ptr->value_type, "uint16") == 0) {
    long long value;
    for (int i = 2; i < argc; ++i) {
      if (RedisModule_StringToLongLong(argv[i], &value) == REDISMODULE_ERR) {
        return RedisModule_ReplyWithError(ctx, "value must be integer");
      }
      if (value < 0 || UINT16_MAX < value) {
        return RedisModule_ReplyWithError(ctx, "value must be uint16");
      }
    }
    size_t new_size = obj_ptr->file_size + obj_ptr->value_size * (argc - 2);
    ftruncate(obj_ptr->fd, new_size);
    if (obj_ptr->mmap != NULL) munmap(obj_ptr->mmap, obj_ptr->file_size);
    obj_ptr->mmap = mmap(NULL, new_size, PROT_READ | PROT_WRITE, MAP_SHARED, obj_ptr->fd, 0);
    for (int i = 2; i < argc; ++i) {
      size_t index = obj_ptr->file_size / obj_ptr->value_size;
      RedisModule_StringToLongLong(argv[i], &value);
      obj_ptr->file_size += obj_ptr->value_size;
      *((uint16_t*)obj_ptr->mmap + index) = (uint16_t)value;
    }
  }
  else if (strcasecmp(obj_ptr->value_type, "int32") == 0) {
    long long value;
    for (int i = 2; i < argc; ++i) {
      if (RedisModule_StringToLongLong(argv[i], &value) == REDISMODULE_ERR) {
        return RedisModule_ReplyWithError(ctx, "value must be integer");
      }
      if (value < INT32_MIN || INT32_MAX < value) {
        return RedisModule_ReplyWithError(ctx, "value must be int32");
      }
    }
    size_t new_size = obj_ptr->file_size + obj_ptr->value_size * (argc - 2);
    ftruncate(obj_ptr->fd, new_size);
    if (obj_ptr->mmap != NULL) munmap(obj_ptr->mmap, obj_ptr->file_size);
    obj_ptr->mmap = mmap(NULL, new_size, PROT_READ | PROT_WRITE, MAP_SHARED, obj_ptr->fd, 0);
    for (int i = 2; i < argc; ++i) {
      size_t index = obj_ptr->file_size / obj_ptr->value_size;
      RedisModule_StringToLongLong(argv[i], &value);
      obj_ptr->file_size += obj_ptr->value_size;
      *((int32_t*)obj_ptr->mmap + index) = (int32_t)value;
    }
  }
  else if (strcasecmp(obj_ptr->value_type, "uint32") == 0) {
    long long value;
    for (int i = 2; i < argc; ++i) {
      if (RedisModule_StringToLongLong(argv[i], &value) == REDISMODULE_ERR) {
        return RedisModule_ReplyWithError(ctx, "value must be integer");
      }
      if (value < 0 || UINT32_MAX < value) {
        return RedisModule_ReplyWithError(ctx, "value must be uint32");
      }
    }
    size_t new_size = obj_ptr->file_size + obj_ptr->value_size * (argc - 2);
    ftruncate(obj_ptr->fd, new_size);
    if (obj_ptr->mmap != NULL) munmap(obj_ptr->mmap, obj_ptr->file_size);
    obj_ptr->mmap = mmap(NULL, new_size, PROT_READ | PROT_WRITE, MAP_SHARED, obj_ptr->fd, 0);
    for (int i = 2; i < argc; ++i) {
      size_t index = obj_ptr->file_size / obj_ptr->value_size;
      RedisModule_StringToLongLong(argv[i], &value);
      obj_ptr->file_size += obj_ptr->value_size;
      *((uint32_t*)obj_ptr->mmap + index) = (uint32_t)value;
    }
  }
  else if (strcasecmp(obj_ptr->value_type, "int64") == 0) {
    long long value;
    for (int i = 2; i < argc; ++i) {
      if (RedisModule_StringToLongLong(argv[i], &value) == REDISMODULE_ERR) {
        return RedisModule_ReplyWithError(ctx, "value must be integer");
      }
      if (value < INT64_MIN || INT64_MAX < value) {
        return RedisModule_ReplyWithError(ctx, "value must be int64");
      }
    }
    size_t new_size = obj_ptr->file_size + obj_ptr->value_size * (argc - 2);
    ftruncate(obj_ptr->fd, new_size);
    if (obj_ptr->mmap != NULL) munmap(obj_ptr->mmap, obj_ptr->file_size);
    obj_ptr->mmap = mmap(NULL, new_size, PROT_READ | PROT_WRITE, MAP_SHARED, obj_ptr->fd, 0);
    for (int i = 2; i < argc; ++i) {
      size_t index = obj_ptr->file_size / obj_ptr->value_size;
      RedisModule_StringToLongLong(argv[i], &value);
      obj_ptr->file_size += obj_ptr->value_size;
      *((int64_t*)obj_ptr->mmap + index) = (int64_t)value;
    }
  }
  else if (strcasecmp(obj_ptr->value_type, "uint64") == 0) {
    long long value;
    for (int i = 2; i < argc; ++i) {
      if (RedisModule_StringToLongLong(argv[i], &value) == REDISMODULE_ERR) {
        return RedisModule_ReplyWithError(ctx, "value must be integer");
      }
      if (value < 0) {
        return RedisModule_ReplyWithError(ctx, "value must be uint64");
      }
    }
    size_t new_size = obj_ptr->file_size + obj_ptr->value_size * (argc - 2);
    ftruncate(obj_ptr->fd, new_size);
    if (obj_ptr->mmap != NULL) munmap(obj_ptr->mmap, obj_ptr->file_size);
    obj_ptr->mmap = mmap(NULL, new_size, PROT_READ | PROT_WRITE, MAP_SHARED, obj_ptr->fd, 0);
    for (int i = 2; i < argc; ++i) {
      size_t index = obj_ptr->file_size / obj_ptr->value_size;
      RedisModule_StringToLongLong(argv[i], &value);
      obj_ptr->file_size += obj_ptr->value_size;
      *((uint64_t*)obj_ptr->mmap + index) = (uint64_t)value;
    }
  }
  else if (strcasecmp(obj_ptr->value_type, "float") == 0) {
    double value;
    for (int i = 2; i < argc; ++i) {
      if (RedisModule_StringToDouble(argv[i], &value) == REDISMODULE_ERR) {
        return RedisModule_ReplyWithError(ctx, "value must be float");
      }
      fprintf(stderr, "%d:%f %f %f\n", __LINE__, value, -FLT_MAX, FLT_MAX);
      if (value < -FLT_MAX || FLT_MAX < value) {
        return RedisModule_ReplyWithError(ctx, "value must be float");
      }
    }
    size_t new_size = obj_ptr->file_size + obj_ptr->value_size * (argc - 2);
    ftruncate(obj_ptr->fd, new_size);
    if (obj_ptr->mmap != NULL) munmap(obj_ptr->mmap, obj_ptr->file_size);
    obj_ptr->mmap = mmap(NULL, new_size, PROT_READ | PROT_WRITE, MAP_SHARED, obj_ptr->fd, 0);
    for (int i = 2; i < argc; ++i) {
      size_t index = obj_ptr->file_size / obj_ptr->value_size;
      RedisModule_StringToDouble(argv[i], &value);
      obj_ptr->file_size += obj_ptr->value_size;
      *((float*)obj_ptr->mmap + index) = (float)value;
    }
  }
  else if (strcasecmp(obj_ptr->value_type, "double") == 0) {
    long double value;
    for (int i = 2; i < argc; ++i) {
      if (RedisModule_StringToLongDouble(argv[i], &value) == REDISMODULE_ERR) {
        return RedisModule_ReplyWithError(ctx, "value must be double");
      }
      if (value < -DBL_MAX || DBL_MAX < value) {
        return RedisModule_ReplyWithError(ctx, "value must be double");
      }
    }
    size_t new_size = obj_ptr->file_size + obj_ptr->value_size * (argc - 2);
    ftruncate(obj_ptr->fd, new_size);
    if (obj_ptr->mmap != NULL) munmap(obj_ptr->mmap, obj_ptr->file_size);
    obj_ptr->mmap = mmap(NULL, new_size, PROT_READ | PROT_WRITE, MAP_SHARED, obj_ptr->fd, 0);
    for (int i = 2; i < argc; ++i) {
      size_t index = obj_ptr->file_size / obj_ptr->value_size;
      RedisModule_StringToLongDouble(argv[i], &value);
      obj_ptr->file_size += obj_ptr->value_size;
      *((double*)obj_ptr->mmap + index) = (double)value;
    }
  }
  else if (strcasecmp(obj_ptr->value_type, "long_double") == 0) {
    long double value;
    for (int i = 2; i < argc; ++i) {
      if (RedisModule_StringToLongDouble(argv[i], &value) == REDISMODULE_ERR) {
        return RedisModule_ReplyWithError(ctx, "value must be long double");
      }
    }
    size_t new_size = obj_ptr->file_size + obj_ptr->value_size * (argc - 2);
    ftruncate(obj_ptr->fd, new_size);
    if (obj_ptr->mmap != NULL) munmap(obj_ptr->mmap, obj_ptr->file_size);
    obj_ptr->mmap = mmap(NULL, new_size, PROT_READ | PROT_WRITE, MAP_SHARED, obj_ptr->fd, 0);
    for (int i = 2; i < argc; ++i) {
      size_t index = obj_ptr->file_size / obj_ptr->value_size;
      RedisModule_StringToLongDouble(argv[i], &value);
      obj_ptr->file_size += obj_ptr->value_size;
      *((long double*)obj_ptr->mmap + index) = (long double)value;
    }
  }
  else if (strcasecmp(obj_ptr->value_type, "string") == 0) {
    size_t value_size;
    for (int i = 2; i < argc; ++i) {
      RedisModule_StringPtrLen(argv[i], &value_size);
      if (obj_ptr->value_size < value_size) {
        return RedisModule_ReplyWithError(ctx, "value is too long");
      }
    }
    size_t new_size = obj_ptr->file_size + obj_ptr->value_size * (argc - 2);
    ftruncate(obj_ptr->fd, new_size);
    if (obj_ptr->mmap != NULL) munmap(obj_ptr->mmap, obj_ptr->file_size);
    obj_ptr->mmap = mmap(NULL, new_size, PROT_READ | PROT_WRITE, MAP_SHARED, obj_ptr->fd, 0);
    for (int i = 2; i < argc; ++i) {
      size_t index = obj_ptr->file_size / obj_ptr->value_size;
      char *value = zcalloc(obj_ptr->value_size + 1);
      memcpy(value, RedisModule_StringPtrLen(argv[i], &value_size), value_size);
      memcpy((char*)obj_ptr->mmap + index * obj_ptr->value_size, value, obj_ptr->value_size);
      obj_ptr->file_size += obj_ptr->value_size;
      zfree(value);
    }
  }
  msync(obj_ptr->mmap, obj_ptr->file_size, MS_ASYNC);
  return RedisModule_ReplyWithLongLong(ctx, argc - 2);
}

// VCOUNT key
int VCount_RedisCommand(RedisModuleCtx *ctx, RedisModuleString **argv, int argc)
{
  RedisModule_AutoMemory(ctx); /* Use automatic memory management. */
  if (argc != 2) return RedisModule_WrongArity(ctx);

  RedisModuleKey *key =
      RedisModule_OpenKey(ctx, argv[1], REDISMODULE_READ | REDISMODULE_WRITE);
  int type = RedisModule_KeyType(key);
  if (type != REDISMODULE_KEYTYPE_EMPTY &&
      RedisModule_ModuleTypeGetType(key) != MMapType) {
    return RedisModule_ReplyWithError(ctx, REDISMODULE_ERRORMSG_WRONGTYPE);
  }

  if (type == REDISMODULE_KEYTYPE_EMPTY) {
    return RedisModule_ReplyWithError(ctx, "You must do MMAP first");
  }

  MMapObject *obj_ptr = RedisModule_ModuleTypeGetValue(key);
  if (obj_ptr == NULL) {
    return RedisModule_ReplyWithNull(ctx);
  }

  return RedisModule_ReplyWithLongLong(ctx, obj_ptr->file_size / obj_ptr->value_size);
}

// VCLEAR key
int VClear_RedisCommand(RedisModuleCtx *ctx, RedisModuleString **argv, int argc)
{
  RedisModule_AutoMemory(ctx); /* Use automatic memory management. */
  if (argc != 2) return RedisModule_WrongArity(ctx);

  RedisModuleKey *key =
      RedisModule_OpenKey(ctx, argv[1], REDISMODULE_READ | REDISMODULE_WRITE);
  int type = RedisModule_KeyType(key);
  if (type != REDISMODULE_KEYTYPE_EMPTY &&
      RedisModule_ModuleTypeGetType(key) != MMapType) {
    return RedisModule_ReplyWithError(ctx, REDISMODULE_ERRORMSG_WRONGTYPE);
  }

  if (type == REDISMODULE_KEYTYPE_EMPTY) {
    return RedisModule_ReplyWithError(ctx, "You must do MMAP first");
  }

  MMapObject *obj_ptr = RedisModule_ModuleTypeGetValue(key);
  if (obj_ptr == NULL) {
    return RedisModule_ReplyWithNull(ctx);
  }

  if (!obj_ptr->writable) {
    return RedisModule_ReplyWithError(ctx, "The file is not writable");
  }

  if (obj_ptr->mmap != NULL) munmap(obj_ptr->mmap, obj_ptr->file_size);
  ftruncate(obj_ptr->fd, 0);
  obj_ptr->mmap = NULL;
  RedisModule_ReplyWithLongLong(ctx, obj_ptr->file_size / obj_ptr->value_size);
  obj_ptr->file_size = 0;
  return REDISMODULE_OK;
}

// VFILEPATH key
int VFilePath_RedisCommand(RedisModuleCtx *ctx, RedisModuleString **argv, int argc)
{
  RedisModule_AutoMemory(ctx); /* Use automatic memory management. */
  if (argc != 2) return RedisModule_WrongArity(ctx);

  RedisModuleKey *key =
      RedisModule_OpenKey(ctx, argv[1], REDISMODULE_READ | REDISMODULE_WRITE);
  int type = RedisModule_KeyType(key);
  if (type != REDISMODULE_KEYTYPE_EMPTY &&
      RedisModule_ModuleTypeGetType(key) != MMapType) {
    return RedisModule_ReplyWithError(ctx, REDISMODULE_ERRORMSG_WRONGTYPE);
  }

  if (type == REDISMODULE_KEYTYPE_EMPTY) {
    return RedisModule_ReplyWithError(ctx, "You must do MMAP first");
  }

  MMapObject *obj_ptr = RedisModule_ModuleTypeGetValue(key);
  if (obj_ptr == NULL) {
    return RedisModule_ReplyWithNull(ctx);
  }

  return RedisModule_ReplyWithCString(ctx, obj_ptr->file_path);
}

// VTYPE key
int VType_RedisCommand(RedisModuleCtx *ctx, RedisModuleString **argv, int argc)
{
  RedisModule_AutoMemory(ctx); /* Use automatic memory management. */
  if (argc != 2) return RedisModule_WrongArity(ctx);

  RedisModuleKey *key =
      RedisModule_OpenKey(ctx, argv[1], REDISMODULE_READ | REDISMODULE_WRITE);
  int type = RedisModule_KeyType(key);
  if (type != REDISMODULE_KEYTYPE_EMPTY &&
      RedisModule_ModuleTypeGetType(key) != MMapType) {
    return RedisModule_ReplyWithError(ctx, REDISMODULE_ERRORMSG_WRONGTYPE);
  }

  if (type == REDISMODULE_KEYTYPE_EMPTY) {
    return RedisModule_ReplyWithError(ctx, "You must do MMAP first");
  }

  MMapObject *obj_ptr = RedisModule_ModuleTypeGetValue(key);
  if (obj_ptr == NULL) {
    return RedisModule_ReplyWithNull(ctx);
  }

  return RedisModule_ReplyWithCString(ctx, obj_ptr->value_type);
}

// VSIZE key
int VSize_RedisCommand(RedisModuleCtx *ctx, RedisModuleString **argv, int argc)
{
  RedisModule_AutoMemory(ctx); /* Use automatic memory management. */
  if (argc != 2) return RedisModule_WrongArity(ctx);

  RedisModuleKey *key =
      RedisModule_OpenKey(ctx, argv[1], REDISMODULE_READ | REDISMODULE_WRITE);
  int type = RedisModule_KeyType(key);
  if (type != REDISMODULE_KEYTYPE_EMPTY &&
      RedisModule_ModuleTypeGetType(key) != MMapType) {
    return RedisModule_ReplyWithError(ctx, REDISMODULE_ERRORMSG_WRONGTYPE);
  }

  if (type == REDISMODULE_KEYTYPE_EMPTY) {
    return RedisModule_ReplyWithError(ctx, "You must do MMAP first");
  }

  MMapObject *obj_ptr = RedisModule_ModuleTypeGetValue(key);
  if (obj_ptr == NULL) {
    return RedisModule_ReplyWithNull(ctx);
  }

  return RedisModule_ReplyWithLongLong(ctx, obj_ptr->value_size);
}

// VPOP key
int VPop_RedisCommand(RedisModuleCtx *ctx, RedisModuleString **argv, int argc)
{
  RedisModule_AutoMemory(ctx); /* Use automatic memory management. */
  if (argc != 2) return RedisModule_WrongArity(ctx);

  RedisModuleKey *key =
      RedisModule_OpenKey(ctx, argv[1], REDISMODULE_READ | REDISMODULE_WRITE);
  int type = RedisModule_KeyType(key);
  if (type != REDISMODULE_KEYTYPE_EMPTY &&
      RedisModule_ModuleTypeGetType(key) != MMapType) {
    return RedisModule_ReplyWithError(ctx, REDISMODULE_ERRORMSG_WRONGTYPE);
  }

  if (type == REDISMODULE_KEYTYPE_EMPTY) {
    return RedisModule_ReplyWithError(ctx, "You must do MMAP first");
  }

  MMapObject *obj_ptr = RedisModule_ModuleTypeGetValue(key);
  if (obj_ptr == NULL) {
    return RedisModule_ReplyWithNull(ctx);
  }

  if (!obj_ptr->writable) {
    return RedisModule_ReplyWithError(ctx, "The file is not writable");
  }

  if (obj_ptr->file_size == 0) {
    RedisModule_ReplyWithNull(ctx);
  }
  else {
    size_t index = obj_ptr->file_size / obj_ptr->value_size - 1;
    if (strcasecmp(obj_ptr->value_type, "int8") == 0) {
      RedisModule_ReplyWithLongLong(ctx, ((int8_t*)obj_ptr->mmap)[index]);
    }
    else if (strcasecmp(obj_ptr->value_type, "uint8") == 0) {
      RedisModule_ReplyWithLongLong(ctx, ((uint8_t*)obj_ptr->mmap)[index]);
    }
    else if (strcasecmp(obj_ptr->value_type, "int16") == 0) {
      RedisModule_ReplyWithLongLong(ctx, ((int16_t*)obj_ptr->mmap)[index]);
    }
    else if (strcasecmp(obj_ptr->value_type, "uint16") == 0) {
      RedisModule_ReplyWithLongLong(ctx, ((uint16_t*)obj_ptr->mmap)[index]);
    }
    else if (strcasecmp(obj_ptr->value_type, "int32") == 0) {
      RedisModule_ReplyWithLongLong(ctx, ((int32_t*)obj_ptr->mmap)[index]);
    }
    else if (strcasecmp(obj_ptr->value_type, "uint32") == 0) {
      RedisModule_ReplyWithLongLong(ctx, ((uint32_t*)obj_ptr->mmap)[index]);
    }
    else if (strcasecmp(obj_ptr->value_type, "int64") == 0) {
      RedisModule_ReplyWithLongLong(ctx, ((int64_t*)obj_ptr->mmap)[index]);
    }
    else if (strcasecmp(obj_ptr->value_type, "uint64") == 0) {
      RedisModule_ReplyWithLongLong(ctx, ((uint64_t*)obj_ptr->mmap)[index]);
    }
    else if (strcasecmp(obj_ptr->value_type, "float") == 0) {
      RedisModule_ReplyWithDouble(ctx, ((float*)obj_ptr->mmap)[index]);
    }
    else if (strcasecmp(obj_ptr->value_type, "double") == 0) {
      RedisModule_ReplyWithDouble(ctx, ((double*)obj_ptr->mmap)[index]);
    }
    else if (strcasecmp(obj_ptr->value_type, "long_double") == 0) {
      RedisModule_ReplyWithLongDouble(ctx, ((long double*)obj_ptr->mmap)[index]);
    }
    else if (strcasecmp(obj_ptr->value_type, "string") == 0) {
      char *buffer = zcalloc(obj_ptr->value_size +1);
      snprintf(buffer, obj_ptr->value_size + 1, "%s", (char *)obj_ptr->mmap + index * obj_ptr->value_size);
      RedisModule_ReplyWithStringBuffer(ctx, buffer, strlen(buffer));
      zfree(buffer);
    }
    else return REDISMODULE_ERR;
    if (obj_ptr->mmap != NULL) munmap(obj_ptr->mmap, obj_ptr->file_size);
    obj_ptr->file_size -= obj_ptr->value_size;
    ftruncate(obj_ptr->fd, obj_ptr->file_size);
    if (0 < obj_ptr->file_size) {
      obj_ptr->mmap = mmap(NULL, obj_ptr->file_size, PROT_READ | PROT_WRITE, MAP_SHARED, obj_ptr->fd, 0);
    }
    else obj_ptr->mmap = NULL;
  }
  return REDISMODULE_OK;
}

void *MRdbLoad(RedisModuleIO *rdb, int encver)
{
  // if (encver != 0) {
  //   RedisModule_Log(rdb->ctx, "warning", "Can't load data with version %d",
  //                   encver);
  //   return NULL;
  // }
  MMapObject *obj_ptr = MCreateObject();
  obj_ptr->file_path = sdsnew(RedisModule_StringPtrLen(RedisModule_LoadString(rdb), NULL));
  obj_ptr->value_type = sdsnew(RedisModule_StringPtrLen(RedisModule_LoadString(rdb), NULL));
  uint64_t value_size = RedisModule_LoadUnsigned(rdb);
  obj_ptr->value_size = (uint8_t)value_size;
  uint64_t writable = RedisModule_LoadUnsigned(rdb);
  obj_ptr->writable = (writable == 0 ? false : true);
  if (obj_ptr->writable) {
    obj_ptr->fd = open(obj_ptr->file_path, O_CREAT | O_RDWR, 0666);
  }
  else {
    obj_ptr->fd = open(obj_ptr->file_path, O_RDONLY);
  }
  if (obj_ptr->fd == -1) {
    MFree(obj_ptr);
    return NULL;
  }
  struct stat sb;
  if (fstat(obj_ptr->fd, &sb) == -1) {
    MFree(obj_ptr);
    return NULL;
  }
  obj_ptr->file_size = sb.st_size;
  if (0 < obj_ptr->file_size) {
    if (obj_ptr->writable) {
      obj_ptr->mmap = mmap(NULL, sb.st_size, PROT_READ | PROT_WRITE, MAP_SHARED, obj_ptr->fd, 0);
    }
    else {
      obj_ptr->mmap = mmap(NULL, sb.st_size, PROT_READ, MAP_SHARED, obj_ptr->fd, 0);
    }
    if (obj_ptr->mmap == MAP_FAILED) {
      MFree(obj_ptr);
      return NULL;
    }
  }
  else obj_ptr->mmap = NULL;
  return obj_ptr;
}

void MRdbSave(RedisModuleIO *rdb, void *value)
{
  MMapObject *obj_ptr = value;
  RedisModule_SaveStringBuffer(rdb, obj_ptr->file_path, sdslen(obj_ptr->file_path));
  RedisModule_SaveStringBuffer(rdb, obj_ptr->value_type, sdslen(obj_ptr->value_type));
  RedisModule_SaveUnsigned(rdb, obj_ptr->value_size);
  RedisModule_SaveUnsigned(rdb, obj_ptr->writable ? 1 : 0);
  msync(obj_ptr->mmap, obj_ptr->file_size, MS_ASYNC);
}

void MAofRewrite(RedisModuleIO *aof, RedisModuleString *key, void *value)
{
  char buffer[0x200];
  MMapObject *obj_ptr = (MMapObject*)value;
  RedisModule_EmitAOF(aof, "MMAP", "scclc",
                      key,
                      obj_ptr->file_path,
                      obj_ptr->value_type,
                      obj_ptr->value_size,
                      "writable");
  RedisModule_EmitAOF(aof, "MCLEAR", "ss", key, obj_ptr->file_path);
  if (strcasecmp(obj_ptr->value_type, "int8") == 0) {
    for (size_t i = 0; i < obj_ptr->file_size; i += sizeof(int8_t)) {
      int8_t value = ((int8_t*)obj_ptr->mmap)[i];
      RedisModule_EmitAOF(aof, "MADD", "sbl", key, value);
    }
  }
  else if (strcasecmp(obj_ptr->value_type, "uint8") == 0) {
    for (size_t i = 0; i < obj_ptr->file_size; i += sizeof(uint8_t)) {
      uint8_t value = ((uint8_t*)obj_ptr->mmap)[i];
      RedisModule_EmitAOF(aof, "MADD", "sbl", key, value);
    }
  }
  else if (strcasecmp(obj_ptr->value_type, "int16") == 0) {
    for (size_t i = 0; i < obj_ptr->file_size; i += sizeof(int16_t)) {
      int16_t value = *(int16_t *)((uint8_t*)obj_ptr->mmap + i);
      RedisModule_EmitAOF(aof, "MADD", "sbl", key, value);
    }
  }
  else if (strcasecmp(obj_ptr->value_type, "uint16") == 0) {
    for (size_t i = 0; i < obj_ptr->file_size; i += sizeof(uint16_t)) {
      uint16_t value = *(uint16_t *)((uint8_t*)obj_ptr->mmap + i);
      RedisModule_EmitAOF(aof, "MADD", "sbl", key, value);
    }
  }
  else if (strcasecmp(obj_ptr->value_type, "int32") == 0) {
    for (size_t i = 0; i < obj_ptr->file_size; i += sizeof(int32_t)) {
      int32_t value = *(int32_t *)((uint8_t*)obj_ptr->mmap + i);
      RedisModule_EmitAOF(aof, "MADD", "sbl", key, value);
    }
  }
  else if (strcasecmp(obj_ptr->value_type, "uint32") == 0) {
    for (size_t i = 0; i < obj_ptr->file_size; i += sizeof(uint32_t)) {
      uint32_t value = *(uint32_t *)((uint8_t*)obj_ptr->mmap + i);
      RedisModule_EmitAOF(aof, "MADD", "sbl", key, value);
    }
  }
  else if (strcasecmp(obj_ptr->value_type, "int64") == 0) {
    for (size_t i = 0; i < obj_ptr->file_size; i += sizeof(int64_t)) {
      int64_t value = *(int64_t *)((uint8_t*)obj_ptr->mmap + i);
      RedisModule_EmitAOF(aof, "MADD", "sbl", key, value);
    }
  }
  else if (strcasecmp(obj_ptr->value_type, "uint64") == 0) {
    for (size_t i = 0; i < obj_ptr->file_size; i += sizeof(uint64_t)) {
      uint64_t value = *(uint64_t *)((uint8_t*)obj_ptr->mmap + i);
      RedisModule_EmitAOF(aof, "MADD", "sbl", key, value);
    }
  }
  else if (strcasecmp(obj_ptr->value_type, "float") == 0) {
    for (size_t i = 0; i < obj_ptr->file_size; i += sizeof(float)) {
      float value = *(float *)((uint8_t*)obj_ptr->mmap + i);
      sprintf(buffer, "%.16f", value);
      RedisModule_EmitAOF(aof, "MADD", "sbc", key, buffer);
    }
  }
  else if (strcasecmp(obj_ptr->value_type, "double") == 0) {
    for (size_t i = 0; i < obj_ptr->file_size; i += sizeof(double)) {
      double value = *(double *)((uint8_t*)obj_ptr->mmap + i);
      sprintf(buffer, "%.16f", value);
      RedisModule_EmitAOF(aof, "MADD", "sbc", key, buffer);
    }
  }
  else if (strcasecmp(obj_ptr->value_type, "long_double") == 0) {
    for (size_t i = 0; i < obj_ptr->file_size; i += sizeof(long double)) {
      long double value = *(long double *)((uint8_t*)obj_ptr->mmap + i);
      sprintf(buffer, "%.16Lf", value);
      RedisModule_EmitAOF(aof, "MADD", "sbc", key, buffer);
    }
  }
  else if (strcasecmp(obj_ptr->value_type, "string") == 0) {
    char *buffer = zmalloc(obj_ptr->value_size + 1);
    for (size_t i = 0; i < obj_ptr->file_size; i += obj_ptr->value_size) {
      memset(buffer, 0, obj_ptr->value_size + 1);
      snprintf(buffer, obj_ptr->value_size + 1, "%s", (char*)obj_ptr->mmap + i);
      RedisModule_EmitAOF(aof, "MADD", "sbc", key, buffer);
    }
    zfree(buffer);
  }

  if (!obj_ptr->writable) {
    RedisModule_EmitAOF(aof, "DEL", "s", key);
    RedisModule_EmitAOF(aof, "MMAP", "sss", key, obj_ptr->file_path, obj_ptr->value_type);
  }

}

size_t MMemUsage(const void *value)
{
  const MMapObject *obj_ptr = value;
  return obj_ptr->file_size;
}


void MDigest(RedisModuleDigest *md, void *value)
{
  REDISMODULE_NOT_USED(md);
  REDISMODULE_NOT_USED(value);
}

#define CREATE_CMD(name, tgt, attr, key_pos, key_last)                     \
  do {                                                                     \
    if (RedisModule_CreateCommand(ctx, name, tgt, attr, key_pos, key_last, \
                                  1) != REDISMODULE_OK) {                  \
      return REDISMODULE_ERR;                                              \
    }                                                                      \
  } while (0);

/* This function must be present on each Redis module. It is used in order
 * to register the commands into the Redis server. */
int RedisModule_OnLoad(RedisModuleCtx *ctx, RedisModuleString **argv, int argc)
{
  REDISMODULE_NOT_USED(argv);
  REDISMODULE_NOT_USED(argc);

  if (RedisModule_Init(ctx, "FuchiMMap", 1, REDISMODULE_APIVER_1) ==
      REDISMODULE_ERR)
    return REDISMODULE_ERR;

  RedisModuleTypeMethods tm = {.version = REDISMODULE_TYPE_METHOD_VERSION,
                               .rdb_load = MRdbLoad,
                               .rdb_save = MRdbSave,
                               .aof_rewrite = MAofRewrite,
                               .mem_usage = MMemUsage,
                               .free = MFree,
                               .digest = MDigest};

  MMapType = RedisModule_CreateDataType(ctx, "FuchiMMap", 0, &tm);
  if (MMapType == NULL) return REDISMODULE_ERR;

  // VMAP key file_path value_type [value_size] [writable]
  CREATE_CMD("MMAP", MMap_RedisCommand, "write fast", 1, 1);

  // VCLEAR key
  CREATE_CMD("VCLEAR", VClear_RedisCommand, "write fast", 1, 1);

  // VADD key value [value ...]
  CREATE_CMD("VADD", VAdd_RedisCommand, "write fast", 1, 1);

  // VGET key index
  CREATE_CMD("VGET", VGet_RedisCommand, "readonly fast", 1, 1);

  // VMGET key index [index ...]
  CREATE_CMD("VMGET", VMGet_RedisCommand, "readonly fast", 1, 1);

  // VALL key
  CREATE_CMD("VALL", VAll_RedisCommand, "readonly fast", 1, 1);

  // VSET key index value [index value ...]
  CREATE_CMD("VSET", VSet_RedisCommand, "write fast", 1, 1);

  // VCOUNT key
  CREATE_CMD("VCOUNT", VCount_RedisCommand, "readonly fast", 1, 1);

  // VPOP key
  CREATE_CMD("VPOP", VPop_RedisCommand, "write fast", 1, 1);

  // VFILEPATH key
  CREATE_CMD("VFILEPATH", VFilePath_RedisCommand, "readonly fast", 1, 1);

  // VTYPE key
  CREATE_CMD("VTYPE", VType_RedisCommand, "readonly fast", 1, 1);

  // VSIZE key
  CREATE_CMD("VSIZE", VSize_RedisCommand, "readonly fast", 1, 1);

  return REDISMODULE_OK;
}

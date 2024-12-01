<a href="https://colab.research.google.com/github/t-fuchi/RedisMMap/blob/main/RedisMMap.ipynb" target="_parent"><img src="https://colab.research.google.com/assets/colab-badge.svg" alt="Open In Colab"/></a>

## I made a module with Google Colab to read values from mmapped files in Redis.

I can't find the ability to read existing files in Redis. (Maybe I just don't know it, so if you do, please let me know.) So I made a module that maps a file with double values written to it with mmap and reads/writes the values at specified locations from it. I've included the full code so that you can try it out by simply opening Colab from the button in the upper left corner and executing the cells in order. Please also refer to this as a way to create a Redis module.






Using this module, the following commands can be used in Redis.

```
// This command binds key to the file at file_path.
MMAP key file_path

// This command gets a value at index position from key
VGET key index

// This command gets values at multiple index positions from key
VMGET key index [index ...]

// This command writes values to the index positions of key
VSET key index value [index value ...]

// This command adds a value to key
VADD key value [value ...]

// This command retrieves and deletes the last value of key
VPOP key

// This command gets the number of values of key
VCOUNT key

// This command erases the contents of key
VCLEAR key
```


First, obtain the Redis source and change the current directory to module's one.


```python
%cd /content
!wget https://download.redis.io/redis-stable.tar.gz
!tar -xzf redis-stable.tar.gz
%cd /content/redis-stable/src/modules
```

    /content
    --2023-06-22 06:07:59--  https://download.redis.io/redis-stable.tar.gz
    Resolving download.redis.io (download.redis.io)... 45.60.121.1
    Connecting to download.redis.io (download.redis.io)|45.60.121.1|:443... connected.
    HTTP request sent, awaiting response... 200 OK
    Length: 3068843 (2.9M) [application/octet-stream]
    Saving to: ‘redis-stable.tar.gz’
    
    redis-stable.tar.gz 100%[===================>]   2.93M  --.-KB/s    in 0.06s   
    
    2023-06-22 06:07:59 (47.1 MB/s) - ‘redis-stable.tar.gz’ saved [3068843/3068843]
    
    /content/redis-stable/src/modules


Let's build the sample code.


```python
!make
```

    cc -I.  -W -Wall -fno-common -g -ggdb -std=c99 -O2 -fPIC -c helloworld.c -o helloworld.xo
    ld -o helloworld.so helloworld.xo -shared  -lc
    cc -I.  -W -Wall -fno-common -g -ggdb -std=c99 -O2 -fPIC -c hellotype.c -o hellotype.xo
    ld -o hellotype.so hellotype.xo -shared  -lc
    cc -I.  -W -Wall -fno-common -g -ggdb -std=c99 -O2 -fPIC -c helloblock.c -o helloblock.xo
    ld -o helloblock.so helloblock.xo -shared  -lpthread -lc
    cc -I.  -W -Wall -fno-common -g -ggdb -std=c99 -O2 -fPIC -c hellocluster.c -o hellocluster.xo
    ld -o hellocluster.so hellocluster.xo -shared  -lc
    cc -I.  -W -Wall -fno-common -g -ggdb -std=c99 -O2 -fPIC -c hellotimer.c -o hellotimer.xo
    ld -o hellotimer.so hellotimer.xo -shared  -lc
    cc -I.  -W -Wall -fno-common -g -ggdb -std=c99 -O2 -fPIC -c hellodict.c -o hellodict.xo
    ld -o hellodict.so hellodict.xo -shared  -lc
    cc -I.  -W -Wall -fno-common -g -ggdb -std=c99 -O2 -fPIC -c hellohook.c -o hellohook.xo
    ld -o hellohook.so hellohook.xo -shared  -lc
    cc -I.  -W -Wall -fno-common -g -ggdb -std=c99 -O2 -fPIC -c helloacl.c -o helloacl.xo
    ld -o helloacl.so helloacl.xo -shared  -lc


Check if the module .so file has been created.


```python
!ls *.so
```

    helloacl.so    hellocluster.so	hellohook.so   hellotype.so
    helloblock.so  hellodict.so	hellotimer.so  helloworld.so


Now let's write the code in fmmap.c. The value to be read/written is a double. Error handling has been omitted to make the code easier to read. If you try it, please do not submit any nasty commands.😏

In addition, the code available [here](https://github.com/t-fuchi/RedisMMap) allows you to choose the type of value to read or write, and is complete with error handling.



First, necessary headers and a convenience macro.


```python
%%writefile fmmap.c

#pragma GCC diagnostic ignored "-Wunused-parameter"

#include <sys/mman.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <fcntl.h>
#include <unistd.h>
#include <stdbool.h>
#include <stdint.h>
#include <strings.h>
#include <string.h>

#include "../redismodule.h"
#include "../sds.h"
#include "../zmalloc.h"

// A macro compare (RedisModuleString *) and (char *)
static inline int mstringcmp(const RedisModuleString *rs1, const char *s2)
{
  return strcasecmp(RedisModule_StringPtrLen(rs1, NULL), s2);
}

int ftruncate(int fildes, off_t length); // It should be in unistd.h, but I get a warning.
```

    Writing fmmap.c


Define MMapObject, packed with the information needed for mmap. sds is a string type used within Redis.


```python
%%writefile -a fmmap.c

typedef struct _MMapObject
{
  sds file_path;
  int fd;
  void *mmap;
  size_t file_size;
} MMapObject;
```

    Appending to fmmap.c


A variable to hold the MMap type and a function to generate the MMapObject.


```python
%%writefile -a fmmap.c

RedisModuleType *MMapType = NULL;

MMapObject *MCreateObject(void)
{
  return (MMapObject *)zcalloc(sizeof(MMapObject));
}
```

    Appending to fmmap.c


Function to release MMapObject.


```python
%%writefile -a fmmap.c

void MFree(void *value)
{
  if (value == NULL) return;
  const MMapObject *obj_ptr = value;
  if (obj_ptr->mmap != NULL) munmap(obj_ptr->mmap, obj_ptr->file_size);
  if (obj_ptr->fd != -1) close(obj_ptr->fd);
  sdsfree(obj_ptr->file_path);
  zfree(value);
}
```

    Appending to fmmap.c


Function to map the file specified by file_path to key.


```python
%%writefile -a fmmap.c

// MMAP key file_path
int MMap_RedisCommand(RedisModuleCtx *ctx, RedisModuleString **argv, int argc)
{
  RedisModule_AutoMemory(ctx); /* Use automatic memory management. */

  if (argc != 3) return RedisModule_WrongArity(ctx);

  RedisModuleKey *key = RedisModule_OpenKey(ctx, argv[1], REDISMODULE_READ | REDISMODULE_WRITE);

  // It confirms the type of key
  int type = RedisModule_KeyType(key);
  if (type != REDISMODULE_KEYTYPE_EMPTY &&
      RedisModule_ModuleTypeGetType(key) != MMapType) {
    return RedisModule_ReplyWithError(ctx, REDISMODULE_ERRORMSG_WRONGTYPE);
  }

  // It creates new file and mmap it if key is empty.
  MMapObject *obj_ptr;
  if (type == REDISMODULE_KEYTYPE_EMPTY) {
    obj_ptr = MCreateObject();
    obj_ptr->file_path = sdsnew(RedisModule_StringPtrLen(argv[2], NULL));
    obj_ptr->fd = open(obj_ptr->file_path, O_RDWR | O_CREAT, 0666);
    if (obj_ptr->fd == -1) {
        MFree(obj_ptr);
        return RedisModule_ReplyWithError(ctx, sdsnew(RedisModule_StringPtrLen(argv[2], NULL)));
    }

    struct stat sb;
    fstat(obj_ptr->fd, &sb);
    obj_ptr->file_size = sb.st_size;
    obj_ptr->mmap = mmap(NULL, sb.st_size, PROT_READ | PROT_WRITE, MAP_SHARED, obj_ptr->fd, 0);
    RedisModule_ModuleTypeSetValue(key, MMapType, obj_ptr);
  }
  // It confirms the file_path if key already exists.
  else {
    obj_ptr = RedisModule_ModuleTypeGetValue(key);
    if (obj_ptr == NULL) {
      RedisModule_ReplyWithNull(ctx);
      return REDISMODULE_ERR;
    }
    // Error if it is different from the existing file_path.
    if (strcmp(obj_ptr->file_path, RedisModule_StringPtrLen(argv[2], NULL)) != 0) {
      return RedisModule_ReplyWithError(ctx, "It is already mapped on another file.");
    }
  }

  return RedisModule_ReplyWithLongLong(ctx, obj_ptr->file_size / sizeof(double));
}

```

    Appending to fmmap.c


Function to get the value at the position specified by index.


```python
%%writefile -a fmmap.c

// VGET key index
int VGet_RedisCommand(RedisModuleCtx *ctx, RedisModuleString **argv, int argc)
{
  RedisModule_AutoMemory(ctx); /* Use automatic memory management. */

  RedisModuleKey *key =
    RedisModule_OpenKey(ctx, argv[1], REDISMODULE_READ | REDISMODULE_WRITE);

  long long index;
  RedisModule_StringToLongLong(argv[2], &index);

  MMapObject *obj_ptr = RedisModule_ModuleTypeGetValue(key);

  // It returns Null if index is out of range.
  if (obj_ptr->file_size <= (size_t)index * sizeof(double) || index < 0) {
    RedisModule_ReplyWithNull(ctx);
  }
  // It returns mmap[index].
  else {
    RedisModule_ReplyWithDouble(ctx, ((double*)obj_ptr->mmap)[index]);
  }
  return REDISMODULE_OK;
}
```

    Appending to fmmap.c


This function gets the values at the positions indicated by multiple indices.


```python
%%writefile -a fmmap.c

// VMGET key index [index ...]
int VMGet_RedisCommand(RedisModuleCtx *ctx, RedisModuleString **argv, int argc)
{
  RedisModule_AutoMemory(ctx); /* Use automatic memory management. */

  RedisModuleKey *key =
      RedisModule_OpenKey(ctx, argv[1], REDISMODULE_READ | REDISMODULE_WRITE);

  MMapObject *obj_ptr = RedisModule_ModuleTypeGetValue(key);

  RedisModule_ReplyWithArray(ctx, argc - 2);
  for (int i = 2; i < argc; i++) {
    long long index;
    RedisModule_StringToLongLong(argv[i], &index);
    // It returns Null if index is out of range.
    if (obj_ptr->file_size <= (size_t)index * sizeof(double) || index < 0) {
      RedisModule_ReplyWithNull(ctx);
    }
    // It returns mmap[index].
    else {
      RedisModule_ReplyWithDouble(ctx, ((double*)obj_ptr->mmap)[index]);
    }
  }
  return REDISMODULE_OK;
}
```

    Appending to fmmap.c


This function writes values at the positions indicated by indeces.


```python
%%writefile -a fmmap.c

// VSET key index value [index value ...]
int VSet_RedisCommand(RedisModuleCtx *ctx, RedisModuleString **argv, int argc)
{
  RedisModule_AutoMemory(ctx); /* Use automatic memory management. */

  RedisModuleKey *key =
      RedisModule_OpenKey(ctx, argv[1], REDISMODULE_READ | REDISMODULE_WRITE);

  MMapObject *obj_ptr = RedisModule_ModuleTypeGetValue(key);

  long long index;
  double value;
  // It confirms the type of value written.
  for (int i = 3; i < argc; i += 2) {
    if (RedisModule_StringToDouble(argv[i], &value) == REDISMODULE_ERR) {
      return RedisModule_ReplyWithError(ctx, "value must be double.");
    }
  }
  int n_factors = 0;
  // It writes a value to mmap[index].
  for (int i = 2; i < argc; i += 2) {
    RedisModule_StringToLongLong(argv[i], &index);
    // It writes a value if index is in range.
    if (0 <= index && (size_t)index * sizeof(double) < obj_ptr->file_size) {
      RedisModule_StringToDouble(argv[i + 1], &value);
      ((double*)obj_ptr->mmap)[index] = (double)value;
      ++n_factors;
    }
  }
  // It returns the number of writings.
  return RedisModule_ReplyWithLongLong(ctx, n_factors);
}

```

    Appending to fmmap.c


This function appends values to the end of a file.


```python
%%writefile -a fmmap.c

// VADD key value [value ...]
int VAdd_RedisCommand(RedisModuleCtx *ctx, RedisModuleString **argv, int argc)
{
  RedisModule_AutoMemory(ctx); /* Use automatic memory management. */

  RedisModuleKey *key =
      RedisModule_OpenKey(ctx, argv[1], REDISMODULE_READ | REDISMODULE_WRITE);

  MMapObject *obj_ptr = RedisModule_ModuleTypeGetValue(key);
  double value;
  // It confirms the type of value added.
  for (int i = 2; i < argc; ++i) {
    if (RedisModule_StringToDouble(argv[i], &value) == REDISMODULE_ERR) {
      return RedisModule_ReplyWithError(ctx, "value must be double.");
    }
  }
  // It extends mmap and write a value at the end of mmap.
  size_t new_size = obj_ptr->file_size + sizeof(double) * (argc - 2);
  ftruncate(obj_ptr->fd, new_size);
  munmap(obj_ptr->mmap, obj_ptr->file_size);
  obj_ptr->mmap = mmap(NULL, new_size, PROT_READ | PROT_WRITE, MAP_SHARED, obj_ptr->fd, 0);
  for (int i = 2; i < argc; ++i) {
    size_t index = obj_ptr->file_size / sizeof(double);
    RedisModule_StringToDouble(argv[i], &value);
    obj_ptr->file_size += sizeof(double);
    ((double*)obj_ptr->mmap)[index] = (double)value;
  }

  // It returns the number of elements added.
  return RedisModule_ReplyWithLongLong(ctx, argc - 2);
}
```

    Appending to fmmap.c


This function gets the number of values in a file.


```python
%%writefile -a fmmap.c

// VCOUNT key
int VCount_RedisCommand(RedisModuleCtx *ctx, RedisModuleString **argv, int argc)
{
  RedisModule_AutoMemory(ctx); /* Use automatic memory management. */

  RedisModuleKey *key =
      RedisModule_OpenKey(ctx, argv[1], REDISMODULE_READ | REDISMODULE_WRITE);

  MMapObject *obj_ptr = RedisModule_ModuleTypeGetValue(key);

  return RedisModule_ReplyWithLongLong(ctx, obj_ptr->file_size / sizeof(double));
}
```

    Appending to fmmap.c


This function erases the contents of a file.


```python
%%writefile -a fmmap.c

// VCLEAR key
int VClear_RedisCommand(RedisModuleCtx *ctx, RedisModuleString **argv, int argc)
{
  RedisModule_AutoMemory(ctx); /* Use automatic memory management. */

  RedisModuleKey *key =
      RedisModule_OpenKey(ctx, argv[1], REDISMODULE_READ | REDISMODULE_WRITE);

  MMapObject *obj_ptr = RedisModule_ModuleTypeGetValue(key);

  munmap(obj_ptr->mmap, obj_ptr->file_size);
  ftruncate(obj_ptr->fd, 0);
  obj_ptr->mmap = mmap(NULL, 0, PROT_READ | PROT_WRITE, MAP_SHARED, obj_ptr->fd, 0);
  RedisModule_ReplyWithLongLong(ctx, obj_ptr->file_size / sizeof(double));
  obj_ptr->file_size = 0;

  return REDISMODULE_OK;
}
```

    Appending to fmmap.c


This function gets a value from the end of a file and delete it.


```python
%%writefile -a fmmap.c

// VPOP key
int VPop_RedisCommand(RedisModuleCtx *ctx, RedisModuleString **argv, int argc)
{
  RedisModule_AutoMemory(ctx); /* Use automatic memory management. */

  RedisModuleKey *key =
      RedisModule_OpenKey(ctx, argv[1], REDISMODULE_READ | REDISMODULE_WRITE);

  MMapObject *obj_ptr = RedisModule_ModuleTypeGetValue(key);

  if (obj_ptr->file_size == 0) {
    RedisModule_ReplyWithNull(ctx);
  }
  else {
    size_t index = obj_ptr->file_size / sizeof(double) - 1;
    RedisModule_ReplyWithDouble(ctx, ((double*)obj_ptr->mmap)[index]);
    munmap(obj_ptr->mmap, obj_ptr->file_size);
    obj_ptr->file_size -= sizeof(double);
    ftruncate(obj_ptr->fd, obj_ptr->file_size);
    obj_ptr->mmap = mmap(NULL, obj_ptr->file_size, PROT_READ | PROT_WRITE, MAP_SHARED, obj_ptr->fd, 0);
  }
  return REDISMODULE_OK;
}
```

    Appending to fmmap.c


This function stores information about mmap in a Redis RDB file.


```python
%%writefile -a fmmap.c

void MRdbSave(RedisModuleIO *rdb, void *value)
{
  MMapObject *obj_ptr = value;
  RedisModule_SaveStringBuffer(rdb, obj_ptr->file_path, sdslen(obj_ptr->file_path));
  msync(obj_ptr->mmap, obj_ptr->file_size, MS_ASYNC);
}
```

    Appending to fmmap.c


This function reads information about mmap from a Redis RDB file.


```python
%%writefile -a fmmap.c

void *MRdbLoad(RedisModuleIO *rdb, int encver)
{
  MMapObject *obj_ptr = MCreateObject();
  obj_ptr->file_path = sdsnew(RedisModule_StringPtrLen(RedisModule_LoadString(rdb), NULL));
  obj_ptr->fd = open(obj_ptr->file_path, O_RDWR);

  struct stat sb;
  fstat(obj_ptr->fd, &sb);
  obj_ptr->file_size = sb.st_size;
  obj_ptr->mmap = mmap(NULL, sb.st_size, PROT_READ | PROT_WRITE, MAP_SHARED, obj_ptr->fd, 0);

  return obj_ptr;
}
```

    Appending to fmmap.c


Function to use AOF in Redis.


```python
%%writefile -a fmmap.c

void MAofRewrite(RedisModuleIO *aof, RedisModuleString *key, void *value)
{
  char buffer[0x200];
  MMapObject *obj_ptr = (MMapObject*)value;
  RedisModule_EmitAOF(aof, "MMAP", "sc",key, obj_ptr->file_path);
  RedisModule_EmitAOF(aof, "MCLEAR", "ss", key, obj_ptr->file_path);
  for (size_t i = 0; i < obj_ptr->file_size; i += sizeof(double)) {
    double value = *(double *)((uint8_t*)obj_ptr->mmap + i);
    sprintf(buffer, "%.16f", value);
    RedisModule_EmitAOF(aof, "MADD", "sbc", key, buffer);
  }
}
```

    Appending to fmmap.c


Other functions required for Redis modules.


```python
%%writefile -a fmmap.c

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
```

    Appending to fmmap.c


Macro to create Redis commands


```python
%%writefile -a fmmap.c

#define CREATE_CMD(name, tgt, attr, key_pos, key_last)                     \
  do {                                                                     \
    if (RedisModule_CreateCommand(ctx, name, tgt, attr, key_pos, key_last, \
                                  1) != REDISMODULE_OK) {                  \
      return REDISMODULE_ERR;                                              \
    }                                                                      \
  } while (0);

```

    Appending to fmmap.c


This function is called at module load time to prepare the module. This is where each command is associated with the function that executes it.


```python
%%writefile -a fmmap.c

int RedisModule_OnLoad(RedisModuleCtx *ctx, RedisModuleString **argv, int argc)
{
  REDISMODULE_NOT_USED(argv);
  REDISMODULE_NOT_USED(argc);

  RedisModule_Init(ctx, "FuchiMMap", 1, REDISMODULE_APIVER_1);

  RedisModuleTypeMethods tm = {.version = REDISMODULE_TYPE_METHOD_VERSION,
                               .rdb_load = MRdbLoad,
                               .rdb_save = MRdbSave,
                               .aof_rewrite = MAofRewrite,
                               .mem_usage = MMemUsage,
                               .free = MFree,
                               .digest = MDigest};

  MMapType = RedisModule_CreateDataType(ctx, "FuchiMMap", 0, &tm);

  // MMAP key file_path
  CREATE_CMD("MMAP", MMap_RedisCommand, "write fast", 1, 1);

  // VCLEAR key
  CREATE_CMD("VCLEAR", VClear_RedisCommand, "write fast", 1, 1);

  // VADD key value [value ...]
  CREATE_CMD("VADD", VAdd_RedisCommand, "write fast", 1, 1);

  // VGET key index
  CREATE_CMD("VGET", VGet_RedisCommand, "readonly fast", 1, 1);

  // VMGET key index [index ...]
  CREATE_CMD("VMGET", VMGet_RedisCommand, "readonly fast", 1, 1);

  // VSET key index value [index value ...]
  CREATE_CMD("VSET", VSet_RedisCommand, "write fast", 1, 1);

  // VCOUNT key
  CREATE_CMD("VCOUNT", VCount_RedisCommand, "readonly fast", 1, 1);

  // VPOP key
  CREATE_CMD("VPOP", VPop_RedisCommand, "write fast", 1, 1);

  return REDISMODULE_OK;
}

```

    Appending to fmmap.c


Makefile for building the source.


```python
%%writefile Makefile.fmmap

SHOBJ_CFLAGS ?= -W -Wall -fno-common -g -ggdb -std=c99 -O2
SHOBJ_LDFLAGS ?= -shared

.SUFFIXES: .c .so .xo .o

all: fmmap.so

.c.xo:
	$(CC) -I. $(CFLAGS) $(SHOBJ_CFLAGS) -fPIC -c $< -o $@

fmmap.xo: ../redismodule.h

fmmap.so: fmmap.xo
	$(LD) -o $@ $^ $(SHOBJ_LDFLAGS) $(LIBS) -lc

clean:
	rm -rf *.xo *.so
```

    Writing Makefile.fmmap


Let's build the module.


```python
!make -f Makefile.fmmap
!ls fmmap.so
```

    cc -I.  -W -Wall -fno-common -g -ggdb -std=c99 -O2 -fPIC -c fmmap.c -o fmmap.xo
    ld -o fmmap.so fmmap.xo -shared  -lc
    fmmap.so


Install Redis.


```python
!sudo yes | add-apt-repository ppa:redislabs/redis
!sudo apt-get update
!sudo apt-get install redis
```

Write something to the configuration file.


```python
%%writefile -a /etc/redis/redis.conf
enable-module-command yes
loadmodule /content/redis-stable/src/modules/fmmap.so
```

    Appending to /etc/redis/redis.conf


Run Redis.


```python
!service redis-server start
```

    Starting redis-server: redis-server.


Check Redis running.


```python
!sleep 1
!ps aux | grep redis | grep -v grep
```

    redis       6204  0.0  0.0  59132  6420 ?        Ssl  06:08   0:00 /usr/bin/redis-server 127.0.0.1:6379


Prepare the place to write a file.


```python
!mkdir /content/db
!chmod 777 /content/db
```

Maps a file to db. The return value is a number of values, 0 because it is new.


```python
!echo "MMAP db /content/db/file.mmap" | redis-cli
```

    (integer) 0


Confirm that the file.mmap file has been created. The file size is still zero.


```python
!ls -l /content/db
```

    total 0
    -rw-rw---- 1 redis redis 0 Jun 22 06:08 file.mmap


Try to add a value. The return value is the number of values added.


```python
!echo "VADD db 0.0" | redis-cli
```

    (integer) 1


You can see that the file size has increased to 8 bytes.


```python
!ls -l /content/db
```

    total 4
    -rw-rw---- 1 redis redis 8 Jun 22 06:08 file.mmap


Write the command to a file and run it.


```python
%%writefile command
vadd db 0.1
vadd db 0.2
vadd db 0.3
vadd db 0.4
vadd db 0.5
vadd db 0.6
vadd db 0.7
vadd db 0.8
vadd db 0.9
vcount db
```

    Writing command


When you run it, you will see that the number of registrations is 10.


```python
!redis-cli < command
```

    (integer) 1
    (integer) 1
    (integer) 1
    (integer) 1
    (integer) 1
    (integer) 1
    (integer) 1
    (integer) 1
    (integer) 1
    (integer) 10


The file has also increased to 80 bytes.


```python
!ls -l /content/db
```

    total 4
    -rw-rw---- 1 redis redis 80 Jun 22 06:08 file.mmap


Multiple additions can be made with a single command. The number of values added is returned.


```python
!echo "VADD db 1.0 1.1 1.2 1.3 1.4 1.5" | redis-cli
```

    (integer) 6


Let's take out the value.


```python
!echo "VGET db 5" | redis-cli
```

    "0.5"


If there is more than one, it will look like this. There is an error because of floating point.


```python
!echo "VMGET db 1 2 3 4 5" | redis-cli
```

    1) "0.10000000000000001"
    2) "0.20000000000000001"
    3) "0.29999999999999999"
    4) "0.40000000000000002"
    5) "0.5"


You can also change the values: set db[5] to 50, db[10] to 100, etc. The return value of VSET is the number of locations you have changed.


```python
!echo "VSET db 5 50 10 100" | redis-cli
!echo "VMGET db 5 10" | redis-cli
```

    (integer) 2
    1) "50"
    2) "100"


Extracts and removes the trailing value.


```python
!echo "VCOUNT db" | redis-cli
!echo "VGET db 15" | redis-cli
!echo "VPOP db" | redis-cli
!echo "VCOUNT db" | redis-cli
```

    (integer) 16
    "1.5"
    "1.5"
    (integer) 15


Stop and restart Redis.


```python
!service redis-server stop
!service redis-server start
```

    Stopping redis-server: redis-server.
    Starting redis-server: redis-server.


The values remain after rebooting.


```python
!echo "VCOUNT db" | redis-cli
```

    (integer) 15


If you delete the db, you can re-map it and get the values since the file is still in its original state.


```python
!echo "KEYS *" | redis-cli
!echo "DEL db" | redis-cli
!echo "KEYS *" | redis-cli
!ls -l /content/db/
!echo "MMAP dba /content/db/file.mmap" | redis-cli
!echo "VGET dba 5" | redis-cli
```

    1) "db"
    (integer) 1
    (empty array)
    total 4
    -rw-rw---- 1 redis redis 120 Jun 22 06:08 file.mmap
    (integer) 15
    "50"


To clear the contents, do this.


```python
!echo "VCLEAR dba" | redis-cli
!echo "VCOUNT dba" | redis-cli
```

    (integer) 15
    (integer) 0


The size of the file will be zero.


```python
!ls -l /content/db/
```

    total 0
    -rw-rw---- 1 redis redis 0 Jun 22 06:08 file.mmap


Delete dba.


```python
!echo "DEL dba" | redis-cli
```

    (integer) 1


Create a file with 100 doubles written in Python.


```python
import struct
with open('/content/db/file.mmap', 'wb') as fout:
    for i in range(100):
        fout.write(struct.pack('d', i))
```

Map the file and check the contents.


```python
!echo "mmap db /content/db/file.mmap" | redis-cli
!echo "vcount db" | redis-cli
!echo "vmget db 1 3 5 7 9" | redis-cli
```

    (integer) 100
    (integer) 100
    1) "1"
    2) "3"
    3) "5"
    4) "7"
    5) "9"


Stop Redis. No redis-server remains.


```python
!echo "shutdown" | redis-cli
!sleep 1
!ps aux | grep redis
```

    root        6347  0.0  0.0   6904  3284 ?        S    06:08   0:00 /bin/bash -c ps aux | grep redis
    root        6349  0.0  0.0   6444   728 ?        S    06:08   0:00 grep redis


The file remains.


```python
!ls -l /content/db
```

    total 4
    -rw-rw---- 1 redis redis 800 Jun 22 06:08 file.mmap


That is all.

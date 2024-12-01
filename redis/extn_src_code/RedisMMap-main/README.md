<a href="https://colab.research.google.com/github/t-fuchi/RedisMMap/blob/main/RedisMMap.ipynb" target="_parent"><img src="https://colab.research.google.com/assets/colab-badge.svg" alt="Open In Colab"/></a>

# RedisMMap
mmap module for redis

In the past, you would have to set up a script outside Redis to load data into Redis, which would then require all the data to be loaded into memory. Now, all you need to do is specify the file where the data is written, and since it’s read on-demand, it’s memory-friendly. 

## Install Redis
[Instruction](https://redis.io/docs/getting-started/installation)

## Build module and setup redis.conf
```
$ git clone https://github.com/t-fuchi/RedisMMap.git
$ cd RedisMMap/src
$ make
$ sudo mv fmmap.so /etc/redis/
$ sudo echo enable-module-command yes >> /etc/redis/redis.conf
$ sudo echo loadmodule /etc/redis/fmmap.so >> /etc/redis/redis.conf
$ service redis-server start
```
## Usage
```
// This command mmap file_path to key.
// return number of values
// value_type is int8, uint8, int16, uint16, int32, uint32, int64, uint64, float, double, long_double or string
MMAP key file_path value_type [value_size] [writable]

// This command clears contents in key (trancate file_path).
// return number of values which are cleared
VCLEAR key

// This command adds value at the end of key.
// return number of values added
VADD key value [value ...]

// This command gets value from key at index.
// return value
VGET key index

// This command gets values from key at indices.
// return array of values
VMGET key index [index ...]

// This command gets all values from key.
// return array of values
VALL key

// This command sets value at index in key.
// return number of values set
VSET key index value [index value ...]

// This command gets number of elements in key.
// return number of values
VCOUNT key

// This command pops the last value in key.
// return the last value
VPOP key

// get file path which is mapped for key
// return file path
VFILEPATH key

// get value type for key (int8, uint32, etc)
// return value type
VTYPE key

// get value size for key (int8:1, uint32:4, etc)
// return value size
VSIZE key

```

## Example
```
$ python3
Python 3.7.2 (v3.7.2:9a3ffc0492, Dec 24 2018, 02:44:43) 
[Clang 6.0 (clang-600.0.57)] on darwin
Type "help", "copyright", "credits" or "license" for more information.
>>> import struct
>>> with open('file.mmap', 'wb') as fout:
...   for i in range(-100, 100):
...     fout.write(struct.pack('d', i))
^D

$ ls -l file.mmap 
-rw-r--r--  1 fuchi  staff  1600  6 22 03:39 file.mmap

$ redis-cli
127.0.0.1:6379> mmap db file.mmap double
(integer) 200
127.0.0.1:6379> vget db 0
"-100"
127.0.0.1:6379> vmget db 0 50 100 150 199
1) "-100"
2) "-50"
3) "0"
4) "50"
5) "99"
127.0.0.1:6379> vget db 200
(error) index exceeds size
127.0.0.1:6379> vset db 0 -1000
(error) The file is not writable
127.0.0.1:6379> del db
(integer) 1
127.0.0.1:6379> mmap db file.mmap double writable
(integer) 200
127.0.0.1:6379> vset db 0 -1000
(integer) 1
127.0.0.1:6379> vget db 0
"-1000"
127.0.0.1:6379> vadd db 500
(integer) 1
127.0.0.1:6379> vget db 200
"500"
127.0.0.1:6379> vtype db
"double"
127.0.0.1:6379> vsize db
(integer) 8
127.0.0.1:6379> vcount db
(integer) 201
127.0.0.1:6379> vfilepath db
"file.mmap"
127.0.0.1:6379> vpop db
"500"
127.0.0.1:6379> vcount db
(integer) 200
127.0.0.1:6379> vclear db
(integer) 200
127.0.0.1:6379> vcount db
(integer) 0
127.0.0.1:6379> vpop db
(nil)
127.0.0.1:6379> del db
(integer) 1
127.0.0.1:6379> ^D
$
```

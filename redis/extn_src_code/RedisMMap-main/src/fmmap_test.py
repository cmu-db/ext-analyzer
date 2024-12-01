#!/usr/bin/env python3

import pytest
import redis
import subprocess
import time
import os
import struct
import time
import numpy as np

@pytest.fixture(scope="module", autouse=True)
def scope_module():
    if os.path.exists('dump.rdb'):
        os.remove('dump.rdb')
    subprocess.Popen(["redis-server", "redis.conf"],
                     stdout=open('log.txt', mode='w', buffering=1))
    time.sleep(1)
    r = redis.Redis()
    yield r
    r.shutdown()

@pytest.mark.parametrize(
    "value_type, value_size, min, max, enc",
    [
      ('int8', 1, np.iinfo(np.int8).min, np.iinfo(np.int8).max, 'b'),
      ('uint8', 1, np.iinfo(np.uint8).min, np.iinfo(np.uint8).max, 'B'),
      ('int16', 2, np.iinfo(np.int16).min, np.iinfo(np.int16).max, 'h'),
      ('uint16', 2, np.iinfo(np.uint16).min, np.iinfo(np.uint16).max, 'H'),
      ('int32', 4, np.iinfo(np.int32).min, np.iinfo(np.int32).max, 'i'),
      ('uint32', 4, np.iinfo(np.uint32).min, np.iinfo(np.uint32).max, 'I'),
      ('int64', 8, -1, 1, 'q'),
      ('uint64', 8, 0, 1, 'Q'),
    ],
)
def test_int(scope_module, value_type, value_size, min, max, enc):
    r = scope_module
    r.execute_command('del db')
    if os.path.exists('file.mmap'):
      os.remove('file.mmap')
    assert r.execute_command(f'mmap db file.mmap {value_type} writable') == 0
    assert r.execute_command('vadd db 0 2 4 6 8 10') == 6
    assert r.execute_command('vcount db') == 6
    assert r.execute_command('vget db 1') == 2
    assert r.execute_command('vmget db 0 1 2 3 4 5') == [0, 2, 4, 6, 8, 10]
    assert r.execute_command('vall db') == [0, 2, 4, 6, 8, 10]
    assert r.execute_command('vpop db') == 10
    assert r.execute_command('vcount db') == 5
    assert r.execute_command('vfilepath db') == b'file.mmap'
    if min < 0:
      assert r.execute_command('vset db 1 -2 2 -4') == 2
    else:
      assert r.execute_command('vset db 1 4 2 8') == 2
    if max != 1:
      with pytest.raises(Exception):
        r.execute_command(f'vadd db {max + 1}')
    if min != -1:
      with pytest.raises(Exception):
        r.execute_command(f'vadd db {min - 1}')
    with pytest.raises(Exception):
      r.execute_command('vset db 5 0')
    with pytest.raises(Exception):
      r.execute_command('vset db -1 0')
    with pytest.raises(Exception):
      r.execute_command('vset db 0 0 0')
    if min < 0:
      assert r.execute_command('vall db') == [0, -2, -4, 6, 8]
    else:
      assert r.execute_command('vall db') == [0, 4, 8, 6, 8]
    assert r.execute_command('vtype db') == value_type.encode('utf8')
    assert r.execute_command('vsize db') == value_size
    assert r.execute_command('del db') == 1

    assert r.execute_command(f'mmap db2 file.mmap {value_type}') == 5
    if min < 0:
      assert r.execute_command('vall db2') == [0, -2, -4, 6, 8]
    else:
      assert r.execute_command('vall db2') == [0, 4, 8, 6, 8]
    assert r.execute_command('vfilepath db2') == b'file.mmap'
    with pytest.raises(Exception):
      r.execute_command('vclear db2')
    with pytest.raises(Exception):
      r.execute_command('vset db2 1 0')
    with pytest.raises(Exception):
      r.execute_command('vadd db2 1')
    with pytest.raises(Exception):
      r.execute_command('vpop db2')
    assert r.execute_command('del db2') == 1

    assert r.execute_command(f'mmap db file.mmap {value_type} writable') == 5
    assert r.execute_command('vclear db') == 5
    assert r.execute_command('vcount db') == 0
    with pytest.raises(Exception):
      r.execute_command('vget db 0')
    with pytest.raises(Exception):
      r.execute_command('vset db 0 0')
    assert r.execute_command('vpop db') == None
    assert r.execute_command('del db') == 1

    assert os.path.getsize('file.mmap') == 0
    with open('file.mmap', 'wb') as fout:
      if min < 0:
        for i in range(-100, 100):
          fout.write(struct.pack(enc, i))
      else:
        for i in range(200):
          fout.write(struct.pack(enc, i))
    assert r.execute_command(f'mmap db3 file.mmap {value_type}') == 200
    for i in range(200):
      if min < 0:
        assert r.execute_command(f'vget db3 {i}') == i - 100
      else:
        assert r.execute_command(f'vget db3 {i}') == i
    assert r.execute_command('del db3') == 1

@pytest.mark.parametrize(
    "value_type, value_size, min, max, enc",
    [
      ('float', 4, np.finfo(np.float32).min, np.finfo(np.float32).max, 'f'),
      ('double', 8, np.finfo(np.float64).min, np.finfo(np.float64).max, 'd'),
      ('long_double', 16, -1, 1, ''),
    ],
)
def test_float(scope_module, value_type, value_size, min, max, enc):
    r = scope_module
    r.execute_command('del db')
    if os.path.exists('file.mmap'):
      os.remove('file.mmap')
    assert r.execute_command(f'mmap db file.mmap {value_type} writable') == 0
    assert r.execute_command('vadd db 0 2 4 6 8 10') == 6
    assert r.execute_command('vcount db') == 6
    assert r.execute_command('vget db 1') == b'2'
    assert r.execute_command('vmget db 0 1 2 3 4 5') == [b'0', b'2', b'4', b'6', b'8', b'10']
    assert r.execute_command('vall db') == [b'0', b'2', b'4', b'6', b'8', b'10']
    assert r.execute_command('vpop db') == b'10'
    assert r.execute_command('vcount db') == 5
    assert r.execute_command('vfilepath db') == b'file.mmap'
    if min < 0:
      assert r.execute_command('vset db 1 -2 2 -4') == 2
    else:
      assert r.execute_command('vset db 1 4 2 8') == 2
    if max != 1:
      with pytest.raises(Exception):
        r.execute_command(f'vadd db {max.astype(np.float128) * 2}')
    if min != -1:
      with pytest.raises(Exception):
        r.execute_command(f'vadd db {min.astype(np.float128) * 2}')
    with pytest.raises(Exception):
      r.execute_command('vset db 5 0')
    with pytest.raises(Exception):
      r.execute_command('vset db -1 0')
    with pytest.raises(Exception):
      r.execute_command('vset db 0 0 0')
    if min < 0:
      assert r.execute_command('vall db') == [b'0', b'-2', b'-4', b'6', b'8']
    else:
      assert r.execute_command('vall db') == [b'0', b'4', b'8', b'6', b'8']
    assert r.execute_command('vtype db') == value_type.encode('utf8')
    assert r.execute_command('vsize db') == value_size
    assert r.execute_command('del db') == 1

    assert r.execute_command(f'mmap db2 file.mmap {value_type}') == 5
    if min < 0:
      assert r.execute_command('vall db2') == [b'0', b'-2', b'-4', b'6', b'8']
    else:
      assert r.execute_command('vall db2') == [b'0', b'4', b'8', b'6', b'8']
    assert r.execute_command('vfilepath db2') == b'file.mmap'
    with pytest.raises(Exception):
      r.execute_command('vclear db2')
    with pytest.raises(Exception):
      r.execute_command('vset db2 1 0')
    with pytest.raises(Exception):
      r.execute_command('vadd db2 1')
    with pytest.raises(Exception):
      r.execute_command('vpop db2')
    assert r.execute_command('del db2') == 1

    assert r.execute_command(f'mmap db file.mmap {value_type} writable') == 5
    assert r.execute_command('vclear db') == 5
    assert r.execute_command('vcount db') == 0
    with pytest.raises(Exception):
      r.execute_command('vget db 0')
    with pytest.raises(Exception):
      r.execute_command('vset db 0 0')
    assert r.execute_command('vpop db') == None
    assert r.execute_command('del db') == 1

    if 0 < len(enc):
      assert os.path.getsize('file.mmap') == 0
      with open('file.mmap', 'wb') as fout:
        if min < 0:
          for i in range(-100, 100):
            fout.write(struct.pack(enc, i))
        else:
          for i in range(200):
            fout.write(struct.pack(enc, i))
      assert r.execute_command(f'mmap db3 file.mmap {value_type}') == 200
      for i in range(200):
        if min < 0:
          assert r.execute_command(f'vget db3 {i}') == f'{i - 100}'.encode('utf8')
        else:
          assert r.execute_command(f'vget db3 {i}') == f'{i}'.encode('utf8')
      assert r.execute_command('del db3') == 1

def test_string(scope_module):
    r = scope_module
    r.execute_command('del db')
    if os.path.exists('file.mmap'):
      os.remove('file.mmap')
    assert r.execute_command(f'mmap db file.mmap string 8 writable') == 0
    assert r.execute_command('vadd', 'db', 'a', 'ab', 'abc', 'abcd', 'abcd e', 'abcd e f') == 6
    assert r.execute_command('vcount db') == 6
    assert r.execute_command('vget db 1') == b'ab'
    assert r.execute_command('vget db 5') == b'abcd e f'
    assert r.execute_command('vmget db 0 1 2 3 4 5') == [b'a', b'ab', b'abc', b'abcd', b'abcd e', b'abcd e f']
    assert r.execute_command('vall db') == [b'a', b'ab', b'abc', b'abcd', b'abcd e', b'abcd e f']
    assert r.execute_command('vpop db') == b'abcd e f'
    assert r.execute_command('vcount db') == 5
    assert r.execute_command('vfilepath db') == b'file.mmap'
    assert r.execute_command('vset db 1 ba 2 cba') == 2
    assert r.execute_command('vall db') == [b'a', b'ba', b'cba', b'abcd', b'abcd e']
    with pytest.raises(Exception):
      r.execute_command('vadd db 123456789')
    with pytest.raises(Exception):
      r.execute_command('vset db 0 123456789')
    with pytest.raises(Exception):
      r.execute_command('vset db 5 0')
    with pytest.raises(Exception):
      r.execute_command('vset db -1 0')
    with pytest.raises(Exception):
      r.execute_command('vset db 0 0 0')
    assert r.execute_command('vtype db') == b'string'
    assert r.execute_command('vsize db') == 8
    assert r.execute_command('del db') == 1

    assert r.execute_command(f'mmap db2 file.mmap string 8') == 5
    assert r.execute_command('vall db2') == [b'a', b'ba', b'cba', b'abcd', b'abcd e']
    assert r.execute_command('vfilepath db2') == b'file.mmap'
    with pytest.raises(Exception):
      r.execute_command('vclear db2')
    with pytest.raises(Exception):
      r.execute_command('vset db2 1 0')
    with pytest.raises(Exception):
      r.execute_command('vadd db2 1')
    with pytest.raises(Exception):
      r.execute_command('vpop db2')
    assert r.execute_command('del db2') == 1

    assert r.execute_command(f'mmap db file.mmap string 8 writable') == 5
    assert r.execute_command('vclear db') == 5
    assert r.execute_command('vcount db') == 0
    with pytest.raises(Exception):
      r.execute_command('vget db 0')
    with pytest.raises(Exception):
      r.execute_command('vset db 0 0')
    assert r.execute_command('vpop db') == None
    assert r.execute_command('del db') == 1

    assert os.path.getsize('file.mmap') == 0
    with open('file.mmap', 'wb') as fout:
      for i in range(200):
        fout.write(f'{i}'.encode('utf8'))
        for _ in range(8 - len(f'{i}')):
          fout.write(b'\0')
    assert r.execute_command(f'mmap db3 file.mmap string 8') == 200
    for i in range(200):
      assert r.execute_command(f'vget db3 {i}') == f'{i}'.encode('utf8')
    assert r.execute_command('del db3') == 1

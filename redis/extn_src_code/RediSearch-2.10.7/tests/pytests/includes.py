
import sys
import os
from RLTest import Defaults

if sys.version_info > (3, 0):
    Defaults.decode_responses = True

try:
    sys.path.insert(0, os.path.join(os.path.dirname(__file__), "../../deps/readies"))
    import paella
except:
    pass

UNSTABLE = os.getenv('UNSTABLE', '0') == '1'
SANITIZER = os.getenv('SANITIZER', '')
COORD = os.getenv('COORD', '0') in ('1', 'oss', 'rlec')
VALGRIND = os.getenv('VALGRIND', '0') == '1'
CODE_COVERAGE = os.getenv('CODE_COVERAGE', '0') == '1'
NO_LIBEXT = os.getenv('NO_LIBEXT', '0') == '1'
CI = os.getenv('CI', '') != ''
GHA = os.getenv('GITHUB_ACTIONS', '') != ''
TEST_DEBUG = os.getenv('TEST_DEBUG', '0') == '1'
MT_BUILD = os.getenv('REDISEARCH_MT_BUILD', '0') == '1'

OSNICK = paella.Platform().osnick
OS = paella.Platform().os
ARCH = paella.Platform().arch

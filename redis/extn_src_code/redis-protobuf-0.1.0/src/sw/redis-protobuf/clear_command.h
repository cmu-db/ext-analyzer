/**************************************************************************
   Copyright (c) 2019 sewenew

   Licensed under the Apache License, Version 2.0 (the "License");
   you may not use this file except in compliance with the License.
   You may obtain a copy of the License at

       http://www.apache.org/licenses/LICENSE-2.0

   Unless required by applicable law or agreed to in writing, software
   distributed under the License is distributed on an "AS IS" BASIS,
   WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
   See the License for the specific language governing permissions and
   limitations under the License.
 *************************************************************************/

#ifndef SEWENEW_REDISPROTOBUF_CLEAR_COMMANDS_H
#define SEWENEW_REDISPROTOBUF_CLEAR_COMMANDS_H

#include "module_api.h"
#include <vector>
#include "utils.h"
#include "field_ref.h"

namespace sw {

namespace redis {

namespace pb {

// command: PB.CLEAR key type [path]
// return:  Integer reply: If the key exist, return 1. Otherwise, return 0.
// error:   If the type doesn't match the protobuf message type of the key,
//          or the path doesn't exist, return an error reply.
class ClearCommand {
public:
    int run(RedisModuleCtx *ctx, RedisModuleString **argv, int argc) const;

private:
    struct Args {
        RedisModuleString *key_name;
        Path path;
    };

    Args _parse_args(RedisModuleString **argv, int argc) const;

    void _clear(gp::Message &msg, const Path &path) const;
};

}

}

}

#endif // end SEWENEW_REDISPROTOBUF_CLEAR_COMMANDS_H

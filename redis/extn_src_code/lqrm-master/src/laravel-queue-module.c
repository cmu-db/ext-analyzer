/*
 * Copyright (c) 2018, Hamid Alaei Varnosfaderani
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions are met:
 *
 * * Redistributions of source code must retain the above copyright notice, this
 *   list of conditions and the following disclaimer.
 *
 * * Redistributions in binary form must reproduce the above copyright notice,
 *   this list of conditions and the following disclaimer in the documentation
 *   and/or other materials provided with the distribution.
 *
 * * Neither the name of the copyright holder nor the names of its
 *   contributors may be used to endorse or promote products derived from
 *   this software without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
 * AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
 * DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE LIABLE
 * FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
 * DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR
 * SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER
 * CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY,
 * OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
 * OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */

#include "laravel-pop.h"
#include "laravel-push.h"
#include "laravel-later.h"
#include "laravel-delete-reserved.h"
#include "laravel-release-reserved.h"
#include "blocking-pop.h"
#include "../vendor/cJSON.h"

cJSON_Hooks cJSONHooks;

void setupMemoryManagement()
{
    cJSONHooks.malloc_fn = RedisModule_Alloc;
    cJSONHooks.free_fn = RedisModule_Free;
    cJSON_InitHooks(&cJSONHooks);
}

int RedisModule_OnLoad(RedisModuleCtx *ctx, RedisModuleString **argv, int argc)
{
    setupMemoryManagement();

    if (RedisModule_Init(ctx, "laravel-queue", 1, REDISMODULE_APIVER_1) == REDISMODULE_ERR) {
        return REDISMODULE_ERR;
    }

    if (initWaitingList() == REDISMODULE_ERR) {
        return REDISMODULE_ERR;
    }

    if (Create_Laravel_Pop_Command(ctx) == REDISMODULE_ERR) {
        return REDISMODULE_ERR;
    }
    if (Create_Laravel_Push_Command(ctx) == REDISMODULE_ERR) {
        return REDISMODULE_ERR;
    }
    if (Create_Laravel_Later_Command(ctx) == REDISMODULE_ERR) {
        return REDISMODULE_ERR;
    }
    if (Create_Laravel_Delete_Reserved_Command(ctx) == REDISMODULE_ERR) {
        return REDISMODULE_ERR;
    }
    if (Create_Laravel_Release_Reserved_Command(ctx) == REDISMODULE_ERR) {
        return REDISMODULE_ERR;
    }
    return REDISMODULE_OK;
}

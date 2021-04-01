/*
 * Copyright (C) [2020-2021] Futurewei Technologies, Inc. All rights reserved.
 *
 * OpenArkCompiler is licensed underthe Mulan Permissive Software License v2.
 * You can use this software according to the terms and conditions of the MulanPSL - 2.0.
 * You may obtain a copy of MulanPSL - 2.0 at:
 *
 *   https://opensource.org/licenses/MulanPSL-2.0
 *
 * THIS SOFTWARE IS PROVIDED ON AN "AS IS" BASIS, WITHOUT WARRANTIES OF ANY KIND, EITHER
 * EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO NON-INFRINGEMENT, MERCHANTABILITY OR
 * FIT FOR A PARTICULAR PURPOSE.
 * See the MulanPSL - 2.0 for more details.
 */

#include <cstdlib>
#include <cstring>
#include "mdebug.h"

namespace maple {

    int debug_engine;
    extern "C" void __initialize_debug_kind() {
        debug_engine = kEngineDebugNone;
        const char* debug_env = std::getenv("MAPLE_ENGINE_DEBUG");
        if(debug_env == nullptr)
            return;
        while(*debug_env != '\0') {
            const char* debug_deli = debug_env;
            while(*debug_deli != ':' && *debug_deli != '\0')
                ++debug_deli;
            const int size = debug_deli - debug_env;
            if(size == sizeof("instruction") - 1 && std::strncmp(debug_env, "instruction", size) == 0)
                debug_engine |= kEngineDebugInstruction;
            else if(size == sizeof("method") - 1 && std::strncmp(debug_env, "method", size) == 0)
                debug_engine |= kEngineDebugMethod;
            else if(size == sizeof("all") - 1 && std::strncmp(debug_env, "all", size) == 0)
                debug_engine |= kEngineDebugAll;
            debug_env = *debug_deli == ':' ? debug_deli + 1 : debug_deli;
        }
    }

}


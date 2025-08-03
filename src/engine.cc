/*
 * engine.cc - Copyright (c) 2004-2025
 *
 * Gregory Montoir, Fabien Sanglard, Olivier Poncet
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 */
#ifdef HAVE_CONFIG_H
#include "config.h"
#endif
#include <cerrno>
#include <cstdio>
#include <cstring>
#include <cstdlib>
#include <cstdarg>
#include <cstdint>
#include <climits>
#include <cassert>
#include <ctime>
#include <algorithm>
#include <stdexcept>
#include <iostream>
#include <utility>
#include <memory>
#include <string>
#include <vector>
#include <thread>
#include <mutex>
#ifdef __EMSCRIPTEN__
#include <emscripten.h>
#endif
#include "logger.h"
#include "engine.h"

// ---------------------------------------------------------------------------
// Engine
// ---------------------------------------------------------------------------

Engine::Engine(System& stub, const std::string& dataDirectory, const std::string& dumpDirectory)
    : SubSystem(*this)
    , dataDir(dataDirectory)
    , dumpDir(dumpDirectory)
    , system(stub)
    , resources(*this)
    , video(*this)
    , audio(*this)
    , music(*this)
    , vm(*this)
{
}

auto Engine::init() -> void
{
    log_debug(SYS_ENGINE, "initializing...");
    system.init();
    resources.init();
    video.init();
    audio.init();
    music.init();
    vm.init();
    log_debug(SYS_ENGINE, "initialized!");
}

auto Engine::fini() -> void
{
    log_debug(SYS_ENGINE, "finalizing...");
    vm.fini();
    music.fini();
    audio.fini();
    video.fini();
    resources.fini();
    system.fini();
    log_debug(SYS_ENGINE, "finalized!");
}

auto Engine::run() -> void
{
    auto iterate = +[](Engine* engine) -> void
    {
        engine->system.processEvents();
        engine->vm.inp_updatePlayer();
        engine->vm.hostFrame();
#ifdef __EMSCRIPTEN__
        if(engine->system.input.quit != false) {
            log_debug(SYS_ENGINE, "stopped!");
            engine->fini();
            engine = (delete engine, nullptr);
            ::emscripten_cancel_main_loop();
            ::emscripten_force_exit(EXIT_SUCCESS);
        }
#endif
    };

#ifdef __EMSCRIPTEN__
    auto loop = [&]() -> void
    {
        init();
        ::emscripten_set_main_loop_arg(reinterpret_cast<em_arg_callback_func>(iterate), this, 0, 1);
    };
#else
    auto loop = [&]() -> void
    {
        init();
        log_debug(SYS_ENGINE, "running...");
        while(system.input.quit == false) {
            const auto currTimeStamp = getTimeStamp();
            const auto nextTimeStamp = vm.getNextTimeStamp();
            if(nextTimeStamp > currTimeStamp) {
                sleepFor((nextTimeStamp - currTimeStamp));
            }
            iterate(this);
        }
        log_debug(SYS_ENGINE, "stopped!");
        fini();
    };
#endif

    return loop();
}

// ---------------------------------------------------------------------------
// End-Of-File
// ---------------------------------------------------------------------------

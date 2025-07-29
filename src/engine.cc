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
// Engine::Emscripten
// ---------------------------------------------------------------------------

struct Engine::Emscripten
{
#ifdef __EMSCRIPTEN__
    static auto startup(Engine* engine) -> void
    {
        engine->start();
        engine->reset();
        log_debug(SYS_ENGINE, "running...");
        ::emscripten_set_main_loop_arg(reinterpret_cast<em_arg_callback_func>(mainloop), engine, 0, 1);
    };

    static auto shutdown(Engine* engine) -> void
    {
        log_debug(SYS_ENGINE, "stopped!");
        engine->stop();
        engine = (delete engine, nullptr);
        ::emscripten_cancel_main_loop();
        ::emscripten_force_exit(EXIT_SUCCESS);
    };

    static auto mainloop(Engine* engine) -> void
    {
        if(engine->isRunning()) {
            engine->processEvents();
            engine->processVirtualMachine();
        }
        if(engine->isStopped()) {
            shutdown(engine);
        }
        if(engine->getReqPartId() != 0) {
            engine->initPart(0);
        }
    };
#endif
};

// ---------------------------------------------------------------------------
// Engine
// ---------------------------------------------------------------------------

Engine::Engine(const std::string& dataDir, const std::string& dumpDir)
    : SubSystem(*this, "Engine")
    , _dataDir(dataDir)
    , _dumpDir(dumpDir)
    , _backend(Backend::create(*this))
    , _resources(*this)
    , _video(*this)
    , _audio(*this)
    , _input(*this)
    , _vm(*this)
{
}

auto Engine::main() -> void
{
    auto run = [&]() -> void
    {
#ifdef __EMSCRIPTEN__
        Emscripten::startup(this);
#else
        start();
        reset();
        log_debug(SYS_ENGINE, "running...");
        while(true) {
            if(isRunning()) {
                processEvents();
                processVirtualMachine();
            }
            if(isStopped()) {
                break;
            }
            if(getReqPartId() != 0) {
                initPart(0);
            }
            sleepUntil(_vm.getNextTicks());
        }
        log_debug(SYS_ENGINE, "stopped!");
        stop();
#endif
    };

    return run();
}

auto Engine::start() -> void
{
    log_debug(SYS_ENGINE, "starting...");
    _backend->start();
    _resources.start();
    _video.start();
    _audio.start();
    _input.start();
    _vm.start();
    log_debug(SYS_ENGINE, "started!");
}

auto Engine::reset() -> void
{
    log_debug(SYS_ENGINE, "resetting...");
    _backend->reset();
    _resources.reset();
    _video.reset();
    _audio.reset();
    _input.reset();
    _vm.reset();
#ifdef SKIP_GAME_PART0
    initPart(GAME_PART1);
#else
    initPart(GAME_PART0);
#endif
    log_debug(SYS_ENGINE, "reset!");
}

auto Engine::stop() -> void
{
    log_debug(SYS_ENGINE, "stopping...");
    _vm.stop();
    _input.stop();
    _audio.stop();
    _video.stop();
    _resources.stop();
    _backend->stop();
    log_debug(SYS_ENGINE, "stopped!");
}

auto Engine::initPart(uint16_t id) -> void
{
    _video.reset();
    _audio.reset();
    _input.reset();
    _resources.loadPart(id);
    _video.setPalettes(_resources.getPalettesData());
    _vm.setByteCode(_resources.getByteCodeData());
}

// ---------------------------------------------------------------------------
// End-Of-File
// ---------------------------------------------------------------------------

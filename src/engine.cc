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
// some useful macros
// ---------------------------------------------------------------------------

#ifndef NDEBUG
#define LOG_DEBUG(format, ...) log_debug(SYS_ENGINE, format, ##__VA_ARGS__)
#else
#define LOG_DEBUG(format, ...) do {} while(0)
#endif

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
        LOG_DEBUG("running...");
        ::emscripten_set_main_loop_arg(reinterpret_cast<em_arg_callback_func>(mainloop), engine, 0, 1);
    };

    static auto shutdown(Engine* engine) -> void
    {
        LOG_DEBUG("stopped!");
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

Engine::Engine(const std::string& datadir, const std::string& dumpdir)
    : SubSystem(*this, "Engine")
    , _datadir(datadir)
    , _dumpdir(dumpdir)
    , _palmode(0)
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
        LOG_DEBUG("running...");
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
        LOG_DEBUG("stopped!");
        stop();
#endif
    };

    return run();
}

auto Engine::start() -> void
{
    LOG_DEBUG("starting...");
    _backend->start();
    _resources.start();
    _video.start();
    _audio.start();
    _input.start();
    _vm.start();
    LOG_DEBUG("started!");
}

auto Engine::reset() -> void
{
    LOG_DEBUG("resetting...");
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
    LOG_DEBUG("reset!");
}

auto Engine::stop() -> void
{
    LOG_DEBUG("stopping...");
    _vm.stop();
    _input.stop();
    _audio.stop();
    _video.stop();
    _resources.stop();
    _backend->stop();
    LOG_DEBUG("stopped!");
}

auto Engine::initPart(uint16_t id) -> void
{
    _video.reset();
    _audio.reset();
    _input.reset();
    _resources.loadPart(id);
    _video.setPalettes(_resources.getPalettesData(), _palmode);
    _vm.setByteCode(_resources.getByteCodeData());
}

auto Engine::switchPalettes() -> void
{
    _palmode = (_palmode + 1) % 5;
    _video.setPalettes(_resources.getPalettesData(), _palmode);
}

// ---------------------------------------------------------------------------
// End-Of-File
// ---------------------------------------------------------------------------

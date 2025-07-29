/*
 * audio.cc - Copyright (c) 2004-2025
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
#include "logger.h"
#include "engine.h"
#include "audio.h"

// ---------------------------------------------------------------------------
// some useful macros
// ---------------------------------------------------------------------------

#ifndef NDEBUG
#define LOG_DEBUG(format, ...) log_debug(SYS_AUDIO, format, ##__VA_ARGS__)
#else
#define LOG_DEBUG(format, ...) do {} while(0)
#endif

// ---------------------------------------------------------------------------
// Audio
// ---------------------------------------------------------------------------

Audio::Audio(Engine& engine)
    : SubSystem(engine, "Audio")
    , _mixer(engine, *this)
    , _sound(engine, *this)
    , _music(engine, *this)
{
}

auto Audio::start() -> void
{
    LOG_DEBUG("starting...");
    _mixer.start();
    _sound.start();
    _music.start();
    LOG_DEBUG("started!");
}

auto Audio::reset() -> void
{
    LOG_DEBUG("resetting...");
    _mixer.reset();
    _sound.reset();
    _music.reset();
    LOG_DEBUG("reset!");
}

auto Audio::stop() -> void
{
    LOG_DEBUG("stopping...");
    _music.stop();
    _sound.stop();
    _mixer.stop();
    LOG_DEBUG("stopped!");
}

// ---------------------------------------------------------------------------
// End-Of-File
// ---------------------------------------------------------------------------

/*
 * input.cc - Copyright (c) 2004-2025
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
#include "input.h"

// ---------------------------------------------------------------------------
// Input
// ---------------------------------------------------------------------------

Input::Input(Engine& engine)
    : SubSystem(engine, "Input")
    , _controls()
{
}

auto Input::start() -> void
{
    log_debug(SYS_INPUT, "starting...");
    log_debug(SYS_INPUT, "started!");
}

auto Input::reset() -> void
{
    log_debug(SYS_INPUT, "resetting...");
    _controls = Controls();
    log_debug(SYS_INPUT, "reset!");
}

auto Input::stop() -> void
{
    log_debug(SYS_INPUT, "stopping...");
    log_debug(SYS_INPUT, "stopped!");
}

// ---------------------------------------------------------------------------
// End-Of-File
// ---------------------------------------------------------------------------

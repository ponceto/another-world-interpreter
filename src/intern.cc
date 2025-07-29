/*
 * intern.cc - Copyright (c) 2004-2025
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
#include "intern.h"

// ---------------------------------------------------------------------------
// some useful macros
// ---------------------------------------------------------------------------

#ifndef NDEBUG
#define LOG_DEBUG(format, ...) log_debug(SYS_ENGINE, format, ##__VA_ARGS__)
#else
#define LOG_DEBUG(format, ...) do {} while(0)
#endif

// ---------------------------------------------------------------------------
// Paula::frequencyTable (formula is: PaulaFrequency / (2 * period))
// ---------------------------------------------------------------------------

const uint16_t Paula::frequencyTable[] = {
   static_cast<uint16_t>(Paula::Carrier / 1076),
   static_cast<uint16_t>(Paula::Carrier / 1016),
   static_cast<uint16_t>(Paula::Carrier /  960),
   static_cast<uint16_t>(Paula::Carrier /  906),
   static_cast<uint16_t>(Paula::Carrier /  856),
   static_cast<uint16_t>(Paula::Carrier /  808),
   static_cast<uint16_t>(Paula::Carrier /  762),
   static_cast<uint16_t>(Paula::Carrier /  720),
   static_cast<uint16_t>(Paula::Carrier /  678),
   static_cast<uint16_t>(Paula::Carrier /  640),
   static_cast<uint16_t>(Paula::Carrier /  604),
   static_cast<uint16_t>(Paula::Carrier /  570),
   static_cast<uint16_t>(Paula::Carrier /  538),
   static_cast<uint16_t>(Paula::Carrier /  508),
   static_cast<uint16_t>(Paula::Carrier /  480),
   static_cast<uint16_t>(Paula::Carrier /  453),
   static_cast<uint16_t>(Paula::Carrier /  428),
   static_cast<uint16_t>(Paula::Carrier /  404),
   static_cast<uint16_t>(Paula::Carrier /  381),
   static_cast<uint16_t>(Paula::Carrier /  360),
   static_cast<uint16_t>(Paula::Carrier /  339),
   static_cast<uint16_t>(Paula::Carrier /  320),
   static_cast<uint16_t>(Paula::Carrier /  302),
   static_cast<uint16_t>(Paula::Carrier /  285),
   static_cast<uint16_t>(Paula::Carrier /  269),
   static_cast<uint16_t>(Paula::Carrier /  254),
   static_cast<uint16_t>(Paula::Carrier /  240),
   static_cast<uint16_t>(Paula::Carrier /  226),
   static_cast<uint16_t>(Paula::Carrier /  214),
   static_cast<uint16_t>(Paula::Carrier /  202),
   static_cast<uint16_t>(Paula::Carrier /  190),
   static_cast<uint16_t>(Paula::Carrier /  180),
   static_cast<uint16_t>(Paula::Carrier /  170),
   static_cast<uint16_t>(Paula::Carrier /  160),
   static_cast<uint16_t>(Paula::Carrier /  151),
   static_cast<uint16_t>(Paula::Carrier /  143),
   static_cast<uint16_t>(Paula::Carrier /  135),
   static_cast<uint16_t>(Paula::Carrier /  127),
   static_cast<uint16_t>(Paula::Carrier /  120),
   static_cast<uint16_t>(Paula::Carrier /  113),
};

// ---------------------------------------------------------------------------
// GameParts
// ---------------------------------------------------------------------------

const GamePart GameParts::data[GAME_NUM_PARTS] = {
    { "Protection"  , 0x14, 0x15, 0x16, 0x00 },
    { "Introduction", 0x17, 0x18, 0x19, 0x00 },
    { "Water"       , 0x1a, 0x1b, 0x1c, 0x11 },
    { "Jail"        , 0x1d, 0x1e, 0x1f, 0x11 },
    { "Cite"        , 0x20, 0x21, 0x22, 0x11 },
    { "Arena"       , 0x23, 0x24, 0x25, 0x00 },
    { "Luxe"        , 0x26, 0x27, 0x28, 0x11 },
    { "Final"       , 0x29, 0x2a, 0x2b, 0x11 },
    { "Password"    , 0x7d, 0x7e, 0x7f, 0x00 },
    { "Password"    , 0x7d, 0x7e, 0x7f, 0x00 },
};

// ---------------------------------------------------------------------------
// SubSystem
// ---------------------------------------------------------------------------

SubSystem::SubSystem(Engine& engine, const std::string& subsystem)
    : _subsystem(subsystem)
    , _engine(engine)
    , _mutex()
    , _currTicks(0)
    , _prevTicks(0)
    , _nextTicks(0)
{
    LOG_DEBUG("creating %s", _subsystem.c_str());
}

SubSystem::~SubSystem()
{
    LOG_DEBUG("destroyed %s", _subsystem.c_str());
}

// ---------------------------------------------------------------------------
// End-Of-File
// ---------------------------------------------------------------------------

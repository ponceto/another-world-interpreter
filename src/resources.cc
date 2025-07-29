/*
 * resources.cc - Copyright (c) 2004-2025
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
#include "file.h"
#include "resources.h"

// ---------------------------------------------------------------------------
// some useful macros
// ---------------------------------------------------------------------------

#ifndef NDEBUG
#define LOG_DEBUG(format, ...) log_debug(SYS_RESOURCES, format, ##__VA_ARGS__)
#else
#define LOG_DEBUG(format, ...) do {} while(0)
#endif

// ---------------------------------------------------------------------------
// Resources
// ---------------------------------------------------------------------------

Resources::Resources(Engine& engine)
    : SubSystem(engine, "Resources")
    , _buffer()
    , _bufptr(_buffer)
    , _bufend(_buffer + countof(_buffer))
    , _dictionaryId(Dictionary::DIC_DEFAULT)
    , _resourcesArray()
    , _resourcesCount(0)
    , _curPartId(0)
    , _reqPartId(0)
    , _segPalettes(nullptr)
    , _segByteCode(nullptr)
    , _segPolygon1(nullptr)
    , _segPolygon2(nullptr)
{
}

auto Resources::start() -> void
{
    LOG_DEBUG("starting...");
    LOG_DEBUG("buffer size is %luK", (sizeof(_buffer) / 1024));
    loadMemList();
    LOG_DEBUG("started!");
}

auto Resources::reset() -> void
{
    LOG_DEBUG("resetting...");
    LOG_DEBUG("reset!");
}

auto Resources::stop() -> void
{
    LOG_DEBUG("stopping...");
    invalidateAll();
    LOG_DEBUG("stopped!");
}

auto Resources::loadPart(uint16_t partId) -> void
{
    const GamePart* part = nullptr;

    if(partId == 0) {
        if((partId = _reqPartId) != 0) {
            _reqPartId = 0;
        }
        else {
            return;
        }
    }
    if((partId >= GAME_PART_FIRST) && (partId <= GAME_PART_LAST)) {
        part = &GameParts::data[partId - GAME_PART_FIRST];
        LOG_DEBUG("load part [id: 0x%04x, name: '%s']", partId, part->name);
        _curPartId = partId;
        _reqPartId = 0;
    }
    else {
        log_error("cannot load invalid part <0x%04x>", partId);
    }
#ifndef PRELOAD_RESOURCES
    invalidateAll();
#endif
    const uint8_t palettesId = part->palettes;
    const uint8_t bytecodeId = part->bytecode;
    const uint8_t polygon1Id = part->polygon1;
    const uint8_t polygon2Id = part->polygon2;

    if(palettesId != 0x00) {
        loadResource(palettesId);
    }
    if(bytecodeId != 0x00) {
        loadResource(bytecodeId);
    }
    if(polygon1Id != 0x00) {
        loadResource(polygon1Id);
    }
    if(polygon2Id != 0x00) {
        loadResource(polygon2Id);
    }

    _segPalettes = _resourcesArray[palettesId].data;
    _segByteCode = _resourcesArray[bytecodeId].data;
    _segPolygon1 = _resourcesArray[polygon1Id].data;
    _segPolygon2 = _resourcesArray[polygon2Id].data;
}

auto Resources::loadResource(uint16_t resourceId) -> void
{
    if(resourceId >= countof(_resourcesArray)) {
        LOG_DEBUG("load resource [part: 0x%04x]", resourceId);
        _reqPartId = resourceId;
    }
    else {
        Resource& resource = _resourcesArray[resourceId];
        if(resource.state != RS_LOADED) {
            resource.state = RS_NEEDED;
            loadResources();
        }
        if(resource.state == RS_LOADED) {
            if(resource.type == RT_BITMAP) {
                _engine.drawBitmap(resource.data);
            }
        }
    }
}

auto Resources::getString(uint16_t stringId) -> const String*
{
    auto get_string_en = [&]() -> const String*
    {
        for(auto& string : Dictionary::dataEN) {
            if(string.id == 0xffff) {
                break;
            }
            if(stringId == string.id) {
                return &string;
            }
        }
        return nullptr;
    };

    auto get_string_fr = [&]() -> const String*
    {
        for(auto& string : Dictionary::dataFR) {
            if(string.id == 0xffff) {
                break;
            }
            if(stringId == string.id) {
                return &string;
            }
        }
        return nullptr;
    };

    auto get_string = [&]() -> const String*
    {
        switch(_dictionaryId) {
            case Dictionary::DIC_DEFAULT:
            case Dictionary::DIC_ENGLISH:
                return get_string_en();
            case Dictionary::DIC_FRENCH:
                return get_string_fr();
            default:
                break;
        }
        return nullptr;
    };

    return get_string();
}

auto Resources::loadMemList() -> void
{
    LOG_DEBUG("load memlist");

    MemList memlist(_engine.getDataDir(), _engine.getDumpDir());
    if(memlist.loadMemList(_resourcesArray) != false) {
        _bufptr = _buffer;
        _resourcesCount = 0;
        for(auto& resource : _resourcesArray) {
            if(resource.type == RT_END) {
                break;
            }
            ++_resourcesCount;
#ifndef PRELOAD_RESOURCES
            resource.state = RS_NOT_NEEDED;
#else
            resource.state = RS_NEEDED;
#endif
            resource.data  = nullptr;
        }
        loadResources();
    }
    else {
        log_fatal("error while loading memlist");
    }
}

auto Resources::loadResources() -> void
{
    MemList memlist(_engine.getDataDir(), _engine.getDumpDir());
    for(auto& resource : _resourcesArray) {
        if(resource.type == RT_END) {
            break;
        }
        if(resource.state == RS_NEEDED) {
            LOG_DEBUG("load resource [resource: 0x%02x, bank: 0x%02x]", resource.id, resource.bankId);
            resource.data = _bufptr;
            _bufptr += ((resource.unpackedSize + 1) + 3) & ~3;
            if(_bufptr > _bufend) {
                log_fatal("cannot load resource, not enough memory");
            }
            if(memlist.loadResource(resource) != false) {
                resource.state = RS_LOADED;
                memlist.dumpResource(resource);
            }
            else {
                log_fatal("error while loading resource");
            }
        }
    }
    LOG_DEBUG("resources memory [consumed: %luK, remaining: %luK]", ((_bufptr - _buffer) / 1024), ((_bufend - _bufptr) / 1024));
}

auto Resources::invalidateAll() -> void
{
    for(auto& resource : _resourcesArray) {
        if(resource.type == RT_END) {
            break;
        }
        LOG_DEBUG("invalidate resource [resource: 0x%02x]", resource.id);
        resource.state = RS_NOT_NEEDED;
        resource.data  = nullptr;
    }
    _bufptr = _buffer;
}

// ---------------------------------------------------------------------------
// End-Of-File
// ---------------------------------------------------------------------------

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
#include "resources.h"

// ---------------------------------------------------------------------------
// Resources
// ---------------------------------------------------------------------------

Resources::Resources(Engine& engine)
    : SubSystem(engine)
    , _bufferBegin(nullptr)
    , _bufferIter(nullptr)
    , _bufferEnd(nullptr)
    , _resourcesTable()
    , _resourcesCount(0)
    , currentPartId(0)
    , requestedPartId(0)
    , segPalettes(nullptr)
    , segByteCode(nullptr)
    , segPolygon(nullptr)
    , segVideo2(nullptr)
    , useSegVideo2(false)
{
    allocBuffer();
}

Resources::~Resources()
{
    freeBuffer();
}

auto Resources::init() -> void
{
    log_debug(SYS_RESOURCES, "initializing...");
    readMemList();
    log_debug(SYS_RESOURCES, "initialized!");
}

auto Resources::fini() -> void
{
    log_debug(SYS_RESOURCES, "finalizing...");
    invalidateAll();
    log_debug(SYS_RESOURCES, "finalized!");
}

auto Resources::setupPart(uint16_t partId) -> void
{
    if(partId == currentPartId) {
        return;
    }

    if((partId >= GAME_PART_FIRST) && (partId <= GAME_PART_LAST)) {
        log_debug(SYS_RESOURCES, "setup part [part: 0x%04x]", partId);
        currentPartId = partId;
    }
    else {
        log_fatal("cannot setup invalid part <0x%04x>", partId);
    }

#if 0
    invalidateAll();
#endif

    const uint16_t partIndex     = partId - GAME_PART_FIRST;
    const uint8_t  paletteIndex  = GameParts::data[partIndex][MEMLIST_PART_PALETTE];
    const uint8_t  bytecodeIndex = GameParts::data[partIndex][MEMLIST_PART_BYTECODE];
    const uint8_t  polygonIndex  = GameParts::data[partIndex][MEMLIST_PART_POLYGON];
    const uint8_t  video2Index   = GameParts::data[partIndex][MEMLIST_PART_VIDEO2];

    if(paletteIndex != 0x00) {
        loadPartOrResource(paletteIndex);
    }
    if(bytecodeIndex != 0x00) {
        loadPartOrResource(bytecodeIndex);
    }
    if(polygonIndex != 0x00) {
        loadPartOrResource(polygonIndex);
    }
    if(video2Index != 0x00) {
        loadPartOrResource(video2Index);
    }

    segPalettes = _resourcesTable[paletteIndex].data;
    segByteCode = _resourcesTable[bytecodeIndex].data;
    segPolygon  = _resourcesTable[polygonIndex].data;
    segVideo2   = _resourcesTable[video2Index].data;
}

auto Resources::loadPartOrResource(uint16_t resourceId) -> void
{
    if(resourceId >= countof(_resourcesTable)) {
        log_debug(SYS_RESOURCES, "load part [part: 0x%04x]", resourceId);
        requestedPartId = resourceId;
    }
    else {
        Resource& resource = _resourcesTable[resourceId];
        if(resource.state != RS_LOADED) {
            resource.state = RS_NEEDED;
            loadMarkedAsNeeded();
        }
        if(resource.state == RS_LOADED) {
            if(resource.type == RT_BITMAP) {
                _engine.video.copyPage(resource.data);
            }
        }
    }
}

auto Resources::allocBuffer() -> void
{
    constexpr uint32_t MEM_BLOCK_SIZE = (2048 * 1024); // 2 megabytes, sufficient for loading all assets

    if(_bufferBegin == nullptr) {
        log_debug(SYS_RESOURCES, "allocate buffer [size: %lu bytes]", MEM_BLOCK_SIZE);
        _bufferBegin = new uint8_t[MEM_BLOCK_SIZE];
        _bufferIter  = _bufferBegin;
        _bufferEnd   = _bufferBegin + MEM_BLOCK_SIZE;
    }
}

auto Resources::freeBuffer() -> void
{
    if(_bufferBegin != nullptr) {
        log_debug(SYS_RESOURCES, "deallocate buffer");
        _bufferBegin = (delete[] _bufferBegin, nullptr);
        _bufferIter  = nullptr;
        _bufferEnd   = nullptr;
    }
}

auto Resources::readMemList() -> void
{
    log_debug(SYS_RESOURCES, "read memlist");

    MemList memlist(_engine.dataDir);
    if(memlist.load(_resourcesTable)) {
        invalidateAll();
    }
    else {
        log_fatal("error while loading memlist");
    }
}

auto Resources::readMemBank(Resource& resource) -> void
{
    log_debug(SYS_RESOURCES, "read membank [bank: 0x%02x, resource: 0x%02x]", resource.bankId, resource.index);

    MemBank membank(_engine.dataDir);
    if(membank.load(resource)) {
        resource.state = RS_LOADED;
    }
    else {
        log_fatal("error while loading resource");
    }
}

auto Resources::invalidateAll() -> void
{
    _resourcesCount = 0;
    for(auto& resource : _resourcesTable) {
        if(resource.type == RT_END) {
            break;
        }
        else {
            ++_resourcesCount;
        }
        log_debug(SYS_RESOURCES, "invalidate resource [resource: 0x%02x]", resource.index);
        resource.state = RS_NOT_NEEDED;
        resource.data  = nullptr;
    }
    _bufferIter = _bufferBegin;
}

auto Resources::loadMarkedAsNeeded() -> void
{
    for(auto& resource : _resourcesTable) {
        if(resource.type == RT_END) {
            break;
        }
        if(resource.state == RS_NEEDED) {
            log_debug(SYS_RESOURCES, "load resource [resource: 0x%02x]", resource.index);
            resource.data = _bufferIter;
            _bufferIter += ((resource.unpackedSize + 1) + 3) & ~3;
            if(_bufferIter >= _bufferEnd) {
                log_fatal("cannot load resource, not enough memory");
            }
            readMemBank(resource);
            log_debug(SYS_RESOURCES, "total consumed memory is %lu bytes", (_bufferIter - _bufferBegin));
        }
    }
    dumpResources();
}

auto Resources::dumpResources() -> void
{
    char filename[PATH_MAX] = "";
    uint32_t sound_packed      = 0;
    uint32_t sound_unpacked    = 0;
    uint32_t music_packed      = 0;
    uint32_t music_unpacked    = 0;
    uint32_t bitmap_packed     = 0;
    uint32_t bitmap_unpacked   = 0;
    uint32_t palette_packed    = 0;
    uint32_t palette_unpacked  = 0;
    uint32_t bytecode_packed   = 0;
    uint32_t bytecode_unpacked = 0;
    uint32_t polygon_packed    = 0;
    uint32_t polygon_unpacked  = 0;
    uint32_t unknown_packed    = 0;
    uint32_t unknown_unpacked  = 0;
    uint32_t total_packed      = 0;
    uint32_t total_unpacked    = 0;

    auto percent = [](uint32_t packed_size, uint32_t unpacked_size) -> int
    {
        if(unpacked_size == 0) {
            return 0;
        }
        return 100 - ((100 * packed_size) / unpacked_size);
    };

    if(!_engine.dumpDir.empty()) {
        ::snprintf(filename, sizeof(filename), "%s/memlist.txt", _engine.dumpDir.c_str());
    }
    if(filename[0] != '\0') {
        FILE* file = ::fopen(filename, "w");
        if(file != nullptr) {
            uint16_t  index = 0;
            uint16_t  count = _resourcesCount;
            Resource* resource = _resourcesTable;
            while(index < count) {
                const char* type = nullptr;
                switch(resource->type) {
                    case RT_SOUND:
                        type = "sound";
                        sound_packed   += resource->packedSize;
                        sound_unpacked += resource->unpackedSize;
                        break;
                    case RT_MUSIC:
                        type = "music";
                        music_packed   += resource->packedSize;
                        music_unpacked += resource->unpackedSize;
                        break;
                    case RT_BITMAP:
                        type = "bitmap";
                        bitmap_packed   += resource->packedSize;
                        bitmap_unpacked += resource->unpackedSize;
                        break;
                    case RT_PALETTE:
                        type = "palette";
                        palette_packed   += resource->packedSize;
                        palette_unpacked += resource->unpackedSize;
                        break;
                    case RT_BYTECODE:
                        type = "bytecode";
                        bytecode_packed   += resource->packedSize;
                        bytecode_unpacked += resource->unpackedSize;
                        break;
                    case RT_POLYGON:
                        type = "polygon";
                        polygon_packed   += resource->packedSize;
                        polygon_unpacked += resource->unpackedSize;
                        break;
                    default:
                        type = "unknown";
                        unknown_packed   += resource->packedSize;
                        unknown_unpacked += resource->unpackedSize;
                        break;
                }
                total_packed   += resource->packedSize;
                total_unpacked += resource->unpackedSize;
                ::fprintf(file, "0x%02x %-14s bank=0x%02x offset=%-6d packed-size=%-5d unpacked-size=%-5d\n", index, type, resource->bankId, resource->bankOffset, resource->packedSize, resource->unpackedSize);
                if(resource->state == RS_LOADED) {
                    dumpResource(*resource);
                }
                ++index;
                ++resource;
            }
            ::fprintf(file, "\n");
            ::fprintf(file, "total_sound    packed-size=%-7d unpacked-size=%-7d compression-ratio=%02d%%\n", sound_packed   , sound_unpacked   , percent(sound_packed   , sound_unpacked   ));
            ::fprintf(file, "total_music    packed-size=%-7d unpacked-size=%-7d compression-ratio=%02d%%\n", music_packed   , music_unpacked   , percent(music_packed   , music_unpacked   ));
            ::fprintf(file, "total_bitmap   packed-size=%-7d unpacked-size=%-7d compression-ratio=%02d%%\n", bitmap_packed  , bitmap_unpacked  , percent(bitmap_packed  , bitmap_unpacked  ));
            ::fprintf(file, "total_palette  packed-size=%-7d unpacked-size=%-7d compression-ratio=%02d%%\n", palette_packed , palette_unpacked , percent(palette_packed , palette_unpacked ));
            ::fprintf(file, "total_bytecode packed-size=%-7d unpacked-size=%-7d compression-ratio=%02d%%\n", bytecode_packed, bytecode_unpacked, percent(bytecode_packed, bytecode_unpacked));
            ::fprintf(file, "total_polygon  packed-size=%-7d unpacked-size=%-7d compression-ratio=%02d%%\n", polygon_packed , polygon_unpacked , percent(polygon_packed , polygon_unpacked ));
            ::fprintf(file, "total_unknown  packed-size=%-7d unpacked-size=%-7d compression-ratio=%02d%%\n", unknown_packed , unknown_unpacked , percent(unknown_packed , unknown_unpacked ));
            ::fprintf(file, "total_data     packed-size=%-7d unpacked-size=%-7d compression-ratio=%02d%%\n", total_packed   , total_unpacked   , percent(total_packed   , total_unpacked   ));
            ::fflush(file);
            file = (::fclose(file), nullptr);
        }
    }
}

auto Resources::dumpResource(const Resource& resource) -> void
{
    char filename[PATH_MAX] = "";

    if(resource.state != RS_LOADED) {
        return;
    }
    if(!_engine.dumpDir.empty()) {
        const char* type = nullptr;
        switch(resource.type) {
            case RT_SOUND:
                type = "sound";
                break;
            case RT_MUSIC:
                type = "music";
                break;
            case RT_BITMAP:
                type = "bitmap";
                break;
            case RT_PALETTE:
                type = "palette";
                break;
            case RT_BYTECODE:
                type = "bytecode";
                break;
            case RT_POLYGON:
                type = "polygon";
                break;
            default:
                type = "unknown";
                break;
        }
        ::snprintf(filename, sizeof(filename), "%s/%02x_%s.data", _engine.dumpDir.c_str(), resource.index, type);
    }
    if(filename[0] != '\0') {
        FILE* file = ::fopen(filename, "w");
        if(file != nullptr) {
            ::fwrite(resource.data, resource.unpackedSize, 1, file);
            ::fflush(file);
            file = (::fclose(file), nullptr);
        }
    }
}

// ---------------------------------------------------------------------------
// End-Of-File
// ---------------------------------------------------------------------------

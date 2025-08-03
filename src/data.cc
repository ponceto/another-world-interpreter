/*
 * data.cc - Copyright (c) 2004-2025
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
#include "bytekiller.h"
#include "file.h"
#include "data.h"

// ---------------------------------------------------------------------------
// MemList
// ---------------------------------------------------------------------------

MemList::MemList(const std::string& dataDir)
    : _dataDir(dataDir)
{
}

auto MemList::load(Resource* list) -> bool
{
    File file("stdc");
    char path[PATH_MAX] = "";

    if(path[0] == '\0') {
        ::snprintf(path, sizeof(path), "%s/MEMLIST.BIN", _dataDir.c_str());
    }
    if(file.open(path, "rb") != false) {
        if(list != nullptr) {
            auto* item = list;
            int   index = 0;
            while(item != nullptr) {
                uint8_t buffer[32];
                if(file.read(buffer, 20)) {
                    Ptr ptr(buffer);
                    item->index        = static_cast<uint8_t>(index++);
                    item->state        = ptr.fetchByte();
                    item->type         = ptr.fetchByte();
                    item->unknown2     = ptr.fetchWordBE();
                    item->unknown4     = ptr.fetchWordBE();
                    item->rankNum      = ptr.fetchByte();
                    item->bankId       = ptr.fetchByte();
                    item->bankOffset   = ptr.fetchLongBE();
                    item->unknownC     = ptr.fetchWordBE();
                    item->packedSize   = ptr.fetchWordBE();
                    item->unknown10    = ptr.fetchWordBE();
                    item->unpackedSize = ptr.fetchWordBE();
                    item->data         = nullptr;
                    if(item->type != RT_END) {
                        ++item;
                    }
                    else {
                        break;
                    }
                }
                else {
                    break;
                }
            }
        }
        dump(list);
    }
    else {
        return false;
    }
    return true;
}

auto MemList::dump(Resource* list) -> bool
{
    ResourceStats total;
    ResourceStats sound;
    ResourceStats music;
    ResourceStats bitmap;
    ResourceStats palette;
    ResourceStats bytecode;
    ResourceStats polygon;
    ResourceStats unknown;

    auto percent = [](uint32_t count, uint32_t total) -> float
    {
        if(total != 0) {
            return 100.0f * (0.0f + (static_cast<float>(count) / static_cast<float>(total)));
        }
        return 0.0f;
    };

    auto gain = [](uint32_t count, uint32_t total) -> float
    {
        if(total != 0) {
            return 100.0f * (1.0f - (static_cast<float>(count) / static_cast<float>(total)));
        }
        return 0.0f;
    };

    auto log_item = [&](const char* type, const Resource& item) -> void
    {
        log_debug(SYS_RESOURCES, "| 0x%02x | %-8s | %7d bytes | %7d bytes | %6.2f%% |", item.index, type, item.packedSize, item.unpackedSize, gain(item.packedSize, item.unpackedSize));
    };

    auto log_items = [&]() -> void
    {
        log_debug(SYS_RESOURCES, "+------+----------+---------------+---------------+---------+");
        log_debug(SYS_RESOURCES, "| id   | type     |   packed-size | unpacked-size |  gain %% |");
        log_debug(SYS_RESOURCES, "+------+----------+---------------+---------------+---------+");
        auto* item = list;
        while(item != nullptr) {
            if(item->type == RT_END) {
                break;
            }
            const char* type = nullptr;
            total.count    += 1;
            total.packed   += item->packedSize;
            total.unpacked += item->unpackedSize;
            switch(item->type) {
                case RT_SOUND:
                    type = "SOUND";
                    sound.count    += 1;
                    sound.packed   += item->packedSize;
                    sound.unpacked += item->unpackedSize;
                    break;
                case RT_MUSIC:
                    type = "MUSIC";
                    music.count    += 1;
                    music.packed   += item->packedSize;
                    music.unpacked += item->unpackedSize;
                    break;
                case RT_BITMAP:
                    type = "BITMAP";
                    bitmap.count    += 1;
                    bitmap.packed   += item->packedSize;
                    bitmap.unpacked += item->unpackedSize;
                    break;
                case RT_PALETTE:
                    type = "PALETTE";
                    palette.count    += 1;
                    palette.packed   += item->packedSize;
                    palette.unpacked += item->unpackedSize;
                    break;
                case RT_BYTECODE:
                    type = "BYTECODE";
                    bytecode.count    += 1;
                    bytecode.packed   += item->packedSize;
                    bytecode.unpacked += item->unpackedSize;
                    break;
                case RT_POLYGON:
                    type = "POLYGON";
                    polygon.count    += 1;
                    polygon.packed   += item->packedSize;
                    polygon.unpacked += item->unpackedSize;
                    break;
                default:
                    type = "UNKNOWN";
                    unknown.count    += 1;
                    unknown.packed   += item->packedSize;
                    unknown.unpacked += item->unpackedSize;
                    break;
            }
            log_item(type, *item);
            ++item;
        }
        log_debug(SYS_RESOURCES, "+------+----------+---------------+---------------+---------+");
    };

    auto log_stat = [&](const char* type, const ResourceStats& stats) -> void
    {
        log_debug(SYS_RESOURCES, "| %-8s | %5d | %7d bytes | %7d bytes | %6.2f%% | %6.2f%% |", type, stats.count, stats.packed, stats.unpacked, gain(stats.packed, stats.unpacked), percent(stats.unpacked, total.unpacked));
    };

    auto log_stats = [&]() -> void
    {
        log_debug(SYS_RESOURCES, "+----------+-------+---------------+---------------+---------+---------+");
        log_debug(SYS_RESOURCES, "| type     | count |   packed-size | unpacked-size |  gain %% | total %% |");
        log_debug(SYS_RESOURCES, "+----------+-------+---------------+---------------+---------+---------+");
        log_stat("SOUND"   , sound   );
        log_stat("MUSIC"   , music   );
        log_stat("BITMAP"  , bitmap  );
        log_stat("PALETTE" , palette );
        log_stat("BYTECODE", bytecode);
        log_stat("POLYGON" , polygon );
        log_stat("UNKNOWN" , unknown );
        log_stat("TOTAL"   , total   );
        log_debug(SYS_RESOURCES, "+----------+-------+---------------+---------------+---------+---------+");
    };

    if(list != nullptr) {
        log_items();
        log_stats();
    }
    return true;
}

// ---------------------------------------------------------------------------
// MemBank
// ---------------------------------------------------------------------------

MemBank::MemBank(const std::string& dataDir)
    : _dataDir(dataDir)
{
}

auto MemBank::load(Resource& item) -> bool
{
    File file("stdc");
    char path[PATH_MAX] = "";

    if(path[0] == '\0') {
        ::snprintf(path, sizeof(path), "%s/BANK%02X", _dataDir.c_str(), item.bankId);
    }
    if(file.open(path, "rb") != false) {
        if(file.seek(item.bankOffset) && file.read(item.data, item.packedSize)) {
            if(item.packedSize != item.unpackedSize) {
                ByteKiller bytekiller(item.data, item.packedSize, item.unpackedSize);

                return bytekiller.unpack();
            }
            return true;
        }
    }
    return false;
}

// ---------------------------------------------------------------------------
// End-Of-File
// ---------------------------------------------------------------------------

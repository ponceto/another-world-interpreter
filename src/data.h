/*
 * data.h - Copyright (c) 2004-2025
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
#ifndef __AW_DATA_H__
#define __AW_DATA_H__

// ---------------------------------------------------------------------------
// ResourceState
// ---------------------------------------------------------------------------

enum ResourceState
{
    RS_NOT_NEEDED = 0x00,
    RS_NEEDED     = 0x01,
    RS_LOADED     = 0x02,
    RS_END        = 0xff,
};

// ---------------------------------------------------------------------------
// ResourceType
// ---------------------------------------------------------------------------

enum ResourceType
{
    RT_SOUND    = 0x00,
    RT_MUSIC    = 0x01,
    RT_BITMAP   = 0x02,
    RT_PALETTE  = 0x03,
    RT_BYTECODE = 0x04,
    RT_POLYGON  = 0x05,
    RT_UNKNOWN  = 0x06,
    RT_END      = 0xff,
};

// ---------------------------------------------------------------------------
// Resource
// ---------------------------------------------------------------------------

struct Resource
{
    uint8_t  index        = 0;
    uint8_t  state        = 0xff; // offset 0x00: state
    uint8_t  type         = 0xff; // offset 0x01: type
    uint16_t unknown2     = 0;    // offset 0x02: unused
    uint16_t unknown4     = 0;    // offset 0x04: unused
    uint8_t  rankNum      = 0;    // offset 0x06: rank num
    uint8_t  bankId       = 0;    // offset 0x07: bank id
    uint32_t bankOffset   = 0;    // offset 0x08: bank offset
    uint16_t unknownC     = 0;    // offset 0x0c: unused
    uint16_t packedSize   = 0;    // offset 0x0e: packed size
    uint16_t unknown10    = 0;    // offset 0x10: unused
    uint16_t unpackedSize = 0;    // offset 0x12: unpacked size
    uint8_t* data         = nullptr;
};

// ---------------------------------------------------------------------------
// ResourceStats
// ---------------------------------------------------------------------------

struct ResourceStats
{
    uint32_t count    = 0;
    uint32_t packed   = 0;
    uint32_t unpacked = 0;
};

// ---------------------------------------------------------------------------
// MemList
// ---------------------------------------------------------------------------

class MemList
{
public: // public interface
    MemList(const std::string& dataDir);

    MemList(MemList&&) = delete;

    MemList(const MemList&) = delete;

    MemList& operator=(MemList&&) = delete;

    MemList& operator=(const MemList&) = delete;

    virtual ~MemList() = default;

    auto load(Resource* list) -> bool;

    auto dump(Resource* list) -> bool;

private: // private data
    const std::string _dataDir;
};

// ---------------------------------------------------------------------------
// MemBank
// ---------------------------------------------------------------------------

class MemBank
{
public: // public interface
    MemBank(const std::string& dataDir);

    MemBank(MemBank&&) = delete;

    MemBank(const MemBank&) = delete;

    MemBank& operator=(MemBank&&) = delete;

    MemBank& operator=(const MemBank&) = delete;

    virtual ~MemBank() = default;

    auto load(Resource& item) -> bool;

private: // private data
    const std::string _dataDir;
};

// ---------------------------------------------------------------------------
// End-Of-File
// ---------------------------------------------------------------------------

#endif /* __AW_DATA_H__ */

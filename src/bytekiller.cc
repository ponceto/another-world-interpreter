/*
 * bytekiller.cc - Copyright (c) 2004-2025
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
#include "bytekiller.h"

// ---------------------------------------------------------------------------
// ByteKiller
// ---------------------------------------------------------------------------

ByteKiller::ByteKiller(uint8_t* buffer, uint32_t packedSize, uint32_t unpackedSize)
    : _buffer(buffer)
    , _srcptr(_buffer + packedSize - 1)
    , _dstptr(_buffer + unpackedSize - 1)
    , _length(0)
    , _check(0)
    , _chunk(0)
{
    _length = fetchLong();
    _check  = fetchLong();
    _chunk  = fetchLong();
    _check  = _check ^_chunk;
}

auto ByteKiller::unpack() -> bool
{
    if(_length != 0) {
        do {
            uint32_t code   = getBit();
            uint32_t count  = 0;
            uint32_t offset = 0;
            if(code == 0) {
                code = (code << 1) | getBits(1);
            }
            else {
                code = (code << 2) | getBits(2);
            }
            switch(code) {
                case 0x00: // 0b00ccc
                    count = getBits(3) + 1;
                    unpackBytes(count);
                    break;
                case 0x07: // 0b111cccccccc
                    count = getBits(8) + 9;
                    unpackBytes(count);
                    break;
                case 0x01: // 0b01oooooooo
                    count  = 2;
                    offset = getBits(8);
                    unpackBytes(offset, count);
                    break;
                case 0x04: // 0b100ooooooooo
                    count  = 3;
                    offset = getBits(9);
                    unpackBytes(offset, count);
                    break;
                case 0x05: // 0b101oooooooooo
                    count  = 4;
                    offset = getBits(10);
                    unpackBytes(offset, count);
                    break;
                case 0x06: // 0b110ccccccccoooooooooooo
                    count  = getBits(8) + 1;
                    offset = getBits(12);
                    unpackBytes(offset, count);
                    break;
                default:
                    log_error("ByteKiller: unhandled code");
                    return false;
            }
        } while(_length != 0);
    }
    else {
        log_error("ByteKiller: already unpacked");
        return false;
    }
    if(_check != 0) {
        log_error("ByteKiller: CRC error while unpacking");
        return false;
    }
    return true;
}

auto ByteKiller::fetchLong() -> uint32_t
{
    uint32_t l = 0;
    if(_srcptr != nullptr) {
        assert(((_srcptr - 4) + 1) >= _buffer);
        l |= (static_cast<uint32_t>(*_srcptr--) <<  0);
        l |= (static_cast<uint32_t>(*_srcptr--) <<  8);
        l |= (static_cast<uint32_t>(*_srcptr--) << 16);
        l |= (static_cast<uint32_t>(*_srcptr--) << 24);
    }
    return l;
}

auto ByteKiller::writeByte(uint8_t byte) -> void
{
    if(_dstptr != nullptr) {
        assert((((_dstptr - 1) + 1) >= _buffer));
        *_dstptr-- = byte;
        --_length;
    }
}

auto ByteKiller::getBit() -> uint32_t
{
    constexpr uint32_t msb = (1 << 31);
    constexpr uint32_t lsb = (1 <<  0);
    uint32_t bit = _chunk & lsb;

    if((_chunk >>= 1) == 0) {
        _chunk = fetchLong();
        _check = _check ^_chunk;
        bit    = _chunk & lsb;
        _chunk = (_chunk >> 1) | msb;
    }
    return bit;
}

auto ByteKiller::getBits(uint32_t count) -> uint32_t
{
    uint32_t bits = 0;

    while(count != 0) {
        bits = (bits << 1) | getBit();
        --count;
    }
    return bits;
}

auto ByteKiller::unpackBytes(uint32_t count) -> void
{
    while(count != 0) {
        writeByte(getBits(8));
        --count;
    }
}

auto ByteKiller::unpackBytes(uint32_t offset, uint32_t count) -> void
{
    while(count != 0) {
        writeByte(_dstptr[offset]);
        --count;
    }
}

// ---------------------------------------------------------------------------
// End-Of-File
// ---------------------------------------------------------------------------

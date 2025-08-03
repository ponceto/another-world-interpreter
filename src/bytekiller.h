/*
 * bytekiller.h - Copyright (c) 2004-2025
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
#ifndef __AW_BYTEKILLER_H__
#define __AW_BYTEKILLER_H__

// ---------------------------------------------------------------------------
// ByteKiller
// ---------------------------------------------------------------------------

class ByteKiller
{
public: // public interface
    ByteKiller(uint8_t* buffer, uint32_t packedSize, uint32_t unpackedSize);

    ByteKiller(ByteKiller&&) = delete;

    ByteKiller(const ByteKiller&) = delete;

    ByteKiller& operator=(ByteKiller&&) = delete;

    ByteKiller& operator=(const ByteKiller&) = delete;

   ~ByteKiller() = default;

    auto unpack() -> bool;

private: // private interface
    auto fetchLong() -> uint32_t;

    auto writeByte(uint8_t byte) -> void;

    auto getBit() -> uint32_t;

    auto getBits(uint32_t bits) -> uint32_t;

    auto unpackBytes(uint32_t count) -> void;

    auto unpackBytes(uint32_t offset, uint32_t count) -> void;

private: // private data
    uint8_t* _buffer;
    uint8_t* _srcptr;
    uint8_t* _dstptr;
    int32_t  _length;
    uint32_t _check;
    uint32_t _chunk;
};

// ---------------------------------------------------------------------------
// End-Of-File
// ---------------------------------------------------------------------------

#endif /* __AW_BYTEKILLER_H__ */

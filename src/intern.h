/*
 * intern.h - Copyright (c) 2004-2025
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
#ifndef __AW_INTERN_H__
#define __AW_INTERN_H__

// ---------------------------------------------------------------------------
// countof
// ---------------------------------------------------------------------------

#ifndef countof
#define countof(array) (sizeof(array) / sizeof(array[0]))
#endif

// ---------------------------------------------------------------------------
// forward declarations
// ---------------------------------------------------------------------------

class Engine;
class System;
class Resources;
class Video;
class Audio;
class Music;
class VirtualMachine;

// ---------------------------------------------------------------------------
// some aliases
// ---------------------------------------------------------------------------

using AudioCallback = void (*)(void* data, uint8_t* buffer, int length);

using TimerCallback = uint32_t (*)(uint32_t delay, void* param);

using LockGuard = std::lock_guard<std::mutex>;

// ---------------------------------------------------------------------------
// SubSystem
// ---------------------------------------------------------------------------

class SubSystem
{
public: // public interface
    SubSystem(Engine& engine);

    SubSystem(SubSystem&&) = delete;

    SubSystem(const SubSystem&) = delete;

    SubSystem& operator=(SubSystem&&) = delete;

    SubSystem& operator=(const SubSystem&) = delete;

    virtual ~SubSystem() = default;

    virtual auto init() -> void = 0;

    virtual auto fini() -> void = 0;

protected: // protected data
    Engine&    _engine;
    std::mutex _mutex;
};

// ---------------------------------------------------------------------------
// Utils
// ---------------------------------------------------------------------------

struct Utils
{
    static inline auto fetchByte(const uint8_t* bytes) -> uint8_t
    {
        uint8_t b = 0;
        if(bytes != nullptr) {
            b |= (static_cast<uint8_t>(*bytes++) << 0);
        }
        return b;
    }

    static inline auto fetchWordBE(const uint8_t* bytes) -> uint16_t
    {
        uint16_t w = 0;
        if(bytes != nullptr) {
            w |= (static_cast<uint16_t>(*bytes++) << 8);
            w |= (static_cast<uint16_t>(*bytes++) << 0);
        }
        return w;
    }

    static inline auto fetchWordLE(const uint8_t* bytes) -> uint16_t
    {
        uint16_t w = 0;
        if(bytes != nullptr) {
            w |= (static_cast<uint16_t>(*bytes++) << 0);
            w |= (static_cast<uint16_t>(*bytes++) << 8);
        }
        return w;
    }

    static inline auto fetchLongBE(const uint8_t* bytes) -> uint32_t
    {
        uint32_t l = 0;
        if(bytes != nullptr) {
            l |= (static_cast<uint32_t>(*bytes++) << 24);
            l |= (static_cast<uint32_t>(*bytes++) << 16);
            l |= (static_cast<uint32_t>(*bytes++) <<  8);
            l |= (static_cast<uint32_t>(*bytes++) <<  0);
        }
        return l;
    }

    static inline auto fetchLongLE(const uint8_t* bytes) -> uint32_t
    {
        uint32_t l = 0;
        if(bytes != nullptr) {
            l |= (static_cast<uint32_t>(*bytes++) <<  0);
            l |= (static_cast<uint32_t>(*bytes++) <<  8);
            l |= (static_cast<uint32_t>(*bytes++) << 16);
            l |= (static_cast<uint32_t>(*bytes++) << 24);
        }
        return l;
    }
};

// ---------------------------------------------------------------------------
// Ptr
// ---------------------------------------------------------------------------

class Ptr
{
public: // public interface
    Ptr()
        : Ptr(nullptr)
    {
    }

    Ptr(const uint8_t* buffer)
        : _buffer(buffer)
        , _bufptr(buffer)
    {
    }

    Ptr(Ptr&&) = delete;

    Ptr(const Ptr&) = default;

    Ptr& operator=(Ptr&&) = delete;

    Ptr& operator=(const Ptr&) = default;

   ~Ptr() = default;

    auto get() const -> const uint8_t*
    {
        return _bufptr;
    }

    auto set(const uint8_t* buffer) -> void
    {
        _buffer = _bufptr = buffer;
    }

    auto advance(uint32_t offset) -> void
    {
        if(_bufptr != nullptr) {
            _bufptr += offset;
        }
    }

    auto rewind(uint32_t offset) -> void
    {
        if(_bufptr != nullptr) {
            _bufptr -= offset;
        }
    }

    auto fetchByte() -> uint8_t
    {
        uint8_t b = 0;
        if(_bufptr != nullptr) {
            b |= (static_cast<uint8_t>(*_bufptr++) << 0);
        }
        return b;
    }

    auto fetchWordBE() -> uint16_t
    {
        uint16_t w = 0;
        if(_bufptr != nullptr) {
            w |= (static_cast<uint16_t>(*_bufptr++) << 8);
            w |= (static_cast<uint16_t>(*_bufptr++) << 0);
        }
        return w;
    }

    auto fetchWordLE() -> uint16_t
    {
        uint16_t w = 0;
        if(_bufptr != nullptr) {
            w |= (static_cast<uint16_t>(*_bufptr++) << 0);
            w |= (static_cast<uint16_t>(*_bufptr++) << 8);
        }
        return w;
    }

    auto fetchLongBE() -> uint32_t
    {
        uint32_t l = 0;
        if(_bufptr != nullptr) {
            l |= (static_cast<uint32_t>(*_bufptr++) << 24);
            l |= (static_cast<uint32_t>(*_bufptr++) << 16);
            l |= (static_cast<uint32_t>(*_bufptr++) <<  8);
            l |= (static_cast<uint32_t>(*_bufptr++) <<  0);
        }
        return l;
    }

    auto fetchLongLE() -> uint32_t
    {
        uint32_t l = 0;
        if(_bufptr != nullptr) {
            l |= (static_cast<uint32_t>(*_bufptr++) <<  0);
            l |= (static_cast<uint32_t>(*_bufptr++) <<  8);
            l |= (static_cast<uint32_t>(*_bufptr++) << 16);
            l |= (static_cast<uint32_t>(*_bufptr++) << 24);
        }
        return l;
    }

private: // private data
    const uint8_t* _buffer;
    const uint8_t* _bufptr;
};

// ---------------------------------------------------------------------------
// ByteCode
// ---------------------------------------------------------------------------

class ByteCode
{
public: // public interface
    ByteCode()
        : ByteCode(nullptr)
    {
    }

    ByteCode(const uint8_t* buffer)
        : _buffer(buffer)
        , _bufptr(buffer)
        , _yield(false)
    {
    }

    ByteCode(ByteCode&&) = delete;

    ByteCode(const ByteCode&) = default;

    ByteCode& operator=(ByteCode&&) = delete;

    ByteCode& operator=(const ByteCode&) = default;

   ~ByteCode() = default;

    auto get() const -> const uint8_t*
    {
        return _bufptr;
    }

    auto set(const uint8_t* buffer) -> void
    {
        _buffer = _bufptr = buffer;
    }

    auto advance(uint32_t offset) -> void
    {
        if(_bufptr != nullptr) {
            _bufptr += offset;
        }
    }

    auto rewind(uint32_t offset) -> void
    {
        if(_bufptr != nullptr) {
            _bufptr -= offset;
        }
    }

    auto fetchByte() -> uint8_t
    {
        uint8_t b = 0;
        if(_bufptr != nullptr) {
            b |= (static_cast<uint8_t>(*_bufptr++) << 0);
        }
        return b;
    }

    auto fetchWord() -> uint16_t
    {
        uint16_t w = 0;
        if(_bufptr != nullptr) {
            w |= (static_cast<uint16_t>(*_bufptr++) << 8);
            w |= (static_cast<uint16_t>(*_bufptr++) << 0);
        }
        return w;
    }

    auto fetchLong() -> uint32_t
    {
        uint32_t l = 0;
        if(_bufptr != nullptr) {
            l |= (static_cast<uint32_t>(*_bufptr++) << 24);
            l |= (static_cast<uint32_t>(*_bufptr++) << 16);
            l |= (static_cast<uint32_t>(*_bufptr++) <<  8);
            l |= (static_cast<uint32_t>(*_bufptr++) <<  0);
        }
        return l;
    }

    auto yield() -> bool
    {
        return _yield;
    }

    auto yield(bool value) -> void
    {
        _yield = value;
    }

private: // private data
    const uint8_t* _buffer;
    const uint8_t* _bufptr;
    bool           _yield;
};

// ---------------------------------------------------------------------------
// Point
// ---------------------------------------------------------------------------

struct Point
{
    Point()
        : Point(0, 0)
    {
    }

    Point(const int16_t px, const int16_t py)
        : x(px)
        , y(py)
    {
    }

    int16_t x;
    int16_t y;
};

// ---------------------------------------------------------------------------
// Polygon
// ---------------------------------------------------------------------------

struct Polygon
{
    Polygon()
        : bbw(0)
        , bbh(0)
        , count(0)
        , points()
    {
    }

    uint16_t bbw;
    uint16_t bbh;
    uint8_t  count;
    Point    points[50];
};

// ---------------------------------------------------------------------------
// AudioChannel
// ---------------------------------------------------------------------------

struct AudioChannel
{
    uint8_t        id      = 0xff;
    uint8_t        active  = 0;
    uint8_t        volume  = 0;
    uint16_t       resId   = 0xffff;
    const uint8_t* bufPtr  = nullptr;
    uint16_t       bufLen  = 0;
    uint16_t       loopPos = 0;
    uint16_t       loopLen = 0;
    uint32_t       dataPos = 0;
    uint32_t       dataInc = 0;
};

// ---------------------------------------------------------------------------
// AudioSample
// ---------------------------------------------------------------------------

struct AudioSample
{
    uint16_t       resId   = 0xffff;
    const uint8_t* bufPtr  = nullptr;
    uint16_t       bufLen  = 0;
    uint16_t       loopPos = 0;
    uint16_t       loopLen = 0;
};

// ---------------------------------------------------------------------------
// MusicSample
// ---------------------------------------------------------------------------

struct MusicSample
{
    uint8_t* data   = nullptr;
    uint16_t volume = 0;
};

// ---------------------------------------------------------------------------
// MusicModule
// ---------------------------------------------------------------------------

struct MusicModule
{
    uint16_t       resId            = 0xffff;
    const uint8_t* data             = nullptr;
    uint16_t       curPos           = 0;
    uint8_t        curOrder         = 0;
    uint8_t        numOrder         = 0;
    uint8_t        orderTable[0x80] = {0};
    MusicSample    samples[15];
};

// ---------------------------------------------------------------------------
// MusicPattern
// ---------------------------------------------------------------------------

struct MusicPattern
{
    uint16_t note_1       = 0;
    uint16_t note_2       = 0;
    uint16_t sampleStart  = 0;
    uint8_t* sampleBuffer = nullptr;
    uint16_t sampleLen    = 0;
    uint16_t loopPos      = 0;
    uint8_t* loopData     = nullptr;
    uint16_t loopLen      = 0;
    uint16_t sampleVolume = 0;
};

// ---------------------------------------------------------------------------
// End-Of-File
// ---------------------------------------------------------------------------

#endif /* __AW_INTERN_H__ */

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
class Backend;
class Resources;
class Video;
class Audio;
class Mixer;
class Sound;
class Music;
class Input;
class VirtualMachine;

// ---------------------------------------------------------------------------
// some aliases
// ---------------------------------------------------------------------------

using Mutex         = std::mutex;
using LockGuard     = std::lock_guard<std::mutex>;
using AudioCallback = void (*)(void* data, uint8_t* buffer, int length);
using TimerCallback = uint32_t (*)(uint32_t delay, void* param);
using EnginePtr     = std::unique_ptr<Engine>;
using BackendPtr    = std::unique_ptr<Backend>;

// ---------------------------------------------------------------------------
// Library
// ---------------------------------------------------------------------------

class Library
{
public: // public interface
    Library() = default;

    Library(Library&&) = delete;

    Library(const Library&) = delete;

    Library& operator=(Library&&) = delete;

    Library& operator=(const Library&) = delete;

    virtual ~Library() = default;
};

// ---------------------------------------------------------------------------
// SubSystem
// ---------------------------------------------------------------------------

class SubSystem
{
public: // public interface
    SubSystem(Engine& engine, const std::string& subsystem);

    SubSystem(SubSystem&&) = delete;

    SubSystem(const SubSystem&) = delete;

    SubSystem& operator=(SubSystem&&) = delete;

    SubSystem& operator=(const SubSystem&) = delete;

    virtual ~SubSystem();

    virtual auto start() -> void = 0;

    virtual auto reset() -> void = 0;

    virtual auto stop() -> void = 0;

    auto getCurrTicks() -> uint32_t
    {
        return _currTicks;
    }

    auto getPrevTicks() -> uint32_t
    {
        return _prevTicks;
    }

    auto getNextTicks() -> uint32_t
    {
        return _nextTicks;
    }

protected: // protected data
    const std::string _subsystem;
    Engine&           _engine;
    Mutex             _mutex;
    uint32_t          _currTicks;
    uint32_t          _prevTicks;
    uint32_t          _nextTicks;
};

// ---------------------------------------------------------------------------
// Data
// ---------------------------------------------------------------------------

class Data
{
public: // public interface
    Data()
        : Data(nullptr, 0)
    {
    }

    Data(const uint8_t* buffer)
        : Data(buffer, 0)
    {
    }

    Data(const uint8_t* buffer, uint32_t offset)
        : _buffer(buffer)
        , _bufptr(buffer)
    {
        if(_bufptr != nullptr) {
            _bufptr += offset;
        }
    }

    Data(Data&&) = delete;

    Data(const Data&) = default;

    Data& operator=(Data&&) = delete;

    Data& operator=(const Data&) = default;

   ~Data() = default;

    auto get() const -> const uint8_t*
    {
        return _bufptr;
    }

    auto reset() -> void
    {
        _buffer = _bufptr = nullptr;
    }

    auto reset(const uint8_t* buffer) -> void
    {
        _buffer = _bufptr = buffer;
    }

    auto reset(const uint8_t* buffer, uint32_t offset) -> void
    {
        _buffer = _bufptr = buffer;
        if(_bufptr != nullptr) {
            _bufptr += offset;
        }
    }

    auto offset() -> uint32_t
    {
        return (_bufptr - _buffer);
    }

    auto seek(uint32_t offset) -> Data&
    {
        if(_buffer != nullptr) {
            _bufptr = _buffer + offset;
        }
        return *this;
    }

    auto advance(uint32_t offset) -> Data&
    {
        if(_bufptr != nullptr) {
            _bufptr += offset;
        }
        return *this;
    }

    auto rewind(uint32_t offset) -> Data&
    {
        if(_bufptr != nullptr) {
            _bufptr -= offset;
        }
        return *this;
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
        : ByteCode(nullptr, 0)
    {
    }

    ByteCode(const uint8_t* buffer)
        : ByteCode(buffer, 0)
    {
    }

    ByteCode(const uint8_t* buffer, uint32_t offset)
        : _buffer(buffer)
        , _bufptr(buffer)
    {
        if(_bufptr != nullptr) {
            _bufptr += offset;
        }
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

    auto reset() -> void
    {
        _buffer = _bufptr = nullptr;
    }

    auto reset(const uint8_t* buffer) -> void
    {
        _buffer = _bufptr = buffer;
    }

    auto offset() -> uint32_t
    {
        return (_bufptr - _buffer);
    }

    auto seek(uint32_t offset) -> void
    {
        if(_buffer != nullptr) {
            _bufptr = _buffer + offset;
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

private: // private data
    const uint8_t* _buffer;
    const uint8_t* _bufptr;
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
// Color3u8
// ---------------------------------------------------------------------------

struct Color3u8
{
    uint8_t r = 0;
    uint8_t g = 0;
    uint8_t b = 0;
};

// ---------------------------------------------------------------------------
// Palette
// ---------------------------------------------------------------------------

struct Palette
{
    uint8_t  id = 0;
    Color3u8 data[16];
};

// ---------------------------------------------------------------------------
// Page
// ---------------------------------------------------------------------------

struct Page
{
    uint8_t id = 0;
    uint8_t data[((320 / 2) * 200)];
};

// ---------------------------------------------------------------------------
// AudioChannel
// ---------------------------------------------------------------------------

struct AudioChannel
{
    uint8_t        channel_id = 0xff;
    uint8_t        active     = 0;
    uint8_t        volume     = 0;
    uint16_t       sample_id  = 0xffff;
    const uint8_t* data_ptr   = nullptr;
    uint32_t       data_len   = 0;
    uint32_t       data_pos   = 0;
    uint32_t       data_inc   = 0;
    uint32_t       loop_pos   = 0;
    uint32_t       loop_len   = 0;
};

// ---------------------------------------------------------------------------
// AudioSample
// ---------------------------------------------------------------------------

struct AudioSample
{
    uint16_t       sample_id = 0xffff;
    uint16_t       frequency = 0;
    uint8_t        volume    = 0;
    const uint8_t* data_ptr  = nullptr;
    uint32_t       data_len  = 0;
    uint32_t       loop_pos  = 0;
    uint32_t       loop_len  = 0;
    uint16_t       unused1   = 0;
    uint16_t       unused2   = 0;
};

// ---------------------------------------------------------------------------
// MusicPattern
// ---------------------------------------------------------------------------

struct MusicPattern
{
    uint16_t word1 = 0;
    uint16_t word2 = 0;
};

// ---------------------------------------------------------------------------
// MusicModule
// ---------------------------------------------------------------------------

struct MusicModule
{
    uint16_t       music_id        = 0xffff;
    uint16_t       music_ticks     = 0;
    const uint8_t* data_ptr        = nullptr;
    uint32_t       data_pos        = 0;
    uint8_t        seq_index       = 0;
    uint8_t        seq_count       = 0;
    uint8_t        seq_table[0x80] = {0};
    AudioSample    samples[15];
};

// ---------------------------------------------------------------------------
// Controls
// ---------------------------------------------------------------------------

struct Controls
{
    static constexpr uint16_t DPAD_RIGHT  = (1 << 0);
    static constexpr uint16_t DPAD_LEFT   = (1 << 1);
    static constexpr uint16_t DPAD_DOWN   = (1 << 2);
    static constexpr uint16_t DPAD_UP     = (1 << 3);
    static constexpr uint16_t DPAD_BUTTON = (1 << 7);

    uint16_t mask  = 0;
    int16_t  horz  = 0;
    int16_t  vert  = 0;
    int16_t  btns  = 0;
    uint8_t  input = 0;
    bool     quit  = false;
    bool     pause = false;
};

// ---------------------------------------------------------------------------
// Paula
// ---------------------------------------------------------------------------

struct Paula
{
    static constexpr uint32_t Frequency = 7159090;
    static constexpr uint32_t Carrier   = Frequency / 2;

    static const uint16_t frequencyTable[40];
};

// ---------------------------------------------------------------------------
// GameParts
// ---------------------------------------------------------------------------

enum GamePartId
{
    GAME_PART_FIRST = 0x3e80,
    GAME_PART0      = 0x3e80,
    GAME_PART1      = 0x3e81,
    GAME_PART2      = 0x3e82,
    GAME_PART3      = 0x3e83,
    GAME_PART4      = 0x3e84,
    GAME_PART5      = 0x3e85,
    GAME_PART6      = 0x3e86,
    GAME_PART7      = 0x3e87,
    GAME_PART8      = 0x3e88,
    GAME_PART9      = 0x3e89,
    GAME_PART_LAST  = 0x3e89,
    GAME_NUM_PARTS  = 10,
};

struct GamePart
{
    const char* name     = nullptr;
    uint16_t    palettes = 0;
    uint16_t    bytecode = 0;
    uint16_t    polygon1 = 0;
    uint16_t    polygon2 = 0;
};

struct GameParts
{
    static const GamePart data[GAME_NUM_PARTS];
};

// ---------------------------------------------------------------------------
// End-Of-File
// ---------------------------------------------------------------------------

#endif /* __AW_INTERN_H__ */

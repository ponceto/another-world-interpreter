/*
 * music.h - Copyright (c) 2004-2025
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
#ifndef __AW_MUSIC_H__
#define __AW_MUSIC_H__

#include "intern.h"

// ---------------------------------------------------------------------------
// Music
// ---------------------------------------------------------------------------

class Music final
    : public SubSystem
{
public: // public interface
    Music(Engine& engine);

    Music(Music&&) = delete;

    Music(const Music&) = delete;

    Music& operator=(Music&&) = delete;

    Music& operator=(const Music&) = delete;

    virtual ~Music() = default;

    virtual auto init() -> void override final;

    virtual auto fini() -> void override final;

public: // public music interface
    auto playMusicModule() -> void;

    auto stopMusicModule() -> void;

    auto setMusicModuleDelay(uint16_t delay) -> void;

    auto loadMusicModule(uint16_t resource, uint16_t delay, uint8_t position) -> void;

private: // private interface
    auto startMusic() -> void;

    auto stopMusic() -> void;

    auto processMusic() -> uint32_t;

    auto prepareSamples(const uint8_t* buffer) -> void;

    auto handlePattern(uint8_t channel, const uint8_t* buffer) -> void;

private: // private data
    MusicModule _module;
    uint32_t    _delay;
    int         _timer;
};

// ---------------------------------------------------------------------------
// End-Of-File
// ---------------------------------------------------------------------------

#endif /* __AW_MUSIC_H__ */

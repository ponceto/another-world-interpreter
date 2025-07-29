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
    Music(Engine& engine, Audio& audio);

    Music(Music&&) = delete;

    Music(const Music&) = delete;

    Music& operator=(Music&&) = delete;

    Music& operator=(const Music&) = delete;

    virtual ~Music() = default;

    virtual auto start() -> void override final;

    virtual auto reset() -> void override final;

    virtual auto stop() -> void override final;

public: // public music interface
    auto playMusic(uint16_t music_id, uint8_t index, uint16_t ticks) -> void;

    auto stopMusic() -> void;

private: // private interface
    auto startTimer() -> void;

    auto resetTimer() -> void;

    auto stopTimer() -> void;

    auto processTimer() -> uint32_t;

    auto playMusicUnlocked(uint16_t id, uint8_t index, uint16_t ticks) -> void;

    auto stopMusicUnlocked() -> void;

    auto processPattern(uint8_t channel, Data& data) -> void;

private: // private data
    Audio&      _audio;
    MusicModule _module;
    int         _timer;
};

// ---------------------------------------------------------------------------
// End-Of-File
// ---------------------------------------------------------------------------

#endif /* __AW_MUSIC_H__ */

/*
 * sound.h - Copyright (c) 2004-2025
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
#ifndef __AW_SOUND_H__
#define __AW_SOUND_H__

#include "intern.h"

// ---------------------------------------------------------------------------
// Sound
// ---------------------------------------------------------------------------

class Sound final
    : public SubSystem
{
public: // public interface
    Sound(Engine& engine, Audio& audio);

    Sound(Sound&&) = delete;

    Sound(const Sound&) = delete;

    Sound& operator=(Sound&&) = delete;

    Sound& operator=(const Sound&) = delete;

    virtual ~Sound() = default;

    virtual auto start() -> void override final;

    virtual auto reset() -> void override final;

    virtual auto stop() -> void override final;

public: // public sound interface
    auto playSound(uint16_t sound_id, uint8_t channel, uint8_t volume, uint8_t pitch) -> void;

    auto stopSound() -> void;

private: // private interface
    auto startTimer() -> void;

    auto resetTimer() -> void;

    auto stopTimer() -> void;

    auto processTimer() -> uint32_t;

private: // private data
    Audio&      _audio;
    AudioSample _samples[4];
    int         _timer;
};

// ---------------------------------------------------------------------------
// End-Of-File
// ---------------------------------------------------------------------------

#endif /* __AW_SOUND_H__ */

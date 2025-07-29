/*
 * audio.h - Copyright (c) 2004-2025
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
#ifndef __AW_AUDIO_H__
#define __AW_AUDIO_H__

#include "intern.h"
#include "mixer.h"
#include "sound.h"
#include "music.h"

// ---------------------------------------------------------------------------
// Audio
// ---------------------------------------------------------------------------

class Audio final
    : public SubSystem
{
public: // public interface
    Audio(Engine& engine);

    Audio(Audio&&) = delete;

    Audio(const Audio&) = delete;

    Audio& operator=(Audio&&) = delete;

    Audio& operator=(const Audio&) = delete;

    virtual ~Audio() = default;

    virtual auto start() -> void override final;

    virtual auto reset() -> void override final;

    virtual auto stop() -> void override final;

public: // public audio interface
    auto playAllChannels() -> void
    {
        return _mixer.playAllChannels();
    }

    auto stopAllChannels() -> void
    {
        return _mixer.stopAllChannels();
    }

    auto playChannel(uint8_t channel, const AudioSample& sample) -> void
    {
        return _mixer.playChannel(channel, sample);
    }

    auto stopChannel(uint8_t channel) -> void
    {
        return _mixer.stopChannel(channel);
    }

    auto setChannelVolume(uint8_t channel, uint8_t volume) -> void
    {
        return _mixer.setChannelVolume(channel, volume);
    }

    auto playSound(uint16_t id, uint8_t channel, uint8_t volume, uint8_t frequency) -> void
    {
        return _sound.playSound(id, channel, volume, frequency);
    }

    auto playMusic(uint16_t id, uint8_t position, uint16_t delay) -> void
    {
        return _music.playMusic(id, position, delay);
    }

private: // private data
    Mixer _mixer;
    Sound _sound;
    Music _music;
};

// ---------------------------------------------------------------------------
// End-Of-File
// ---------------------------------------------------------------------------

#endif /* __AW_AUDIO_H__ */

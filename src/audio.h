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

    virtual auto init() -> void override final;

    virtual auto fini() -> void override final;

public: // public audio interface
    auto playAllAudioChannels() -> void;

    auto stopAllAudioChannels() -> void;

    auto playAudioChannel(uint8_t channel, const AudioSample& sample, uint16_t frequency, uint8_t volume) -> void;

    auto stopAudioChannel(uint8_t channel) -> void;

    auto setAudioChannelVolume(uint8_t channel, uint8_t volume) -> void;

private: // private interface
    auto startAudio() -> void;

    auto stopAudio() -> void;

    auto processAudio(float* buffer, int length) -> void;

private: // private data
    AudioChannel _channels[4];
};

// ---------------------------------------------------------------------------
// End-Of-File
// ---------------------------------------------------------------------------

#endif /* __AW_AUDIO_H__ */

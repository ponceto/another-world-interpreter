/*
 * sound.cc - Copyright (c) 2004-2025
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
#include "engine.h"
#include "sound.h"

// ---------------------------------------------------------------------------
// Sound
// ---------------------------------------------------------------------------

Sound::Sound(Engine& engine, Audio& audio)
    : SubSystem(engine, "Sound")
    , _audio(audio)
    , _samples()
{
}

auto Sound::start() -> void
{
    log_debug(SYS_SOUND, "starting...");
    log_debug(SYS_SOUND, "started!");
}

auto Sound::reset() -> void
{
    log_debug(SYS_SOUND, "resetting...");
    for(auto& sample : _samples) {
        sample = AudioSample();
    }
    log_debug(SYS_SOUND, "reset!");
}

auto Sound::stop() -> void
{
    log_debug(SYS_SOUND, "stopping...");
    log_debug(SYS_SOUND, "stopped!");
}

auto Sound::playSound(uint16_t sound_id, uint8_t channel, uint8_t volume, uint8_t frequency) -> void
{
    log_debug(SYS_SOUND, "play sound [sound_id: 0x%02x, channel: %d, volume: %d, frequency: %d]", sound_id, channel, volume, frequency);

    Resource* resource = _engine.getResource(sound_id);

    if(resource == nullptr) {
        log_alert("resource not found [sound_id: 0x%02x]", sound_id);
    }
    else if(resource->type != RT_SOUND) {
        log_alert("resource is invalid [sound_id: 0x%02x]", sound_id);
    }
    else if(resource->state != RS_LOADED) {
        log_alert("resource not loaded [sound_id: 0x%02x]", sound_id);
    }
    else if(volume == 0) {
        _audio.stopChannel(channel);
    }
    else {
        Ptr sndptr(resource->data);
        const uint32_t data_len = static_cast<uint32_t>(sndptr.fetchWordBE()) * 2;
        const uint32_t loop_len = static_cast<uint32_t>(sndptr.fetchWordBE()) * 2;
        const uint16_t unused1  = sndptr.fetchWordBE();
        const uint16_t unused2  = sndptr.fetchWordBE();
        const uint8_t* data_ptr = sndptr.get();
        auto& sample(_samples[channel & 3]);
        sample.sample_id = sound_id;
        sample.data_ptr  = data_ptr;
        sample.data_len  = data_len;
        sample.loop_pos  = 0;
        sample.loop_len  = 0;
        sample.unused1   = unused1;
        sample.unused2   = unused2;
        sample.volume    = volume;
        if(loop_len != 0) {
            sample.data_len += loop_len;
            sample.loop_pos += data_len;
            sample.loop_len += loop_len;
        }
        if(frequency < countof(Paula::frequencyTable)) {
            _audio.playChannel(channel, sample, Paula::frequencyTable[frequency], (volume < 0x3f ? volume : 0x3f));
        }
        else {
            log_error("cannot play sound, invalid frequency [sound_id: 0x%02x, frequency: %d]", sound_id, frequency);
        }
    }
}

// ---------------------------------------------------------------------------
// End-Of-File
// ---------------------------------------------------------------------------

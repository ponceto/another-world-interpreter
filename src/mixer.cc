/*
 * mixer.cc - Copyright (c) 2004-2025
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
#include "mixer.h"

// ---------------------------------------------------------------------------
// Mixer
// ---------------------------------------------------------------------------

Mixer::Mixer(Engine& engine, Audio& audio)
    : SubSystem(engine, "Mixer")
    , _audio(audio)
    , _channels()
    , _samplerate(engine.getAudioSampleRate())
{
    int count = 0;
    for(auto& channel : _channels) {
        channel.channel_id = count++;
    }
}

auto Mixer::start() -> void
{
    log_debug(SYS_MIXER, "starting...");
    stopAllChannels();
    log_debug(SYS_MIXER, "started!");
}

auto Mixer::reset() -> void
{
    log_debug(SYS_MIXER, "resetting...");
    stopAllChannels();
    log_debug(SYS_MIXER, "reset!");
}

auto Mixer::stop() -> void
{
    log_debug(SYS_MIXER, "stopping...");
    stopAllChannels();
    log_debug(SYS_MIXER, "stopped!");
}

auto Mixer::mixAllChannels(float* buffer, int length) -> void
{
    const LockGuard lock(_mutex);

    auto add_clamp = [](const float a, const float b) -> float
    {
        float value = a + b;
        if(value < -1.0f) value = -1.0f;
        if(value > +1.0f) value = +1.0f;
        return value;
    };

    auto mix_one_channel = [&](AudioChannel& channel) -> void
    {
        float* bufptr = buffer;
        auto const volume   = channel.volume;
        auto const data_ptr = channel.data_ptr;
        auto const data_end = channel.data_len;
        auto       curr_pos = channel.data_pos;
        auto       next_pos = channel.data_pos;
        auto const data_inc = channel.data_inc;
        auto const loop_pos = channel.loop_pos;
        auto const loop_len = channel.loop_len;
        auto const loop_end = loop_pos + loop_len;
        for(int count = length; count != 0; --count) {
            curr_pos  = next_pos;
            next_pos += data_inc;
            uint32_t p1 = curr_pos >> 8;
            uint32_t p2 = p1 + 1;
            if(loop_len == 0) {
                if(p2 >= data_end) {
                    channel.active = false;
                    next_pos = 0;
                    break;
                }
            }
            else {
                if(p2 >= loop_end) {
                    p2 = loop_pos;
                    next_pos = (loop_pos << 8);
                }
            }
            const int32_t ic = (curr_pos & 0xff);                 // retrieve reminder
            const int32_t i1 = (0xff - ic);                       // compute 1st factor
            const int32_t i2 = (0x00 + ic);                       // compute 2nd factor
            const int32_t s1 = static_cast<int8_t>(data_ptr[p1]); // 1st sample
            const int32_t s2 = static_cast<int8_t>(data_ptr[p2]); // 2nd sample
            const int32_t s3 = ((s1 * i1) + (s2 * i2)) >> 8;      // interpolate samples
            const float sample = static_cast<float>(s3 * volume) / (63.0f * 128.0f * 4.0f);
            *bufptr = add_clamp(*bufptr, sample);
            ++bufptr;
        }
        channel.data_pos = next_pos;
    };

    auto mix_all_channels = [&]() -> void
    {
        if(_engine.isPaused()) {
            return;
        }
        for(auto& channel : _channels) {
            if((channel.active != 0) && (channel.data_ptr != nullptr)) {
                mix_one_channel(channel);
            }
        }
    };

    return mix_all_channels();
}

auto Mixer::playAllChannels() -> void
{
    const LockGuard lock(_mutex);

    for(auto& channel : _channels) {
        log_debug(SYS_MIXER, "play channel [channel: %d]", channel.channel_id);
        channel.active = true;
    }
}

auto Mixer::stopAllChannels() -> void
{
    const LockGuard lock(_mutex);

    for(auto& channel : _channels) {
        log_debug(SYS_MIXER, "stop channel [channel: %d]", channel.channel_id);
        channel.active = false;
    }
}

auto Mixer::playChannel(uint8_t index, const AudioSample& sample, uint16_t frequency, uint8_t volume) -> void
{
    const LockGuard lock(_mutex);

    if(index < countof(_channels)) {
        AudioChannel& channel(_channels[index]);
        log_debug(SYS_MIXER, "play channel [channel: %d, sample: 0x%02x, frequency: %d, volume: %d]", channel.channel_id, sample.sample_id, frequency, volume);
        channel.active    = true;
        channel.volume    = volume;
        channel.sample_id = sample.sample_id;
        channel.data_ptr  = sample.data_ptr;
        channel.data_len  = static_cast<uint32_t>(sample.data_len);
        channel.data_pos  = static_cast<uint32_t>(0);
        channel.data_inc  = (static_cast<uint32_t>(frequency) << 8) / _samplerate;
        channel.loop_pos  = static_cast<uint32_t>(sample.loop_pos);
        channel.loop_len  = static_cast<uint32_t>(sample.loop_len);
    }
}

auto Mixer::stopChannel(uint8_t index) -> void
{
    const LockGuard lock(_mutex);

    if(index < countof(_channels)) {
        AudioChannel& channel = _channels[index];
        log_debug(SYS_MIXER, "stop channel [channel: %d]", channel.channel_id);
        channel.active = false;
    }
}

auto Mixer::setChannelVolume(uint8_t index, uint8_t volume) -> void
{
    const LockGuard lock(_mutex);

    if(index < countof(_channels)) {
        AudioChannel& channel = _channels[index];
        log_debug(SYS_MIXER, "set channel volume [channel: %d, volume: %d]", channel.channel_id);
        channel.volume = volume;
    }
}

// ---------------------------------------------------------------------------
// End-Of-File
// ---------------------------------------------------------------------------

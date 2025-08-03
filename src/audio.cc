/*
 * audio.cc - Copyright (c) 2004-2025
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
#include "audio.h"

// ---------------------------------------------------------------------------
// Audio
// ---------------------------------------------------------------------------

Audio::Audio(Engine& engine)
    : SubSystem(engine)
    , _channels()
{
    int count = 0;
    for(auto& channel : _channels) {
        channel.id = count++;
    }
}

auto Audio::init() -> void
{
    log_debug(SYS_AUDIO, "initializing...");
    stopAllAudioChannels();
    startAudio();
    log_debug(SYS_AUDIO, "initialized!");
}

auto Audio::fini() -> void
{
    log_debug(SYS_AUDIO, "finalizing...");
    stopAllAudioChannels();
    stopAudio();
    log_debug(SYS_AUDIO, "finalized!");
}

auto Audio::playAllAudioChannels() -> void
{
    const LockGuard lock(_mutex);

    for(auto& channel : _channels) {
        log_debug(SYS_AUDIO, "play channel [channel: %d]", channel.id);
        channel.active = true;
    }
}

auto Audio::stopAllAudioChannels() -> void
{
    const LockGuard lock(_mutex);

    for(auto& channel : _channels) {
        log_debug(SYS_AUDIO, "stop channel [channel: %d]", channel.id);
        channel.active = false;
    }
}

auto Audio::playAudioChannel(uint8_t index, const AudioSample& sample, uint16_t frequency, uint8_t volume) -> void
{
    const LockGuard lock(_mutex);

    if(index < countof(_channels)) {
        AudioChannel& channel(_channels[index]);
        log_debug(SYS_AUDIO, "play sample [channel: %d, sample: 0x%04x, frequency: %d, volume: %d]", channel.id, sample.resId, frequency, volume);
        channel.active  = true;
        channel.volume  = volume;
        channel.resId   = sample.resId;
        channel.bufPtr  = sample.bufPtr;
        channel.bufLen  = sample.bufLen;
        channel.loopPos = sample.loopPos;
        channel.loopLen = sample.loopLen;
        channel.dataPos = 0;
        channel.dataInc = (frequency << 8) / _engine.getAudioSampleRate();
    }
}

auto Audio::stopAudioChannel(uint8_t index) -> void
{
    const LockGuard lock(_mutex);

    if(index < countof(_channels)) {
        AudioChannel& channel = _channels[index];
        log_debug(SYS_AUDIO, "stop channel [channel: %d]", channel.id);
        channel.active = false;
    }
}

auto Audio::setAudioChannelVolume(uint8_t index, uint8_t volume) -> void
{
    const LockGuard lock(_mutex);

    if(index < countof(_channels)) {
        AudioChannel& channel = _channels[index];
        log_debug(SYS_AUDIO, "set volume [channel: %d, volume: %d]", channel.id);
        channel.volume = volume;
    }
}

auto Audio::startAudio() -> void
{
    const LockGuard lock(_mutex);

    auto callback = +[](void* data, uint8_t* buffer, int length) -> void
    {
        static_cast<void>(::memset(buffer, 0, length));

        reinterpret_cast<Audio*>(data)->processAudio(reinterpret_cast<float*>(buffer), (length / sizeof(float)));
    };

    log_debug(SYS_AUDIO, "start audio");

    return _engine.startAudio(callback, this);
}

auto Audio::stopAudio() -> void
{
    const LockGuard lock(_mutex);

    log_debug(SYS_AUDIO, "stop audio");

    return _engine.stopAudio();
}

auto Audio::processAudio(float* buffer, int length) -> void
{
    const LockGuard lock(_mutex);

    auto addClamp = [](const float a, const float b) -> float
    {
        float value = a + b;
        if(value < -1.0f) value = -1.0f;
        if(value > +1.0f) value = +1.0f;
        return value;
    };

    auto processOneChannel = [&](AudioChannel& channel) -> void
    {
        float* bufptr = buffer;
        for(int index = 0; index < length; ++index) {
            uint16_t p1  = channel.dataPos >> 8;
            uint16_t p2  = p1 + 1;
            channel.dataPos += channel.dataInc;
            if(channel.loopLen == 0) {
                if(p2 >= channel.bufLen) {
                    channel.active = false;
                    break;
                }
            }
            else {
                if(p2 >= (channel.loopPos + channel.loopLen)) {
                    p2 = channel.dataPos = channel.loopPos;
                }
            }
            const int32_t ic = (channel.dataPos & 0xff);                              // retrieve reminder
            const int32_t i1 = (0xff - ic);                                           // compute 1st factor
            const int32_t i2 = (0x00 + ic);                                           // compute 2nd factor
            const int32_t s1 = *reinterpret_cast<const int8_t*>(channel.bufPtr + p1); // 1st sample
            const int32_t s2 = *reinterpret_cast<const int8_t*>(channel.bufPtr + p2); // 2nd sample
            const int32_t s3 = ((s1 * i1) + (s2 * i2)) >> 8;                          // interpolate samples
            const float sample = static_cast<float>(s3 * channel.volume) / (64.0f * 127.0f);
            *bufptr = addClamp(*bufptr, sample);
            ++bufptr;
        }
    };

    auto processAllChannels = [&]() -> void
    {
        if(_engine.paused()) {
            return;
        }
        for(auto& channel : _channels) {
            if((channel.active != 0) && (channel.bufPtr != nullptr)) {
                processOneChannel(channel);
            }
        }
    };

    return processAllChannels();
}

// ---------------------------------------------------------------------------
// End-Of-File
// ---------------------------------------------------------------------------

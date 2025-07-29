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
// some useful macros
// ---------------------------------------------------------------------------

#ifndef NDEBUG
#define LOG_DEBUG(format, ...) log_debug(SYS_SOUND, format, ##__VA_ARGS__)
#else
#define LOG_DEBUG(format, ...) do {} while(0)
#endif

// ---------------------------------------------------------------------------
// Sound
// ---------------------------------------------------------------------------

Sound::Sound(Engine& engine, Audio& audio)
    : SubSystem(engine, "Sound")
    , _audio(audio)
    , _samples()
    , _timer(0)
{
}

auto Sound::start() -> void
{
    LOG_DEBUG("starting...");
    stopSound();
    startTimer();
    LOG_DEBUG("started!");
}

auto Sound::reset() -> void
{
    LOG_DEBUG("resetting...");
    stopSound();
    resetTimer();
    LOG_DEBUG("reset!");
}

auto Sound::stop() -> void
{
    LOG_DEBUG("stopping...");
    stopSound();
    stopTimer();
    LOG_DEBUG("stopped!");
}

auto Sound::playSound(uint16_t sound_id, uint8_t channel, uint8_t volume, uint8_t pitch) -> void
{
    const LockGuard lock(_mutex);

    LOG_DEBUG("play sound [sound_id: 0x%02x, channel: %d, volume: %d, pitch: %d]", sound_id, channel, volume, pitch);

    auto get_frequency = [](uint8_t pitch) -> uint16_t
    {
        if(pitch >= countof(Paula::frequencyTable)) {
            pitch = 0;
        }
        return Paula::frequencyTable[pitch];
    };

    auto get_volume = [](uint8_t volume) -> uint8_t
    {
        if(volume > 0x3f) {
            volume = 0x3f;
        }
        return volume;
    };

    auto play_sound = [&](Resource* resource) -> void
    {
        if(resource == nullptr) {
            log_alert("resource not found [sound_id: 0x%02x]", sound_id);
        }
        else if(resource->type != RT_SOUND) {
            log_alert("resource is invalid [sound_id: 0x%02x]", sound_id);
        }
        else if(resource->state != RS_LOADED) {
            log_alert("resource not loaded [sound_id: 0x%02x]", sound_id);
        }
        else {
            Data data(resource->data);
            const uint32_t data_len = static_cast<uint32_t>(data.fetchWordBE()) * 2;
            const uint32_t loop_len = static_cast<uint32_t>(data.fetchWordBE()) * 2;
            const uint16_t unused1  = data.fetchWordBE();
            const uint16_t unused2  = data.fetchWordBE();
            const uint8_t* data_ptr = data.get();
            auto& sample(_samples[channel & 3]);
            sample.sample_id = sound_id;
            sample.frequency = get_frequency(pitch);
            sample.volume    = get_volume(volume);
            sample.data_ptr  = data_ptr;
            sample.data_len  = data_len;
            sample.loop_pos  = 0;
            sample.loop_len  = 0;
            sample.unused1   = unused1;
            sample.unused2   = unused2;
            if(loop_len != 0) {
                sample.data_len += loop_len;
                sample.loop_pos += data_len;
                sample.loop_len += loop_len;
            }
        }
    };

    return play_sound(_engine.getResource(sound_id));
}

auto Sound::stopSound() -> void
{
    const LockGuard lock(_mutex);

    for(auto& sample : _samples) {
        sample = AudioSample();
    }
}

auto Sound::startTimer() -> void
{
    const LockGuard lock(_mutex);

    auto callback = +[](uint32_t interval, void* data) -> uint32_t
    {
        return reinterpret_cast<Sound*>(data)->processTimer();
    };

    if(_timer == 0) {
        _currTicks = _engine.getTicks();
        _nextTicks = _currTicks + 20;
        _prevTicks = 0;
        _timer = _engine.addTimer(callback, (_nextTicks - _prevTicks), this);
    }
}

auto Sound::resetTimer() -> void
{
    const LockGuard lock(_mutex);

    if(_timer != 0) {
        _currTicks = _engine.getTicks();
        _nextTicks = _currTicks + 20;
        _prevTicks = 0;
    }
}

auto Sound::stopTimer() -> void
{
    const LockGuard lock(_mutex);

    if(_timer != 0) {
        _currTicks = 0;
        _nextTicks = 0;
        _prevTicks = 0;
        _timer = (_engine.removeTimer(_timer), 0);
    }
}

auto Sound::processTimer() -> uint32_t
{
    const LockGuard lock(_mutex);

    auto is_ready = [&]() -> bool
    {
        _currTicks = _engine.getTicks();
        if(_engine.isStopped() || _engine.isPaused()) {
            _nextTicks = _currTicks + 100;
            return false;
        }
        if(_currTicks >= _nextTicks) {
            _prevTicks = _nextTicks;
            return true;
        }
        return false;
    };

    auto process = [&]() -> void
    {
        int channel = 0;
        for(auto& sample : _samples) {
            if(sample.sample_id != 0xffff) {
                if(sample.volume != 0) {
                    _audio.playChannel(channel, sample);
                }
                else {
                    _audio.stopChannel(channel);
                }
                sample = AudioSample();
            }
            ++channel;
        }
    };

    auto schedule = [&]() -> void
    {
        const uint32_t delay = 20;

        _currTicks = _engine.getTicks();
        _nextTicks = _prevTicks + delay;

        if(_nextTicks <= _currTicks) {
            _nextTicks = _currTicks + 1;
        }
    };

    auto delay = [&]() -> uint32_t
    {
        return _nextTicks - _currTicks;
    };

    if(is_ready()) {
        process();
        schedule();
    }
    return delay();
}

// ---------------------------------------------------------------------------
// End-Of-File
// ---------------------------------------------------------------------------

/*
 * music.cc - Copyright (c) 2004-2025
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
#include "music.h"

// ---------------------------------------------------------------------------
// some useful macros
// ---------------------------------------------------------------------------

#ifndef NDEBUG
#define LOG_DEBUG(format, ...) log_debug(SYS_MUSIC, format, ##__VA_ARGS__)
#else
#define LOG_DEBUG(format, ...) do {} while(0)
#endif

// ---------------------------------------------------------------------------
// Music
// ---------------------------------------------------------------------------

Music::Music(Engine& engine, Audio& audio)
    : SubSystem(engine, "Music")
    , _audio(audio)
    , _module()
    , _timer(0)
{
}

auto Music::start() -> void
{
    LOG_DEBUG("starting...");
    stopMusic();
    startTimer();
    LOG_DEBUG("started!");
}

auto Music::reset() -> void
{
    LOG_DEBUG("resetting...");
    stopMusic();
    resetTimer();
    LOG_DEBUG("reset!");
}

auto Music::stop() -> void
{
    LOG_DEBUG("stopping...");
    stopMusic();
    stopTimer();
    LOG_DEBUG("stopped!");
}

auto Music::playMusic(uint16_t music_id, uint8_t index, uint16_t ticks) -> void
{
    const LockGuard lock(_mutex);

    if(music_id == 0x0000) {
        music_id = 0xffff;
    }
    if(music_id != 0xffff) {
        stopMusicUnlocked();
        playMusicUnlocked(music_id, index, ticks);
    }
    else if((index != 0) || (ticks != 0)) {
        if(index != 0) {
            _module.seq_index = index;
        }
        if(ticks != 0) {
            _module.music_ticks = ticks;
        }
    }
    else {
        stopMusicUnlocked();
    }
}

auto Music::stopMusic() -> void
{
    const LockGuard lock(_mutex);

    stopMusicUnlocked();
}

auto Music::startTimer() -> void
{
    const LockGuard lock(_mutex);

    auto callback = +[](uint32_t interval, void* data) -> uint32_t
    {
        return reinterpret_cast<Music*>(data)->processTimer();
    };

    if(_timer == 0) {
        _currTicks = _engine.getTicks();
        _nextTicks = _currTicks + 20;
        _prevTicks = 0;
        _timer = _engine.addTimer(callback, (_nextTicks - _prevTicks), this);
    }
}

auto Music::resetTimer() -> void
{
    const LockGuard lock(_mutex);

    if(_timer != 0) {
        _currTicks = _engine.getTicks();
        _nextTicks = _currTicks + 20;
        _prevTicks = 0;
    }
}

auto Music::stopTimer() -> void
{
    const LockGuard lock(_mutex);

    if(_timer != 0) {
        _currTicks = 0;
        _nextTicks = 0;
        _prevTicks = 0;
        _timer = (_engine.removeTimer(_timer), 0);
    }
}

auto Music::processTimer() -> uint32_t
{
    const LockGuard lock(_mutex);

    auto is_ready = [&]() -> bool
    {
        _currTicks = _engine.getTicks();
        if(_engine.isStopped() || _engine.isPaused()) {
            _nextTicks = _currTicks + 100;
            return false;
        }
        if(_module.music_id == 0xffff) {
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
        if(_module.seq_index < _module.seq_count) {
            const uint8_t sequence = _module.seq_table[_module.seq_index];
            LOG_DEBUG("process music [music_id: 0x%02x, sequence: 0x%02x, position: 0x%04x]", _module.music_id, sequence, _module.data_pos);
            Data data(_module.data_ptr + _module.data_pos + sequence * 1024);
            for(uint8_t channel = 0; channel < 4; ++channel) {
                processPattern(channel, data);
            }
            _module.data_pos += data.offset();
            if(_module.data_pos >= 1024) {
                _module.data_pos = 0;
                const uint8_t seq_index = _module.seq_index + 1;
                const uint8_t seq_count = _module.seq_count;
                if(seq_index >= seq_count) {
                    LOG_DEBUG("music is over [music_id: 0x%02x]", _module.music_id);
                    stopMusicUnlocked();
                }
                else {
                    _module.seq_index = seq_index;
                }
            }
        }
        else {
            LOG_DEBUG("music is over [music_id: 0x%02x]", _module.music_id);
            stopMusicUnlocked();
        }
    };

    auto schedule = [&]() -> void
    {
        constexpr uint32_t hfreq = 15625;
        constexpr uint32_t bpm   = 125;
        const uint32_t     ticks = (_module.music_ticks != 0 ? _module.music_ticks : hfreq);
        const uint32_t     delay = ((ticks * bpm) / hfreq);

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

auto Music::playMusicUnlocked(uint16_t music_id, uint8_t index, uint16_t ticks) -> void
{
    LOG_DEBUG("play music [music_id: 0x%02x, index: %d, ticks: %d]", music_id, index, ticks);

    auto get_frequency = [](uint16_t period) -> uint16_t
    {
        if(period < 55) {
            return 65535;
        }
        return static_cast<uint16_t>(Paula::Carrier / period);
    };

    auto get_volume = [](uint8_t volume) -> uint8_t
    {
        if(volume > 0x3f) {
            volume = 0x3f;
        }
        return volume;
    };

    auto load_module = [&](Data& data) -> void
    {
        _module             = MusicModule();
        _module.music_id    = music_id;
        _module.music_ticks = data.seek(0x00).fetchWordBE();
        _module.data_ptr    = data.seek(0xc0).get();
        _module.data_pos    = 0;
        _module.seq_index   = index;
        _module.seq_count   = data.seek(0x3e).fetchWordBE();
        if(ticks != 0) {
            _module.music_ticks = ticks;
        }
    };

    auto load_samples = [&](Data& data) -> void
    {
        data.seek(0x02);
        for(auto& sample : _module.samples) {
            const uint16_t sample_id = data.fetchWordBE();
            const uint16_t volume    = data.fetchWordBE();
            if(sample_id != 0) {
                LOG_DEBUG("load sample [sound_id: 0x%02x, volume: %d]", sample_id, volume);
                Resource* resource = _engine.getResource(sample_id);
                if(resource == nullptr) {
                    log_alert("resource not found [sound_id: 0x%02x]", sample_id);
                }
                else if(resource->type != RT_SOUND) {
                    log_alert("resource is invalid [sound_id: 0x%02x]", sample_id);
                }
                else if(resource->state != RS_LOADED) {
                    log_alert("resource not loaded [sound_id: 0x%02x]", sample_id);
                }
                else {
                    Data data(resource->data);
                    const uint32_t data_len = static_cast<uint32_t>(data.fetchWordBE()) * 2;
                    const uint32_t loop_len = static_cast<uint32_t>(data.fetchWordBE()) * 2;
                    const uint16_t unused1  = data.fetchWordBE();
                    const uint16_t unused2  = data.fetchWordBE();
                    const uint8_t* data_ptr = data.get();
                    sample.sample_id = sample_id;
                    sample.frequency = get_frequency(109);
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
            }
        }
    };

    auto load_seq_table = [&](Data& data) -> void
    {
        data.seek(0x40);
        for(auto& sequence : _module.seq_table) {
            sequence = data.fetchByte();
        }
    };

    auto play_music = [&](Resource* resource) -> void
    {
        if(resource == nullptr) {
            log_alert("resource not found [music_id: 0x%02x]", music_id);
        }
        else if(resource->type != RT_MUSIC) {
            log_alert("resource is invalid [music_id: 0x%02x]", music_id);
        }
        else if(resource->state != RS_LOADED) {
            log_alert("resource not loaded [music_id: 0x%02x]", music_id);
        }
        else {
            Data data(resource->data);
            load_module(data);
            load_samples(data);
            load_seq_table(data);
        }
    };

    return play_music(_engine.getResource(music_id));
}

auto Music::stopMusicUnlocked() -> void
{
    LOG_DEBUG("stop music [music_id: 0x%02x]", _module.music_id);

    auto stop_music = [&]() -> void
    {
        if(_module.music_id != 0xffff) {
            _audio.stopAllChannels();
            _module = MusicModule();
        }
    };

    return stop_music();
}

auto Music::processPattern(uint8_t channel, Data& data) -> void
{
    auto get_frequency = [](uint16_t period) -> uint16_t
    {
        if(period < 55) {
            return 65535;
        }
        return static_cast<uint16_t>(Paula::Carrier / period);
    };

    auto get_volume = [](int16_t volume) -> uint8_t
    {
        if(volume < 0x00) {
            volume = 0x00;
        }
        if(volume > 0x3f) {
            volume = 0x3f;
        }
        return static_cast<uint8_t>(volume);
    };

    MusicPattern pattern;
    pattern.word1 = data.fetchWordBE();
    pattern.word2 = data.fetchWordBE();
    if(pattern.word1 == 0x0000) {
        return;
    }
    else if(pattern.word1 == 0xfffd) {
        _engine.setMusicMark(pattern.word2);
    }
    else if(pattern.word1 == 0xfffe) {
        _audio.stopChannel(channel);
    }
    else {
        const uint16_t period_value = ((pattern.word1 & 0x0fff) >>  0);
        const uint8_t  sample_index = ((pattern.word2 & 0xf000) >> 12);
        const uint8_t  effect_index = ((pattern.word2 & 0x0f00) >>  8);
        const uint8_t  effect_value = ((pattern.word2 & 0x00ff) >>  0);
        if(sample_index != 0) {
            auto sample(_module.samples[sample_index - 1]);
            int16_t volume = sample.volume;
            switch(effect_index) {
                case 0x0: // no effect
                    break;
                case 0x5: // volume up
                    volume += effect_value;
                    break;
                case 0x6: // volume down
                    volume -= effect_value;
                    break;
                default:
                    LOG_DEBUG("unsupported effect $%x", effect_index);
                    break;
            }
            sample.frequency = get_frequency(period_value);
            sample.volume    = get_volume(volume);
            _audio.playChannel(channel, sample);
        }
    }
}

// ---------------------------------------------------------------------------
// End-Of-File
// ---------------------------------------------------------------------------

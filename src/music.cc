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
    auto callback = +[](uint32_t interval, void* data) -> uint32_t
    {
        return reinterpret_cast<Music*>(data)->processMusic();
    };

    log_debug(SYS_MUSIC, "starting...");
    stopMusic();
    if(_timer == 0) {
        _timer = _engine.addTimer(callback, (_module.music_timer != 0 ? _module.music_timer : 100), this);
    }
    log_debug(SYS_MUSIC, "started!");
}

auto Music::reset() -> void
{
    log_debug(SYS_MUSIC, "resetting...");
    stopMusic();
    log_debug(SYS_MUSIC, "reset!");
}

auto Music::stop() -> void
{
    log_debug(SYS_MUSIC, "stopping...");
    stopMusic();
    if(_timer != 0) {
        _timer = (_engine.removeTimer(_timer), 0);
    }
    log_debug(SYS_MUSIC, "stopped!");
}

auto Music::playMusic(uint16_t music_id, uint8_t index, uint16_t delay) -> void
{
    const LockGuard lock(_mutex);

    if(music_id == 0x0000) {
        music_id = 0xffff;
    }
    if(music_id != 0xffff) {
        stopMusicUnlocked();
        playMusicUnlocked(music_id, index, delay);
    }
    else if((index != 0) || (delay != 0)) {
        if(index != 0) {
            _module.order_index = index;
        }
        if(delay != 0) {
            _module.music_delay = delay;
            _module.music_timer = (static_cast<uint32_t>(_module.music_delay) * 60) / 7050;
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

auto Music::playMusicUnlocked(uint16_t music_id, uint8_t index, uint16_t delay) -> void
{
    log_debug(SYS_MUSIC, "play music [music_id: 0x%02x, index: %d, delay: %d]", music_id, index, delay);

    auto load_module = [&](Ptr& modptr) -> void
    {
        _module             = MusicModule();
        _module.music_id    = music_id;
        _module.music_delay = modptr.seek(0x00).fetchWordBE();
        _module.music_timer = (static_cast<uint32_t>(_module.music_delay) * 60) / 7050;
        _module.data_ptr    = modptr.seek(0xc0).get();
        _module.data_pos    = 0;
        _module.order_index = index;
        _module.order_count = modptr.seek(0x3e).fetchWordBE();
        if(delay != 0) {
            _module.music_delay = delay;
            _module.music_timer = (static_cast<uint32_t>(_module.music_delay) * 60) / 7050;
        }
    };

    auto load_samples = [&](Ptr& modptr) -> void
    {
        modptr.seek(0x02);
        for(auto& sample : _module.samples) {
            sample = AudioSample();
            const uint16_t sample_id = modptr.fetchWordBE();
            const uint16_t volume    = modptr.fetchWordBE();
            if(sample_id != 0) {
                log_debug(SYS_MUSIC, "load sample [sound_id: 0x%02x, volume: %d]", sample_id, volume);
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
                    Ptr sndptr(resource->data);
                    const uint32_t data_len = static_cast<uint32_t>(sndptr.fetchWordBE()) * 2;
                    const uint32_t loop_len = static_cast<uint32_t>(sndptr.fetchWordBE()) * 2;
                    const uint16_t unused1  = sndptr.fetchWordBE();
                    const uint16_t unused2  = sndptr.fetchWordBE();
                    const uint8_t* data_ptr = sndptr.get();
                    sample.sample_id = sample_id;
                    sample.data_ptr  = data_ptr;
                    sample.data_len  = data_len;
                    sample.loop_pos  = 0;
                    sample.loop_len  = 0;
                    sample.unused1   = unused1;
                    sample.unused2   = unused2;
                    sample.volume    = static_cast<uint8_t>(volume <= 0x3f ? volume : 0x3f);
                    if(loop_len != 0) {
                        sample.data_len += loop_len;
                        sample.loop_pos += data_len;
                        sample.loop_len += loop_len;
                    }
                }
            }
        }
    };

    auto load_order_table = [&](Ptr& modptr) -> void
    {
        modptr.seek(0x40);
        for(auto& order : _module.order_table) {
            order = modptr.fetchByte();
        }
    };

    auto load_music = [&]() -> void
    {
        Resource* resource = _engine.getResource(music_id);

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
            Ptr modptr(resource->data);
            load_module(modptr);
            load_samples(modptr);
            load_order_table(modptr);
        }
    };

    return load_music();
}

auto Music::stopMusicUnlocked() -> void
{
    log_debug(SYS_MUSIC, "stop music [music_id: 0x%02x]", _module.music_id);

    auto stop_music = [&]() -> void
    {
        if(_module.music_id != 0xffff) {
            _audio.stopAllChannels();
            _module = MusicModule();
        }
    };

    return stop_music();
}

auto Music::processMusic() -> uint32_t
{
    const LockGuard lock(_mutex);

    if(_engine.isPaused()) {
        return 100;
    }
    if(_module.music_id == 0xffff) {
        return 100;
    }
    if(_module.order_index < _module.order_count) {
        const uint8_t order = _module.order_table[_module.order_index];
        log_debug(SYS_MUSIC, "process music [music_id: 0x%02x, order: 0x%02x, position: 0x%04x]", _module.music_id, order, _module.data_pos);
        Ptr buffer(_module.data_ptr + _module.data_pos + order * 1024);
        for(uint8_t channel = 0; channel < 4; ++channel) {
            processPattern(channel, buffer);
        }
        _module.data_pos += buffer.offset();
        if(_module.data_pos >= 1024) {
            _module.data_pos = 0;
            const uint8_t order_index = _module.order_index + 1;
            const uint8_t order_count = _module.order_count;
            if(order_index >= order_count) {
                log_debug(SYS_MUSIC, "music is over [music_id: 0x%02x]", _module.music_id);
                stopMusicUnlocked();
            }
            else {
                _module.order_index = order_index;
            }
        }
    }
    else {
        log_debug(SYS_MUSIC, "music is over [music_id: 0x%02x]", _module.music_id);
        stopMusicUnlocked();
    }
    return (_module.music_timer != 0 ? _module.music_timer : 100);
}

auto Music::processPattern(uint8_t channel, Ptr& buffer) -> void
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
    pattern.word1 = buffer.fetchWordBE();
    pattern.word2 = buffer.fetchWordBE();
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
            auto& sample(_module.samples[sample_index - 1]);
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
                    log_debug(SYS_MUSIC, "unsupported effect $%x", effect_index);
                    break;
            }
            _audio.playChannel(channel, sample, get_frequency(period_value), get_volume(volume));
        }
    }
}

// ---------------------------------------------------------------------------
// End-Of-File
// ---------------------------------------------------------------------------

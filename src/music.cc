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

Music::Music(Engine& engine)
    : SubSystem(engine)
    , _module()
    , _delay(0)
    , _timer(0)
{
}

auto Music::init() -> void
{
    log_debug(SYS_MUSIC, "initializing...");
    stopMusicModule();
    startMusic();
    log_debug(SYS_MUSIC, "initialized!");
}

auto Music::fini() -> void
{
    log_debug(SYS_MUSIC, "finalizing...");
    stopMusicModule();
    stopMusic();
    log_debug(SYS_MUSIC, "finalized!");
}

auto Music::playMusicModule() -> void
{
    log_debug(SYS_MUSIC, "play module [module: 0x%02x]", _module.resId);
    if(_module.curPos != 0) {
        _module.curPos = 0;
    }
}

auto Music::stopMusicModule() -> void
{
    log_debug(SYS_MUSIC, "stop module [module: 0x%02x]", _module.resId);
    if(_module.resId != 0) {
        _module.resId = 0;
    }
}

auto Music::setMusicModuleDelay(uint16_t delay) -> void
{
    log_debug(SYS_MUSIC, "set module delay [delay: %d]", delay);
    if(delay != 0) {
        _delay = static_cast<uint32_t>(delay) * 60 / 7050;
    }
    else {
        _delay = 0;
    }
}

auto Music::loadMusicModule(uint16_t resId, uint16_t delay, uint8_t pos) -> void
{
    log_debug(SYS_MUSIC, "load module [module: 0x%02x, delay: %d, pos: %d]", resId, delay, pos);

    Resource* resource = _engine.getResource(resId);
    if(resource->state == RS_LOADED && resource->type == RT_MUSIC) {
        const MusicModule dummy;
        _module = dummy;
        _module.resId    = resId;
        _module.curOrder = pos;
        _module.numOrder = Utils::fetchWordBE(resource->data + 0x3e);
        for(int i = 0; i < 0x80; ++i) {
            _module.orderTable[i] = *(resource->data + 0x40 + i);
        }
        if(delay == 0) {
            _delay = static_cast<uint32_t>(Utils::fetchWordBE(resource->data)) * 60 / 7050;
        }
        else {
            _delay = delay * 60 / 7050;
        }
        _module.data = resource->data + 0xc0;
        prepareSamples(resource->data + 2);
    }
    else {
        log_fatal("error while loading module 0x%02x", resId);
    }
}

auto Music::prepareSamples(const uint8_t* buffer) -> void
{
    const MusicSample dummy;
    for(auto& sample : _module.samples) {
        sample = dummy;
    }
    for(int i = 0; i < 15; ++i) {
        MusicSample* sample = &_module.samples[i];
        uint16_t resId = Utils::fetchWordBE(buffer); buffer += 2;
        if(resId != 0) {
            sample->volume = Utils::fetchWordBE(buffer);
            Resource* resource = _engine.getResource(resId);
            if(resource->state == RS_LOADED && resource->type == RT_SOUND) {
                sample->data = resource->data;
            //  memset(sample->data + 8, 0, 4); // WTF??
                log_debug(SYS_MUSIC, "load sample [id: 0x%02x, index: %d, volume: %d]", resId, i, sample->volume);
            }
            else {
                log_fatal("error while loading sample 0x%02x", resId);
            }
        }
        buffer += 2; // skip volume
    }
}

auto Music::startMusic() -> void
{
    log_debug(SYS_MUSIC, "start music");

    auto callback = +[](uint32_t interval, void* data) -> uint32_t
    {
        return reinterpret_cast<Music*>(data)->processMusic();
    };

    if(_timer == 0) {
        _timer = _engine.addTimer(callback, (_delay != 0 ? _delay : 100), this);
    }
}

auto Music::stopMusic() -> void
{
    log_debug(SYS_MUSIC, "stop music");

    if(_timer != 0) {
        _timer = (_engine.removeTimer(_timer), 0);
    }
}

auto Music::processMusic() -> uint32_t
{
    if(_engine.paused()) {
        return 100;
    }
    if(_module.resId != 0) {
        uint8_t order = _module.orderTable[_module.curOrder];
        log_debug(SYS_MUSIC, "process music [order: 0x%02x, position: 0x%04x]", order, _module.curPos);
        const uint8_t* patternData = _module.data + _module.curPos + order * 1024;
        for(uint8_t ch = 0; ch < 4; ++ch) {
            handlePattern(ch, patternData);
            patternData += 4;
        }
        _module.curPos += 4 * 4;
        if(_module.curPos >= 1024) {
            _module.curPos = 0;
            order = _module.curOrder + 1;
            if(order == _module.numOrder) {
                _module.resId = 0;
                _engine.stopAllAudioChannels();
            }
            _module.curOrder = order;
        }
    }
    return (_delay != 0 ? _delay : 100);
}

auto Music::handlePattern(uint8_t channel, const uint8_t* buffer) -> void
{
    MusicPattern pat;
    pat.note_1 = Utils::fetchWordBE(buffer + 0);
    pat.note_2 = Utils::fetchWordBE(buffer + 2);
    if(pat.note_1 != 0xfffd) {
        uint16_t sample = (pat.note_2 & 0xf000) >> 12;
        if(sample != 0) {
            uint8_t* ptr = _module.samples[sample - 1].data;
            if(ptr != 0) {
                pat.sampleVolume = _module.samples[sample - 1].volume;
                pat.sampleStart = 8;
                pat.sampleBuffer = ptr;
                pat.sampleLen = Utils::fetchWordBE(ptr) * 2;
                uint16_t loopLen = Utils::fetchWordBE(ptr + 2) * 2;
                if(loopLen != 0) {
                    pat.loopPos = pat.sampleLen;
                    pat.loopData = ptr;
                    pat.loopLen = loopLen;
                }
                else {
                    pat.loopPos = 0;
                    pat.loopData = 0;
                    pat.loopLen = 0;
                }
                int16_t m = pat.sampleVolume;
                uint8_t effect = (pat.note_2 & 0x0f00) >> 8;
                if(effect == 5) { // volume up
                    uint8_t volume = (pat.note_2 & 0xff);
                    m += volume;
                    if(m > 0x3f) {
                        m = 0x3f;
                    }
                }
                else if(effect == 6) { // volume down
                    uint8_t volume = (pat.note_2 & 0xff);
                    m -= volume;
                    if(m < 0) {
                        m = 0;
                    }
                }
                _engine.setAudioChannelVolume(channel, m);
                pat.sampleVolume = m;
            }
        }
    }
    if(pat.note_1 == 0xfffd) {
        _engine.setMusicMark(pat.note_2);
    }
    else if(pat.note_1 != 0) {
        if(pat.note_1 == 0xfffe) {
            _engine.stopAudioChannel(channel);
        }
        else if(pat.sampleBuffer != 0) {
            AudioSample sample;
            sample.bufPtr  = pat.sampleBuffer + pat.sampleStart;
            sample.bufLen  = pat.sampleLen;
            sample.loopPos = pat.loopPos;
            sample.loopLen = pat.loopLen;
            assert(pat.note_1 >= 0x37 && pat.note_1 < 0x1000);
            // convert amiga period value to hz
            uint16_t freq = 7159092 / (pat.note_1 * 2);
            _engine.playAudioChannel(channel, sample, freq, pat.sampleVolume);
        }
    }
}

// ---------------------------------------------------------------------------
// End-Of-File
// ---------------------------------------------------------------------------

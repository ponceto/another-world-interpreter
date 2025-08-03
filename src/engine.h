/*
 * engine.h - Copyright (c) 2004-2025
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
#ifndef __AW_ENGINE_H__
#define __AW_ENGINE_H__

#include "intern.h"
#include "system.h"
#include "resources.h"
#include "video.h"
#include "audio.h"
#include "music.h"
#include "vm.h"

// ---------------------------------------------------------------------------
// Engine
// ---------------------------------------------------------------------------

class Engine final
    : public SubSystem
{
public: // public interface
    Engine(System& stub, const std::string& dataDirectory, const std::string& dumpDirectory);

    Engine(Engine&&) = delete;

    Engine(const Engine&) = delete;

    Engine& operator=(Engine&&) = delete;

    Engine& operator=(const Engine&) = delete;

    virtual ~Engine() = default;

    virtual auto init() -> void override final;

    virtual auto fini() -> void override final;

    auto run() -> void;

public: // public system interface
    auto running() -> bool
    {
        return system.input.pause == false;
    }

    auto paused() -> bool
    {
        return system.input.pause != false;
    }

    auto startAudio(AudioCallback callback, void* data) -> void
    {
        return system.startAudio(callback, data);
    }

    auto stopAudio() -> void
    {
        return system.stopAudio();
    }

    auto getAudioSampleRate() -> uint32_t
    {
        return system.getAudioSampleRate();
    }

    auto addTimer(TimerCallback callback, uint32_t delay, void* data) -> int
    {
        return system.addTimer(delay, callback, data);
    }

    auto removeTimer(int timer) -> void
    {
        return system.removeTimer(timer);
    }

    auto getTimeStamp() -> uint32_t
    {
        return system.getTimeStamp();
    }

    auto sleepFor(uint32_t delay) -> void
    {
        return system.sleepFor(delay);
    }

public: // public resource interface
    auto getResource(uint16_t resourceId) -> Resource*
    {
        return resources.getResource(resourceId);
    }

public: // public audio interface
    auto playAllAudioChannels() -> void
    {
        return audio.playAllAudioChannels();
    }

    auto stopAllAudioChannels() -> void
    {
        return audio.stopAllAudioChannels();
    }

    auto playAudioChannel(uint8_t channel, const AudioSample& sample, uint16_t frequency, uint8_t volume) -> void
    {
        return audio.playAudioChannel(channel, sample, frequency, volume);
    }

    auto stopAudioChannel(uint8_t channel) -> void
    {
        return audio.stopAudioChannel(channel);
    }

    auto setAudioChannelVolume(uint8_t channel, uint8_t volume) -> void
    {
        return audio.setAudioChannelVolume(channel, volume);
    }

public: // public music interface
    auto playMusicModule() -> void
    {
        return music.playMusicModule();
    }

    auto stopMusicModule() -> void
    {
        return music.stopMusicModule();
    }

    auto setMusicModuleDelay(uint16_t delay) -> void
    {
        return music.setMusicModuleDelay(delay);
    }

    auto loadMusicModule(uint16_t resource, uint16_t delay, uint8_t position) -> void
    {
        return music.loadMusicModule(resource, delay, position);
    }

    auto setMusicMark(uint16_t value) -> void
    {
        vm.vmVariables[VM_VARIABLE_MUS_MARK] = value;
    }

public: // public data
    const std::string dataDir;
    const std::string dumpDir;
    System&           system;
    Resources         resources;
    Video             video;
    Audio             audio;
    Music             music;
    VirtualMachine    vm;
};

// ---------------------------------------------------------------------------
// End-Of-File
// ---------------------------------------------------------------------------

#endif /* __AW_ENGINE_H__ */

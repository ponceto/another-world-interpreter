/*
 * resources.h - Copyright (c) 2004-2025
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
#ifndef __AW_RESOURCES_H__
#define __AW_RESOURCES_H__

#include "intern.h"
#include "parts.h"
#include "data.h"

// ---------------------------------------------------------------------------
// Resources
// ---------------------------------------------------------------------------

class Resources final
    : public SubSystem
{
public: // public interface
    Resources(Engine& engine);

    Resources(Resources&&) = delete;

    Resources(const Resources&) = delete;

    Resources& operator=(Resources&&) = delete;

    Resources& operator=(const Resources&) = delete;

    virtual ~Resources();

    virtual auto init() -> void override final;

    virtual auto fini() -> void override final;

    auto setupPart(uint16_t partId) -> void;

    auto loadPartOrResource(uint16_t num) -> void;

    auto getResource(uint16_t resourceId) -> Resource*
    {
        if(resourceId < _resourcesCount) {
            return &_resourcesTable[resourceId];
        }
        return nullptr;
    }

private: // private interface
    auto allocBuffer() -> void;

    auto freeBuffer() -> void;

    auto readMemList() -> void;

    auto readMemBank(Resource& me) -> void;

    auto invalidateAll() -> void;

    auto loadMarkedAsNeeded() -> void;

    auto dumpResources() -> void;

    auto dumpResource(const Resource& resource) -> void;

private: // private data
    uint8_t* _bufferBegin;
    uint8_t* _bufferIter;
    uint8_t* _bufferEnd;
    Resource _resourcesTable[256];
    uint16_t _resourcesCount;

public: // public data
    uint16_t  currentPartId;
    uint16_t  requestedPartId;
    uint8_t*  segPalettes;
    uint8_t*  segByteCode;
    uint8_t*  segPolygon;
    uint8_t*  segVideo2;
    bool      useSegVideo2;
};

// ---------------------------------------------------------------------------
// End-Of-File
// ---------------------------------------------------------------------------

#endif /* __AW_RESOURCES_H__ */

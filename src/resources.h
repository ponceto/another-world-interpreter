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

    virtual ~Resources() = default;

    virtual auto start() -> void override final;

    virtual auto reset() -> void override final;

    virtual auto stop() -> void override final;

public: // public resources interface
    auto loadPart(uint16_t partId) -> void;

    auto loadResource(uint16_t resourceId) -> void;

    auto getString(uint16_t stringId) -> const String*;

    auto getResource(uint16_t resourceId) -> Resource*
    {
        if(resourceId < _resourcesCount) {
            return &_resourcesArray[resourceId];
        }
        return nullptr;
    }

    auto getCurPartId() const -> uint16_t
    {
        return _curPartId;
    }

    auto getReqPartId() const -> uint16_t
    {
        return _reqPartId;
    }

    auto requestPartId(uint16_t partId) -> void
    {
        _reqPartId = partId;
    }

    auto getPalettesData() const -> const uint8_t*
    {
        return _segPalettes;
    }

    auto getByteCodeData() const -> const uint8_t*
    {
        return _segByteCode;
    }

    auto getPolygon1Data() const -> const uint8_t*
    {
        return _segPolygon1;
    }

    auto getPolygon2Data() const -> const uint8_t*
    {
        return _segPolygon2;
    }

    auto getPolygonData(int index) -> const uint8_t*
    {
        switch(index) {
            case 1:
                return _segPolygon1;
            case 2:
                return _segPolygon2;
            default:
                break;
        }
        return nullptr;
    }

private: // private interface
    static constexpr uint32_t BLOCK_COUNT = 1792;
    static constexpr uint32_t BLOCK_SIZE  = 1024;
    static constexpr uint32_t TOTAL_SIZE  = (BLOCK_COUNT * BLOCK_SIZE);

    auto loadMemList() -> void;

    auto loadResources() -> void;

    auto invalidateAll() -> void;

private: // private data
    uint8_t  _buffer[TOTAL_SIZE];
    uint8_t* _bufptr;
    uint8_t* _bufend;
    uint8_t  _dictionaryId;
    Resource _resourcesArray[256];
    uint16_t _resourcesCount;
    uint16_t _curPartId;
    uint16_t _reqPartId;
    uint8_t* _segPalettes;
    uint8_t* _segByteCode;
    uint8_t* _segPolygon1;
    uint8_t* _segPolygon2;
};

// ---------------------------------------------------------------------------
// End-Of-File
// ---------------------------------------------------------------------------

#endif /* __AW_RESOURCES_H__ */

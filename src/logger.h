/*
 * logger.h - Copyright (c) 2004-2025
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
#ifndef __AW_LOGGER_H__
#define __AW_LOGGER_H__

// ---------------------------------------------------------------------------
// Panic
// ---------------------------------------------------------------------------

class Panic
{
public: // public interface
    Panic() = default;

    virtual ~Panic() = default;

    virtual auto operator()() const -> void;
};

// ---------------------------------------------------------------------------
// debug masks
// ---------------------------------------------------------------------------

enum
{
    LOG_DEBUG     = (1u <<  0),
    LOG_PRINT     = (1u <<  1),
    LOG_ALERT     = (1u <<  2),
    LOG_ERROR     = (1u <<  3),
    LOG_FATAL     = (1u <<  4),
    SYS_ENGINE    = (1u <<  5),
    SYS_BACKEND   = (1u <<  6),
    SYS_RESOURCES = (1u <<  7),
    SYS_VIDEO     = (1u <<  8),
    SYS_AUDIO     = (1u <<  9),
    SYS_MIXER     = (1u << 10),
    SYS_SOUND     = (1u << 11),
    SYS_MUSIC     = (1u << 12),
    SYS_INPUT     = (1u << 13),
    SYS_VM        = (1u << 14),
};

// ---------------------------------------------------------------------------
// debug mask
// ---------------------------------------------------------------------------

extern FILE*    g_logger_out;
extern FILE*    g_logger_err;
extern uint32_t g_logger_mask;

// ---------------------------------------------------------------------------
// logging functions
// ---------------------------------------------------------------------------

extern auto log_debug(uint32_t mask, const char* format, ...) -> void;

extern auto log_debug(const char* format, ...) -> void;

extern auto log_print(const char* format, ...) -> void;

extern auto log_alert(const char* format, ...) -> void;

extern auto log_error(const char* format, ...) -> void;

extern auto log_fatal(const char* format, ...) -> void;

// ---------------------------------------------------------------------------
// End-Of-File
// ---------------------------------------------------------------------------

#endif /* __AW_LOGGER_H__ */

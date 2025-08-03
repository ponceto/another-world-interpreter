/*
 * logger.cc - Copyright (c) 2004-2025
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

// ---------------------------------------------------------------------------
// Panic
// ---------------------------------------------------------------------------

auto Panic::operator()() const -> void
{
    std::cerr << "*** Engine panic ***" << std::endl;
}

// ---------------------------------------------------------------------------
// debug mask
// ---------------------------------------------------------------------------

FILE*    g_loggerOut  = stdout;
FILE*    g_loggerErr  = stderr;
uint32_t g_loggerMask = 0u
                      | LOG_DEBUG
                      | LOG_PRINT
                      | LOG_ALERT
                      | LOG_ERROR
                      | LOG_FATAL
                      ;

// ---------------------------------------------------------------------------
// logging functions
// ---------------------------------------------------------------------------

auto log_debug(uint32_t mask, const char* format, ...) -> void
{
    auto get_domain = [&]() -> const char*
    {
        if(mask & SYS_ENGINE)    { return "ENGINE"; };
        if(mask & SYS_RESOURCES) { return "RESRCS"; };
        if(mask & SYS_VIDEO)     { return "VIDEO "; };
        if(mask & SYS_AUDIO)     { return "AUDIO "; };
        if(mask & SYS_MUSIC)     { return "MUSIC "; };
        if(mask & SYS_INPUT)     { return "INPUT "; };
        if(mask & SYS_VM)        { return "VM    "; };
        return "UNKNOWN";
    };

    if((g_loggerMask & LOG_DEBUG) != 0) {
        if((g_loggerMask & mask) != 0) {
            va_list args;
            va_start(args, format);
            (void) ::fputc('D', g_loggerOut);
            (void) ::fputc('\t', g_loggerOut);
            (void) ::fputs(get_domain(), g_loggerOut);
            (void) ::fputc('\t', g_loggerOut);
            (void) ::vfprintf(g_loggerOut, format, args);
            (void) ::fputc('\n', g_loggerOut);
            (void) ::fflush(g_loggerOut);
            va_end(args);
        }
    }
}

auto log_debug(const char* format, ...) -> void
{
    if((g_loggerMask & LOG_DEBUG) != 0) {
        va_list args;
        va_start(args, format);
        (void) ::fputc('D', g_loggerOut);
        (void) ::fputc('\t', g_loggerOut);
        (void) ::vfprintf(g_loggerOut, format, args);
        (void) ::fputc('\n', g_loggerOut);
        (void) ::fflush(g_loggerOut);
        va_end(args);
    }
}

auto log_print(const char* format, ...) -> void
{
    if((g_loggerMask & LOG_PRINT) != 0) {
        va_list args;
        va_start(args, format);
        (void) ::fputc('I', g_loggerOut);
        (void) ::fputc('\t', g_loggerOut);
        (void) ::vfprintf(g_loggerOut, format, args);
        (void) ::fputc('\n', g_loggerOut);
        (void) ::fflush(g_loggerOut);
        va_end(args);
    }
}

auto log_alert(const char* format, ...) -> void
{
    if((g_loggerMask & LOG_ALERT) != 0) {
        va_list args;
        va_start(args, format);
        (void) ::fputc('W', g_loggerErr);
        (void) ::fputc('\t', g_loggerErr);
        (void) ::vfprintf(g_loggerErr, format, args);
        (void) ::fputc('\n', g_loggerErr);
        (void) ::fflush(g_loggerErr);
        va_end(args);
    }
}

auto log_error(const char* format, ...) -> void
{
    if((g_loggerMask & LOG_ERROR) != 0) {
        va_list args;
        va_start(args, format);
        (void) ::fputc('E', g_loggerErr);
        (void) ::fputc('\t', g_loggerErr);
        (void) ::vfprintf(g_loggerErr, format, args);
        (void) ::fputc('\n', g_loggerErr);
        (void) ::fflush(g_loggerErr);
        va_end(args);
    }
}

auto log_fatal(const char* format, ...) -> void
{
    if((g_loggerMask & LOG_FATAL) != 0) {
        va_list args;
        va_start(args, format);
        (void) ::fputc('F', g_loggerErr);
        (void) ::fputc('\t', g_loggerErr);
        (void) ::vfprintf(g_loggerErr, format, args);
        (void) ::fputc('\n', g_loggerErr);
        (void) ::fflush(g_loggerErr);
        va_end(args);
    }
    throw Panic();
}

// ---------------------------------------------------------------------------
// End-Of-File
// ---------------------------------------------------------------------------

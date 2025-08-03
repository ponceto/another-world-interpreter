/*
 * main.cc - Copyright (c) 2004-2025
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
#include "main.h"

// ---------------------------------------------------------------------------
// <anonymous>::globals
// ---------------------------------------------------------------------------

namespace {

std::string g_aw_program = "another-world";
std::string g_aw_datadir = "share/another-world";
std::string g_aw_dumpdir = "";
bool        g_aw_usage   = false;

}

// ---------------------------------------------------------------------------
// <anonymous>::CommandLine
// ---------------------------------------------------------------------------

namespace {

struct CommandLine
{
    static auto program(const std::string& arg) -> bool
    {
        const char* str = arg.c_str();
        const char* sep = ::strrchr(str, '/');
        if(sep != nullptr) {
            g_aw_program = (sep + 1);
        }
        else {
            g_aw_program = arg;
        }
        return true;
    }

    static auto option(const std::string& arg) -> bool
    {
        const char* str = arg.c_str();
        const char* equ = ::strchr(str, '=');
        if(equ == nullptr) {
            if((arg == "-h") || (arg == "--help")) {
                g_aw_usage = true;
                return true;
            }
            if(arg == "--quiet") {
                g_loggerMask &= ~LOG_DEBUG;
                g_loggerMask &= ~LOG_PRINT;
                g_loggerMask &= ~LOG_ALERT;
                g_loggerMask &= ~LOG_ERROR;
                g_loggerMask &= ~LOG_FATAL;
                return true;
            }
            if(arg == "--debug") {
                g_loggerMask |= LOG_DEBUG;
                return true;
            }
            if(arg == "--debug-all") {
                g_loggerMask |= LOG_DEBUG;
                g_loggerMask |= SYS_ENGINE;
                g_loggerMask |= SYS_RESOURCES;
                g_loggerMask |= SYS_VIDEO;
                g_loggerMask |= SYS_AUDIO;
                g_loggerMask |= SYS_MUSIC;
                g_loggerMask |= SYS_INPUT;
                g_loggerMask |= SYS_VM;
                return true;
            }
            if(arg == "--debug-engine") {
                g_loggerMask |= LOG_DEBUG;
                g_loggerMask |= SYS_ENGINE;
                return true;
            }
            if(arg == "--debug-resources") {
                g_loggerMask |= LOG_DEBUG;
                g_loggerMask |= SYS_RESOURCES;
                return true;
            }
            if(arg == "--debug-video") {
                g_loggerMask |= LOG_DEBUG;
                g_loggerMask |= SYS_VIDEO;
                return true;
            }
            if(arg == "--debug-audio") {
                g_loggerMask |= LOG_DEBUG;
                g_loggerMask |= SYS_AUDIO;
                return true;
            }
            if(arg == "--debug-music") {
                g_loggerMask |= LOG_DEBUG;
                g_loggerMask |= SYS_MUSIC;
                return true;
            }
            if(arg == "--debug-input") {
                g_loggerMask |= LOG_DEBUG;
                g_loggerMask |= SYS_INPUT;
                return true;
            }
            if(arg == "--debug-vm") {
                g_loggerMask |= LOG_DEBUG;
                g_loggerMask |= SYS_VM;
                return true;
            }
        }
        else {
            const size_t len = (equ - str);
            if(arg.compare(0, len, "--datadir") == 0) {
                g_aw_datadir = (equ + 1);
                return true;
            }
            if(arg.compare(0, len, "--dumpdir") == 0) {
                g_aw_dumpdir = (equ + 1);
                return true;
            }
        }
        return false;
    }

    static auto usage() -> void
    {
        std::cout << "Usage: " << g_aw_program << " [OPTIONS...]"                    << std::endl;
        std::cout << ""                                                              << std::endl;
        std::cout << "Options:"                                                      << std::endl;
        std::cout << ""                                                              << std::endl;
        std::cout << "  -h, --help            display this help and exit"            << std::endl;
        std::cout << ""                                                              << std::endl;
        std::cout << "  --datadir=PATH        directory where data files are stored" << std::endl;
        std::cout << "  --dumpdir=PATH        directory where dump files are stored" << std::endl;
        std::cout << ""                                                              << std::endl;
        std::cout << "  --quiet               quiet mode"                            << std::endl;
        std::cout << "  --debug               debug mode"                            << std::endl;
        std::cout << "  --debug-all           debug all subsystems"                  << std::endl;
        std::cout << "  --debug-engine        debug the engine subsystem"            << std::endl;
        std::cout << "  --debug-resources     debug the resources subsystem"         << std::endl;
        std::cout << "  --debug-video         debug the video subsystem"             << std::endl;
        std::cout << "  --debug-audio         debug the audio subsystem"             << std::endl;
        std::cout << "  --debug-music         debug the music subsystem"             << std::endl;
        std::cout << "  --debug-input         debug the input subsystem"             << std::endl;
        std::cout << "  --debug-vm            debug the vm subsystem"                << std::endl;
        std::cout << ""                                                              << std::endl;
    }
};

}

// ---------------------------------------------------------------------------
// <anonymous>::Program
// ---------------------------------------------------------------------------

namespace {

struct Program
{
    using EnginePtr = std::unique_ptr<Engine>;

    static auto main() -> int
    {
        EnginePtr engine;

        try {
            engine = std::make_unique<Engine>(*stub, g_aw_datadir, g_aw_dumpdir);
            engine->run();
        }
        catch(const Panic& panic) {
            panic();
            return EXIT_FAILURE;
        }
        catch(...) {
            std::cerr << "unhandled exception" << std::endl;
            return EXIT_FAILURE;
        }
        return EXIT_SUCCESS;
    }
};

}

// ---------------------------------------------------------------------------
// main
// ---------------------------------------------------------------------------

int main(int argc, char* argv[])
{
#ifdef __EMSCRIPTEN__
    g_loggerMask &= ~LOG_DEBUG;
    g_loggerMask &= ~LOG_PRINT;
    g_loggerMask &= ~LOG_ALERT;
    g_loggerMask &= ~LOG_ERROR;
    g_loggerMask &= ~LOG_FATAL;
#else
    g_loggerMask &= ~LOG_DEBUG;
#endif
    for(int argi = 0; argi < argc; ++argi) {
        const std::string arg(argv[argi]);
        if(argi == 0) {
            if(CommandLine::program(arg) == false) {
                std::cerr << "Error: invalid argument " << '<' << arg << '>' << std::endl;
                return EXIT_FAILURE;
            }
        }
        else {
            if(CommandLine::option(arg) == false) {
                std::cerr << "Error: invalid argument " << '<' << arg << '>' << std::endl;
                return EXIT_FAILURE;
            }
        }
        if(g_aw_usage != false) {
            CommandLine::usage();
            return EXIT_SUCCESS;
        }
    }
    return Program::main();
}

// ---------------------------------------------------------------------------
// End-Of-File
// ---------------------------------------------------------------------------

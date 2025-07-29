/*
 * config.h - Copyright (c) 2004-2025
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
#ifndef __AW_CONFIG_H__
#define __AW_CONFIG_H__

// ---------------------------------------------------------------------------
// force no debug when compiling with emscripten
// ---------------------------------------------------------------------------

#ifdef __EMSCRIPTEN__
#ifndef NDEBUG
#define NDEBUG
#endif
#endif

// ---------------------------------------------------------------------------
// force no debug
// ---------------------------------------------------------------------------

#if 0
#ifndef NDEBUG
#define NDEBUG
#endif
#endif

// ---------------------------------------------------------------------------
// Another World / Out Of This World
// ---------------------------------------------------------------------------

#if 0
#ifndef OUT_OF_THIS_WORLD
#define OUT_OF_THIS_WORLD
#endif
#endif

// ---------------------------------------------------------------------------
// skip protection screen
// ---------------------------------------------------------------------------

#if 1
#ifndef SKIP_GAME_PART0
#define SKIP_GAME_PART0
#endif
#endif

// ---------------------------------------------------------------------------
// bypass protection
// ---------------------------------------------------------------------------

#if 1
#ifndef BYPASS_PROTECTION
#define BYPASS_PROTECTION
#endif
#endif

// ---------------------------------------------------------------------------
// preload resources
// ---------------------------------------------------------------------------

#if 1
#ifndef PRELOAD_RESOURCES
#define PRELOAD_RESOURCES
#endif
#endif

// ---------------------------------------------------------------------------
// audio sample rate
// ---------------------------------------------------------------------------

#ifndef AUDIO_SAMPLE_RATE
#define AUDIO_SAMPLE_RATE 44100
#endif

// ---------------------------------------------------------------------------
// End-Of-File
// ---------------------------------------------------------------------------

#endif /* __AW_CONFIG_H__ */

/*
 * system.h - Copyright (c) 2004-2025
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
#ifndef __AW_SYSTEM_H__
#define __AW_SYSTEM_H__

#include "intern.h"

// ---------------------------------------------------------------------------
// XXX
// ---------------------------------------------------------------------------

#define NUM_COLORS 16
#define BYTE_PER_PIXEL 3

struct PlayerInput
{
	enum
	{
		DIR_UP     = (1 << 0),
		DIR_DOWN   = (1 << 1),
		DIR_LEFT   = (1 << 2),
		DIR_RIGHT  = (1 << 3),
		DIR_BUTTON = (1 << 4),
	};

	uint8_t dpad  = 0;
	bool    code  = false;
	bool    pause = false;
	bool    quit  = false;
};

/*
	System is an abstract class so any find of system can be plugged underneath.
*/
class System
{
public:
	PlayerInput input;

	virtual ~System() {}

	virtual void init() = 0;
	virtual void fini() = 0;

	virtual void setPalette(const uint8_t *buf) = 0;
	virtual void updateDisplay(const uint8_t *buf) = 0;

	virtual void processEvents() = 0;
	virtual void sleepFor(uint32_t duration) = 0;
	virtual uint32_t getTimeStamp() = 0;

	virtual void startAudio(AudioCallback callback, void *param) = 0;
	virtual void stopAudio() = 0;
	virtual uint32_t getAudioSampleRate() = 0;

	virtual int addTimer(uint32_t delay, TimerCallback callback, void *param) = 0;
	virtual void removeTimer(int timerId) = 0;
};

// ---------------------------------------------------------------------------
// global system stub
// ---------------------------------------------------------------------------

extern System* stub;

// ---------------------------------------------------------------------------
// End-Of-File
// ---------------------------------------------------------------------------

#endif /* __AW_SYSTEM_H__ */

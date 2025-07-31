/* Raw - Another World Interpreter
 * Copyright (C) 2004 Gregory Montoir
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * as published by the Free Software Foundation; either version 2
 * of the License, or (at your option) any later version.

 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.

 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA  02111-1307, USA.
 */

#include "mixer.h"
#include "serializer.h"
#include "sys.h"


Mixer::Mixer(System *stub) 
	: sys(stub) {
}

void Mixer::init() {
	memset(_channels, 0, sizeof(_channels));
	_mutex = sys->createMutex();
	sys->startAudio(Mixer::mixCallback, this);
}

void Mixer::free() {
	stopAll();
	sys->stopAudio();
	sys->destroyMutex(_mutex);
}

void Mixer::playChannel(uint8_t channel, const MixerChunk *mc, uint16_t freq, uint8_t volume) {
	debug(DBG_SND, "Mixer::playChannel(%d, %d, %d)", channel, freq, volume);
	assert(channel < AUDIO_NUM_CHANNELS);

	// The mutex is acquired in the constructor
	const MutexStack lock(sys, _mutex);
	MixerChannel *ch = &_channels[channel];
	ch->active = true;
	ch->volume = volume;
	ch->chunk = *mc;
	ch->chunkPos = 0;
	ch->chunkInc = (freq << 8) / sys->getOutputSampleRate();

	//At the end of the scope the MutexStack destructor is called and the mutex is released. 
}

void Mixer::stopChannel(uint8_t channel) {
	debug(DBG_SND, "Mixer::stopChannel(%d)", channel);
	assert(channel < AUDIO_NUM_CHANNELS);
	const MutexStack lock(sys, _mutex);	
	_channels[channel].active = false;
}

void Mixer::setChannelVolume(uint8_t channel, uint8_t volume) {
	debug(DBG_SND, "Mixer::setChannelVolume(%d, %d)", channel, volume);
	assert(channel < AUDIO_NUM_CHANNELS);
	const MutexStack lock(sys, _mutex);
	_channels[channel].volume = volume;
}

void Mixer::stopAll() {
	debug(DBG_SND, "Mixer::stopAll()");
	const MutexStack lock(sys, _mutex);
	for (uint8_t i = 0; i < AUDIO_NUM_CHANNELS; ++i) {
		_channels[i].active = false;		
	}
}

// This is SDL callback. Called in order to populate the buf with len bytes.  
// The mixer iterates through all active channels and combine all sounds.

// Since there is no way to know when SDL will ask for a buffer fill, we need
// to synchronize with a mutex so the channels remain stable during the execution
// of this method.
void Mixer::mix(float *buf, int len) {
	const MutexStack lock(sys, _mutex);

	auto addclamp = [](float a, float b) -> float
	{
		float val = a + b;
		if (val < -1.0) {
			val = -1.0;
		}
		if (val > +1.0) {
			val = +1.0;
		}
		return val;
	};

	for (uint8_t i = 0; i < AUDIO_NUM_CHANNELS; ++i) {
		MixerChannel *ch = &_channels[i];
		if (!ch->active) 
			continue;

		float *pBuf = buf;
		for (int j = 0; j < len; ++j, ++pBuf) {

			uint16_t p1, p2;
			uint16_t ilc = (ch->chunkPos & 0xFF);
			p1 = ch->chunkPos >> 8;
			ch->chunkPos += ch->chunkInc;

			if (ch->chunk.loopLen != 0) {
				if (p1 == ch->chunk.loopPos + ch->chunk.loopLen - 1) {
					debug(DBG_SND, "Looping sample on channel %d", i);
					p2 = ch->chunkPos = ch->chunk.loopPos;
				} else {
					p2 = p1 + 1;
				}
			} else {
				if (p1 == ch->chunk.len - 1) {
					debug(DBG_SND, "Stopping sample on channel %d", i);
					ch->active = false;
					break;
				} else {
					p2 = p1 + 1;
				}
			}
			// interpolate
			int8_t b1 = *(int8_t *)(ch->chunk.data + p1);
			int8_t b2 = *(int8_t *)(ch->chunk.data + p2);
			int8_t b = (int8_t)((b1 * (0xFF - ilc) + b2 * ilc) >> 8);
            float  f = static_cast<float>((int)b * ch->volume / 0x40) / 127.0;  //0x40=64

			// set volume and clamp
			*pBuf = addclamp(*pBuf, f);
		}
		
	}
}

void Mixer::mixCallback(void *data, uint8_t *buf, int len) {
	memset(buf, 0, len);
	reinterpret_cast<Mixer*>(data)->mix(reinterpret_cast<float*>(buf), len / sizeof(float));
}

void Mixer::saveOrLoad(Serializer &ser) {
	const MutexStack lock(sys, _mutex);
	for (int i = 0; i < AUDIO_NUM_CHANNELS; ++i) {
		MixerChannel *ch = &_channels[i];
		Serializer::Entry entries[] = {
			SE_INT(&ch->active, Serializer::SES_BOOL, VER(2)),
			SE_INT(&ch->volume, Serializer::SES_INT8, VER(2)),
			SE_INT(&ch->chunkPos, Serializer::SES_INT32, VER(2)),
			SE_INT(&ch->chunkInc, Serializer::SES_INT32, VER(2)),
			SE_PTR(&ch->chunk.data, VER(2)),
			SE_INT(&ch->chunk.len, Serializer::SES_INT16, VER(2)),
			SE_INT(&ch->chunk.loopPos, Serializer::SES_INT16, VER(2)),
			SE_INT(&ch->chunk.loopLen, Serializer::SES_INT16, VER(2)),
			SE_END()
		};
		ser.saveOrLoadEntries(entries);
	}
};

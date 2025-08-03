/*
 * system.cc - Copyright (c) 2004-2025
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
#include <SDL2/SDL.h>
#include "logger.h"
#include "system.h"

// ---------------------------------------------------------------------------
// XXX
// ---------------------------------------------------------------------------

struct TextureLocker {
    TextureLocker(SDL_Texture* lockable)
        : texture(lockable)
        , pixels(nullptr)
        , pitch(0)
    {
        if(SDL_LockTexture(texture, nullptr, &pixels, &pitch) != 0) {
            log_fatal("unable to lock texture");
        }
    }

   ~TextureLocker()
    {
        SDL_UnlockTexture(texture);
    }

    SDL_Texture* texture;
    void*        pixels;
    int          pitch;
};

struct SDLStub : System {
	typedef void (SDLStub::*ScaleProc)(uint16_t *dst, uint16_t dstPitch, const uint16_t *src, uint16_t srcPitch, uint16_t w, uint16_t h);

	enum {
		SCREEN_W = 320,
		SCREEN_H = 200,
	};

	int DEFAULT_SCALE = 3;

	SDL_Window*   _window   = nullptr;
	SDL_Renderer* _renderer = nullptr;
	SDL_Texture*  _texture  = nullptr;
	uint8_t _scale = DEFAULT_SCALE;

	virtual ~SDLStub() {}
	virtual void init();
	virtual void fini();
	virtual void setPalette(const uint8_t *buf);
	virtual void updateDisplay(const uint8_t *src);
	virtual void processEvents();
	virtual void sleepFor(uint32_t duration);
	virtual uint32_t getTimeStamp();
	virtual void startAudio(AudioCallback callback, void *param);
	virtual void stopAudio();
	virtual uint32_t getAudioSampleRate();
	virtual int addTimer(uint32_t delay, TimerCallback callback, void *param);
	virtual void removeTimer(int timerId);

	void prepareGfxMode();
	void cleanupGfxMode();
	void switchGfxMode();
};

void SDLStub::init() {
	SDL_Init(SDL_INIT_VIDEO | SDL_INIT_AUDIO | SDL_INIT_TIMER);
//	SDL_EnableKeyRepeat(SDL_DEFAULT_REPEAT_DELAY, SDL_DEFAULT_REPEAT_INTERVAL);
	SDL_ShowCursor(SDL_DISABLE);

	SDL_ShowCursor( SDL_ENABLE );
	SDL_CaptureMouse(SDL_TRUE);

	_scale = DEFAULT_SCALE;
	prepareGfxMode();
}

void SDLStub::fini() {
	cleanupGfxMode();
	SDL_Quit();
}

static SDL_Color palette[NUM_COLORS];
void SDLStub::setPalette(const uint8_t *p) {
  // The incoming palette is in 565 format.
  for (int i = 0; i < NUM_COLORS; ++i)
  {
    uint8_t c1 = *(p + 0);
    uint8_t c2 = *(p + 1);
    palette[i].r = (((c1 & 0x0F) << 2) | ((c1 & 0x0F) >> 2)) << 2; // r
    palette[i].g = (((c2 & 0xF0) >> 2) | ((c2 & 0xF0) >> 6)) << 2; // g
    palette[i].b = (((c2 & 0x0F) >> 2) | ((c2 & 0x0F) << 2)) << 2; // b
    palette[i].a = 0xFF;
    p += 2;
  }
}

void SDLStub::prepareGfxMode() {
  int w = SCREEN_W;
  int h = SCREEN_H;

  _window = SDL_CreateWindow("Another World", SDL_WINDOWPOS_UNDEFINED, SDL_WINDOWPOS_UNDEFINED, w * _scale, h * _scale, SDL_WINDOW_SHOWN | SDL_WINDOW_RESIZABLE);
  _renderer = SDL_CreateRenderer(_window, -1, SDL_RENDERER_ACCELERATED | SDL_RENDERER_PRESENTVSYNC);
  _texture = SDL_CreateTexture(_renderer, SDL_PIXELFORMAT_RGB24, SDL_TEXTUREACCESS_STREAMING, w, h);
}

void SDLStub::updateDisplay(const uint8_t *src) {
	/* update texture */ {
		const TextureLocker texture(_texture);
		const uint16_t screen_w = SCREEN_W / 2;
		const uint16_t screen_h = SCREEN_H;
		uint8_t*       dst      = reinterpret_cast<uint8_t*>(texture.pixels);
		for(int y = screen_h; y != 0; --y) {
			uint8_t* ptr = dst;
			for(int x = screen_w; x != 0; --x) {
				const uint8_t pixels = *src++;
				SDL_Color& color1(palette[(pixels >> 4) & 0xF]);
				SDL_Color& color2(palette[(pixels >> 0) & 0xF]);
				*ptr++ = color1.r;
				*ptr++ = color1.g;
				*ptr++ = color1.b;
				*ptr++ = color2.r;
				*ptr++ = color2.g;
				*ptr++ = color2.b;
			}
			dst += texture.pitch;
		}
	}
	/* blit to screen */ {
		SDL_RenderCopy(_renderer, _texture, nullptr, nullptr);
		SDL_RenderPresent(_renderer);
	}
}

void SDLStub::processEvents() {
	SDL_Event ev;
	while(SDL_PollEvent(&ev)) {
		switch (ev.type) {
			case SDL_QUIT:
				input.quit = true;
				break;
			case SDL_KEYUP:
				switch(ev.key.keysym.sym) {
					case SDLK_UP:
						input.dpad &= ~PlayerInput::DIR_UP;
						break;
					case SDLK_DOWN:
						input.dpad &= ~PlayerInput::DIR_DOWN;
						break;
					case SDLK_LEFT:
						input.dpad &= ~PlayerInput::DIR_LEFT;
						break;
					case SDLK_RIGHT:
						input.dpad &= ~PlayerInput::DIR_RIGHT;
						break;
					case SDLK_SPACE:
					case SDLK_RETURN:
						input.dpad &= ~PlayerInput::DIR_BUTTON;
						break;
					default:
						break;
				}
				break;
			case SDL_KEYDOWN:
				switch(ev.key.keysym.sym) {
					case SDLK_UP:
						input.dpad |= PlayerInput::DIR_UP;
						break;
					case SDLK_DOWN:
						input.dpad |= PlayerInput::DIR_DOWN;
						break;
					case SDLK_LEFT:
						input.dpad |= PlayerInput::DIR_LEFT;
						break;
					case SDLK_RIGHT:
						input.dpad |= PlayerInput::DIR_RIGHT;
						break;
					case SDLK_SPACE:
					case SDLK_RETURN:
						input.dpad |= PlayerInput::DIR_BUTTON;
						break;
					case SDLK_c:
						input.code = true;
						break;
					case SDLK_p:
                        if(input.pause == false) {
                            input.pause = true;
                        }
                        else {
                            input.pause = false;
                        }
						break;
					case SDLK_TAB :
						if ((_scale = _scale + 1) > 4) {
                            _scale = 1;
                        }
						switchGfxMode();
						break;
					case SDLK_ESCAPE:
						input.quit = true;
						break;
					default:
						break;
				}
				break;
			default:
				break;
		}
	}
}

void SDLStub::sleepFor(uint32_t duration) {
	SDL_Delay(duration);
}

uint32_t SDLStub::getTimeStamp() {
	return SDL_GetTicks();
}

void SDLStub::startAudio(AudioCallback callback, void *param) {
	SDL_AudioSpec desired;
	memset(&desired, 0, sizeof(desired));

	desired.freq = DEFAULT_AUDIO_SAMPLE_RATE;
	desired.format = AUDIO_F32SYS;
	desired.channels = 1;
	desired.samples = DEFAULT_AUDIO_SAMPLE_RATE / 25;
	desired.callback = callback;
	desired.userdata = param;
	if (SDL_OpenAudio(&desired, NULL) == 0) {
		SDL_PauseAudio(0);
	} else {
		log_fatal("SDLStub::startAudio() unable to open sound device");
	}
}

void SDLStub::stopAudio() {
	SDL_CloseAudio();
}

uint32_t SDLStub::getAudioSampleRate() {
	return DEFAULT_AUDIO_SAMPLE_RATE;
}

int SDLStub::addTimer(uint32_t delay, TimerCallback callback, void *param) {
	return SDL_AddTimer(delay, (SDL_TimerCallback)callback, param);
}

void SDLStub::removeTimer(int timerId) {
	SDL_RemoveTimer(timerId);
}

void SDLStub::cleanupGfxMode() {
	if (_texture != nullptr) {
		SDL_DestroyTexture(_texture);
		_texture = nullptr;
	}

	if (_renderer != nullptr) {
		SDL_DestroyRenderer(_renderer);
		_renderer = nullptr;
	}

	if (_window != nullptr) {
		SDL_DestroyWindow(_window);
		_window = nullptr;
	}
}

void SDLStub::switchGfxMode() {
	cleanupGfxMode();
	prepareGfxMode();
}

SDLStub sysImplementation;
System *stub = &sysImplementation;

// ---------------------------------------------------------------------------
// End-Of-File
// ---------------------------------------------------------------------------

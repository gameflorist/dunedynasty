/* ScummVM - Graphic Adventure Engine
 *
 * ScummVM is the legal property of its developers, whose names
 * are too numerous to list here. Please refer to the COPYRIGHT
 * file distributed with this source distribution.
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * as published by the Free Software Foundation; either version 2
 * of the License, or (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301, USA.
 *
 * LGPL License
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either
 * version 2.1 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public
 * License along with this library; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301  USA
 *
 */

#ifndef SOUND_ADLIB_H
#define SOUND_ADLIB_H

#ifdef __USE_SDL__
#include <SDL_rwops.h>
#else
#include <allegro5/allegro.h>
#endif
#include <stdint.h>
#include <vector>
#ifdef __USE_SDL__
#include <SDL.h>
#include <SDL_mixer.h>
#endif

class AdLibDriver;

/**
 * AdLib implementation of the sound output device.
 *
 * It uses a special sound file format special to
 * Dune II, Kyrandia 1 and 2. While Dune II and
 * Kyrandia 1 are using exact the same format, the
 * one of Kyrandia 2 slightly differs.
 *
 * See AdLibDriver for more information.
 * @see AdLibDriver
 */
class SoundAdLibPC {
public:
	typedef int8_t Sint8;
	typedef uint8_t Uint8;
	typedef int16_t Sint16;
	typedef uint16_t Uint16;

#ifdef __USE_SDL__
	SoundAdLibPC(SDL_RWops* rwop, bool bMAME = true);
#endif
	SoundAdLibPC(ALLEGRO_FILE* rwop, int freq, bool bMAME = true);
	~SoundAdLibPC();

	static void callback(void *, Uint8 *, int);

	std::vector<int> getSubsongs();

	bool init();
	void process();

	void playTrack(uint8_t track);
	void haltTrack();

	bool isPlaying() const;

	void playSoundEffect(uint8_t track);

	void beginFadeOut();

#ifdef __USE_SDL__
	Mix_Chunk* getSubsong(int Num);
#endif
private:
	void internalLoadFile(ALLEGRO_FILE* rwop);

	void play(uint8_t track);

	AdLibDriver *_driver;

	uint8_t _trackEntries[500];
	uint8_t *_soundDataPtr;
	int _sfxPlayingSound;

	uint8_t _sfxPriority;
	uint8_t _sfxFourthByteOfSong;

	int _numSoundTriggers;
	const int *_soundTriggers;

	unsigned char getsampsize();

	int m_channels;
	int m_freq;
	Uint16 m_format;

	bool bJustStartedPlaying;
};

#endif

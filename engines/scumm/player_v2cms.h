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

 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.

 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301, USA.
 *
 * $URL$
 * $Id$
 *
 */

#ifndef SCUMM_PLAYER_V2CMS_H
#define SCUMM_PLAYER_V2CMS_H

#include "scumm/player_v2.h"	// for channel_data

class CMSEmulator;

namespace Scumm {

class ScummEngine;


/**
 * Scumm V2 CMS/Gameblaster MIDI driver.
 */
class Player_V2CMS : public Audio::AudioStream, public MusicEngine {
public:
	Player_V2CMS(ScummEngine *scumm, Audio::Mixer *mixer);
	virtual ~Player_V2CMS();

	virtual void setMusicVolume(int vol);
	virtual void startSound(int sound);
	virtual void stopSound(int sound);
	virtual void stopAllSounds();
	virtual int  getMusicTimer();
	virtual int  getSoundStatus(int sound) const;

	// AudioStream API
	int readBuffer(int16 *buffer, const int numSamples);
	bool isStereo() const { return true; }
	bool endOfData() const { return false; }
	int getRate() const { return _sampleRate; }

protected:

#include "common/pack-start.h"	// START STRUCT PACKING
	struct Voice {
		byte attack;
		byte decay;
		byte sustain;
		byte release;
		byte octadd;
		int16 vibrato;
		int16 vibrato2;
		int16 noise;
	} PACKED_STRUCT;

	struct Voice2 {
		byte *amplitudeOutput;
		byte *freqOutput;
		byte *octaveOutput;

		uint8 channel;
		int8 sustainLevel;
		int8 attackRate;
		uint8 maxAmpl;
		int8 decayRate;
		int8 sustainRate;
		int8 releaseRate;
		int8 releaseTime;
		int8 vibratoRate;
		int8 vibratoDepth;

		int8 curVibratoRate;
		int8 curVibratoUnk;

		int8 unkVibratoRate;
		int8 unkVibratoDepth;

		int8 unkRate;
		int8 unkCount;

		int nextProcessState;
		int8 curVolume;
		int8 curOctave;
		int8 curFreq;

		int8 octaveAdd;

		int8 playingNote;
		Voice2 *nextVoice;

		byte chanNumber;
	} PACKED_STRUCT;

	struct MusicChip {
		byte ampl[4];
		byte freq[4];
		byte octave[2];
	} PACKED_STRUCT;
#include "common/pack-end.h"	// END STRUCT PACKING

	Voice _cmsVoicesBase[16];
	Voice2 _cmsVoices[8];
	MusicChip _cmsChips[2];

	int8 _tempo;
	int8 _tempoSum;
	byte _looping;
	byte _octaveMask;
	int16 _midiDelay;
	Voice2 *_midiChannel[16];
	byte _midiChannelUse[16];
	byte *_midiData;
	byte *_midiSongBegin;

	int _loadedMidiSong;

	byte _lastMidiCommand;
	uint _outputTableReady;
	byte _clkFrequenz;
	byte _restart;
	byte _curSno;

	void loadMidiData(byte *data, int sound);
	void play();

	void processChannel(Voice2 *channel);
	void processRelease(Voice2 *channel);
	void processAttack(Voice2 *channel);
	void processDecay(Voice2 *channel);
	void processSustain(Voice2 *channel);
	void processVibrato(Voice2 *channel);

	void playMusicChips(const MusicChip *table);
	void playNote(byte *&data);
	void clearNote(byte *&data);
	void offAllChannels();
	void playVoice();
	void processMidiData(uint ticks);

	Voice2 *getFreeVoice();
	Voice2 *getPlayVoice(byte param);

	// from Player_V2
protected:
	bool _isV3Game;
	Audio::Mixer *_mixer;
	Audio::SoundHandle _soundHandle;
	ScummEngine *_vm;

	CMSEmulator *_cmsEmu;

	int _header_len;

	uint32 _sampleRate;
	uint32 _next_tick;
	uint32 _tick_len;

	int _timer_count[4];
	int _timer_output;

	int   _current_nr;
	byte *_current_data;
	int   _next_nr;
	byte *_next_data;
	byte *_retaddr;

	Common::Mutex _mutex;

private:
	union ChannelInfo {
		channel_data d;
		uint16 array[sizeof(channel_data)/2];
	};

	int _music_timer;
	int _music_timer_ctr;
	int _ticks_per_music_timer;

	ChannelInfo _channels[5];

protected:
	virtual void nextTick();
	virtual void clear_channel(int i);
	virtual void chainSound(int nr, byte *data);
	virtual void chainNextSound();

private:
	void do_mix(int16 *buf, uint len);

	void execute_cmd(ChannelInfo *channel);
	void next_freqs(ChannelInfo *channel);
};

} // End of namespace Scumm

#endif

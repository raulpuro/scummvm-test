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

#ifndef TOWNS_PC98_FMSYNTH_H
#define TOWNS_PC98_FMSYNTH_H

#include "sound/audiostream.h"
#include "sound/mixer.h"
#include "common/list.h"

class TownsPC98_FmSynthOperator;
class TownsPC98_FmSynthSquareSineSource;
class TownsPC98_FmSynthPercussionSource;

enum EnvelopeState {
	kEnvReady,
	kEnvAttacking,
	kEnvDecaying,
	kEnvSustaining,
	kEnvReleasing
};

class TownsPC98_FmSynth : public Audio::AudioStream {
public:
	enum EmuType {
		kTypeTowns,
		kType26,
		kType86
	};

	TownsPC98_FmSynth(Audio::Mixer *mixer, EmuType type);
	virtual ~TownsPC98_FmSynth();

	virtual bool init();
	virtual void reset();

	void writeReg(uint8 part, uint8 regAddress, uint8 value);

	// AudioStream interface
	int readBuffer(int16 *buffer, const int numSamples);
	bool isStereo() const { return true; }
	bool endOfData() const { return false; }
	int getRate() const { return _mixer->getOutputRate(); }

protected:
	// Implement this in your inherited class if your driver generates
	// additional output that has to be inserted into the buffer.
	virtual void nextTickEx(int32 *buffer, uint32 bufferSize) {}

	void toggleRegProtection(bool prot) { _regProtectionFlag = prot; }
	uint8 readSSGStatus();

	virtual void timerCallbackA() = 0;
	virtual void timerCallbackB() = 0;

	// The audio driver can store and apply two different audio settings
	// (usually for music and sound effects). The channel mask will determine
	// which channels get effected by the setting. The first bits will be
	// the normal fm channels, the next bits the ssg channels and the final
	// bit the rhythm channel.
	void setVolumeIntern(int volA, int volB);
	void setVolumeChannelMasks(int channelMaskA, int channelMaskB);

	const int _numChan;
	const int _numSSG;
	const bool _hasPercussion;

	Common::Mutex _mutex;
private:
	void generateTables();
	void nextTick(int32 *buffer, uint32 bufferSize);
	void generateOutput(int32 &leftSample, int32 &rightSample, int32 *del, int32 *feed);

	struct ChanInternal {
		ChanInternal() {
			memset(this, 0, sizeof(ChanInternal));
		}

		~ChanInternal() {
			for (uint i = 0; i < ARRAYSIZE(opr); ++i)
				delete opr[i];
		}

		void ampModSensitivity(uint32 value) { ampModSvty = (1 << (3 - value)) - (((value >> 1) & 1) | (value & 1)); }
		void frqModSensitivity(uint32 value) { frqModSvty = value << 5; }

		uint16 frqTemp;
		bool enableLeft;
		bool enableRight;
		bool updateEnvelopeParameters;
		int32 feedbuf[3];
		uint8 algorithm;

		uint32 ampModSvty;
		uint32 frqModSvty;


		TownsPC98_FmSynthOperator *opr[4];
	};

	TownsPC98_FmSynthSquareSineSource *_ssg;
	TownsPC98_FmSynthPercussionSource *_prc;
	ChanInternal *_chanInternal;

	uint8 *_oprRates;
	uint8 *_oprRateshift;
	uint8 *_oprAttackDecay;
	uint32 *_oprFrq;
	uint32 *_oprSinTbl;
	int32 *_oprLevelOut;
	int32 *_oprDetune;

	bool _regProtectionFlag;

	typedef void (TownsPC98_FmSynth::*ChipTimerProc)();

	struct ChipTimer {
		bool enabled;
		uint16 value;

		int32 smpTillCb;
		uint32 smpTillCbRem;
		int32 smpPerCb;
		uint32 smpPerCbRem;

		ChipTimerProc cb;
	};

	ChipTimer _timers[2];

	int _volMaskA, _volMaskB;
	uint16 _volumeA, _volumeB;

	const float _baserate;
	uint32 _timerbase;

	Audio::Mixer *_mixer;
	Audio::SoundHandle _soundHandle;

	static const uint8 _percussionData[];
	static const uint32 _adtStat[];
	static const uint8 _detSrc[];
	static const int _ssgTables[];

	bool _ready;
};

#endif


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

#include "sound/audiostream.h"
#include "sound/raw.h"
#include "common/config-manager.h"

#include "sci/sci.h"
#include "sci/console.h"
#include "sci/resource.h"
#include "sci/engine/kernel.h"
#include "sci/engine/state.h"
#include "sci/sound/midiparser_sci.h"
#include "sci/sound/music.h"

namespace Sci {

// When defined, volume fading immediately sets the final sound volume
#define DISABLE_VOLUME_FADING

SciMusic::SciMusic(SciVersion soundVersion)
	: _soundVersion(soundVersion), _soundOn(true), _masterVolume(0) {

	// Reserve some space in the playlist, to avoid expensive insertion
	// operations
	_playList.reserve(10);
}

SciMusic::~SciMusic() {
	if (_pMidiDrv) {
		_pMidiDrv->close();
		delete _pMidiDrv;
	}
}

void SciMusic::init() {
	// system init
	_pMixer = g_system->getMixer();
	// SCI sound init
	_dwTempo = 0;

	MidiDriverType midiType = MidiDriver::detectMusicDriver(MDT_PCSPK | MDT_ADLIB | MDT_MIDI);

	switch (midiType) {
	case MD_ADLIB:
		// FIXME: There's no Amiga sound option, so we hook it up to AdLib
		if (((SciEngine *)g_engine)->getPlatform() == Common::kPlatformAmiga)
			_pMidiDrv = MidiPlayer_Amiga_create();
		else
			_pMidiDrv = MidiPlayer_AdLib_create();
		break;
	case MD_PCJR:
		_pMidiDrv = MidiPlayer_PCJr_create();
		break;
	case MD_PCSPK:
		_pMidiDrv = MidiPlayer_PCSpeaker_create();
		break;
	default:
		_pMidiDrv = MidiPlayer_Midi_create();
	}

	if (_pMidiDrv) {
		_pMidiDrv->open();
		_pMidiDrv->setTimerCallback(this, &miditimerCallback);
		_dwTempo = _pMidiDrv->getBaseTempo();
	} else
		warning("Can't initialise music driver");
	_bMultiMidi = ConfMan.getBool("multi_midi");
}

void SciMusic::clearPlayList() {
	Common::StackLock lock(_mutex);

	while (!_playList.empty()) {
		soundStop(_playList[0]);
		soundKill(_playList[0]);
	}
}

void SciMusic::pauseAll(bool pause) {
	Common::StackLock lock(_mutex);

	const MusicList::iterator end = _playList.end();
	for (MusicList::iterator i = _playList.begin(); i != end; ++i) {
		soundToggle(*i, pause);
	}
}

void SciMusic::stopAll() {
	Common::StackLock lock(_mutex);

	const MusicList::iterator end = _playList.end();
	for (MusicList::iterator i = _playList.begin(); i != end; ++i) {
		soundStop(*i);
	}
}


void SciMusic::miditimerCallback(void *p) {
	SciMusic *aud = (SciMusic *)p;

	Common::StackLock lock(aud->_mutex);
	aud->onTimer();
}

void SciMusic::soundSetSoundOn(bool soundOnFlag) {
	Common::StackLock lock(_mutex);

	_soundOn = soundOnFlag;
	_pMidiDrv->playSwitch(soundOnFlag);
}

uint16 SciMusic::soundGetVoices() {
	Common::StackLock lock(_mutex);

	return _pMidiDrv->getPolyphony();
}

MusicEntry *SciMusic::getSlot(reg_t obj) {
	Common::StackLock lock(_mutex);

	const MusicList::iterator end = _playList.end();
	for (MusicList::iterator i = _playList.begin(); i != end; ++i) {
		if ((*i)->soundObj == obj)
			return *i;
	}

	return NULL;
}

void SciMusic::setReverb(byte reverb) {
	Common::StackLock lock(_mutex);
	_pMidiDrv->setReverb(reverb);
}

static int f_compare(const void *arg1, const void *arg2) {
	return ((const MusicEntry *)arg2)->prio - ((const MusicEntry *)arg1)->prio;
}

void SciMusic::sortPlayList() {
	MusicEntry ** pData = _playList.begin();
	qsort(pData, _playList.size(), sizeof(MusicEntry *), &f_compare);
}
void SciMusic::soundInitSnd(MusicEntry *pSnd) {
	int channelFilterMask = 0;
	SoundResource::Track *track = pSnd->soundRes->getTrackByType(_pMidiDrv->getPlayId(_soundVersion));

	if (track) {
		// If MIDI device is selected but there is no digital track in sound resource
		// try to use adlib's digital sample if possible
		if (_bMultiMidi && (track->digitalChannelNr == -1)) {
			SoundResource::Track *digital = pSnd->soundRes->getDigitalTrack();
			if (digital)
				track = digital;
		}

		// Play digital sample
		if (track->digitalChannelNr != -1) {
			byte *channelData = track->channels[track->digitalChannelNr].data;
			delete pSnd->pStreamAud;
			pSnd->pStreamAud = Audio::makeRawMemoryStream(channelData, track->digitalSampleSize, DisposeAfterUse::NO, track->digitalSampleRate, Audio::FLAG_UNSIGNED);
			delete pSnd->pLoopStream;
			pSnd->pLoopStream = 0;
			pSnd->soundType = Audio::Mixer::kSFXSoundType;
			pSnd->hCurrentAud = Audio::SoundHandle();
		} else {
			// play MIDI track
			_mutex.lock();
			pSnd->soundType = Audio::Mixer::kMusicSoundType;
			if (pSnd->pMidiParser == NULL) {
				pSnd->pMidiParser = new MidiParser_SCI(_soundVersion);
				pSnd->pMidiParser->setMidiDriver(_pMidiDrv);
				pSnd->pMidiParser->setTimerRate(_dwTempo);
			}

			pSnd->pauseCounter = 0;

			// Find out what channels to filter for SCI0
			channelFilterMask = pSnd->soundRes->getChannelFilterMask(_pMidiDrv->getPlayId(_soundVersion), _pMidiDrv->hasRhythmChannel());
			pSnd->pMidiParser->loadMusic(track, pSnd, channelFilterMask, _soundVersion);

			// Fast forward to the last position and perform associated events when loading
			pSnd->pMidiParser->jumpToTick(pSnd->ticker, true);
			_mutex.unlock();
		}
	}
}

void SciMusic::onTimer() {
	const MusicList::iterator end = _playList.end();
	for (MusicList::iterator i = _playList.begin(); i != end; ++i)
		(*i)->onTimer();
}

void SciMusic::soundPlay(MusicEntry *pSnd) {
	_mutex.lock();

	uint sz = _playList.size(), i;
	// searching if sound is already in _playList
	for (i = 0; i < sz && _playList[i] != pSnd; i++)
		;
	if (i == sz) {// not found
		_playList.push_back(pSnd);
		sortPlayList();
	}
	
	_mutex.unlock();	// unlock to perform mixer-related calls

	if (pSnd->pStreamAud && !_pMixer->isSoundHandleActive(pSnd->hCurrentAud)) {
		if (pSnd->loop > 1) {
			pSnd->pLoopStream = new Audio::LoopingAudioStream(pSnd->pStreamAud,
			                                                  pSnd->loop, DisposeAfterUse::NO);
			_pMixer->playInputStream(pSnd->soundType, &pSnd->hCurrentAud,
			                         pSnd->pLoopStream, -1, pSnd->volume, 0,
			                         DisposeAfterUse::NO);
		} else {
			_pMixer->playInputStream(pSnd->soundType, &pSnd->hCurrentAud,
			                         pSnd->pStreamAud, -1, pSnd->volume, 0,
			                         DisposeAfterUse::NO);
		}
	} else {
		_mutex.lock();
		if (pSnd->pMidiParser) {
			pSnd->pMidiParser->setVolume(pSnd->volume);
			if (pSnd->status == kSoundStopped)
				pSnd->pMidiParser->jumpToTick(0);
		}
		_mutex.unlock();
	}

	pSnd->status = kSoundPlaying;
}

void SciMusic::soundStop(MusicEntry *pSnd) {
	pSnd->status = kSoundStopped;
	if (pSnd->pStreamAud)
		_pMixer->stopHandle(pSnd->hCurrentAud);

	_mutex.lock();
	if (pSnd->pMidiParser)
		pSnd->pMidiParser->stop();
	_mutex.unlock();
}

void SciMusic::soundSetVolume(MusicEntry *pSnd, byte volume) {
	assert(volume <= MUSIC_VOLUME_MAX);
	if (pSnd->pStreamAud) {
		_pMixer->setChannelVolume(pSnd->hCurrentAud, volume * 2); // Mixer is 0-255, SCI is 0-127
	} else if (pSnd->pMidiParser) {
		_mutex.lock();
		pSnd->pMidiParser->setVolume(volume);
		_mutex.unlock();
	}
}

void SciMusic::soundSetPriority(MusicEntry *pSnd, byte prio) {
	Common::StackLock lock(_mutex);

	pSnd->prio = prio;
	sortPlayList();
}

void SciMusic::soundKill(MusicEntry *pSnd) {
	pSnd->status = kSoundStopped;

	_mutex.lock();
	if (pSnd->pMidiParser) {
		pSnd->pMidiParser->unloadMusic();
		delete pSnd->pMidiParser;
		pSnd->pMidiParser = NULL;
	}
	_mutex.unlock();

	if (pSnd->pStreamAud) {
		_pMixer->stopHandle(pSnd->hCurrentAud);
		delete pSnd->pStreamAud;
		pSnd->pStreamAud = NULL;
		delete pSnd->pLoopStream;
		pSnd->pLoopStream = 0;
	}

	_mutex.lock();
	uint sz = _playList.size(), i;
	// Remove sound from playlist
	for (i = 0; i < sz; i++) {
		if (_playList[i] == pSnd) {
			delete _playList[i]->soundRes;
			delete _playList[i];
			_playList.remove_at(i);
			break;
		}
	}
	_mutex.unlock();
}

void SciMusic::soundPause(MusicEntry *pSnd) {
	pSnd->pauseCounter++;
	if (pSnd->status != kSoundPlaying)
		return;
	pSnd->status = kSoundPaused;
	if (pSnd->pStreamAud) {
		_pMixer->pauseHandle(pSnd->hCurrentAud, true);
	} else {
		_mutex.lock();
		if (pSnd->pMidiParser)
			pSnd->pMidiParser->pause();
		_mutex.unlock();
	}
}

void SciMusic::soundResume(MusicEntry *pSnd) {
	if (pSnd->pauseCounter > 0)
		pSnd->pauseCounter--;
	if (pSnd->pauseCounter != 0)
		return;
	if (pSnd->status != kSoundPaused)
		return;
	soundPlay(pSnd);
}

void SciMusic::soundToggle(MusicEntry *pSnd, bool pause) {
	if (pause)
		soundPause(pSnd);
	else
		soundResume(pSnd);
}

uint16 SciMusic::soundGetMasterVolume() {
	return _masterVolume;
}

void SciMusic::soundSetMasterVolume(uint16 vol) {
	_masterVolume = vol;

	Common::StackLock lock(_mutex);

	if (_pMidiDrv)
		_pMidiDrv->setVolume(vol);
}

void SciMusic::printPlayList(Console *con) {
	Common::StackLock lock(_mutex);

	const char *musicStatus[] = { "Stopped", "Initialized", "Paused", "Playing" };

	for (uint32 i = 0; i < _playList.size(); i++) {
		MusicEntry *song = _playList[i];
		con->DebugPrintf("%d: %04x:%04x, resource id: %d, status: %s, %s type\n", i, 
						PRINT_REG(song->soundObj), song->resnum,
						musicStatus[song->status], song->pMidiParser ? "MIDI" : "digital audio");
	}
}

void SciMusic::printSongInfo(reg_t obj, Console *con) {
	Common::StackLock lock(_mutex);

	const char *musicStatus[] = { "Stopped", "Initialized", "Paused", "Playing" };

	const MusicList::iterator end = _playList.end();
	for (MusicList::iterator i = _playList.begin(); i != end; ++i) {
		MusicEntry *song = *i;
		if (song->soundObj == obj) {
			con->DebugPrintf("Resource id: %d, status: %s\n", song->resnum, musicStatus[song->status]);
			con->DebugPrintf("dataInc: %d, hold: %d, loop: %d\n", song->dataInc, song->hold, song->loop);
			con->DebugPrintf("signal: %d, priority: %d\n", song->signal, song->prio);
			con->DebugPrintf("ticker: %d, volume: %d\n", song->ticker, song->volume);

			if (song->pMidiParser) {
				con->DebugPrintf("Type: MIDI\n");
				if (song->soundRes) {
					SoundResource::Track *track = song->soundRes->getTrackByType(_pMidiDrv->getPlayId(_soundVersion));
					con->DebugPrintf("Channels: %d\n", track->channelCount);
				}
			} else if (song->pStreamAud || song->pLoopStream) {
				con->DebugPrintf("Type: digital audio (%s), sound active: %s\n",
					song->pStreamAud ? "non looping" : "looping",
					_pMixer->isSoundHandleActive(song->hCurrentAud) ? "yes" : "no");
				if (song->soundRes) {
					con->DebugPrintf("Sound resource information:\n");
					SoundResource::Track *track = song->soundRes->getTrackByType(_pMidiDrv->getPlayId(_soundVersion));
					if (track && track->digitalChannelNr != -1) {
						con->DebugPrintf("Sample size: %d, sample rate: %d, channels: %d, digital channel number: %d\n", 
							track->digitalSampleSize, track->digitalSampleRate, track->channelCount, track->digitalChannelNr);
					}
				}
			}

			return;
		}
	}

	con->DebugPrintf("Song object not found in playlist");
}

MusicEntry::MusicEntry() {
	soundObj = NULL_REG;

	soundRes = 0;
	resnum = 0;

	dataInc = 0;
	ticker = 0;
	signal = 0;
	prio = 0;
	loop = 0;
	volume = MUSIC_VOLUME_DEFAULT;
	hold = 0;

	pauseCounter = 0;
	sampleLoopCounter = 0;

	fadeTo = 0;
	fadeStep = 0;
	fadeTicker = 0;
	fadeTickerStep = 0;
	fadeSetVolume = false;
	fadeCompleted = false;
	stopAfterFading = false;

	status = kSoundStopped;

	soundType = Audio::Mixer::kMusicSoundType;

	pStreamAud = 0;
	pLoopStream = 0;
	pMidiParser = 0;
}

MusicEntry::~MusicEntry() {
}

void MusicEntry::onTimer() {
	if (status != kSoundPlaying)
		return;

	// Fade MIDI and digital sound effects
	if (fadeStep)
		doFade();

	// Only process MIDI streams in this thread, not digital sound effects
	if (pMidiParser) {
		pMidiParser->onTimer();
		ticker = (uint16)pMidiParser->getTick();
	}
}

void MusicEntry::doFade() {
	if (fadeTicker)
		fadeTicker--;
	else {
		int16 fadeVolume = volume;
		fadeTicker = fadeTickerStep;
		fadeVolume += fadeStep;
		if (((fadeStep > 0) && (fadeVolume >= fadeTo)) || ((fadeStep < 0) && (fadeVolume <= fadeTo))) {
			fadeVolume = fadeTo;
			fadeStep = 0;
			fadeCompleted = true;
		}
		volume = fadeVolume;

		// Only process MIDI streams in this thread, not digital sound effects
		if (pMidiParser)
#ifndef DISABLE_VOLUME_FADING
			pMidiParser->setVolume(volume);
#else
			pMidiParser->setVolume(fadeTo);
#endif
		fadeSetVolume = true; // set flag so that SoundCommandParser::cmdUpdateCues will set the volume of the stream
	}
}

} // End of namespace Sci

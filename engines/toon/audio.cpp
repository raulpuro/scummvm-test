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

#include "toon/audio.h"

namespace Toon {

static int ADPCM_index[8] = {
	-1, -1, -1, -1, 2 , 4 , 6 , 8
};
static int ADPCM_table[89] = {
	7,     8,     9,     10,    11,    12,    13,    14,    16,    17,
	19,    21,    23,    25,    28,    31,    34,    37,    41,    45,
	50,    55,    60,    66,    73,    80,    88,    97,    107,   118,
	130,   143,   157,   173,   190,   209,   230,   253,   279,   307,
	337,   371,   408,   449,   494,   544,   598,   658,   724,   796,
	876,   963,   1060,  1166,  1282,  1411,  1552,  1707,  1878,  2066,
	2272,  2499,  2749,  3024,  3327,  3660,  4026,  4428,  4871,  5358,
	5894,  6484,  7132,  7845,  8630,  9493,  10442, 11487, 12635, 13899,
	15289, 16818, 18500, 20350, 22385, 24623, 27086, 29794, 32767
};

AudioManager::AudioManager(ToonEngine *vm, Audio::Mixer *mixer) : _vm(vm), _mixer(mixer) {
	for (int32 i = 0; i < 16; i++)
		_channels[i] = 0;

	voiceMuted = false;
	musicMuted = false;
	sfxMuted = false;
}

AudioManager::~AudioManager(void) {
}

void AudioManager::muteMusic(bool muted) {
	setMusicVolume(muted ? 0 : 255);
	musicMuted = muted;
}

void AudioManager::muteVoice(bool muted) {
	if(voiceStillPlaying() && _channels[2]) {
		_channels[2]->setVolume(muted ? 0 : 255);
	}
	voiceMuted = muted;
}

void AudioManager::muteSfx(bool muted) {
	sfxMuted = muted;
}

void AudioManager::removeInstance(AudioStreamInstance *inst) {
	debugC(1, kDebugAudio, "removeInstance(inst)");

	for (int32 i = 0; i < 16; i++) {
		if (inst == _channels[i])
			_channels[i] = 0;
	}
}

void AudioManager::playMusic(Common::String dir, Common::String music) {
	debugC(1, kDebugAudio, "playMusic(%s, %s)", dir.c_str(), music.c_str());

	// two musics can be played at same time
	Common::String path = Common::String::printf("act%d/%s/%s.mus", _vm->state()->_currentChapter, dir.c_str(), music.c_str());

	if (_currentMusicName == music)
		return;

	_currentMusicName = music;

	Common::SeekableReadStream *srs = _vm->resources()->openFile(path);
	if (!srs)
		return;

	// see what channel to take
	if (_channels[0] && _channels[0]->isPlaying() && _channels[1] && _channels[1]->isPlaying()) {
		// take the one that is fading
		if (_channels[0]->isFading()) {
			_channels[0]->stop(false);
			_channels[1]->stop(true);
			_currentMusicChannel = 0;
		} else {
			_channels[1]->stop(false);
			_channels[0]->stop(true);
			_currentMusicChannel = 1;
		}
	} else if (_channels[0] && _channels[0]->isPlaying()) {
		_channels[0]->stop(true);
		_currentMusicChannel = 1;
	} else {
		if (_channels[1] && _channels[1]->isPlaying())
			_channels[1]->stop(true);
		_currentMusicChannel = 0;
	}

	
	//if (!_channels[_currentMusicChannel])
	//	delete _channels[_currentMusicChannel];
	_channels[_currentMusicChannel] = new AudioStreamInstance(this, _mixer, srs, true);
	_channels[_currentMusicChannel]->setVolume(musicMuted ? 0 : 255);
	_channels[_currentMusicChannel]->play(true, Audio::Mixer::kMusicSoundType);
}

bool AudioManager::voiceStillPlaying() {
	if (!_channels[2])
		return false;

	return _channels[2]->isPlaying();
}

void AudioManager::playVoice(int32 id, bool genericVoice) {
	debugC(1, kDebugAudio, "playVoice(%d, %d)", id, (genericVoice) ? 1 : 0);

	if (voiceStillPlaying()) {
		// stop current voice
		_channels[2]->stop(false);
	}

	Common::SeekableReadStream *stream;
	if (genericVoice)
		stream = _audioPacks[0]->getStream(id);
	else
		stream = _audioPacks[1]->getStream(id);

	_channels[2] = new AudioStreamInstance(this, _mixer, stream);
	_channels[2]->play(false, Audio::Mixer::kSpeechSoundType);
	_channels[2]->setVolume(voiceMuted ? 0 : 255);

}

void AudioManager::playSFX(int32 id, int volume , bool genericSFX) {
	debugC(4, kDebugAudio, "playSFX(%d, %d)", id, (genericSFX) ? 1 : 0);

	// find a free SFX channel
	Common::SeekableReadStream *stream;

	if (genericSFX)
		stream = _audioPacks[2]->getStream(id, true);
	else
		stream = _audioPacks[3]->getStream(id, true);

	if (stream->size() == 0)
		return;

	for (int32 i = 3; i < 16; i++) {
		if (!_channels[i]) {
			_channels[i] = new AudioStreamInstance(this, _mixer, stream);
			_channels[i]->play(false, Audio::Mixer::kSFXSoundType);
			_channels[i]->setVolume(sfxMuted ? 0 : volume);
			break;
		}
	}
}

void AudioManager::stopCurrentVoice() {
	debugC(1, kDebugAudio, "stopCurrentVoice()");

	if (_channels[2] && _channels[2]->isPlaying())
		_channels[2]->stop(false);
}

bool AudioManager::loadAudioPack(int32 id, Common::String indexFile, Common::String packFile) {
	debugC(4, kDebugAudio, "loadAudioPack(%d, %s, %s)", id, indexFile.c_str(), packFile.c_str());

	_audioPacks[id] = new AudioStreamPackage(_vm);
	return _audioPacks[id]->loadAudioPackage(indexFile, packFile);
}

void AudioManager::setMusicVolume(int32 volume) {
	debugC(1, kDebugAudio, "setMusicVolume(%d)", volume);
	if (_channels[0])
		_channels[0]->setVolume(volume);

	if (_channels[1])
		_channels[1]->setVolume(volume);
}

void AudioManager::stopMusic() {
	debugC(1, kDebugAudio, "stopMusic()");

	if (_channels[0])
		_channels[0]->stop(true);
	if (_channels[1])
		_channels[1]->stop(true);
}
AudioStreamInstance::~AudioStreamInstance() {
	if (_man)
		_man->removeInstance(this);
}

AudioStreamInstance::AudioStreamInstance(AudioManager *man, Audio::Mixer *mixer, Common::SeekableReadStream *stream , bool looping) {
	_compBufferSize = 0;
	_buffer = 0;
	_bufferMaxSize = 0;
	_mixer = mixer;
	_compBuffer = 0;
	_bufferOffset = 0;
	_lastADPCMval1 = 0;
	_lastADPCMval2 = 0;
	_file = stream;
	_fadingIn = false;
	_fadingOut = false;
	_fadeTime = 0;
	_stopped = false;
	_volume = 255;
	_totalSize = stream->size();
	_currentReadSize = 8;
	_man = man;
	_looping = looping;
	_musicAttenuation = 1000;

	// preload one packet
	if (_totalSize > 0) {
		_file->skip(8);
		readPacket();
	} else {
		stopNow();
	}
}

int AudioStreamInstance::readBuffer(int16 *buffer, const int numSamples) {
	debugC(5, kDebugAudio, "readBuffer(buffer, %d)", numSamples);

	handleFade(numSamples);
	int32 leftSamples = numSamples;
	int32 destOffset = 0;
	if ((_bufferOffset + leftSamples) * 2 >= _bufferSize) {
		if (_bufferSize - _bufferOffset * 2 > 0) {
			memcpy(buffer, &_buffer[_bufferOffset], _bufferSize - _bufferOffset * 2);
			leftSamples -= (_bufferSize - _bufferOffset * 2) / 2;
			destOffset += (_bufferSize - _bufferOffset * 2) / 2;
		}
		if (!readPacket())
			return 0;

		_bufferOffset = 0;
	}

	if (leftSamples >= 0) {
		memcpy(buffer + destOffset, &_buffer[_bufferOffset], MIN(leftSamples * 2, _bufferSize));
		_bufferOffset += leftSamples;
	}

	return numSamples;
}

bool AudioStreamInstance::readPacket() {
	debugC(5, kDebugAudio, "readPacket()");

	if (_file->eos() || (_currentReadSize >= _totalSize)) {
		if (_looping) {
			_file->seek(8);
			_currentReadSize = 8;
			_lastADPCMval1 = 0;
			_lastADPCMval2 = 0;
		} else {
			_bufferSize = 0;
			stopNow();
			return false;
		}
	}
	int16 numCompressedBytes = _file->readSint16LE();
	int16 numDecompressedBytes = _file->readSint16LE();
	_file->readSint32LE();

	if (numCompressedBytes > _compBufferSize) {
		if (_compBuffer)
			delete[] _compBuffer;
		_compBufferSize = numCompressedBytes;
		_compBuffer = new uint8[_compBufferSize];
	}

	if (numDecompressedBytes > _bufferMaxSize) {
		if (_buffer)
			delete [] _buffer;
		_bufferMaxSize = numDecompressedBytes;
		_buffer = new int16[numDecompressedBytes];
	}

	_bufferSize = numDecompressedBytes;
	_file->read(_compBuffer, numCompressedBytes);
	_currentReadSize += 8 + numCompressedBytes;

	decodeADPCM(_compBuffer, _buffer, numCompressedBytes);
	return true;
}

void AudioStreamInstance::decodeADPCM(uint8 *comp, int16 *dest, int32 packetSize) {
	debugC(5, kDebugAudio, "decodeADPCM(comp, dest, %d)", packetSize);

	int32 numSamples = 2 * packetSize;
	int32 v18 = _lastADPCMval1;
	int32 v19 = _lastADPCMval2;
	for (int32 i = 0; i < numSamples; i++) {
		uint8 comm = *comp;

		int32 v29 = i & 1;
		int32 v30;
		if (v29 == 0)
			v30 = comm & 0xf;
		else
			v30 = (comm & 0xf0) >> 4;

		int32 v31 = v30 & 0x8;
		int32 v32 = v30 & 0x7;
		int32 v33 = ADPCM_table[v19];
		int32 v34 = v33 >> 3;
		if (v32 & 4)
			v34 += v33;

		if (v32 & 2)
			v34 += v33 >> 1;

		if (v32 & 1)
			v34 += v33 >> 2;

		v19 += ADPCM_index[v32];
		if (v19 < 0)
			v19 = 0;

		if (v19 > 88)
			v19 = 88;

		if (v31)
			v18 -= v34;
		else
			v18 += v34;

		if (v18 > 32767)
			v18 = 32767;
		else if (v18 < -32768)
			v18 = -32768;

		*dest = v18;
		comp += v29;
		dest++;
	}

	_lastADPCMval1 = v18;
	_lastADPCMval2 = v19;
}

void AudioStreamInstance::play(bool fade, Audio::Mixer::SoundType soundType) {
	debugC(1, kDebugAudio, "play(%d)", (fade) ? 1 : 0);

	Audio::SoundHandle soundHandle;
	_stopped = false;
	_fadingIn = fade;
	_fadeTime = 0;
	_soundType = soundType;
	_musicAttenuation = 1000; // max volume
	_mixer->playStream(soundType, &_handle, this, -1);
	handleFade(0);
}

void AudioStreamInstance::handleFade(int32 numSamples) {
	debugC(5, kDebugAudio, "handleFade(%d)", numSamples);

	// Fading enabled only for music
	if (_soundType != Audio::Mixer::kMusicSoundType) 
		return;

	int32 finalVolume = _volume;

	if (_fadingOut) {
		_fadeTime += numSamples;

		if (_fadeTime > 40960) {
			_fadeTime = 40960;
			stopNow();
			_fadingOut = false;
		}
		finalVolume = _volume - _fadeTime * _volume / 40960;
	} else {
		if (_fadingIn) {
			_fadeTime += numSamples;
			if (_fadeTime > 40960) {
				_fadeTime = 40960;
				_fadingIn = false;
			}

			finalVolume = _volume * _fadeTime / 40960;
		}
	}

	// the music is too loud when someone is talking
	// smoothing to avoid big volume changes
	if (_man->voiceStillPlaying()) {
		_musicAttenuation -= numSamples >> 4;
		if (_musicAttenuation < 250)
			_musicAttenuation = 250;
	} else {
		_musicAttenuation += numSamples >> 5;
		if (_musicAttenuation > 1000) 
			_musicAttenuation = 1000;
	}


	_mixer->setChannelVolume(_handle, finalVolume * _musicAttenuation / 1000);

}

void AudioStreamInstance::stop(bool fade /*= false*/) {
	debugC(1, kDebugAudio, "stop(%d)", (fade) ? 1 : 0);

	if (fade) {
		_fadingIn = false;
		_fadingOut = true;
		_fadeTime = 0;
	} else {
		stopNow();
	}
}

void AudioStreamInstance::stopNow() {
	debugC(1, kDebugAudio, "stopNow()");

	_stopped = true;
}

void AudioStreamInstance::setVolume(int32 volume) {
	debugC(1, kDebugAudio, "setVolume(%d)", volume);

	_volume = volume;
	_mixer->setChannelVolume(_handle, volume);
}

AudioStreamPackage::AudioStreamPackage(ToonEngine *vm) : _vm(vm) {
	_indexBuffer = 0;
}

AudioStreamPackage::~AudioStreamPackage() {
	delete[] _indexBuffer;
}

bool AudioStreamPackage::loadAudioPackage(Common::String indexFile, Common::String streamFile) {
	debugC(4, kDebugAudio, "loadAudioPackage(%s, %s)", indexFile.c_str(), streamFile.c_str());

	uint32 size = 0;
	uint8 *fileData = _vm->resources()->getFileData(indexFile, &size);
	if (!fileData)
		return false;

	delete[] _indexBuffer;

	_indexBuffer = new uint32[size / 4];
	memcpy(_indexBuffer, fileData, size);

	_file = _vm->resources()->openFile(streamFile);
	if (!_file)
		return false;

	return true;
}

void AudioStreamPackage::getInfo(int32 id, int32 *offset, int32 *size) {
	debugC(1, kDebugAudio, "getInfo(%d, offset, size)", id);

	*offset = READ_LE_UINT32(_indexBuffer + id);
	*size = READ_LE_UINT32(_indexBuffer + id + 1) - READ_LE_UINT32(_indexBuffer + id);
}

Common::SeekableReadStream *AudioStreamPackage::getStream(int32 id, bool ownMemory) {
	debugC(1, kDebugAudio, "getStream(%d, %d)", id, (ownMemory) ? 1 : 0);

	int32 offset = 0;
	int32 size = 0;
	getInfo(id, &offset, &size);
	if (ownMemory) {
		byte *memory = new byte[size];
		_file->seek(offset);
		_file->read(memory, size);
		return new Common::MemoryReadStream(memory, size, DisposeAfterUse::YES);
	} else {
		return new Common::SeekableSubReadStream(_file, offset, size + offset);
	}
}

} // End of namespace Toon


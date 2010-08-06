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

/* 
 * This code is based on Broken Sword 2.5 engine
 *
 * Copyright (c) Malte Thiesen, Daniel Queteschiner and Michael Elsdoerfer
 *
 * Licensed under GNU GPL v2
 *
 */

#ifndef SWORD25_FRAMECOUNTER_H
#define SWORD25_FRAMECOUNTER_H

// Includes
#include "sword25/kernel/common.h"
#include "sword25/kernel/bs_stdint.h"

namespace Sword25 {

/**
 * A simple class that implements a frame counter
 */
class BS_Framecounter {
private:
	enum {
		DEFAULT_UPDATE_FREQUENCY = 10
	};

public:
	/**
	 * Creates a new BS_Framecounter object
	 * @param UpdateFrequency	Specifies how often the frame counter should be updated in a sceond.
	 * The default value is 10.
	 */
	BS_Framecounter(int UpdateFrequency = DEFAULT_UPDATE_FREQUENCY);

	/**
	 * Determines how often the frame counter should be updated in a second.
	 * @param UpdateFrequency	Specifies how often the frame counter should be updated in a second.
	 */
	inline void SetUpdateFrequency(int UpdateFrequency);

	/**
	 * This method must be called once per frame.
	 */
	void Update();

	/**
	 * Returns the current FPS value.
	 */
	int GetFPS() const { return m_FPS; }

private:
	int	m_FPS;
	int m_FPSCount;
	int64_t m_LastUpdateTime;
	uint64_t m_UpdateDelay;
};

// Inlines
void BS_Framecounter::SetUpdateFrequency(int UpdateFrequency) {
	// Frequency in time (converted to microseconds)
	m_UpdateDelay = 1000000 / UpdateFrequency;
}

} // End of namespace Sword25

#endif

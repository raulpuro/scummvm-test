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

#include "common/config-manager.h"
#include "common/advancedDetector.h"
#include "common/savefile.h"
#include "common/system.h"

#include "base/plugins.h"

#include "tucker/tucker.h"

static const PlainGameDescriptor tuckerGames[] = {
	{ "tucker", "Bud Tucker in Double Trouble" },
	{ 0, 0 }
};

static const Common::ADGameDescription tuckerGameDescriptions[] = {
	{
		"tucker",
		"",
		AD_ENTRY1s("infobar.txt", "f1e42a95972643462b9c3c2ea79d6683", 543),
		Common::FR_FRA,
		Common::kPlatformPC,
		Common::ADGF_NO_FLAGS
	},
	{
		"tucker",
		"",
		AD_ENTRY1s("infobar.txt", "9c1ddeafc5283b90d1a284bd0924831c", 462),
		Common::EN_ANY,
		Common::kPlatformPC,
		Common::ADGF_NO_FLAGS
	},
	{
		"tucker",
		"",
		AD_ENTRY1s("infobar.txt", "1b3ea79d8528ea3c7df83dd0ed345e37", 525),
		Common::ES_ESP,
		Common::kPlatformPC,
		Common::ADGF_NO_FLAGS
	},
	AD_TABLE_END_MARKER
};

static const Common::ADParams detectionParams = {
	(const byte *)tuckerGameDescriptions,
	sizeof(Common::ADGameDescription),
	512,
	tuckerGames,
	0,
	"tucker",
	0,
	0
};

class TuckerMetaEngine : public Common::AdvancedMetaEngine {
public:
	TuckerMetaEngine() : Common::AdvancedMetaEngine(detectionParams) {
	}

	virtual const char *getName() const {
		return "Tucker Engine";
	}

	virtual const char *getCopyright() const {
		return "Bud Tucker in Double Trouble (C) Merit Studios";
	}

	virtual bool createInstance(OSystem *syst, Engine **engine, const Common::ADGameDescription *desc) const {
		if (desc) {
			*engine = new Tucker::TuckerEngine(syst, desc->language);
		}
		return desc != 0;
	}
};

#if PLUGIN_ENABLED_DYNAMIC(TUCKER)
	REGISTER_PLUGIN_DYNAMIC(TUCKER, PLUGIN_TYPE_ENGINE, TuckerMetaEngine);
#else
	REGISTER_PLUGIN_STATIC(TUCKER, PLUGIN_TYPE_ENGINE, TuckerMetaEngine);
#endif

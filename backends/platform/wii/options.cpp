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
 * Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA  02111-1307, USA.
 *
 */

#include <errno.h>
#include <network.h>
#include <gfx/gfx.h>

#include "common/config-manager.h"
#include "gui/dialog.h"
#include "backends/fs/wii/wii-fs-factory.h"

#include "options.h"

WiiOptionsDialog::WiiOptionsDialog(bool doubleStrike) :
	Dialog((640 - 400) / 2, (480 - 340) / 2, 400, 340),
	_doubleStrike(doubleStrike) {

	if (_doubleStrike) {
		_strUnderscanX = "wii_video_ds_underscan_x";
		_strUnderscanY = "wii_video_ds_underscan_y";
	} else {
		_strUnderscanX = "wii_video_default_underscan_x";
		_strUnderscanY = "wii_video_default_underscan_y";
	}

	new ButtonWidget(this, _w - 108 - 16, _h - 24 - 16, 108, 24, "Ok", 'k');
	new ButtonWidget(this, _w - 216 - 32, _h - 24 - 16, 108, 24, "Cancel", 'c');
	_tab = new TabWidget(this, 0, 0, _w, _h - 54);

	_tabVideo = _tab->addTab("Video");

	new StaticTextWidget(_tab, 16, 16, 128, 16,
							"Current video mode:", Graphics::kTextAlignRight);
	new StaticTextWidget(_tab, 160, 16, 128, 16,
							_doubleStrike ? "Double-strike" : "Default",
							Graphics::kTextAlignLeft);

	new StaticTextWidget(_tab, 16, 48, 128, 16,
							"Horizontal underscan:", Graphics::kTextAlignRight);
	_sliderUnderscanX = new SliderWidget(_tab, 160, 47, 128, 18, 'x');
	_sliderUnderscanX->setMinValue(0);
	_sliderUnderscanX->setMaxValue(32);

	new StaticTextWidget(_tab, 16, 80, 128, 16,
							"Vertical underscan:", Graphics::kTextAlignRight);
	_sliderUnderscanY = new SliderWidget(_tab, 160, 79, 128, 18, 'y');
	_sliderUnderscanY->setMinValue(0);
	_sliderUnderscanY->setMaxValue(32);

	_tabInput = _tab->addTab("Input");

	new StaticTextWidget(_tab, 16, 16, 128, 16,
							"GC Pad sensitivity:", Graphics::kTextAlignRight);
	_sliderPadSensitivity = new SliderWidget(_tab, 160, 15, 128, 18, 'x');
	_sliderPadSensitivity->setMinValue(0);
	_sliderPadSensitivity->setMaxValue(64);

	new StaticTextWidget(_tab, 16, 44, 128, 16,
							"GC Pad acceleration:", Graphics::kTextAlignRight);
	_sliderPadAcceleration = new SliderWidget(_tab, 160, 43, 128, 18, 'y');
	_sliderPadAcceleration->setMinValue(0);
	_sliderPadAcceleration->setMaxValue(8);

#ifdef USE_WII_DI
	_tabDVD = _tab->addTab("DVD");

	new StaticTextWidget(_tab, 16, 16, 64, 16,
							"Status:", Graphics::kTextAlignRight);
	_textDVDStatus = new StaticTextWidget(_tab, 96, 16, 272, 16, "Unknown",
											Graphics::kTextAlignLeft);

	new ButtonWidget(_tab, 16, 48, 108, 24, "Mount DVD", 'mdvd');
	new ButtonWidget(_tab, 140, 48, 108, 24, "Unmount DVD", 'udvd');
#endif

#ifdef USE_WII_SMB
	_tabSMB = _tab->addTab("SMB");

	new StaticTextWidget(_tab, 16, 16, 64, 16,
							"Status:", Graphics::kTextAlignRight);
	_textSMBStatus = new StaticTextWidget(_tab, 96, 16, 272, 16, "Unknown",
											Graphics::kTextAlignLeft);

	new StaticTextWidget(_tab, 16, 52, 64, 16,
							"Server:", Graphics::kTextAlignRight);
	_editSMBServer = new EditTextWidget(_tab, 96, 48, _w - 96 - 32, 24, "");

	new StaticTextWidget(_tab, 16, 92, 64, 16,
							"Share:", Graphics::kTextAlignRight);
	_editSMBShare = new EditTextWidget(_tab, 96, 88, _w - 96 - 32, 24, "");

	new StaticTextWidget(_tab, 16, 132, 64, 16,
							"Username:", Graphics::kTextAlignRight);
	_editSMBUsername = new EditTextWidget(_tab, 96, 128, _w - 96 - 32, 24, "");

	new StaticTextWidget(_tab, 16, 172, 64, 16,
							"Password:", Graphics::kTextAlignRight);
	_editSMBPassword = new EditTextWidget(_tab, 96, 168, _w - 96 - 32, 24, "");

	new ButtonWidget(_tab, 16, 208, 108, 24, "Init network", 'net');

	new ButtonWidget(_tab, 140, 208, 108, 24, "Mount SMB", 'msmb');
	new ButtonWidget(_tab, 264, 208, 108, 24, "Unmount SMB", 'usmb');
#endif

	_tab->setActiveTab(_tabVideo);

	load();
}

WiiOptionsDialog::~WiiOptionsDialog() {
}

void WiiOptionsDialog::handleTickle() {
#ifndef GAMECUBE
	WiiFilesystemFactory &fsf = WiiFilesystemFactory::instance();

	int tab = _tab->getActiveTab();
#endif

#ifdef USE_WII_DI
	if (tab == _tabDVD) {
		if (fsf.isMounted(WiiFilesystemFactory::kDVD)) {
			_textDVDStatus->setLabel("DVD Mounted successfully");
		} else {
			if (fsf.failedToMount(WiiFilesystemFactory::kDVD))
				_textDVDStatus->setLabel("Error while mounting the DVD");
			else
				_textDVDStatus->setLabel("DVD not mounted");
		}
	}
#endif

#ifdef USE_WII_SMB
	if (tab == _tabSMB) {
		s32 status = net_get_status();
		String label;

		switch (status) {
		case 0:
			if (fsf.isMounted(WiiFilesystemFactory::kSMB)) {
				label = "Network up, share mounted";
			} else {
				label = "Network up";

				if (fsf.failedToMount(WiiFilesystemFactory::kSMB))
					label += ", error while mounting the share";
				else
					label += ", share not mounted";
			}

			break;

		case -ENETDOWN:
			label = "Network down";
			break;

		case -EBUSY:
			label = "Initialising network";
			break;

		case -ETIMEDOUT:
			label = "Timeout while initialising network";
			break;

		default:
			label = String::printf("Network not initialsed (%d)", status);
			break;
		}

		_textSMBStatus->setLabel(label);
	}
#endif

	Dialog::handleTickle();
}

void WiiOptionsDialog::handleCommand(CommandSender *sender, uint32 cmd,
										uint32 data) {
#ifndef GAMECUBE
	WiiFilesystemFactory &fsf = WiiFilesystemFactory::instance();
#endif

	switch (cmd) {
	case 'x':
	case 'y':
		gfx_set_underscan(_sliderUnderscanX->getValue(),
							_sliderUnderscanY->getValue());
		break;

	case 'k':
		save();
		close();
		break;

	case 'c':
		revert();
		close();
		break;

#ifdef USE_WII_DI
	case 'mdvd':
		fsf.mount(WiiFilesystemFactory::kDVD);
		break;

	case 'udvd':
		fsf.umount(WiiFilesystemFactory::kDVD);
		break;
#endif

#ifdef USE_WII_SMB
	case 'net':
		fsf.asyncInitNetwork();
		break;

	case 'msmb':
		fsf.setSMBLoginData(_editSMBServer->getEditString(),
							_editSMBShare->getEditString(),
							_editSMBUsername->getEditString(),
							_editSMBPassword->getEditString());
		fsf.mount(WiiFilesystemFactory::kSMB);
		break;

	case 'usmb':
		fsf.umount(WiiFilesystemFactory::kSMB);
		break;
#endif

	default:
		Dialog::handleCommand(sender, cmd, data);
		break;
	}
}

void WiiOptionsDialog::revert() {
	gfx_set_underscan(ConfMan.getInt(_strUnderscanX,
									Common::ConfigManager::kApplicationDomain),
						ConfMan.getInt(_strUnderscanY,
									Common::ConfigManager::kApplicationDomain));
}

void WiiOptionsDialog::load() {
	int i;

	i = ConfMan.getInt(_strUnderscanX,
							Common::ConfigManager::kApplicationDomain);
	_sliderUnderscanX->setValue(i);

	i = ConfMan.getInt(_strUnderscanY,
							Common::ConfigManager::kApplicationDomain);
	_sliderUnderscanY->setValue(i);

	i = ConfMan.getInt("wii_pad_sensitivity",
							Common::ConfigManager::kApplicationDomain);
	_sliderPadSensitivity->setValue(i);

	i = ConfMan.getInt("wii_pad_acceleration",
							Common::ConfigManager::kApplicationDomain);
	_sliderPadAcceleration->setValue(i);

#ifdef USE_WII_SMB
	_editSMBServer->setEditString(ConfMan.get("wii_smb_server",
									Common::ConfigManager::kApplicationDomain));
	_editSMBShare->setEditString(ConfMan.get("wii_smb_share",
									Common::ConfigManager::kApplicationDomain));
	_editSMBUsername->setEditString(ConfMan.get("wii_smb_username",
									Common::ConfigManager::kApplicationDomain));
	_editSMBPassword->setEditString(ConfMan.get("wii_smb_password",
									Common::ConfigManager::kApplicationDomain));
#endif
}

void WiiOptionsDialog::save() {
	ConfMan.setInt(_strUnderscanX,
					_sliderUnderscanX->getValue(),
					Common::ConfigManager::kApplicationDomain);
	ConfMan.setInt(_strUnderscanY,
					_sliderUnderscanY->getValue(),
					Common::ConfigManager::kApplicationDomain);

	ConfMan.setInt("wii_pad_sensitivity",
					_sliderPadSensitivity->getValue(),
					Common::ConfigManager::kApplicationDomain);
	ConfMan.setInt("wii_pad_acceleration",
					_sliderPadAcceleration->getValue(),
					Common::ConfigManager::kApplicationDomain);

#ifdef USE_WII_SMB
	ConfMan.set("wii_smb_server", _editSMBServer->getEditString(),
				Common::ConfigManager::kApplicationDomain);
	ConfMan.set("wii_smb_share", _editSMBShare->getEditString(),
				Common::ConfigManager::kApplicationDomain);
	ConfMan.set("wii_smb_username", _editSMBUsername->getEditString(),
				Common::ConfigManager::kApplicationDomain);
	ConfMan.set("wii_smb_password", _editSMBPassword->getEditString(),
				Common::ConfigManager::kApplicationDomain);
#endif

	ConfMan.flushToDisk();
}


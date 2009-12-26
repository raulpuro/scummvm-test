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
 * $URL$
 * $Id$
 */

#include "teenagent/teenagent.h"
#include "common/system.h"
#include "common/events.h"
#include "common/debug.h"
#include "common/savefile.h"
#include "common/config-manager.h"
#include "engines/advancedDetector.h"
#include "sound/mixer.h"
#include "graphics/thumbnail.h"
#include "teenagent/scene.h"
#include "teenagent/objects.h"
#include "teenagent/music.h"
#include "teenagent/console.h"

#include "graphics/cursorman.h"

namespace TeenAgent {

TeenAgentEngine::TeenAgentEngine(OSystem *system, const ADGameDescription *gd) : Engine(system), action(kActionNone), _gameDescription(gd) {
	music = new MusicPlayer();
}

bool TeenAgentEngine::trySelectedObject() {
	InventoryObject *inv = inventory->selectedObject();
	if (inv == NULL)
		return false;

	Resources *res = Resources::instance();
	debug(0, "checking active object %u on %u", inv->id, dst_object->id);
	
	//mouse time challenge hack:
	if (
		(res->dseg.get_byte(0) == 1 && inv->id == 49 && dst_object->id == 5) ||
		(res->dseg.get_byte(0) == 2 && inv->id == 29 && dst_object->id == 5)
	) {
		//putting rock into hole or superglue on rock
		processCallback(0x8d57);
		return true;
	}

	const Common::Array<UseHotspot> &hotspots = use_hotspots[scene->getId() - 1];
	for (uint i = 0; i < hotspots.size(); ++i) {
		const UseHotspot &spot = hotspots[i];
		if (spot.inventory_id == inv->id && dst_object->id == spot.object_id) {
			debug(0, "use object on hotspot!");
			spot.dump();
			if (spot.actor_x != 0xffff && spot.actor_y != 0xffff)
				moveTo(spot.actor_x, spot.actor_y, spot.orientation);
			inventory->resetSelectedObject();
			if (!processCallback(TO_LE_16(spot.callback)))
				debug(0, "fixme! display proper description");
			return true;
		}
	}

	//error
	inventory->resetSelectedObject();
	displayMessage(0x3457);
	return true;
}

void TeenAgentEngine::processObject() {
	if (dst_object == NULL)
		return;

	Resources *res = Resources::instance();
	switch (action) {
	case kActionExamine: {
		if (trySelectedObject())
			break;

		byte *dcall = res->dseg.ptr(0xb5ce);
		dcall = res->dseg.ptr(READ_LE_UINT16(dcall + scene->getId() * 2 - 2));
		dcall += 2 * dst_object->id - 2;
		uint16 callback = READ_LE_UINT16(dcall);
		if (callback == 0 || !processCallback(callback)) {
			Common::String desc = dst_object->description;
			displayMessage(desc);
			//debug(0, "%s[%u]: description: %s", current_object->name, current_object->id, desc.c_str());
		}
	}
	break;
	case kActionUse: {
		if (trySelectedObject())
			break;
		
		byte *dcall = res->dseg.ptr(0xb89c);
		dcall = res->dseg.ptr(READ_LE_UINT16(dcall + scene->getId() * 2 - 2));
		dcall += 2 * dst_object->id - 2;
		uint16 callback = READ_LE_UINT16(dcall);
		if (!processCallback(callback))
			scene->displayMessage(dst_object->description);
	}
	break;

	case kActionNone:
		break;
	}
}


void TeenAgentEngine::use(Object *object) {
	if (object == NULL || scene->eventRunning())
		return;

	dst_object = object;
	object->rect.dump();
	object->actor_rect.dump();

	action = kActionUse;
	if (object->actor_rect.valid())
		scene->moveTo(Common::Point(object->actor_rect.right, object->actor_rect.bottom), object->actor_orientation);
	else if (object->actor_orientation > 0)
		scene->setOrientation(object->actor_orientation);
}

void TeenAgentEngine::examine(const Common::Point &point, Object *object) {
	if (scene->eventRunning())
		return;

	if (object != NULL) {
		Common::Point dst = object->actor_rect.center();
		debug(0, "click %d, %d, object %d, %d", point.x, point.y, dst.x, dst.y);
		action = kActionExamine;
		if (object->actor_rect.valid())
			scene->moveTo(dst, object->actor_orientation);
		dst_object = object;
	} else {
		debug(0, "click %d, %d", point.x, point.y);
		action = kActionNone;
		scene->moveTo(point, 0, true);
		dst_object = NULL;
	}
}

void TeenAgentEngine::init() {
	Resources * res = Resources::instance();
	use_hotspots.resize(42);
	byte *scene_hotspots = res->dseg.ptr(0xbb87);
	for (byte i = 0; i < 42; ++i) {
		Common::Array<UseHotspot> & hotspots = use_hotspots[i];
		byte * hotspots_ptr = res->dseg.ptr(READ_LE_UINT16(scene_hotspots + i * 2));
		while (*hotspots_ptr) {
			UseHotspot h;
			h.load(hotspots_ptr);
			hotspots_ptr += 9;
			hotspots.push_back(h);
		}
	}
}

void TeenAgentEngine::deinit() {
	_mixer->stopAll();
	delete scene;
	scene = NULL;
	delete inventory;
	inventory = NULL;
	//delete music;
	//music = NULL;
	use_hotspots.clear();
	Resources::instance()->deinit();
	CursorMan.popCursor();
}

Common::Error TeenAgentEngine::loadGameState(int slot) {
	debug(0, "loading from slot %d", slot);
	Common::InSaveFile *in = _saveFileMan->openForLoading(Common::String::printf("teenagent.%02d", slot));
	if (in == NULL)
		in = _saveFileMan->openForLoading(Common::String::printf("teenagent.%d", slot));

	if (in == NULL)
		return Common::kReadPermissionDenied;

	Resources *res = Resources::instance();

	assert(res->dseg.size() >= 0x6478 + 0x777a);
	char data[0x777a];
	in->seek(0);
	if (in->read(data, 0x777a) != 0x777a) {
		delete in;
		return Common::kReadingFailed;
	}

	delete in;

	memcpy(res->dseg.ptr(0x6478), data, sizeof(data));

	scene->clear();
	inventory->activate(false);

	setMusic(Resources::instance()->dseg.get_byte(0xDB90));

	int id = res->dseg.get_byte(0xB4F3);
	uint16 x = res->dseg.get_word(0x64AF), y = res->dseg.get_word(0x64B1);
	scene->loadObjectData();
	scene->init(id, Common::Point(x, y));
	return Common::kNoError;
}

Common::Error TeenAgentEngine::saveGameState(int slot, const char *desc) {
	debug(0, "saving to slot %d", slot);
	Common::OutSaveFile *out = _saveFileMan->openForSaving(Common::String::printf("teenagent.%02d", slot));
	if (out == NULL)
		return Common::kWritePermissionDenied;

	Resources *res = Resources::instance();
	res->dseg.set_byte(0xB4F3, scene->getId());
	Common::Point pos = scene->getPosition();
	res->dseg.set_word(0x64AF, pos.x);
	res->dseg.set_word(0x64B1, pos.y);

	assert(res->dseg.size() >= 0x6478 + 0x777a);
	strncpy((char *)res->dseg.ptr(0x6478), desc, 0x16);
	out->write(res->dseg.ptr(0x6478), 0x777a);
	if (!Graphics::saveThumbnail(*out))
		warning("saveThumbnail failed");
	delete out;

	return Common::kNoError;
}

Common::Error TeenAgentEngine::run() {
	Resources *res = Resources::instance();
	if (!res->loadArchives(_gameDescription))
		return Common::kUnknownError;

	Common::EventManager *_event = _system->getEventManager();

	initGraphics(320, 200, false);

	scene = new Scene;
	inventory = new Inventory;
	console = new Console(this);

	scene->init(this, _system);
	inventory->init(this);
	
	init();

	CursorMan.pushCursor(res->dseg.ptr(0x00da), 8, 12, 0, 0, 1);
	CursorMan.showMouse(true);

	syncSoundSettings();

	music->load(1);
	_mixer->playInputStream(Audio::Mixer::kMusicSoundType, &_musicHandle, music, -1, 255, 0, true, false);
	music->start();

	{
		int load_slot = Common::ConfigManager::instance().getInt("save_slot");
		debug(0, "slot: %d", load_slot);
		if (load_slot >= 0) {
			loadGameState(load_slot);
		} else {
			scene->intro = true;
			scene_busy = true;
			processCallback(0x24c);
		}
	}

	uint32 frame = 0;

	Common::Event event;
	Common::Point mouse;

	do {
		uint32 t0 = _system->getMillis();
		Object *current_object = scene->findObject(mouse);

		while (_event->pollEvent(event)) {
			if (event.type == Common::EVENT_RTL) {
				deinit();
				return Common::kNoError;
			}

			if ((!scene_busy && inventory->processEvent(event)) || scene->processEvent(event))
				continue;

			//debug(0, "event");
			switch (event.type) {
			case Common::EVENT_KEYDOWN:
				if ((event.kbd.flags == Common::KBD_CTRL && event.kbd.keycode == 'd') ||
					event.kbd.ascii == '~' || event.kbd.ascii == '#') {
					console->attach();
				}
				if (event.kbd.flags == 0 && event.kbd.keycode == Common::KEYCODE_F5)
					openMainMenuDialog();
				break;
			case Common::EVENT_LBUTTONDOWN:
				if (scene->getId() < 0)
					break;
				examine(event.mouse, current_object);
				break;
			case Common::EVENT_RBUTTONDOWN:
				//if (current_object)
				//	debug(0, "%d, %s", current_object->id, current_object->name.c_str());
				if (scene->getId() < 0)
					break;
				if (res->dseg.get_byte(0) == 3 && current_object->id == 1) {
					processCallback(0x5189); //boo!
					break;
				}
				if (res->dseg.get_byte(0) == 4 && current_object->id == 5) {
					processCallback(0x99e0); //getting an anchor
					break;
				}
				use(current_object);
				break;
			case Common::EVENT_MOUSEMOVE:
				mouse = event.mouse;
				break;
			default:
				;
			}
		}

		_system->showMouse(scene->getMessage().empty());
		uint32 f0 = frame * 12 / 25, f1 = (frame + 1) * 12 / 25;
		if (f0 != f1) {
			bool b = scene->render(_system);
			scene_busy = b;
			if (!inventory->active() && !scene_busy && action != kActionNone) {
				processObject();
				action = kActionNone;
				dst_object = NULL;
			}
		}
		bool busy = inventory->active() || scene_busy;

		Graphics::Surface *surface = _system->lockScreen();

		if (!busy) {
			InventoryObject *selected_object = inventory->selectedObject();
			if (current_object || selected_object) {
				Common::String name;
				if (selected_object) {
					name += selected_object->name;
					name += " & ";
				}
				if (current_object)
					name += current_object->name;

				uint w = res->font7.render(NULL, 0, 0, name, 0xd1);
				res->font7.render(surface, (320 - w) / 2, 180, name, 0xd1, true);
				if (current_object) {
					//current_object->rect.render(surface, 0x80);
					//current_object->actor_rect.render(surface, 0x81);
				}
			}
		}

		inventory->render(surface);

		_system->unlockScreen();

		_system->updateScreen();

		if (console->isAttached()) {
			console->onFrame();
		}

		uint32 dt = _system->getMillis() - t0;
		if (dt < 40)
			_system->delayMillis(40 - dt);

		++frame;
	} while (!_event->shouldQuit());

	deinit();
	return Common::kNoError;
}

Common::String TeenAgentEngine::parseMessage(uint16 addr) {
	Common::String message;
	for (
	    const char *str = (const char *)Resources::instance()->dseg.ptr(addr);
	    str[0] != 0 || str[1] != 0;
	    ++str) {
		char c = str[0];
		message += c != 0 && c != -1 ? c : '\n';
	}
	if (message.empty()) {
		warning("empty message parsed for %04x", addr);
	}
	return message;
}

void TeenAgentEngine::displayMessage(const Common::String &str, byte color, uint16 position) {
	if (str.empty()) {
		return;
	}

	{
		SceneEvent e(SceneEvent::kPlayAnimation);
		e.animation = 0;
		e.slot = 0x80;
		scene->push(e);
	}

	{
		SceneEvent event(SceneEvent::kMessage);
		event.message = str;
		event.color = color;
		event.slot = 0;
		event.dst.x = position % 320;
		event.dst.y = position / 320;
		scene->push(event);
	}

	{
		SceneEvent e(SceneEvent::kPauseAnimation);
		e.animation = 0;
		e.slot = 0x80;
		scene->push(e);
	}
}

void TeenAgentEngine::displayMessage(uint16 addr, byte color, uint16 position) {
	displayMessage(parseMessage(addr), color, position);
}

void TeenAgentEngine::displayAsyncMessage(uint16 addr, uint16 position, uint16 first_frame, uint16 last_frame, byte color) {
	SceneEvent event(SceneEvent::kMessage);
	event.message = parseMessage(addr);
	event.slot = 0;
	event.color = color;
	event.dst.x = position % 320;
	event.dst.y = position / 320;
	event.first_frame = first_frame;
	event.last_frame = last_frame;

	scene->push(event);
}

void TeenAgentEngine::displayAsyncMessageInSlot(uint16 addr, byte slot, uint16 first_frame, uint16 last_frame, byte color) {
	SceneEvent event(SceneEvent::kMessage);
	event.message = parseMessage(addr);
	event.slot = slot + 1;
	event.color = color;
	event.first_frame = first_frame;
	event.last_frame = last_frame;

	scene->push(event);
}


void TeenAgentEngine::displayCredits(uint16 addr, uint16 timer) {
	SceneEvent event(SceneEvent::kCreditsMessage);

	const byte *src = Resources::instance()->dseg.ptr(addr);
	event.orientation = *src++;
	event.color = *src++;
	event.lan = 8;

	event.dst.y = *src;
	while (true) {
		++src; //skip y position
		Common::String line((const char *)src);
		event.message += line;
		src += line.size() + 1;
		if (*src == 0)
			break;
		event.message += "\n";
	}
	int w = Resources::instance()->font8.render(NULL, 0, 0, event.message, 0xd1);
	event.dst.x = (320 - w) / 2;
	event.timer = timer;
	scene->push(event);
}

void TeenAgentEngine::displayCredits() {
	SceneEvent event(SceneEvent::kCredits);
	event.message = parseMessage(0xe488);
	event.dst.y = 200;

	int lines = 1;
	for(uint i = 0; i < event.message.size(); ++i)
		if (event.message[i] == '\n')
			++lines;
	event.dst.x = (320 - Resources::instance()->font7.render(NULL, 0, 0, event.message, 0xd1)) / 2;
	event.timer = 11 * lines - event.dst.y + 22;
	//debug(0, "credits = %s", event.message.c_str());
	scene->push(event);
}

void TeenAgentEngine::displayCutsceneMessage(uint16 addr, uint16 position) {
	SceneEvent event(SceneEvent::kCreditsMessage);

	event.message = parseMessage(addr);
	event.dst.x = position % 320;
	event.dst.y = position / 320;
	event.lan = 7;
	
	scene->push(event);
}


void TeenAgentEngine::moveTo(const Common::Point &dst, byte o, bool warp) {
	moveTo(dst.x, dst.y, o, warp);
}

void TeenAgentEngine::moveTo(Object *obj) {
	moveTo(obj->actor_rect.right, obj->actor_rect.bottom, obj->actor_orientation);
}

void TeenAgentEngine::moveTo(uint16 x, uint16 y, byte o, bool warp) {
	SceneEvent event(SceneEvent::kWalk);
	event.dst.x = x;
	event.dst.y = y;
	if (o > 4) {
		warning("invalid orientation %d", o);
		o = 0;
	}
	event.orientation = o;
	event.color = warp ? 1 : 0;
	scene->push(event);
}

void TeenAgentEngine::moveRel(int16 x, int16 y, byte o, bool warp) {
	SceneEvent event(SceneEvent::kWalk);
	event.dst.x = x;
	event.dst.y = y;
	event.orientation = o;
	event.color = (warp ? 1 : 0) | 2;
	scene->push(event);
}

void TeenAgentEngine::playAnimation(uint16 id, byte slot, bool async, bool ignore, bool loop) {
	SceneEvent event(SceneEvent::kPlayAnimation);
	event.animation = id;
	event.slot = (slot + 1) | (ignore? 0x20: 0) | (loop? 0x80: 0);
	scene->push(event);
	if (!async)
		waitAnimation();
}

void TeenAgentEngine::playActorAnimation(uint16 id, bool async, bool ignore) {
	SceneEvent event(SceneEvent::kPlayActorAnimation);
	event.animation = id;
	event.slot = ignore? 0x20: 0;
	scene->push(event);
	if (!async)
		waitAnimation();
}


void TeenAgentEngine::loadScene(byte id, const Common::Point &pos, byte o) {
	loadScene(id, pos.x, pos.y, o);
}

void TeenAgentEngine::loadScene(byte id, uint16 x, uint16 y, byte o) {
	SceneEvent event(SceneEvent::kLoadScene);
	event.scene = id;
	event.dst.x = x;
	event.dst.y = y;
	event.orientation = o;
	scene->push(event);
}

void TeenAgentEngine::setOns(byte id, byte value, byte scene_id) {
	SceneEvent event(SceneEvent::kSetOn);
	event.ons = id + 1;
	event.color = value;
	event.scene = scene_id;
	scene->push(event);
}

void TeenAgentEngine::setLan(byte id, byte value, byte scene_id) {
	if (id == 0)
		error("setting lan 0 is invalid");
	SceneEvent event(SceneEvent::kSetLan);
	event.lan = id;
	event.color = value;
	event.scene = scene_id;
	scene->push(event);
}

void TeenAgentEngine::reloadLan() {
	SceneEvent event(SceneEvent::kSetLan);
	event.lan = 0;
	scene->push(event);
}


void TeenAgentEngine::playMusic(byte id) {
	SceneEvent event(SceneEvent::kPlayMusic);
	event.music = id;
	scene->push(event);
}

void TeenAgentEngine::playSound(byte id, byte skip_frames) {
	if (skip_frames > 0)
		--skip_frames;
	SceneEvent event(SceneEvent::kPlaySound);
	event.sound = id;
	event.color = skip_frames;
	scene->push(event);
}

void TeenAgentEngine::enableObject(byte id, byte scene_id) {
	SceneEvent event(SceneEvent::kEnableObject);
	event.object = id + 1;
	event.color = 1;
	event.scene = scene_id;
	scene->push(event);
}

void TeenAgentEngine::disableObject(byte id, byte scene_id) {
	SceneEvent event(SceneEvent::kEnableObject);
	event.object = id + 1;
	event.color = 0;
	event.scene = scene_id;
	scene->push(event);
}

void TeenAgentEngine::hideActor() {
	SceneEvent event(SceneEvent::kHideActor);
	event.color = 1;
	scene->push(event);
}

void TeenAgentEngine::showActor() {
	SceneEvent event(SceneEvent::kHideActor);
	event.color = 0;
	scene->push(event);
}

void TeenAgentEngine::waitAnimation() {
	SceneEvent event(SceneEvent::kWaitForAnimation);
	scene->push(event);
}

void TeenAgentEngine::waitLanAnimationFrame(byte slot, uint16 frame) {
	SceneEvent event(SceneEvent::kWaitLanAnimationFrame);
	if (frame > 0)
		--frame;
	
	event.slot = slot - 1;
	event.animation = frame;
	scene->push(event);
}

void TeenAgentEngine::setTimerCallback(uint16 addr, uint16 frames) {
	SceneEvent event(SceneEvent::kTimer);
	event.callback = addr;
	event.timer = frames;
	scene->push(event);
}

void TeenAgentEngine::shakeScreen() {
	SceneEvent event(SceneEvent::kEffect);
	scene->push(event);
}

void TeenAgentEngine::playSoundNow(byte id) {
	Resources *res = Resources::instance();
	Common::SeekableReadStream *in = res->sam_sam.getStream(id);
	if (in == NULL) {
		debug(0, "skipping invalid sound %u", id);
		return;
	}

	uint size = in->size();
	char *data = new char[size];
	in->read(data, size);
	//debug(0, "playing %u samples...", size);

	_mixer->playRaw(Audio::Mixer::kSFXSoundType, &_soundHandle, data, size, 11025, Audio::Mixer::FLAG_AUTOFREE);
}


void TeenAgentEngine::setMusic(byte id) {
	debug(0, "starting music %u", id);
	if (!music->load(id))
		return;
	*Resources::instance()->dseg.ptr(0xDB90) = id;
	music->start();
}


bool TeenAgentEngine::hasFeature(EngineFeature f) const {
	switch (f) {
	case kSupportsRTL:
	case kSupportsLoadingDuringRuntime:
	case kSupportsSavingDuringRuntime:
		return true;
	default:
		return false;
	}
}

} // End of namespace TeenAgent

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

#include "teenagent/scene.h"
#include "teenagent/resources.h"
#include "teenagent/surface.h"
#include "common/debug.h"
#include "teenagent/objects.h"
#include "teenagent/teenagent.h"
#include "teenagent/dialog.h"
#include "teenagent/music.h"

namespace TeenAgent {

Scene::Scene() : _engine(NULL), 
	_system(NULL), 
	_id(0), ons(0), walkboxes(0), 
	orientation(Object::ActorRight), 
	current_event(SceneEvent::None) {}

void Scene::warp(const Common::Point & _point, byte o) { 
	Common::Point point(_point);
	destination = position = position0 = point; 
	progress = 0; progress_total = 1; 
	if (o)
		orientation = o;
}

void Scene::moveTo(const Common::Point & _point, byte orient, bool validate) {
	Common::Point point(_point);
	debug(0, "moveTo(%d, %d, %u)", point.x, point.y, orient);
	if (validate) {
		for (byte i = 0; i < walkboxes; ++i) {
			Walkbox * w = walkbox[i];
			if (w->rect.in(point)) {
				debug(0, "bumped into walkbox %u", i);
				byte o = w->orientation;
				switch(o) {
				case 1:
					point.y = w->rect.top - 1;
					break;
				case 2: 
					point.x = w->rect.right + 1;
					break;
				case 3:
					point.y = w->rect.bottom + 1;
					break;
				case 4:
					point.x = w->rect.left - 1;
					break;
				default:
					return;
				}
			}
		}
	}
	if (point == position) {
		if (orient != 0)
			orientation = orient;
		nextEvent();
		return;
	}
	destination = point;
	orientation = orient;
	position0 = position; 
	progress_total = 1 + (int)(sqrt((float)position.sqrDist(destination)) / 10); 
	progress = 0;
}


void Scene::init(TeenAgentEngine *engine, OSystem * system) {
	_engine = engine;
	_system = system;
	
	Resources * res = Resources::instance();
	Common::SeekableReadStream * s = res->varia.getStream(1);
	if (s == NULL)
		error("invalid resource data");

	teenagent.load(s, Animation::TypeVaria);
	if (teenagent.empty())
		error("invalid mark animation");
	
	s = res->varia.getStream(2);
	if (s == NULL)
		error("invalid resource data");
	
	teenagent_idle.load(s, Animation::TypeVaria);
	if (teenagent_idle.empty())
		error("invalid mark animation");
}

byte *Scene::getOns(int id) {
	Resources * res = Resources::instance();
	return res->dseg.ptr(res->dseg.get_word(0xb4f5 + (id - 1) * 2));
}

byte * Scene::getLans(int id) {
	Resources * res = Resources::instance();
	return res->dseg.ptr(0xd89e + (id - 1) * 4);
}

void Scene::loadOns() {
	debug(0, "loading ons animation");
	Resources * res = Resources::instance();

	uint16 addr = res->dseg.get_word(0xb4f5 + (_id - 1) * 2);
	//debug(0, "ons index: %04x", addr);
	
	ons_count = 0;
	byte b;
	byte on_id[16];
	while ((b = res->dseg.get_byte(addr)) != 0xff) {
		debug(0, "on: %04x = %02x", addr, b);
		++addr;
		if (b == 0)
			continue;

		on_id[ons_count++] = b;
	}

	delete[] ons;
	ons = NULL;
	
	if (ons_count > 0) {
		ons = new Surface[ons_count];
		for (uint32 i = 0; i < ons_count; ++i) {
			Common::SeekableReadStream * s = res->ons.getStream(on_id[i]);
			if (s != NULL)
				ons[i].load(s, Surface::TypeOns);
		}
	}
}

void Scene::loadLans() {
	debug(0, "loading lans animation");
	Resources * res = Resources::instance();
	//load lan000
	byte * table_27 = res->dseg.ptr(0x32C7);
	memset(table_27, 0, 27 * 4);
	
	for (int i = 0; i < 4; ++i) {
		animations[i].free();
		
		uint16 bx = 0xd89e + (_id - 1) * 4 + i;
		byte bxv = res->dseg.get_byte(bx);
		debug(0, "lan: [%04x] = %02x", bx, bxv);
		if (bxv == 0)
			continue;

		Common::SeekableReadStream * s = res->loadLan000(4 * (_id - 1) + i + 1);
		if (s != NULL) {
			animations[i].load(s, Animation::TypeLan);
			if (bxv != 0 && bxv != 0xff) 
				animations[i].id = bxv;
			delete s;
		}
		
		//uint16 bp = res->dseg.get_word();
	}

}

void Scene::init(int id, const Common::Point &pos) {
	debug(0, "init(%d)", id);
	_id = id;
	
	if (background.pixels == NULL)
		background.create(320, 200, 1);
		
	warp(pos);

	Resources * res = Resources::instance();
	res->loadOff(background, palette, id);
	if (id == 24) {
		//dark scene
		if (res->dseg.get_byte(0xDBA4) != 1) {
			//dim down palette
			uint i;
			for (i = 0; i < 624; ++i) {
				palette[i] = palette[i] > 0x20? palette[i] - 0x20: 0;
			}
			for (i = 726; i < 768; ++i) {
				palette[i] = palette[i] > 0x20? palette[i] - 0x20: 0;
			}
		}
	}
	setPalette(_system, palette, 4);

	Common::SeekableReadStream *stream = res->on.getStream(id);
	on.load(stream, Surface::TypeOn);
	delete stream;

	loadOns();
	loadLans();
	
	byte * walkboxes_base = res->dseg.ptr(READ_LE_UINT16(res->dseg.ptr(0x6746 + (id - 1) * 2)));
	walkboxes = *walkboxes_base++;

	debug(0, "found %d walkboxes", walkboxes);
	for (byte i = 0; i < walkboxes; ++i) {
		walkbox[i] = (Walkbox *)(walkboxes_base + 14 * i);
		walkbox[i]->dump();
	}
	
	//check music
	int now_playing = _engine->music->getId();
	
	if (now_playing != res->dseg.get_byte(0xDB90)) 
		_engine->music->load(res->dseg.get_byte(0xDB90));
}

void Scene::playAnimation(byte idx, uint id, bool loop) {
	assert(idx < 4);
	Common::SeekableReadStream * s = Resources::instance()->loadLan(id + 1);
	if (s == NULL)
		error("playing animation %u failed", id);

	custom_animations[idx].load(s);
	custom_animations[idx].loop = loop;
}

void Scene::playActorAnimation(uint id, bool loop) {
	Common::SeekableReadStream * s = Resources::instance()->loadLan(id + 1);
	if (s == NULL)
		error("playing animation %u failed", id);

	actor_animation.load(s);
	actor_animation.loop = loop;
}

void Scene::push(const SceneEvent &event) {
	//debug(0, "push");
	//event.dump();
	events.push_back(event);
}

bool Scene::processEvent(const Common::Event &event) {
	if (!message.empty()) {
		if (
			event.type == Common::EVENT_LBUTTONDOWN ||
			event.type == Common::EVENT_RBUTTONDOWN
		) {
			message.clear();
			for(int i = 0; i < 4; ++i) {
				if (custom_animations[i].loop)
					custom_animations[i].free();
			}
			nextEvent();
			return true;
		}
	}
	return false;
}

bool Scene::render(OSystem * system) {
	//render background
	bool busy = false;
	
	system->copyRectToScreen((const byte *)background.pixels, background.pitch, 0, 0, background.w, background.h);
	
	Graphics::Surface * surface = system->lockScreen();

	if (ons != NULL) {
		for (uint32 i = 0; i < ons_count; ++i) {
			Surface* s = ons + i;
			if (s != NULL)
				s->render(surface);
		}
	}
	
	//render on
	if (on.pixels != NULL) {
		on.render(surface);
	}

	bool got_any_animation = false;

	for (int i = 3; i >= 0; --i) {
		Animation *a = custom_animations + i;
		Surface *s = a->currentFrame();
		if (s != NULL) {
			s->render(surface);
			busy = true;
			got_any_animation = true;
			continue;
		}

		a = animations + i;
		s = a->currentFrame();
		if (s == NULL)
			continue;

		s->render(surface);

		if (a->id == 0)
			continue;

		Object * obj = getObject(a->id);
		if (obj != NULL) {
			obj->rect.left = s->x;
			obj->rect.top = s->y;
			obj->rect.right = s->w + s->x;
			obj->rect.bottom = s->h + s->y;
			//obj->dump();
		}
	}
	
	Surface * mark = actor_animation.currentFrame();
	if (mark == NULL) {
		actor_animation.free();

		if (destination != position) {
			Common::Point dp(destination.x - position0.x, destination.y - position0.y);
			int o;
			if (ABS(dp.x) > ABS(dp.y))
				o = dp.x > 0? Object::ActorRight: Object::ActorLeft;
			else
				o = dp.y > 0? Object::ActorDown: Object::ActorUp;
			
			position.x = position0.x + dp.x * progress / progress_total;
			position.y = position0.y + dp.y * progress / progress_total;
			teenagent.render(surface, position, o, 1);
			++progress;
			if (progress >= progress_total) {
				position = destination;
				if (orientation == 0)
					orientation = o; //save last orientation
				nextEvent();
			} else 
				busy = true;
		} else 
			teenagent.render(surface, position, orientation, 0);
	} else {
		mark->render(surface);
		busy = true;
		got_any_animation = true;
	}

	if (current_event.type == SceneEvent::WaitForAnimation && !got_any_animation) {
		nextEvent();
	}
		
	busy |= processEventQueue();
	//if (!current_event.empty())
	//	current_event.dump();
	/*
	for (byte i = 0; i < walkboxes; ++i) {
		Walkbox * w = walkbox[i];
		w->rect.render(surface, 0xd0 + i);
	}
	*/
	
	if (!message.empty()) {
		Resources::instance()->font7.render(surface, message_pos.x, message_pos.y, message);
		busy = true;
	}
	
	system->unlockScreen();
	
	
	for(Sounds::iterator i = sounds.begin(); i != sounds.end(); ) {
		Sound &sound = *i;
		if (sound.delay == 0) {
			debug(0, "sound %u started", sound.id);
			_engine->playSoundNow(sound.id);
			i = sounds.erase(i);
		} else {
			--sound.delay;
			++i;
		}
	}
	
	return busy;
}

bool Scene::processEventQueue() {
	while (!events.empty() && current_event.empty()) {
		//debug(0, "processing next event");
		current_event = events.front();
		events.pop_front();
		switch(current_event.type) {
		
		case SceneEvent::SetOn: {
			byte * ptr = getOns(current_event.scene == 0? _id: current_event.scene);
			debug(0, "on[%u] = %02x", current_event.ons - 1, current_event.color);
			ptr[current_event.ons - 1] = current_event.color;
			loadOns();
			current_event.clear();
		} break;
		
		case SceneEvent::SetLan: {
			if (current_event.lan != 0) {
				debug(0, "lan[%u] = %02x", current_event.lan - 1, current_event.color);
				byte * ptr = getLans(current_event.scene == 0? _id: current_event.scene);
				ptr[current_event.lan - 1] = current_event.color;
			}
			loadLans();
			current_event.clear();
		} break;
		
		case SceneEvent::LoadScene: {
			init(current_event.scene, current_event.dst);
			sounds.clear();
			current_event.clear();
		} break;
		
		case SceneEvent::Walk: {
			Common::Point dst = current_event.dst;
			if ((current_event.color & 2) != 0) { //relative move
				dst.x += position.x;
				dst.y += position.y;
			}
			if ((current_event.color & 1) != 0) {
				warp(dst, current_event.orientation);
				current_event.clear();
			} else
				moveTo(dst, current_event.orientation);
		} break;
		
		case SceneEvent::Message: {
			//debug(0, "pop(%04x)", current_event.message);
			message = current_event.message;
			message_pos = messagePosition(message, position);
		} break;
		
		case SceneEvent::PlayAnimation: {
			debug(0, "playing animation %u", current_event.animation);
			playAnimation(current_event.color & 0x3 /*slot actually :)*/, current_event.animation, (current_event.color & 0x40) != 0);
			current_event.clear();
		} break;

		case SceneEvent::PlayActorAnimation: {
			debug(0, "playing actor animation %u", current_event.animation);
			playActorAnimation(current_event.animation, (current_event.color & 0x40) != 0);
			current_event.clear();
		} break;
		
		case SceneEvent::PlayMusic: {
			debug(0, "setting music %u", current_event.music);
			_engine->setMusic(current_event.music);
			Resources::instance()->dseg.set_byte(0xDB90, current_event.music);
			current_event.clear();
		} break;

		case SceneEvent::PlaySound: {
			debug(0, "playing sound %u, delay: %u", current_event.sound, current_event.color);
			if (current_event.color == 0) {
				_engine->playSoundNow(current_event.sound);
			} else {
				sounds.push_back(Sound(current_event.sound, current_event.color));
			}
			
			current_event.clear();
		} break;
		
		case SceneEvent::EnableObject: {
			debug(0, "%s object #%u", current_event.color?"enabling":"disabling", current_event.object - 1);
			Object * obj = getObject(current_event.object - 1, current_event.scene == 0? _id: current_event.scene);
			obj->enabled = current_event.color;
			current_event.clear();
		} break;
		
		case SceneEvent::WaitForAnimation:
			debug(0, "waiting for the animation");
			break;
			
		case SceneEvent::Quit:
			debug(0, "quit!");
			_engine->quitGame();
			break;
		
		default: 
			error("empty/unhandler event[%d]", (int)current_event.type);
		}
	}
	return !current_event.empty();
}

void Scene::setPalette(OSystem *system, const byte * buf, unsigned mul) {
	byte p[1024];

	memset(p, 0, 1024);
	for (int i = 0; i < 256; ++i) {
		for (int c = 0; c < 3; ++c) 
			p[i * 4 + c] = buf[i * 3 + c] * mul;
	}

	system->setPalette(p, 0, 256);
}

Object * Scene::getObject(int id, int scene_id) {
	if (scene_id == 0)
		scene_id = _id;
	
	Resources * res = Resources::instance();
	uint16 addr = res->dseg.get_word(0x7254 + (scene_id - 1) * 2);
	//debug(0, "object base: %04x, x: %d, %d", addr, point.x, point.y);
	uint16 object = res->dseg.get_word(addr + 2 * id - 2);

	Object *obj = (Object *)res->dseg.ptr(object);
	return obj;
}

Common::Point Scene::messagePosition(const Common::String &str, const Common::Point & position) {
	Resources * res = Resources::instance();
	uint w = res->font7.render(NULL, 0, 0, str);
	Common::Point message_pos = position;
	message_pos.x -= w / 2;
	message_pos.y -= 62;
	if (message_pos.x + w > 320)
		message_pos.x = 320 - w;
	if (message_pos.x < 0)
		message_pos.x = 0;

	return message_pos;
}

void Scene::displayMessage(const Common::String &str) {
	debug(0, "displayMessage: %s", str.c_str());
	message = str;
	message_pos = messagePosition(str, position);
}

void Scene::clear() {
	events.clear();
	current_event.clear();
}

} // End of namespace TeenAgent

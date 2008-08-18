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
 *
 */

#ifndef COMMON_VIRTUAL_KEYBOARD_PARSER
#define COMMON_VIRTUAL_KEYBOARD_PARSER

#include "common/xmlparser.h"
#include "backends/vkeybd/virtual-keyboard.h"

/**
 TODO - information about optional attributes and their default values


                   ***************************************
                   ** Virtual Keyboard Pack File Format **
                   ***************************************

The new virtual keyboard for ScummVM is implemented in the same way as a HTML 
ImageMap. It uses a single bitmap of the entire keyboard layout and then a 
image map description allows certain areas of the bitmap to be given special 
actions. Most of these actions will be a virtual key press event, but there 
will also be special keys that will change the keyboard layout or close the 
keyboard. The HTML image map description is contained in a larger XML file that
can describe all the different modes of the keyboard, and also different 
keyboard layouts for different screen resolutions.

                ********************************************
                ** Example keyboard pack description file **
                ********************************************

<keyboard modes="normal,caps" initial_mode="normal" v_align="bottom" h_align="centre">
  <mode name="normal" resolutions="640x400,320x200">
    <layout resolution="640x400" bitmap="normal_640x400.bmp" transparent_color="255,0,255">
      <map>
        <area shape="poly" coords="65,50,67,48,94,48,96,50,96,77,94,79,67,79,65,77" target="q" />
        <area shape="poly" coords="105,50,107,48,134,48,136,50,136,77,134,79,107,79,105,77" target="w" />
        <area shape="poly" coords="146,50,148,48,174,48,176,50,176,77,174,79,148,79,146,77" target="e" />
		...
        <area shape="poly" coords="11,89,12,88,69,88,70,89,70,116,69,117,12,117,11,116" target="caps" />
      </map>
	</layout>
	<layout resolution="320x200" bitmap="normal_320x200.bmp" transparent_color="255,0,255">
	  ...
	</layout>
    <event name="a" type="key" code="97" ascii="97" modifiers="" />
    <event name="b" type="key" code="98" ascii="98" modifiers="" />
    <event name="c" type="key" code="99" ascii="99" modifiers="" />
	...
	<event name="caps" type="switch_mode" mode="caps" />
  </mode>

  <mode name="caps" resolutions="640x400">
    <layout resolution="640x400" bitmap="caps_640x480.bmp" transparent_color="255,0,255">
      <map>
        <area shape="poly" coords="65,50,67,48,94,48,96,50,96,77,94,79,67,79,65,77" target="Q" />
		...
      </map>
    </layout>
    <event name="A" type="key" code="97" ascii="65" modifiers="shift" />
    <event name="B" type="key" code="98" ascii="66" modifiers="shift" />
    <event name="C" type="key" code="99" ascii="67" modifiers="shift" />
	...
  </mode>
</keyboard>

*************************
** Description of tags **
*************************

<keyboard>

This is the required, root element of the file format. 

attributes:
 - modes: lists all the modes that the keyboard pack contains
 - initial_mode: which mode the keyboard should show initially
 - v_align/h_align: where on the screen should the keyboard appear initially

child tags:
 - mode

-------------------------------------------------------------------------------

<mode>

This tag encapsulates a single mode of the keyboard. Within are a number of 
layouts, which provide the specific implementation at different resolutions.

attributes:
 - name: the name of the mode 
 - resolutions: list of the different layout resolutions 

child tags:
 - layout
 - event

-------------------------------------------------------------------------------

<event>

These tags describe a particular event that will be triggered by a mouse click
on a particular area. The target attribute of each image map area should be the
same as an event's name.

attributes:
 - name: name of the event
 - type: what sort of event is it (key | switch_mode | close)
 - for key events
   - code / ascii / modifiers: describe a key press in ScummVM KeyState format
 - for switch_mode events
   - mode: the mode that should be switched to

-------------------------------------------------------------------------------

<layout>

These tags encapsulate an implementation of a mode at a particular resolution.

attributes:
 - resolution: the screen resolution that this layout is designed for
 - bitmap: filename of the 24-bit bitmap that will be used for this layout
 - transparent_color: color in r,b,g format that will be used for keycolor 
   transparency.

child nodes:
 - map: this describes the image map using the same format as html image maps

-------------------------------------------------------------------------------

<map>

These tags describe the image map for a particular layout. It uses the exact 
same format as HTML image maps. The only area shapes that are supported are 
rectangles and polygons. The target attribute of each area should be the name
of an event for this mode (see <event> tag). For information on HTML image map
format see 
 - http://www.w3schools.com/TAGS/tag_map.asp 
 - http://www.w3schools.com/TAGS/tag_area.asp

*/

namespace Common {

enum ParseMode {
	kParseFull,				// when loading keyboard pack for first time
	kParseCheckResolutions  // when re-parsing following a change in screen size
};

class VirtualKeyboardParser : public Common::XMLParser {

public:

	VirtualKeyboardParser(VirtualKeyboard *kbd);
	void setParseMode(ParseMode m) {
		_parseMode = m;
	}

protected:
	CUSTOM_XML_PARSER(VirtualKeyboardParser) {
		XML_KEY(keyboard)
			XML_PROP(initial_mode, true)
			XML_PROP(v_align, false)
			XML_PROP(h_align, false)
			XML_KEY(mode)
				XML_PROP(name, true)
				XML_PROP(resolutions, true)
				XML_KEY(layout)
					XML_PROP(resolution, true)
					XML_PROP(bitmap, true)
					XML_PROP(transparent_color, false)
					XML_PROP(display_font_color, false)
					XML_KEY(map)
						XML_KEY(area)
							XML_PROP(shape, true)
							XML_PROP(coords, true)
							XML_PROP(target, true)
						KEY_END()
					KEY_END()
				KEY_END()
				XML_KEY(event)
					XML_PROP(name, true)
					XML_PROP(type, true)
					XML_PROP(code, false)
					XML_PROP(ascii, false)
					XML_PROP(modifiers, false)
					XML_PROP(mode, false)
				KEY_END()
			KEY_END()
		KEY_END()
	} PARSER_END()

protected:
	VirtualKeyboard *_keyboard;

	/** internal state variables of parser */
	ParseMode _parseMode;
	VirtualKeyboard::Mode *_mode; // pointer to mode currently being parsed
	String _initialModeName;
	bool _kbdParsed;
	bool _layoutParsed;

	void cleanup();

	bool parserCallback_keyboard(ParserNode *node);
	bool parserCallback_mode(ParserNode *node);
	bool parserCallback_event(ParserNode *node);
	bool parserCallback_layout(ParserNode *node);
	bool parserCallback_map(ParserNode *node);
	bool parserCallback_area(ParserNode *node);
	
	bool closedKeyCallback(ParserNode *node);

	byte parseFlags(const String& flags);
	bool parseRect(Rect *rect, const String& coords);
	bool parsePolygon(Polygon *poly, const String& coords);
	bool parseRectAsPolygon(Polygon *poly, const String& coords);
};

} // end of namespace GUI

#endif

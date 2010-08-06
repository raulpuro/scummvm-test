MODULE := engines/sword25

MODULE_OBJS := \
	detection.o \
	sword25.o \
	fmv/movieplayer.o \
	fmv/movieplayer_script.o \
	fmv/theora_decoder.o \
	gfx/animation.o \
	gfx/animationdescription.o \
	gfx/animationresource.o \
	gfx/animationtemplate.o \
	gfx/animationtemplateregistry.o \
	gfx/bitmap.o \
	gfx/bitmapresource.o \
	gfx/dynamicbitmap.o \
	gfx/fontresource.o \
	gfx/framecounter.o \
	gfx/graphicengine.o \
	gfx/graphicengine_script.o \
	gfx/image/b25sloader.o \
	gfx/image/imageloader.o \
	gfx/image/pngloader.o \
	gfx/image/vectorimage.o \
	gfx/image/vectorimagerenderer.o \
	gfx/opengl/glimage.o \
	gfx/opengl/glvectorimageblit.o \
	gfx/opengl/openglgfx.o \
	gfx/opengl/swimage.o \
	gfx/panel.o \
	gfx/renderobject.o \
	gfx/renderobjectmanager.o \
	gfx/renderobjectregistry.o \
	gfx/screenshot.o \
	gfx/staticbitmap.o \
	gfx/text.o \
	gfx/timedrenderobject.o \
	input/inputengine.o \
	input/inputengine_script.o \
	input/stdwininput.o \
	kernel/callbackregistry.o \
	kernel/filesystemutil.o \
	kernel/inputpersistenceblock.o \
	kernel/kernel.o \
	kernel/kernel_script.o \
	kernel/log.o \
	kernel/memleaks.o \
	kernel/outputpersistenceblock.o \
	kernel/persistenceservice.o \
	kernel/resmanager.o \
	kernel/resource.o \
	kernel/scummvmwindow.o \
	kernel/window.o \
	math/geometry.o \
	math/geometry_script.o \
	math/polygon.o \
	math/region.o \
	math/regionregistry.o \
	math/vertex.o \
	math/walkregion.o \
	package/packagemanager.o \
	package/packagemanager_script.o \
	package/scummvmpackagemanager.o \
	script/luabindhelper.o \
	script/luacallback.o \
	script/luascript.o \
	script/lua_extensions.o \
	sfx/fmodexchannel.o \
	sfx/fmodexresource.o \
	sfx/fmodexsound.o \
	sfx/soundengine.o \
	sfx/soundengine_script.o \
	util/glsprites/internal/core.o \
	util/glsprites/internal/glswindow.o \
	util/glsprites/internal/sprite.o \
	util/glsprites/internal/sprite_pow2.o \
	util/glsprites/internal/sprite_rectangle.o \
	util/glsprites/internal/sprite_tiled.o \
	util/glsprites/internal/util.o \
	util/lua/src/lapi.o \
	util/lua/src/lauxlib.o \
	util/lua/src/lbaselib.o \
	util/lua/src/lcode.o \
	util/lua/src/ldblib.o \
	util/lua/src/ldebug.o \
	util/lua/src/ldo.o \
	util/lua/src/ldump.o \
	util/lua/src/lfunc.o \
	util/lua/src/lgc.o \
	util/lua/src/linit.o \
	util/lua/src/liolib.o \
	util/lua/src/llex.o \
	util/lua/src/lmathlib.o \
	util/lua/src/lmem.o \
	util/lua/src/loadlib.o \
	util/lua/src/lobject.o \
	util/lua/src/lopcodes.o \
	util/lua/src/loslib.o \
	util/lua/src/lparser.o \
	util/lua/src/lstate.o \
	util/lua/src/lstring.o \
	util/lua/src/lstrlib.o \
	util/lua/src/ltable.o \
	util/lua/src/ltablib.o \
	util/lua/src/ltm.o \
	util/lua/src/lua.o \
	util/lua/src/luac.o \
	util/lua/src/lundump.o \
	util/lua/src/lvm.o \
	util/lua/src/lzio.o \
	util/lua/src/print.o \
	util/pluto/pdep.o \
	util/pluto/pluto.o \
	util/pluto/plzio.o

# This module can be built as a plugin
ifeq ($(ENABLE_SWORD25), DYNAMIC_PLUGIN)
PLUGIN := 1
endif

# Include common rules
include $(srcdir)/rules.mk

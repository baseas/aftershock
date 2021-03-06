/*
===========================================================================
Copyright (C) 1999-2005 Id Software, Inc.

This file is part of Quake III Arena source code.

Quake III Arena source code is free software; you can redistribute it
and/or modify it under the terms of the GNU General Public License as
published by the Free Software Foundation; either version 2 of the License,
or (at your option) any later version.

Quake III Arena source code is distributed in the hope that it will be
useful, but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU General Public License for more details.

You should have received a copy of the GNU General Public License
along with Quake III Arena source code; if not, write to the Free Software
Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA  02110-1301  USA
===========================================================================
*/
//
// ui_sound.c -- sound options menu

#include "ui_local.h"

enum {
	ID_GRAPHICS = 10,
	ID_DISPLAY,
	ID_SOUND,
	ID_NETWORK,
	ID_EFFECTSVOLUME,
	ID_MUSICVOLUME,
	ID_QUALITY,
	ID_SOUNDSYSTEM,
	// ID_A3D,
	ID_BACK,
	ID_APPLY
};

#define DEFAULT_SDL_SND_SPEED 22050

static const char *quality_items[] = {
	"Low", "Medium", "High", NULL
};

#define UISND_SDL		0
#define UISND_OPENAL	1

static const char *soundSystem_items[] = {
	"SDL", "OpenAL", NULL
};

typedef struct {
	menuframework_s		menu;

	menutext_s			banner;

	menutext_s			graphics;
	menutext_s			display;
	menutext_s			sound;
	menutext_s			network;

	menufield_s			volume;
	menulist_s			soundSystem;
	menulist_s			quality;

	menubutton_s		back;
	menubutton_s		apply;

	float				volume_original;
	int					soundSystem_original;
	int					quality_original;
} soundOptionsInfo_t;

static soundOptionsInfo_t	soundOptionsInfo;

static void UI_SoundOptionsMenu_Event(void *ptr, int event)
{
	if (event != QM_ACTIVATED) {
		return;
	}

	switch (((menucommon_s *)ptr)->id) {
	case ID_GRAPHICS:
		UI_PopMenu();
		UI_GraphicsOptionsMenu();
		break;

	case ID_DISPLAY:
		UI_PopMenu();
		UI_DisplayOptionsMenu();
		break;

	case ID_SOUND:
		break;

	case ID_NETWORK:
		UI_PopMenu();
		UI_NetworkOptionsMenu();
		break;

	case ID_BACK:
		UI_PopMenu();
		break;

	case ID_APPLY:
		trap_Cvar_SetValue("s_volume", atof(soundOptionsInfo.volume.field.buffer) / 100);
		soundOptionsInfo.volume_original = atof(soundOptionsInfo.volume.field.buffer);

		// Check if something changed that requires the sound system to be restarted.
		if (soundOptionsInfo.quality_original != soundOptionsInfo.quality.curvalue
			|| soundOptionsInfo.soundSystem_original != soundOptionsInfo.soundSystem.curvalue)
		{
			int speed;

			switch (soundOptionsInfo.quality.curvalue) {
				default:
				case 0:
					speed = 11025;
					break;
				case 1:
					speed = 22050;
					break;
				case 2:
					speed = 44100;
					break;
			}

			if (speed == DEFAULT_SDL_SND_SPEED) {
				speed = 0;
			}

			trap_Cvar_SetValue("s_sdlSpeed", speed);
			soundOptionsInfo.quality_original = soundOptionsInfo.quality.curvalue;

			trap_Cvar_SetValue("s_useOpenAL", (soundOptionsInfo.soundSystem.curvalue == UISND_OPENAL));
			soundOptionsInfo.soundSystem_original = soundOptionsInfo.soundSystem.curvalue;

			UI_ForceMenuOff();
			trap_Cmd_ExecuteText(EXEC_APPEND, "snd_restart\n");
		}
		break;
	}
}

static void SoundOptions_UpdateMenuItems(void)
{
	if (soundOptionsInfo.soundSystem.curvalue == UISND_SDL) {
		soundOptionsInfo.quality.generic.flags &= ~QMF_GRAYED;
	} else {
		soundOptionsInfo.quality.generic.flags |= QMF_GRAYED;
	}

	soundOptionsInfo.apply.generic.flags |= QMF_HIDDEN|QMF_INACTIVE;

	if (soundOptionsInfo.volume_original != atof(soundOptionsInfo.volume.field.buffer)) {
		soundOptionsInfo.apply.generic.flags &= ~(QMF_HIDDEN|QMF_INACTIVE);
	}
	if (soundOptionsInfo.soundSystem_original != soundOptionsInfo.soundSystem.curvalue) {
		soundOptionsInfo.apply.generic.flags &= ~(QMF_HIDDEN|QMF_INACTIVE);
	}
	if (soundOptionsInfo.quality_original != soundOptionsInfo.quality.curvalue) {
		soundOptionsInfo.apply.generic.flags &= ~(QMF_HIDDEN|QMF_INACTIVE);
	}
}

void SoundOptions_MenuDraw (void)
{
//APSFIX - rework this
	SoundOptions_UpdateMenuItems();

	Menu_Draw(&soundOptionsInfo.menu);
}

static void UI_SoundOptionsMenu_Init(void)
{
	int	y;
	int	speed;

	memset(&soundOptionsInfo, 0, sizeof soundOptionsInfo);

	UI_SoundOptionsMenu_Cache();
	soundOptionsInfo.menu.wrapAround = qtrue;
	soundOptionsInfo.menu.fullscreen = qtrue;
	soundOptionsInfo.menu.draw		= SoundOptions_MenuDraw;

	soundOptionsInfo.banner.generic.type		= MTYPE_BTEXT;
	soundOptionsInfo.banner.generic.flags		= QMF_CENTER_JUSTIFY;
	soundOptionsInfo.banner.generic.x			= 320;
	soundOptionsInfo.banner.generic.y			= 16;
	soundOptionsInfo.banner.string				= "SYSTEM SETUP";
	soundOptionsInfo.banner.color				= colorBanner;
	soundOptionsInfo.banner.style				= FONT_CENTER | FONT_SHADOW;

	soundOptionsInfo.graphics.generic.type		= MTYPE_PTEXT;
	soundOptionsInfo.graphics.generic.flags		= QMF_RIGHT_JUSTIFY|QMF_PULSEIFFOCUS;
	soundOptionsInfo.graphics.generic.id		= ID_GRAPHICS;
	soundOptionsInfo.graphics.generic.callback	= UI_SoundOptionsMenu_Event;
	soundOptionsInfo.graphics.generic.x			= 216;
	soundOptionsInfo.graphics.generic.y			= 240 - 2 * PROP_HEIGHT;
	soundOptionsInfo.graphics.string			= "GRAPHICS";
	soundOptionsInfo.graphics.style				= FONT_RIGHT;
	soundOptionsInfo.graphics.color				= colorRed;

	soundOptionsInfo.display.generic.type		= MTYPE_PTEXT;
	soundOptionsInfo.display.generic.flags		= QMF_RIGHT_JUSTIFY|QMF_PULSEIFFOCUS;
	soundOptionsInfo.display.generic.id			= ID_DISPLAY;
	soundOptionsInfo.display.generic.callback	= UI_SoundOptionsMenu_Event;
	soundOptionsInfo.display.generic.x			= 216;
	soundOptionsInfo.display.generic.y			= 240 - PROP_HEIGHT;
	soundOptionsInfo.display.string				= "DISPLAY";
	soundOptionsInfo.display.style				= FONT_RIGHT;
	soundOptionsInfo.display.color				= colorRed;

	soundOptionsInfo.sound.generic.type			= MTYPE_PTEXT;
	soundOptionsInfo.sound.generic.flags		= QMF_RIGHT_JUSTIFY;
	soundOptionsInfo.sound.generic.id			= ID_SOUND;
	soundOptionsInfo.sound.generic.callback		= UI_SoundOptionsMenu_Event;
	soundOptionsInfo.sound.generic.x			= 216;
	soundOptionsInfo.sound.generic.y			= 240;
	soundOptionsInfo.sound.string				= "SOUND";
	soundOptionsInfo.sound.style				= FONT_RIGHT;
	soundOptionsInfo.sound.color				= colorRed;

	soundOptionsInfo.network.generic.type		= MTYPE_PTEXT;
	soundOptionsInfo.network.generic.flags		= QMF_RIGHT_JUSTIFY|QMF_PULSEIFFOCUS;
	soundOptionsInfo.network.generic.id			= ID_NETWORK;
	soundOptionsInfo.network.generic.callback	= UI_SoundOptionsMenu_Event;
	soundOptionsInfo.network.generic.x			= 216;
	soundOptionsInfo.network.generic.y			= 240 + PROP_HEIGHT;
	soundOptionsInfo.network.string				= "NETWORK";
	soundOptionsInfo.network.style				= FONT_RIGHT;
	soundOptionsInfo.network.color				= colorRed;

	y = 240 - 2 * (BIGCHAR_SIZE + 2);
	soundOptionsInfo.volume.generic.type		= MTYPE_FIELD;
	soundOptionsInfo.volume.generic.name		= "Volume:";
	soundOptionsInfo.volume.generic.flags		= QMF_NUMBERSONLY | QMF_SMALLFONT;
	soundOptionsInfo.volume.generic.callback	= UI_SoundOptionsMenu_Event;
	soundOptionsInfo.volume.generic.id			= ID_EFFECTSVOLUME;
	soundOptionsInfo.volume.generic.x			= 400;
	soundOptionsInfo.volume.generic.y			= y;
	soundOptionsInfo.volume.field.widthInChars	= 2;
	soundOptionsInfo.volume.field.maxchars		= 2;

	y += BIGCHAR_SIZE+2;
	soundOptionsInfo.soundSystem.generic.type		= MTYPE_SPINCONTROL;
	soundOptionsInfo.soundSystem.generic.name		= "Sound System:";
	soundOptionsInfo.soundSystem.generic.flags		= QMF_PULSEIFFOCUS|QMF_SMALLFONT;
	soundOptionsInfo.soundSystem.generic.callback	= UI_SoundOptionsMenu_Event;
	soundOptionsInfo.soundSystem.generic.id			= ID_SOUNDSYSTEM;
	soundOptionsInfo.soundSystem.generic.x			= 400;
	soundOptionsInfo.soundSystem.generic.y			= y;
	soundOptionsInfo.soundSystem.itemnames			= soundSystem_items;

	y += BIGCHAR_SIZE+2;
	soundOptionsInfo.quality.generic.type		= MTYPE_SPINCONTROL;
	soundOptionsInfo.quality.generic.name		= "SDL Sound Quality:";
	soundOptionsInfo.quality.generic.flags		= QMF_PULSEIFFOCUS|QMF_SMALLFONT;
	soundOptionsInfo.quality.generic.callback	= UI_SoundOptionsMenu_Event;
	soundOptionsInfo.quality.generic.id			= ID_QUALITY;
	soundOptionsInfo.quality.generic.x			= 400;
	soundOptionsInfo.quality.generic.y			= y;
	soundOptionsInfo.quality.itemnames			= quality_items;

	soundOptionsInfo.back.generic.type			= MTYPE_BUTTON;
	soundOptionsInfo.back.generic.flags			= QMF_LEFT_JUSTIFY;
	soundOptionsInfo.back.generic.callback		= UI_SoundOptionsMenu_Event;
	soundOptionsInfo.back.generic.id			= ID_BACK;
	soundOptionsInfo.back.generic.x				= 0;
	soundOptionsInfo.back.generic.y				= 480-64;
	soundOptionsInfo.back.width					= 128;
	soundOptionsInfo.back.height				= 64;
	soundOptionsInfo.back.string				= "Back";

	soundOptionsInfo.apply.generic.type			= MTYPE_BUTTON;
	soundOptionsInfo.apply.generic.flags		= QMF_RIGHT_JUSTIFY | QMF_HIDDEN | QMF_INACTIVE;
	soundOptionsInfo.apply.generic.callback		= UI_SoundOptionsMenu_Event;
	soundOptionsInfo.apply.generic.id			= ID_APPLY;
	soundOptionsInfo.apply.generic.x			= 640;
	soundOptionsInfo.apply.generic.y			= 480-64;
	soundOptionsInfo.apply.width				= 128;
	soundOptionsInfo.apply.height				= 64;
	soundOptionsInfo.apply.string				= "Apply";

	Menu_AddItem(&soundOptionsInfo.menu, (void *) &soundOptionsInfo.banner);
	Menu_AddItem(&soundOptionsInfo.menu, (void *) &soundOptionsInfo.graphics);
	Menu_AddItem(&soundOptionsInfo.menu, (void *) &soundOptionsInfo.display);
	Menu_AddItem(&soundOptionsInfo.menu, (void *) &soundOptionsInfo.sound);
	Menu_AddItem(&soundOptionsInfo.menu, (void *) &soundOptionsInfo.network);
	Menu_AddItem(&soundOptionsInfo.menu, (void *) &soundOptionsInfo.volume);
	Menu_AddItem(&soundOptionsInfo.menu, (void *) &soundOptionsInfo.soundSystem);
	Menu_AddItem(&soundOptionsInfo.menu, (void *) &soundOptionsInfo.quality);
	Menu_AddItem(&soundOptionsInfo.menu, (void *) &soundOptionsInfo.back);
	Menu_AddItem(&soundOptionsInfo.menu, (void *) &soundOptionsInfo.apply);

	Com_sprintf(soundOptionsInfo.volume.field.buffer, sizeof soundOptionsInfo.volume.field.buffer,
		"%g", trap_Cvar_VariableValue("s_volume") * 100);

	if (trap_Cvar_VariableValue("s_useOpenAL")) {
		soundOptionsInfo.soundSystem_original = UISND_OPENAL;
	} else {
		soundOptionsInfo.soundSystem_original = UISND_SDL;
	}

	soundOptionsInfo.soundSystem.curvalue = soundOptionsInfo.soundSystem_original;

	speed = trap_Cvar_VariableValue("s_sdlSpeed");
	if (!speed) { // Check for default
		speed = DEFAULT_SDL_SND_SPEED;
	}

	if (speed <= 11025) {
		soundOptionsInfo.quality_original = 0;
	} else if (speed <= 22050) {
		soundOptionsInfo.quality_original = 1;
	} else { // 44100
		soundOptionsInfo.quality_original = 2;
	}
	soundOptionsInfo.quality.curvalue = soundOptionsInfo.quality_original;
}

void UI_SoundOptionsMenu_Cache(void) { }

void UI_SoundOptionsMenu(void)
{
	UI_SoundOptionsMenu_Init();
	UI_PushMenu(&soundOptionsInfo.menu);
	Menu_SetCursorToItem(&soundOptionsInfo.menu, &soundOptionsInfo.sound);
}


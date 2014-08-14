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
// ui_display.c -- display options menu

#include "ui_local.h"

enum {
	ID_GRAPHICS = 10,
	ID_DISPLAY,
	ID_SOUND,
	ID_NETWORK,
	ID_BRIGHTNESS,
	ID_BACK
};

typedef struct {
	menuframework_s	menu;

	menutext_s		banner;

	menutext_s		graphics;
	menutext_s		display;
	menutext_s		sound;
	menutext_s		network;

	menufield_s		brightness;

	menubutton_s	back;
} displayOptionsInfo_t;

static displayOptionsInfo_t	displayOptionsInfo;

static void UI_DisplayOptionsMenu_Event(void* ptr, int event)
{
	if (event != QM_ACTIVATED) {
		return;
	}

	switch (((menucommon_s*)ptr)->id) {
	case ID_GRAPHICS:
		UI_PopMenu();
		UI_GraphicsOptionsMenu();
		break;

	case ID_DISPLAY:
		break;

	case ID_SOUND:
		UI_PopMenu();
		UI_SoundOptionsMenu();
		break;

	case ID_NETWORK:
		UI_PopMenu();
		UI_NetworkOptionsMenu();
		break;

	case ID_BRIGHTNESS:
		trap_Cvar_SetValue("r_gamma", atof(displayOptionsInfo.brightness.field.buffer) / 100);
		break;

	case ID_BACK:
		UI_PopMenu();
		break;
	}
}

static void UI_DisplayOptionsMenu_Init(void)
{
	int		y;

	memset(&displayOptionsInfo, 0, sizeof(displayOptionsInfo));

	displayOptionsInfo.menu.wrapAround = qtrue;
	displayOptionsInfo.menu.fullscreen = qtrue;

	displayOptionsInfo.banner.generic.type		= MTYPE_BTEXT;
	displayOptionsInfo.banner.generic.flags		= QMF_CENTER_JUSTIFY;
	displayOptionsInfo.banner.generic.x			= 320;
	displayOptionsInfo.banner.generic.y			= 16;
	displayOptionsInfo.banner.string			= "SYSTEM SETUP";
	displayOptionsInfo.banner.color				= colorBanner;
	displayOptionsInfo.banner.style				= FONT_CENTER | FONT_SHADOW;

	displayOptionsInfo.graphics.generic.type		= MTYPE_PTEXT;
	displayOptionsInfo.graphics.generic.flags		= QMF_RIGHT_JUSTIFY|QMF_PULSEIFFOCUS;
	displayOptionsInfo.graphics.generic.id			= ID_GRAPHICS;
	displayOptionsInfo.graphics.generic.callback	= UI_DisplayOptionsMenu_Event;
	displayOptionsInfo.graphics.generic.x			= 216;
	displayOptionsInfo.graphics.generic.y			= 240 - 2 * PROP_HEIGHT;
	displayOptionsInfo.graphics.string				= "GRAPHICS";
	displayOptionsInfo.graphics.style				= FONT_RIGHT;
	displayOptionsInfo.graphics.color				= colorRed;

	displayOptionsInfo.display.generic.type			= MTYPE_PTEXT;
	displayOptionsInfo.display.generic.flags		= QMF_RIGHT_JUSTIFY;
	displayOptionsInfo.display.generic.id			= ID_DISPLAY;
	displayOptionsInfo.display.generic.callback		= UI_DisplayOptionsMenu_Event;
	displayOptionsInfo.display.generic.x			= 216;
	displayOptionsInfo.display.generic.y			= 240 - PROP_HEIGHT;
	displayOptionsInfo.display.string				= "DISPLAY";
	displayOptionsInfo.display.style				= FONT_RIGHT;
	displayOptionsInfo.display.color				= colorRed;

	displayOptionsInfo.sound.generic.type			= MTYPE_PTEXT;
	displayOptionsInfo.sound.generic.flags			= QMF_RIGHT_JUSTIFY|QMF_PULSEIFFOCUS;
	displayOptionsInfo.sound.generic.id				= ID_SOUND;
	displayOptionsInfo.sound.generic.callback		= UI_DisplayOptionsMenu_Event;
	displayOptionsInfo.sound.generic.x				= 216;
	displayOptionsInfo.sound.generic.y				= 240;
	displayOptionsInfo.sound.string					= "SOUND";
	displayOptionsInfo.sound.style					= FONT_RIGHT;
	displayOptionsInfo.sound.color					= colorRed;

	displayOptionsInfo.network.generic.type			= MTYPE_PTEXT;
	displayOptionsInfo.network.generic.flags		= QMF_RIGHT_JUSTIFY|QMF_PULSEIFFOCUS;
	displayOptionsInfo.network.generic.id			= ID_NETWORK;
	displayOptionsInfo.network.generic.callback		= UI_DisplayOptionsMenu_Event;
	displayOptionsInfo.network.generic.x			= 216;
	displayOptionsInfo.network.generic.y			= 240 + PROP_HEIGHT;
	displayOptionsInfo.network.string				= "NETWORK";
	displayOptionsInfo.network.style				= FONT_RIGHT;
	displayOptionsInfo.network.color				= colorRed;

	y = 240 - 1 * (BIGCHAR_SIZE+2);
	displayOptionsInfo.brightness.generic.type		= MTYPE_FIELD;
	displayOptionsInfo.brightness.generic.name		= "Brightness:";
	displayOptionsInfo.brightness.generic.flags		= QMF_NUMBERSONLY | QMF_SMALLFONT;
	displayOptionsInfo.brightness.generic.callback	= UI_DisplayOptionsMenu_Event;
	displayOptionsInfo.brightness.generic.id		= ID_BRIGHTNESS;
	displayOptionsInfo.brightness.generic.x			= 400;
	displayOptionsInfo.brightness.generic.y			= y;
	displayOptionsInfo.brightness.field.widthInChars	= 3;
	displayOptionsInfo.brightness.field.maxchars		= 3;
	if (!uis.glconfig.deviceSupportsGamma) {
		displayOptionsInfo.brightness.generic.flags |= QMF_GRAYED;
	}

	displayOptionsInfo.back.generic.type		= MTYPE_BUTTON;
	displayOptionsInfo.back.generic.flags		= QMF_LEFT_JUSTIFY;
	displayOptionsInfo.back.generic.callback	= UI_DisplayOptionsMenu_Event;
	displayOptionsInfo.back.generic.id			= ID_BACK;
	displayOptionsInfo.back.generic.x			= 0;
	displayOptionsInfo.back.generic.y			= 480-64;
	displayOptionsInfo.back.string				= "Back";

	Menu_AddItem(&displayOptionsInfo.menu, (void *) &displayOptionsInfo.banner);
	Menu_AddItem(&displayOptionsInfo.menu, (void *) &displayOptionsInfo.graphics);
	Menu_AddItem(&displayOptionsInfo.menu, (void *) &displayOptionsInfo.display);
	Menu_AddItem(&displayOptionsInfo.menu, (void *) &displayOptionsInfo.sound);
	Menu_AddItem(&displayOptionsInfo.menu, (void *) &displayOptionsInfo.network);
	Menu_AddItem(&displayOptionsInfo.menu, (void *) &displayOptionsInfo.brightness);
	Menu_AddItem(&displayOptionsInfo.menu, (void *) &displayOptionsInfo.back);

	Com_sprintf(displayOptionsInfo.brightness.field.buffer, sizeof displayOptionsInfo.brightness.field.buffer,
		"%g", trap_Cvar_VariableValue("r_gamma") * 100);
}

void UI_DisplayOptionsMenu_Cache(void) { }

void UI_DisplayOptionsMenu(void)
{
	UI_DisplayOptionsMenu_Init();
	UI_PushMenu(&displayOptionsInfo.menu);
	Menu_SetCursorToItem(&displayOptionsInfo.menu, &displayOptionsInfo.display);
}


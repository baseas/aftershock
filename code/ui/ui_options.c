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
// ui_options.c -- system configuration menu

#include "ui_local.h"

#define VERTICAL_SPACING	34

enum {
	ID_GRAPHICS = 10,
	ID_DISPLAY,
	ID_SOUND,
	ID_NETWORK,
	ID_BACK
};

typedef struct {
	menuframework_s	menu;

	menutext_s		banner;

	menutext_s		graphics;
	menutext_s		display;
	menutext_s		sound;
	menutext_s		network;
	menubutton_s	back;
} optionsmenu_t;

static optionsmenu_t	s_options;

static void Options_Event(void* ptr, int event)
{
	if (event != QM_ACTIVATED) {
		return;
	}

	switch (((menucommon_s*)ptr)->id) {
	case ID_GRAPHICS:
		UI_GraphicsOptionsMenu();
		break;

	case ID_DISPLAY:
		UI_DisplayOptionsMenu();
		break;

	case ID_SOUND:
		UI_SoundOptionsMenu();
		break;

	case ID_NETWORK:
		UI_NetworkOptionsMenu();
		break;

	case ID_BACK:
		UI_PopMenu();
		break;
	}
}

void Options_MenuInit(void)
{
	int				y;
	uiClientState_t	cstate;

	memset(&s_options, 0, sizeof(optionsmenu_t));

	s_options.menu.wrapAround = qtrue;

	trap_GetClientState(&cstate);
	if (cstate.connState >= CA_CONNECTED) {
		s_options.menu.fullscreen = qfalse;
	}
	else {
		s_options.menu.fullscreen = qtrue;
	}

	s_options.banner.generic.type	= MTYPE_BTEXT;
	s_options.banner.generic.flags	= QMF_CENTER_JUSTIFY;
	s_options.banner.generic.x		= 320;
	s_options.banner.generic.y		= 16;
	s_options.banner.string		    = "SYSTEM SETUP";
	s_options.banner.color			= colorBanner;
	s_options.banner.style			= FONT_CENTER | FONT_SHADOW;

	y = 168;
	s_options.graphics.generic.type		= MTYPE_PTEXT;
	s_options.graphics.generic.flags	= QMF_CENTER_JUSTIFY|QMF_PULSEIFFOCUS;
	s_options.graphics.generic.callback	= Options_Event;
	s_options.graphics.generic.id		= ID_GRAPHICS;
	s_options.graphics.generic.x		= 320;
	s_options.graphics.generic.y		= y;
	s_options.graphics.string			= "GRAPHICS";
	s_options.graphics.color			= colorRed;
	s_options.graphics.style			= FONT_CENTER;

	y += VERTICAL_SPACING;
	s_options.display.generic.type		= MTYPE_PTEXT;
	s_options.display.generic.flags		= QMF_CENTER_JUSTIFY|QMF_PULSEIFFOCUS;
	s_options.display.generic.callback	= Options_Event;
	s_options.display.generic.id		= ID_DISPLAY;
	s_options.display.generic.x			= 320;
	s_options.display.generic.y			= y;
	s_options.display.string			= "DISPLAY";
	s_options.display.color				= colorRed;
	s_options.display.style				= FONT_CENTER;

	y += VERTICAL_SPACING;
	s_options.sound.generic.type		= MTYPE_PTEXT;
	s_options.sound.generic.flags		= QMF_CENTER_JUSTIFY|QMF_PULSEIFFOCUS;
	s_options.sound.generic.callback	= Options_Event;
	s_options.sound.generic.id			= ID_SOUND;
	s_options.sound.generic.x			= 320;
	s_options.sound.generic.y			= y;
	s_options.sound.string				= "SOUND";
	s_options.sound.color				= colorRed;
	s_options.sound.style				= FONT_CENTER;

	y += VERTICAL_SPACING;
	s_options.network.generic.type		= MTYPE_PTEXT;
	s_options.network.generic.flags		= QMF_CENTER_JUSTIFY|QMF_PULSEIFFOCUS;
	s_options.network.generic.callback	= Options_Event;
	s_options.network.generic.id		= ID_NETWORK;
	s_options.network.generic.x			= 320;
	s_options.network.generic.y			= y;
	s_options.network.string			= "NETWORK";
	s_options.network.color				= colorRed;
	s_options.network.style				= FONT_CENTER;

	s_options.back.generic.type		= MTYPE_BUTTON;
	s_options.back.generic.flags	= QMF_LEFT_JUSTIFY;
	s_options.back.generic.callback = Options_Event;
	s_options.back.generic.id		= ID_BACK;
	s_options.back.generic.x		= 0;
	s_options.back.generic.y		= 480 - 64;
	s_options.back.string			= "Back";

	Menu_AddItem(&s_options.menu, (void *) &s_options.graphics);
	Menu_AddItem(&s_options.menu, (void *) &s_options.display);
	Menu_AddItem(&s_options.menu, (void *) &s_options.sound);
	Menu_AddItem(&s_options.menu, (void *) &s_options.network);
	Menu_AddItem(&s_options.menu, (void *) &s_options.back);
}

void UI_SystemConfigMenu(void)
{
	Options_MenuInit();
	UI_PushMenu (&s_options.menu);
}


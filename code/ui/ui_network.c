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
// ui_network.c -- network options menu

#include "ui_local.h"

enum {
	ID_GRAPHICS = 10,
	ID_DISPLAY,
	ID_SOUND,
	ID_NETWORK,
	ID_RATE,
	ID_BACK
};

static const char *rate_items[] = {
	"<= 28.8K",
	"33.6K",
	"56K",
	"ISDN",
	"LAN/Cable/xDSL",
	NULL
};

typedef struct {
	menuframework_s	menu;

	menutext_s		banner;

	menutext_s		graphics;
	menutext_s		display;
	menutext_s		sound;
	menutext_s		network;

	menulist_s		rate;

	menubutton_s	back;
} networkOptionsInfo_t;

static networkOptionsInfo_t	networkOptionsInfo;

static void UI_NetworkOptionsMenu_Event(void *ptr, int event)
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
		UI_PopMenu();
		UI_DisplayOptionsMenu();
		break;

	case ID_SOUND:
		UI_PopMenu();
		UI_SoundOptionsMenu();
		break;

	case ID_NETWORK:
		break;

	case ID_RATE:
		if (networkOptionsInfo.rate.curvalue == 0) {
			trap_Cvar_SetValue("rate", 2500);
		} else if (networkOptionsInfo.rate.curvalue == 1) {
			trap_Cvar_SetValue("rate", 3000);
		} else if (networkOptionsInfo.rate.curvalue == 2) {
			trap_Cvar_SetValue("rate", 4000);
		} else if (networkOptionsInfo.rate.curvalue == 3) {
			trap_Cvar_SetValue("rate", 5000);
		} else if (networkOptionsInfo.rate.curvalue == 4) {
			trap_Cvar_SetValue("rate", 25000);
		}
		break;

	case ID_BACK:
		UI_PopMenu();
		break;
	}
}

static void UI_NetworkOptionsMenu_Init(void)
{
	int		y;
	int		rate;

	memset(&networkOptionsInfo, 0, sizeof(networkOptionsInfo));

	UI_NetworkOptionsMenu_Cache();
	networkOptionsInfo.menu.wrapAround = qtrue;
	networkOptionsInfo.menu.fullscreen = qtrue;

	networkOptionsInfo.banner.generic.type		= MTYPE_BTEXT;
	networkOptionsInfo.banner.generic.flags		= QMF_CENTER_JUSTIFY;
	networkOptionsInfo.banner.generic.x			= 320;
	networkOptionsInfo.banner.generic.y			= 16;
	networkOptionsInfo.banner.string			= "SYSTEM SETUP";
	networkOptionsInfo.banner.color				= colorBanner;
	networkOptionsInfo.banner.style				= FONT_CENTER | FONT_SHADOW;

	networkOptionsInfo.graphics.generic.type		= MTYPE_PTEXT;
	networkOptionsInfo.graphics.generic.flags		= QMF_RIGHT_JUSTIFY|QMF_PULSEIFFOCUS;
	networkOptionsInfo.graphics.generic.id			= ID_GRAPHICS;
	networkOptionsInfo.graphics.generic.callback	= UI_NetworkOptionsMenu_Event;
	networkOptionsInfo.graphics.generic.x			= 216;
	networkOptionsInfo.graphics.generic.y			= 240 - 2 * PROP_HEIGHT;
	networkOptionsInfo.graphics.string				= "GRAPHICS";
	networkOptionsInfo.graphics.style				= FONT_RIGHT;
	networkOptionsInfo.graphics.color				= colorRed;

	networkOptionsInfo.display.generic.type			= MTYPE_PTEXT;
	networkOptionsInfo.display.generic.flags		= QMF_RIGHT_JUSTIFY|QMF_PULSEIFFOCUS;
	networkOptionsInfo.display.generic.id			= ID_DISPLAY;
	networkOptionsInfo.display.generic.callback		= UI_NetworkOptionsMenu_Event;
	networkOptionsInfo.display.generic.x			= 216;
	networkOptionsInfo.display.generic.y			= 240 - PROP_HEIGHT;
	networkOptionsInfo.display.string				= "DISPLAY";
	networkOptionsInfo.display.style				= FONT_RIGHT;
	networkOptionsInfo.display.color				= colorRed;

	networkOptionsInfo.sound.generic.type			= MTYPE_PTEXT;
	networkOptionsInfo.sound.generic.flags			= QMF_RIGHT_JUSTIFY|QMF_PULSEIFFOCUS;
	networkOptionsInfo.sound.generic.id				= ID_SOUND;
	networkOptionsInfo.sound.generic.callback		= UI_NetworkOptionsMenu_Event;
	networkOptionsInfo.sound.generic.x				= 216;
	networkOptionsInfo.sound.generic.y				= 240;
	networkOptionsInfo.sound.string					= "SOUND";
	networkOptionsInfo.sound.style					= FONT_RIGHT;
	networkOptionsInfo.sound.color					= colorRed;

	networkOptionsInfo.network.generic.type			= MTYPE_PTEXT;
	networkOptionsInfo.network.generic.flags		= QMF_RIGHT_JUSTIFY;
	networkOptionsInfo.network.generic.id			= ID_NETWORK;
	networkOptionsInfo.network.generic.callback		= UI_NetworkOptionsMenu_Event;
	networkOptionsInfo.network.generic.x			= 216;
	networkOptionsInfo.network.generic.y			= 240 + PROP_HEIGHT;
	networkOptionsInfo.network.string				= "NETWORK";
	networkOptionsInfo.network.style				= FONT_RIGHT;
	networkOptionsInfo.network.color				= colorRed;

	y = 240 - 1 * (BIGCHAR_SIZE+2);
	networkOptionsInfo.rate.generic.type		= MTYPE_SPINCONTROL;
	networkOptionsInfo.rate.generic.name		= "Data Rate:";
	networkOptionsInfo.rate.generic.flags		= QMF_PULSEIFFOCUS|QMF_SMALLFONT;
	networkOptionsInfo.rate.generic.callback	= UI_NetworkOptionsMenu_Event;
	networkOptionsInfo.rate.generic.id			= ID_RATE;
	networkOptionsInfo.rate.generic.x			= 400;
	networkOptionsInfo.rate.generic.y			= y;
	networkOptionsInfo.rate.itemnames			= rate_items;

	networkOptionsInfo.back.generic.type		= MTYPE_BUTTON;
	networkOptionsInfo.back.generic.flags		= QMF_LEFT_JUSTIFY;
	networkOptionsInfo.back.generic.callback	= UI_NetworkOptionsMenu_Event;
	networkOptionsInfo.back.generic.id			= ID_BACK;
	networkOptionsInfo.back.generic.x			= 0;
	networkOptionsInfo.back.generic.y			= 480-64;
	networkOptionsInfo.back.width				= 128;
	networkOptionsInfo.back.height				= 64;
	networkOptionsInfo.back.string				= "Back";

	Menu_AddItem(&networkOptionsInfo.menu, (void *) &networkOptionsInfo.banner);
	Menu_AddItem(&networkOptionsInfo.menu, (void *) &networkOptionsInfo.graphics);
	Menu_AddItem(&networkOptionsInfo.menu, (void *) &networkOptionsInfo.display);
	Menu_AddItem(&networkOptionsInfo.menu, (void *) &networkOptionsInfo.sound);
	Menu_AddItem(&networkOptionsInfo.menu, (void *) &networkOptionsInfo.network);
	Menu_AddItem(&networkOptionsInfo.menu, (void *) &networkOptionsInfo.rate);
	Menu_AddItem(&networkOptionsInfo.menu, (void *) &networkOptionsInfo.back);

	rate = trap_Cvar_VariableValue("rate");
	if (rate <= 2500) {
		networkOptionsInfo.rate.curvalue = 0;
	} else if (rate <= 3000) {
		networkOptionsInfo.rate.curvalue = 1;
	} else if (rate <= 4000) {
		networkOptionsInfo.rate.curvalue = 2;
	} else if (rate <= 5000) {
		networkOptionsInfo.rate.curvalue = 3;
	} else {
		networkOptionsInfo.rate.curvalue = 4;
	}
}

void UI_NetworkOptionsMenu_Cache(void) { }

void UI_NetworkOptionsMenu(void)
{
	UI_NetworkOptionsMenu_Init();
	UI_PushMenu(&networkOptionsInfo.menu);
	Menu_SetCursorToItem(&networkOptionsInfo.menu, &networkOptionsInfo.network);
}


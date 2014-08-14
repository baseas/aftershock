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
// ui_preferences.c -- game options menu

#include "ui_local.h"

#define PREFERENCES_X_POS		360

enum {
	ID_CROSSHAIR = 127,
	ID_SIMPLEITEMS,
	ID_HIGHQUALITYSKY,
	ID_MUZZLEFLASH,
	ID_WALLMARKS,
	ID_DYNAMICLIGHTS,
	ID_IDENTIFYTARGET,
	ID_ALLOWDOWNLOAD,
	ID_BACK
};

typedef struct {
	menuframework_s		menu;

	menutext_s			banner;

	menulist_s			crosshair;
	menuradiobutton_s	simpleitems;
	menuradiobutton_s	muzzle;
	menuradiobutton_s	wallmarks;
	menuradiobutton_s	dynamiclights;
	menuradiobutton_s	identifytarget;
	menuradiobutton_s	highqualitysky;
	menuradiobutton_s	synceveryframe;
	menuradiobutton_s	forcemodel;
	menulist_s			drawteamoverlay;
	menuradiobutton_s	allowdownload;
	menubutton_s		back;

	qhandle_t			crosshairShader[NUM_CROSSHAIRS];
} preferences_t;

static preferences_t s_preferences;

static void Preferences_SetMenuItems(void)
{
	s_preferences.crosshair.curvalue		= (int)trap_Cvar_VariableValue("cg_drawCrosshair") % NUM_CROSSHAIRS;
	s_preferences.simpleitems.curvalue		= trap_Cvar_VariableValue("cg_simpleItems") != 0;
	s_preferences.muzzle.curvalue			= trap_Cvar_VariableValue("cg_muzzleFlash") != 0;
	s_preferences.wallmarks.curvalue		= trap_Cvar_VariableValue("cg_marks") != 0;
	s_preferences.identifytarget.curvalue	= trap_Cvar_VariableValue("cg_drawCrosshairNames") != 0;
	s_preferences.dynamiclights.curvalue	= trap_Cvar_VariableValue("r_dynamiclight") != 0;
	s_preferences.highqualitysky.curvalue	= trap_Cvar_VariableValue ("r_fastsky") == 0;
	s_preferences.allowdownload.curvalue	= trap_Cvar_VariableValue("cl_allowDownload") != 0;
}

static void Preferences_Event(void* ptr, int notification)
{
	if (notification != QM_ACTIVATED) {
		return;
	}

	switch (((menucommon_s*)ptr)->id) {
	case ID_CROSSHAIR:
		trap_Cvar_SetValue("cg_drawCrosshair", s_preferences.crosshair.curvalue);
		break;

	case ID_SIMPLEITEMS:
		trap_Cvar_SetValue("cg_simpleItems", s_preferences.simpleitems.curvalue);
		break;

	case ID_HIGHQUALITYSKY:
		trap_Cvar_SetValue("r_fastsky", !s_preferences.highqualitysky.curvalue);
		break;

	case ID_MUZZLEFLASH:
		if (s_preferences.muzzle.curvalue)
			trap_Cvar_Reset("cg_muzzleTime");
		else
			trap_Cvar_SetValue("cg_muzzleTime", 0);
		break;

	case ID_WALLMARKS:
		trap_Cvar_SetValue("cg_marks", s_preferences.wallmarks.curvalue);
		break;

	case ID_DYNAMICLIGHTS:
		trap_Cvar_SetValue("r_dynamiclight", s_preferences.dynamiclights.curvalue);
		break;

	case ID_IDENTIFYTARGET:
		trap_Cvar_SetValue("cg_drawCrosshairNames", s_preferences.identifytarget.curvalue);
		break;

	case ID_ALLOWDOWNLOAD:
		trap_Cvar_SetValue("cl_allowDownload", s_preferences.allowdownload.curvalue);
		trap_Cvar_SetValue("sv_allowDownload", s_preferences.allowdownload.curvalue);
		break;

	case ID_BACK:
		UI_PopMenu();
		break;
	}
}

static void Crosshair_Draw(void *self)
{
	menulist_s	*s;
	float		*color;
	int			x, y;
	int			fontStyle = 0;
	qboolean	focus;

	s = (menulist_s *)self;
	x = s->generic.x;
	y =	s->generic.y;

	focus = (s->generic.parent->cursor == s->generic.menuPosition);

	if (s->generic.flags & QMF_GRAYED) {
		color = colorTextDisabled;
	} else if (focus) {
		color = colorTextHighlight;
		fontStyle |= FONT_PULSE;
	} else if (s->generic.flags & QMF_BLINK) {
		color = colorTextHighlight;
		fontStyle |= FONT_BLINK;
	} else {
		color = colorTextNormal;
	}

	if (focus) {
		// draw cursor
		SCR_FillRect(s->generic.left, s->generic.top, s->generic.right - s->generic.left + 1,
			s->generic.bottom - s->generic.top + 1, colorListbar);
		SCR_DrawString(x, y, "\15", SMALLCHAR_SIZE, FONT_CENTER | FONT_BLINK, color);
	}

	SCR_DrawString(x - SMALLCHAR_SIZE, y, s->generic.name, SMALLCHAR_SIZE, fontStyle | FONT_RIGHT, color);
	if (!s->curvalue) {
		return;
	}

	y += (SMALLCHAR_SIZE - 24) / 2;
	UI_DrawHandlePic(x + SMALLCHAR_SIZE, y, 24, 24, s_preferences.crosshairShader[s->curvalue]);
}

static void Preferences_MenuInit(void)
{
	int	y;

	memset(&s_preferences, 0, sizeof s_preferences);

	Preferences_Cache();

	s_preferences.menu.wrapAround = qtrue;
	s_preferences.menu.fullscreen = qtrue;

	s_preferences.banner.generic.type	= MTYPE_BTEXT;
	s_preferences.banner.generic.x		= 320;
	s_preferences.banner.generic.y		= 16;
	s_preferences.banner.string			= "GAME OPTIONS";
	s_preferences.banner.color			= colorBanner;
	s_preferences.banner.style			= FONT_CENTER | FONT_SHADOW;

	y = 144;
	s_preferences.crosshair.generic.type		= MTYPE_SPINCONTROL;
	s_preferences.crosshair.generic.flags		= QMF_PULSEIFFOCUS|QMF_SMALLFONT|QMF_NODEFAULTINIT|QMF_OWNERDRAW;
	s_preferences.crosshair.generic.x			= PREFERENCES_X_POS;
	s_preferences.crosshair.generic.y			= y;
	s_preferences.crosshair.generic.name		= "Crosshair:";
	s_preferences.crosshair.generic.callback	= Preferences_Event;
	s_preferences.crosshair.generic.ownerdraw	= Crosshair_Draw;
	s_preferences.crosshair.generic.id			= ID_CROSSHAIR;
	s_preferences.crosshair.generic.top			= y - 10;
	s_preferences.crosshair.generic.bottom		= y + 20;
	s_preferences.crosshair.generic.left		= PREFERENCES_X_POS - ((strlen(s_preferences.crosshair.generic.name) + 1) * SMALLCHAR_SIZE);
	s_preferences.crosshair.generic.right		= PREFERENCES_X_POS + 48;
	s_preferences.crosshair.numitems			= NUM_CROSSHAIRS;

	y += BIGCHAR_SIZE + 2 + 4;
	s_preferences.simpleitems.generic.type		= MTYPE_RADIOBUTTON;
	s_preferences.simpleitems.generic.name		= "Simple Items";
	s_preferences.simpleitems.generic.flags		= QMF_PULSEIFFOCUS|QMF_SMALLFONT;
	s_preferences.simpleitems.generic.callback	= Preferences_Event;
	s_preferences.simpleitems.generic.id		= ID_SIMPLEITEMS;
	s_preferences.simpleitems.generic.x			= PREFERENCES_X_POS;
	s_preferences.simpleitems.generic.y			= y;

	y += BIGCHAR_SIZE;
	s_preferences.wallmarks.generic.type		= MTYPE_RADIOBUTTON;
	s_preferences.wallmarks.generic.name		= "Marks on Walls";
	s_preferences.wallmarks.generic.flags		= QMF_PULSEIFFOCUS|QMF_SMALLFONT;
	s_preferences.wallmarks.generic.callback	= Preferences_Event;
	s_preferences.wallmarks.generic.id			= ID_WALLMARKS;
	s_preferences.wallmarks.generic.x			= PREFERENCES_X_POS;
	s_preferences.wallmarks.generic.y			= y;

	y += BIGCHAR_SIZE + 2;
	s_preferences.muzzle.generic.type			= MTYPE_RADIOBUTTON;
	s_preferences.muzzle.generic.name			= "Muzzle Flash";
	s_preferences.muzzle.generic.flags			= QMF_PULSEIFFOCUS|QMF_SMALLFONT;
	s_preferences.muzzle.generic.callback		= Preferences_Event;
	s_preferences.muzzle.generic.id				= ID_MUZZLEFLASH;
	s_preferences.muzzle.generic.x				= PREFERENCES_X_POS;
	s_preferences.muzzle.generic.y				= y;

	y += BIGCHAR_SIZE + 2;
	s_preferences.dynamiclights.generic.type		= MTYPE_RADIOBUTTON;
	s_preferences.dynamiclights.generic.name		= "Dynamic Lights";
	s_preferences.dynamiclights.generic.flags		= QMF_PULSEIFFOCUS|QMF_SMALLFONT;
	s_preferences.dynamiclights.generic.callback	= Preferences_Event;
	s_preferences.dynamiclights.generic.id			= ID_DYNAMICLIGHTS;
	s_preferences.dynamiclights.generic.x			= PREFERENCES_X_POS;
	s_preferences.dynamiclights.generic.y			= y;

	y += BIGCHAR_SIZE + 2;
	s_preferences.identifytarget.generic.type		= MTYPE_RADIOBUTTON;
	s_preferences.identifytarget.generic.name		= "Identify Target";
	s_preferences.identifytarget.generic.flags		= QMF_PULSEIFFOCUS|QMF_SMALLFONT;
	s_preferences.identifytarget.generic.callback	= Preferences_Event;
	s_preferences.identifytarget.generic.id			= ID_IDENTIFYTARGET;
	s_preferences.identifytarget.generic.x			= PREFERENCES_X_POS;
	s_preferences.identifytarget.generic.y			= y;

	y += BIGCHAR_SIZE + 2;
	s_preferences.highqualitysky.generic.type		= MTYPE_RADIOBUTTON;
	s_preferences.highqualitysky.generic.name		= "High Quality Sky";
	s_preferences.highqualitysky.generic.flags		= QMF_PULSEIFFOCUS|QMF_SMALLFONT;
	s_preferences.highqualitysky.generic.callback	= Preferences_Event;
	s_preferences.highqualitysky.generic.id			= ID_HIGHQUALITYSKY;
	s_preferences.highqualitysky.generic.x			= PREFERENCES_X_POS;
	s_preferences.highqualitysky.generic.y			= y;

	y += BIGCHAR_SIZE + 2;
	s_preferences.allowdownload.generic.type		= MTYPE_RADIOBUTTON;
	s_preferences.allowdownload.generic.name		= "Automatic Downloading";
	s_preferences.allowdownload.generic.flags		= QMF_PULSEIFFOCUS|QMF_SMALLFONT;
	s_preferences.allowdownload.generic.callback	= Preferences_Event;
	s_preferences.allowdownload.generic.id			= ID_ALLOWDOWNLOAD;
	s_preferences.allowdownload.generic.x			= PREFERENCES_X_POS;
	s_preferences.allowdownload.generic.y			= y;

	y += BIGCHAR_SIZE + 2;
	s_preferences.back.generic.type		= MTYPE_BITMAP;
	s_preferences.back.generic.flags	= QMF_LEFT_JUSTIFY;
	s_preferences.back.generic.callback	= Preferences_Event;
	s_preferences.back.generic.id		= ID_BACK;
	s_preferences.back.generic.x		= 0;
	s_preferences.back.generic.y		= 480-64;
	s_preferences.back.width			= 128;
	s_preferences.back.height			= 64;
	s_preferences.back.string			= "Back";

	Menu_AddItem(&s_preferences.menu, &s_preferences.banner);

	Menu_AddItem(&s_preferences.menu, &s_preferences.crosshair);
	Menu_AddItem(&s_preferences.menu, &s_preferences.simpleitems);
	Menu_AddItem(&s_preferences.menu, &s_preferences.wallmarks);
	Menu_AddItem(&s_preferences.menu, &s_preferences.muzzle);
	Menu_AddItem(&s_preferences.menu, &s_preferences.dynamiclights);
	Menu_AddItem(&s_preferences.menu, &s_preferences.identifytarget);
	Menu_AddItem(&s_preferences.menu, &s_preferences.highqualitysky);
	Menu_AddItem(&s_preferences.menu, &s_preferences.allowdownload);

	Menu_AddItem(&s_preferences.menu, &s_preferences.back);

	Preferences_SetMenuItems();
}

void Preferences_Cache(void)
{
	int	i;

	for (i = 0; i < NUM_CROSSHAIRS; i++) {
		s_preferences.crosshairShader[i] = trap_R_RegisterShaderNoMip(va("crosshair%d", i));
	}
}

void UI_PreferencesMenu(void)
{
	Preferences_MenuInit();
	UI_PushMenu(&s_preferences.menu);
}


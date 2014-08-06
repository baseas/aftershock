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
// ui_setup.c

#include "ui_local.h"

#define SETUP_MENU_VERTICAL_SPACING		34

#define ART_BACK0		"menu/art/back_0"
#define ART_BACK1		"menu/art/back_1"

enum {
	ID_CUSTOMIZEPLAYER = 10,
	ID_CUSTOMIZECONTROLS,
	ID_SYSTEMCONFIG,
	ID_GAME,
	ID_LOAD,
	ID_SAVE,
	ID_DEFAULTS,
	ID_BACK
};

typedef struct {
	menuframework_s	menu;

	menutext_s		banner;
	menutext_s		setupplayer;
	menutext_s		setupcontrols;
	menutext_s		setupsystem;
	menutext_s		game;
	menutext_s		defaults;
	menubitmap_s	back;
} setupMenuInfo_t;

static setupMenuInfo_t	setupMenuInfo;

static void Setup_ResetDefaults_Action(qboolean result)
{
	if (!result) {
		return;
	}
	trap_Cmd_ExecuteText(EXEC_APPEND, "exec default.cfg\n");
	trap_Cmd_ExecuteText(EXEC_APPEND, "cvar_restart\n");
	trap_Cmd_ExecuteText(EXEC_APPEND, "vid_restart\n");
}

static void Setup_ResetDefaults_Draw(void)
{
	SCR_DrawPropString(SCREEN_WIDTH / 2, 356,
		"WARNING: This will reset *ALL*", FONT_CENTER | FONT_SMALL, color_yellow);
	SCR_DrawPropString(SCREEN_WIDTH / 2, 356 + PROP_HEIGHT * 1,
		"options to their default values.", FONT_CENTER | FONT_SMALL, color_yellow);
}

static void UI_SetupMenu_Event(void *ptr, int event)
{
	if (event != QM_ACTIVATED) {
		return;
	}

	switch (((menucommon_s *)ptr)->id) {
	case ID_CUSTOMIZEPLAYER:
		UI_PlayerSettingsMenu();
		break;

	case ID_CUSTOMIZECONTROLS:
		UI_ControlsMenu();
		break;

	case ID_SYSTEMCONFIG:
		UI_GraphicsOptionsMenu();
		break;

	case ID_GAME:
		UI_PreferencesMenu();
		break;

	case ID_DEFAULTS:
		UI_ConfirmMenu("SET TO DEFAULTS?", Setup_ResetDefaults_Draw, Setup_ResetDefaults_Action);
		break;

	case ID_BACK:
		UI_PopMenu();
		break;
	}
}

static void UI_SetupMenu_Init(void)
{
	int	y;

	UI_SetupMenu_Cache();

	memset(&setupMenuInfo, 0, sizeof(setupMenuInfo));
	setupMenuInfo.menu.wrapAround = qtrue;
	setupMenuInfo.menu.fullscreen = qtrue;

	setupMenuInfo.banner.generic.type				= MTYPE_BTEXT;
	setupMenuInfo.banner.generic.x					= 320;
	setupMenuInfo.banner.generic.y					= 16;
	setupMenuInfo.banner.string						= "SETUP";
	setupMenuInfo.banner.color						= color_white;
	setupMenuInfo.banner.style						= FONT_CENTER;

	y = 134;
	setupMenuInfo.setupplayer.generic.type			= MTYPE_PTEXT;
	setupMenuInfo.setupplayer.generic.flags			= QMF_CENTER_JUSTIFY|QMF_PULSEIFFOCUS;
	setupMenuInfo.setupplayer.generic.x				= 320;
	setupMenuInfo.setupplayer.generic.y				= y;
	setupMenuInfo.setupplayer.generic.id			= ID_CUSTOMIZEPLAYER;
	setupMenuInfo.setupplayer.generic.callback		= UI_SetupMenu_Event; 
	setupMenuInfo.setupplayer.string				= "PLAYER";
	setupMenuInfo.setupplayer.color					= color_red;
	setupMenuInfo.setupplayer.style					= FONT_CENTER;

	y += SETUP_MENU_VERTICAL_SPACING;
	setupMenuInfo.setupcontrols.generic.type		= MTYPE_PTEXT;
	setupMenuInfo.setupcontrols.generic.flags		= QMF_CENTER_JUSTIFY|QMF_PULSEIFFOCUS;
	setupMenuInfo.setupcontrols.generic.x			= 320;
	setupMenuInfo.setupcontrols.generic.y			= y;
	setupMenuInfo.setupcontrols.generic.id			= ID_CUSTOMIZECONTROLS;
	setupMenuInfo.setupcontrols.generic.callback	= UI_SetupMenu_Event; 
	setupMenuInfo.setupcontrols.string				= "CONTROLS";
	setupMenuInfo.setupcontrols.color				= color_red;
	setupMenuInfo.setupcontrols.style				= FONT_CENTER;

	y += SETUP_MENU_VERTICAL_SPACING;
	setupMenuInfo.setupsystem.generic.type			= MTYPE_PTEXT;
	setupMenuInfo.setupsystem.generic.flags			= QMF_CENTER_JUSTIFY|QMF_PULSEIFFOCUS;
	setupMenuInfo.setupsystem.generic.x				= 320;
	setupMenuInfo.setupsystem.generic.y				= y;
	setupMenuInfo.setupsystem.generic.id			= ID_SYSTEMCONFIG;
	setupMenuInfo.setupsystem.generic.callback		= UI_SetupMenu_Event; 
	setupMenuInfo.setupsystem.string				= "SYSTEM";
	setupMenuInfo.setupsystem.color					= color_red;
	setupMenuInfo.setupsystem.style					= FONT_CENTER;

	y += SETUP_MENU_VERTICAL_SPACING;
	setupMenuInfo.game.generic.type					= MTYPE_PTEXT;
	setupMenuInfo.game.generic.flags				= QMF_CENTER_JUSTIFY|QMF_PULSEIFFOCUS;
	setupMenuInfo.game.generic.x					= 320;
	setupMenuInfo.game.generic.y					= y;
	setupMenuInfo.game.generic.id					= ID_GAME;
	setupMenuInfo.game.generic.callback				= UI_SetupMenu_Event; 
	setupMenuInfo.game.string						= "GAME OPTIONS";
	setupMenuInfo.game.color						= color_red;
	setupMenuInfo.game.style						= FONT_CENTER;

	if (!trap_Cvar_VariableValue("cl_paused")) {
#if 0
		y += SETUP_MENU_VERTICAL_SPACING;
		setupMenuInfo.load.generic.type					= MTYPE_PTEXT;
		setupMenuInfo.load.generic.flags				= QMF_CENTER_JUSTIFY|QMF_PULSEIFFOCUS;
		setupMenuInfo.load.generic.x					= 320;
		setupMenuInfo.load.generic.y					= y;
		setupMenuInfo.load.generic.id					= ID_LOAD;
		setupMenuInfo.load.generic.callback				= UI_SetupMenu_Event; 
		setupMenuInfo.load.string						= "LOAD";
		setupMenuInfo.load.color						= color_red;
		setupMenuInfo.load.style						= FONT_CENTER;

		y += SETUP_MENU_VERTICAL_SPACING;
		setupMenuInfo.save.generic.type					= MTYPE_PTEXT;
		setupMenuInfo.save.generic.flags				= QMF_CENTER_JUSTIFY|QMF_PULSEIFFOCUS;
		setupMenuInfo.save.generic.x					= 320;
		setupMenuInfo.save.generic.y					= y;
		setupMenuInfo.save.generic.id					= ID_SAVE;
		setupMenuInfo.save.generic.callback				= UI_SetupMenu_Event; 
		setupMenuInfo.save.string						= "SAVE";
		setupMenuInfo.save.color						= color_red;
		setupMenuInfo.save.style						= FONT_CENTER;
#endif

		y += SETUP_MENU_VERTICAL_SPACING;
		setupMenuInfo.defaults.generic.type				= MTYPE_PTEXT;
		setupMenuInfo.defaults.generic.flags			= QMF_CENTER_JUSTIFY|QMF_PULSEIFFOCUS;
		setupMenuInfo.defaults.generic.x				= 320;
		setupMenuInfo.defaults.generic.y				= y;
		setupMenuInfo.defaults.generic.id				= ID_DEFAULTS;
		setupMenuInfo.defaults.generic.callback			= UI_SetupMenu_Event; 
		setupMenuInfo.defaults.string					= "DEFAULTS";
		setupMenuInfo.defaults.color					= color_red;
		setupMenuInfo.defaults.style					= FONT_CENTER;
	}

	setupMenuInfo.back.generic.type					= MTYPE_BITMAP;
	setupMenuInfo.back.generic.name					= ART_BACK0;
	setupMenuInfo.back.generic.flags				= QMF_LEFT_JUSTIFY|QMF_PULSEIFFOCUS;
	setupMenuInfo.back.generic.id					= ID_BACK;
	setupMenuInfo.back.generic.callback				= UI_SetupMenu_Event;
	setupMenuInfo.back.generic.x					= 0;
	setupMenuInfo.back.generic.y					= 480-64;
	setupMenuInfo.back.width						= 128;
	setupMenuInfo.back.height						= 64;
	setupMenuInfo.back.focuspic						= ART_BACK1;

	Menu_AddItem(&setupMenuInfo.menu, &setupMenuInfo.banner);
	Menu_AddItem(&setupMenuInfo.menu, &setupMenuInfo.setupplayer);
	Menu_AddItem(&setupMenuInfo.menu, &setupMenuInfo.setupcontrols);
	Menu_AddItem(&setupMenuInfo.menu, &setupMenuInfo.setupsystem);
	Menu_AddItem(&setupMenuInfo.menu, &setupMenuInfo.game);
	if (!trap_Cvar_VariableValue("cl_paused")) {
		Menu_AddItem(&setupMenuInfo.menu, &setupMenuInfo.defaults);
	}
	Menu_AddItem(&setupMenuInfo.menu, &setupMenuInfo.back);
}

void UI_SetupMenu_Cache(void)
{
	trap_R_RegisterShaderNoMip(ART_BACK0);
	trap_R_RegisterShaderNoMip(ART_BACK1);
}

void UI_SetupMenu(void)
{
	UI_SetupMenu_Init();
	UI_PushMenu(&setupMenuInfo.menu);
}


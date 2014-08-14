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
// ui_ungame.c

#include "ui_local.h"

#define INGAME_FRAME					"menu/art/addbotframe"
#define INGAME_MENU_VERTICAL_SPACING	28

enum {
	ID_TEAM = 10,
	ID_ADDBOTS,
	ID_REMOVEBOTS,
	ID_SETUP,
	ID_SERVERINFO,
	ID_LEAVEARENA,
	ID_RESTART,
	ID_QUIT,
	ID_CALLVOTE
};

typedef struct {
	menuframework_s	menu;

	menubitmap_s	frame;
	menutext_s		team;
	menutext_s		setup;
	menutext_s		server;
	menutext_s		leave;
	menutext_s		restart;
	menutext_s		addbots;
	menutext_s		removebots;
	menutext_s		quit;
	menutext_s		callvote;
} ingamemenu_t;

static ingamemenu_t	s_ingame;

static void InGame_RestartAction(qboolean result)
{
	if (!result) {
		return;
	}

	UI_PopMenu();
	trap_Cmd_ExecuteText(EXEC_APPEND, "map_restart\n");
}

static void InGame_QuitAction(qboolean result)
{
	if (!result) {
		return;
	}
	trap_Cmd_ExecuteText(EXEC_APPEND, "quit\n");
}

void InGame_Event(void *ptr, int notification)
{
	if (notification != QM_ACTIVATED) {
		return;
	}

	switch (((menucommon_s *)ptr)->id) {
	case ID_TEAM:
		UI_TeamMainMenu();
		break;

	case ID_SETUP:
		UI_SetupMenu();
		break;

	case ID_LEAVEARENA:
		trap_Cmd_ExecuteText(EXEC_APPEND, "disconnect\n");
		break;

	case ID_RESTART:
		UI_ConfirmMenu("RESTART ARENA?", 0, InGame_RestartAction);
		break;

	case ID_QUIT:
		UI_ConfirmMenu("EXIT GAME?",  0, InGame_QuitAction);
		break;

	case ID_SERVERINFO:
		UI_ServerInfoMenu();
		break;

	case ID_ADDBOTS:
		UI_AddBotsMenu();
		break;

	case ID_REMOVEBOTS:
		UI_RemoveBotsMenu();
		break;

	case ID_CALLVOTE:
		// TODO
		break;
	}
}

void InGame_MenuInit(void)
{
	int		y;

	memset(&s_ingame, 0 ,sizeof(ingamemenu_t));

	InGame_Cache();

	s_ingame.menu.wrapAround = qtrue;
	s_ingame.menu.fullscreen = qfalse;

	s_ingame.frame.generic.type			= MTYPE_BITMAP;
	s_ingame.frame.generic.flags		= QMF_INACTIVE;
	s_ingame.frame.generic.name			= INGAME_FRAME;
	s_ingame.frame.generic.x			= 320-233;
	s_ingame.frame.generic.y			= 240-166;
	s_ingame.frame.width				= 466;
	s_ingame.frame.height				= 332;

	y = 88;
	s_ingame.team.generic.type			= MTYPE_PTEXT;
	s_ingame.team.generic.flags			= QMF_CENTER_JUSTIFY|QMF_PULSEIFFOCUS;
	s_ingame.team.generic.x				= 320;
	s_ingame.team.generic.y				= y;
	s_ingame.team.generic.id			= ID_TEAM;
	s_ingame.team.generic.callback		= InGame_Event;
	s_ingame.team.string				= "START";
	s_ingame.team.color					= colorRed;
	s_ingame.team.style					= FONT_CENTER|FONT_SMALL;

	y += INGAME_MENU_VERTICAL_SPACING;
	s_ingame.addbots.generic.type		= MTYPE_PTEXT;
	s_ingame.addbots.generic.flags		= QMF_CENTER_JUSTIFY|QMF_PULSEIFFOCUS;
	s_ingame.addbots.generic.x			= 320;
	s_ingame.addbots.generic.y			= y;
	s_ingame.addbots.generic.id			= ID_ADDBOTS;
	s_ingame.addbots.generic.callback	= InGame_Event;
	s_ingame.addbots.string				= "ADD BOTS";
	s_ingame.addbots.color				= colorRed;
	s_ingame.addbots.style				= FONT_CENTER|FONT_SMALL;
	if (!trap_Cvar_VariableValue("sv_running") || !trap_Cvar_VariableValue("bot_enable")) {
		s_ingame.addbots.generic.flags |= QMF_GRAYED;
	}

	y += INGAME_MENU_VERTICAL_SPACING;
	s_ingame.removebots.generic.type		= MTYPE_PTEXT;
	s_ingame.removebots.generic.flags		= QMF_CENTER_JUSTIFY|QMF_PULSEIFFOCUS;
	s_ingame.removebots.generic.x			= 320;
	s_ingame.removebots.generic.y			= y;
	s_ingame.removebots.generic.id			= ID_REMOVEBOTS;
	s_ingame.removebots.generic.callback	= InGame_Event;
	s_ingame.removebots.string				= "REMOVE BOTS";
	s_ingame.removebots.color				= colorRed;
	s_ingame.removebots.style				= FONT_CENTER|FONT_SMALL;
	if (!trap_Cvar_VariableValue("sv_running") || !trap_Cvar_VariableValue("bot_enable")) {
		s_ingame.removebots.generic.flags |= QMF_GRAYED;
	}

	y += INGAME_MENU_VERTICAL_SPACING;
	s_ingame.setup.generic.type			= MTYPE_PTEXT;
	s_ingame.setup.generic.flags		= QMF_CENTER_JUSTIFY|QMF_PULSEIFFOCUS;
	s_ingame.setup.generic.x			= 320;
	s_ingame.setup.generic.y			= y;
	s_ingame.setup.generic.id			= ID_SETUP;
	s_ingame.setup.generic.callback		= InGame_Event;
	s_ingame.setup.string				= "SETUP";
	s_ingame.setup.color				= colorRed;
	s_ingame.setup.style				= FONT_CENTER|FONT_SMALL;

	y += INGAME_MENU_VERTICAL_SPACING;
	s_ingame.server.generic.type		= MTYPE_PTEXT;
	s_ingame.server.generic.flags		= QMF_CENTER_JUSTIFY|QMF_PULSEIFFOCUS;
	s_ingame.server.generic.x			= 320;
	s_ingame.server.generic.y			= y;
	s_ingame.server.generic.id			= ID_SERVERINFO;
	s_ingame.server.generic.callback	= InGame_Event;
	s_ingame.server.string				= "SERVER INFO";
	s_ingame.server.color				= colorRed;
	s_ingame.server.style				= FONT_CENTER|FONT_SMALL;

	y += INGAME_MENU_VERTICAL_SPACING;
	s_ingame.restart.generic.type		= MTYPE_PTEXT;
	s_ingame.restart.generic.flags		= QMF_CENTER_JUSTIFY|QMF_PULSEIFFOCUS;
	s_ingame.restart.generic.x			= 320;
	s_ingame.restart.generic.y			= y;
	s_ingame.restart.generic.id			= ID_RESTART;
	s_ingame.restart.generic.callback	= InGame_Event;
	s_ingame.restart.string				= "RESTART ARENA";
	s_ingame.restart.color				= colorRed;
	s_ingame.restart.style				= FONT_CENTER|FONT_SMALL;
	if (!trap_Cvar_VariableValue("sv_running")) {
		s_ingame.restart.generic.flags |= QMF_GRAYED;
	}

	y += INGAME_MENU_VERTICAL_SPACING;
	s_ingame.callvote.generic.type			= MTYPE_PTEXT;
	s_ingame.callvote.generic.flags			= QMF_CENTER_JUSTIFY|QMF_PULSEIFFOCUS;
	s_ingame.callvote.generic.x				= 320;
	s_ingame.callvote.generic.y				= y;
	s_ingame.callvote.generic.id			= ID_CALLVOTE;
	s_ingame.callvote.generic.callback		= InGame_Event;
	s_ingame.callvote.string				= "CALL VOTE";
	s_ingame.callvote.color					= colorRed;
	s_ingame.callvote.style					= FONT_CENTER|FONT_SMALL;

	y += INGAME_MENU_VERTICAL_SPACING;
	s_ingame.leave.generic.type			= MTYPE_PTEXT;
	s_ingame.leave.generic.flags		= QMF_CENTER_JUSTIFY|QMF_PULSEIFFOCUS;
	s_ingame.leave.generic.x			= 320;
	s_ingame.leave.generic.y			= y;
	s_ingame.leave.generic.id			= ID_LEAVEARENA;
	s_ingame.leave.generic.callback		= InGame_Event;
	s_ingame.leave.string				= "LEAVE ARENA";
	s_ingame.leave.color				= colorRed;
	s_ingame.leave.style				= FONT_CENTER|FONT_SMALL;

	y += INGAME_MENU_VERTICAL_SPACING;
	s_ingame.quit.generic.type			= MTYPE_PTEXT;
	s_ingame.quit.generic.flags			= QMF_CENTER_JUSTIFY|QMF_PULSEIFFOCUS;
	s_ingame.quit.generic.x				= 320;
	s_ingame.quit.generic.y				= y;
	s_ingame.quit.generic.id			= ID_QUIT;
	s_ingame.quit.generic.callback		= InGame_Event;
	s_ingame.quit.string				= "EXIT GAME";
	s_ingame.quit.color					= colorRed;
	s_ingame.quit.style					= FONT_CENTER|FONT_SMALL;

	Menu_AddItem(&s_ingame.menu, &s_ingame.frame);
	Menu_AddItem(&s_ingame.menu, &s_ingame.team);
	Menu_AddItem(&s_ingame.menu, &s_ingame.addbots);
	Menu_AddItem(&s_ingame.menu, &s_ingame.removebots);
	Menu_AddItem(&s_ingame.menu, &s_ingame.setup);
	Menu_AddItem(&s_ingame.menu, &s_ingame.server);
	Menu_AddItem(&s_ingame.menu, &s_ingame.restart);
	Menu_AddItem(&s_ingame.menu, &s_ingame.callvote);
	Menu_AddItem(&s_ingame.menu, &s_ingame.leave);
	Menu_AddItem(&s_ingame.menu, &s_ingame.quit);
}

void InGame_Cache(void)
{
	trap_R_RegisterShaderNoMip(INGAME_FRAME);
}

void UI_InGameMenu(void)
{
	// force as top level menu
	uis.menusp = 0;

	// set menu cursor to a nice location
	uis.cursorx = 319;
	uis.cursory = 80;

	InGame_MenuInit();
	UI_PushMenu(&s_ingame.menu);
}


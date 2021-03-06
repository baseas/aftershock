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
// ui_menu.c

#include "ui_local.h"

enum {
	ID_SINGLEPLAYER = 10,
	ID_MULTIPLAYER,
	ID_SETUP,
	ID_DEMOS,
	ID_EXIT
};

#define MAIN_MENU_TOP_Y					150
#define MAIN_MENU_VERTICAL_SPACING		34

typedef struct {
	menuframework_s	menu;

	menutext_s		singleplayer;
	menutext_s		multiplayer;
	menutext_s		setup;
	menutext_s		demos;
	menutext_s		cinematics;
	menutext_s		exit;
} mainmenu_t;

static mainmenu_t s_main;

typedef struct {
	menuframework_s menu;
	char errorMessage[4096];
} errorMessage_t;

static errorMessage_t s_errorMessage;

static void MainMenu_ExitAction(void)
{
	trap_Cmd_ExecuteText(EXEC_APPEND, "quit\n");
}

void Main_MenuEvent (void *ptr, int event)
{
	if (event != QM_ACTIVATED) {
		return;
	}

	switch (((menucommon_s*)ptr)->id) {
	case ID_SINGLEPLAYER:
		UI_StartServerMenu(qtrue);
		break;

	case ID_MULTIPLAYER:
		UI_ArenaServersMenu();
		break;

	case ID_SETUP:
		UI_SetupMenu();
		break;

	case ID_DEMOS:
		UI_DemosMenu();
		break;

	case ID_EXIT:
		MainMenu_ExitAction();
		break;
	}
}

sfxHandle_t ErrorMessage_Key(int key)
{
	trap_Cvar_Set("com_errorMessage", "");
	UI_MainMenu();
	return (menu_null_sound);
}

static void Main_MenuDraw(void)
{
	int				i;
	vec4_t			color = {0.5, 0, 0, 1};

	if (strlen(s_errorMessage.errorMessage)) {
		UI_DrawProportionalString_AutoWrapped(MENU_XPOS, 192, 600, 20, s_errorMessage.errorMessage,
			FONT_CENTER | FONT_SMALL | FONT_SHADOW, colorMenuText);
	} else {
		// standard menu drawing
		Menu_Draw(&s_main.menu);
	}

	UI_DrawNamedPic(32, 26, 220, 30, "banner");
	UI_DrawNamedPic(291, 127, 298, 253, "logo");
	for (i = 0; i < 5; ++i) {
		int	y;
		y = MAIN_MENU_TOP_Y + (i + 0.5f) * MAIN_MENU_VERTICAL_SPACING - 13;
		UI_DrawNamedPic(65, y, 10, 15, "white_arrow_small");
	}

	SCR_DrawString(320, 450, "aftershock-fps.com", SMALLCHAR_SIZE, FONT_CENTER | FONT_SHADOW, color);
}

void UI_MainMenu(void)
{
	int		y;
	int		style = FONT_SMALL;

	trap_Cvar_Set("sv_killserver", "1");

	memset(&s_main, 0 ,sizeof(mainmenu_t));
	memset(&s_errorMessage, 0 ,sizeof(errorMessage_t));

	trap_Cvar_VariableStringBuffer("com_errorMessage", s_errorMessage.errorMessage, sizeof(s_errorMessage.errorMessage));
	if (*s_errorMessage.errorMessage) {
		s_errorMessage.menu.draw = Main_MenuDraw;
		s_errorMessage.menu.key = ErrorMessage_Key;
		s_errorMessage.menu.fullscreen = qtrue;
		s_errorMessage.menu.wrapAround = qtrue;
		s_errorMessage.menu.showlogo = qtrue;

		trap_Key_SetCatcher(KEYCATCH_UI);
		uis.menusp = 0;
		UI_PushMenu (&s_errorMessage.menu);

		return;
	}

	s_main.menu.draw = Main_MenuDraw;
	s_main.menu.fullscreen = qtrue;
	s_main.menu.wrapAround = qtrue;
	s_main.menu.showlogo = qtrue;

	y = MAIN_MENU_TOP_Y;
	s_main.singleplayer.generic.type		= MTYPE_PTEXT;
	s_main.singleplayer.generic.flags		= QMF_PULSEIFFOCUS;
	s_main.singleplayer.generic.x			= MENU_XPOS;
	s_main.singleplayer.generic.y			= y;
	s_main.singleplayer.generic.id			= ID_SINGLEPLAYER;
	s_main.singleplayer.generic.callback	= Main_MenuEvent;
	s_main.singleplayer.string				= "CREATE GAME";
	s_main.singleplayer.color				= colorRed;
	s_main.singleplayer.style				= style;

	y += MAIN_MENU_VERTICAL_SPACING;
	s_main.multiplayer.generic.type			= MTYPE_PTEXT;
	s_main.multiplayer.generic.flags		= QMF_PULSEIFFOCUS;
	s_main.multiplayer.generic.x			= MENU_XPOS;
	s_main.multiplayer.generic.y			= y;
	s_main.multiplayer.generic.id			= ID_MULTIPLAYER;
	s_main.multiplayer.generic.callback		= Main_MenuEvent;
	s_main.multiplayer.string				= "MULTIPLAYER";
	s_main.multiplayer.color				= colorRed;
	s_main.multiplayer.style				= style;

	y += MAIN_MENU_VERTICAL_SPACING;
	s_main.setup.generic.type				= MTYPE_PTEXT;
	s_main.setup.generic.flags				= QMF_PULSEIFFOCUS;
	s_main.setup.generic.x					= MENU_XPOS;
	s_main.setup.generic.y					= y;
	s_main.setup.generic.id					= ID_SETUP;
	s_main.setup.generic.callback			= Main_MenuEvent;
	s_main.setup.string						= "SETUP";
	s_main.setup.color						= colorRed;
	s_main.setup.style						= style;

	y += MAIN_MENU_VERTICAL_SPACING;
	s_main.demos.generic.type				= MTYPE_PTEXT;
	s_main.demos.generic.flags				= QMF_PULSEIFFOCUS;
	s_main.demos.generic.x					= MENU_XPOS;
	s_main.demos.generic.y					= y;
	s_main.demos.generic.id					= ID_DEMOS;
	s_main.demos.generic.callback			= Main_MenuEvent;
	s_main.demos.string						= "DEMOS";
	s_main.demos.color						= colorRed;
	s_main.demos.style						= style;

	y += MAIN_MENU_VERTICAL_SPACING;
	s_main.exit.generic.type				= MTYPE_PTEXT;
	s_main.exit.generic.flags				= QMF_PULSEIFFOCUS;
	s_main.exit.generic.x					= MENU_XPOS;
	s_main.exit.generic.y					= y;
	s_main.exit.generic.id					= ID_EXIT;
	s_main.exit.generic.callback			= Main_MenuEvent;
	s_main.exit.string						= "EXIT";
	s_main.exit.color						= colorRed;
	s_main.exit.style						= style;

	Menu_AddItem(&s_main.menu, &s_main.singleplayer);
	Menu_AddItem(&s_main.menu, &s_main.multiplayer);
	Menu_AddItem(&s_main.menu, &s_main.setup);
	Menu_AddItem(&s_main.menu, &s_main.demos);
	Menu_AddItem(&s_main.menu, &s_main.exit);

	trap_Key_SetCatcher(KEYCATCH_UI);
	uis.menusp = 0;
	UI_PushMenu (&s_main.menu);
}


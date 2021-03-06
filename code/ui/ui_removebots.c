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
// ui_removebots.c -- remove bots menu

#include "ui_local.h"

#define ART_BACKGROUND		"menu/art/addbotframe"
#define ART_ARROWS			"menu/art/arrows_vert_0"
#define ART_ARROWUP			"menu/art/arrows_vert_top"
#define ART_ARROWDOWN		"menu/art/arrows_vert_bot"

enum {
	ID_UP = 10,
	ID_DOWN,
	ID_DELETE,
	ID_BACK,
	ID_BOTNAME0,
	ID_BOTNAME1,
	ID_BOTNAME2,
	ID_BOTNAME3,
	ID_BOTNAME4,
	ID_BOTNAME5,
	ID_BOTNAME6
};

typedef struct {
	menuframework_s	menu;

	menutext_s		banner;
	menubitmap_s	background;

	menubitmap_s	arrows;
	menubitmap_s	up;
	menubitmap_s	down;

	menutext_s		bots[7];

	menubutton_s	delete;
	menubutton_s	back;

	int				numBots;
	int				baseBotNum;
	int				selectedBotNum;
	char			botnames[7][32];
	int				botClientNums[MAX_BOTS];
} removeBotsMenuInfo_t;

static removeBotsMenuInfo_t	removeBotsMenuInfo;

static void UI_RemoveBotsMenu_SetBotNames(void)
{
	int		n;
	char	info[MAX_INFO_STRING];

	for (n = 0; (n < 7) && (removeBotsMenuInfo.baseBotNum + n < removeBotsMenuInfo.numBots); n++) {
		trap_GetConfigString(CS_PLAYERS + removeBotsMenuInfo.botClientNums[removeBotsMenuInfo.baseBotNum + n], info, MAX_INFO_STRING);
		Q_strncpyz(removeBotsMenuInfo.botnames[n], Info_ValueForKey(info, "n"), sizeof(removeBotsMenuInfo.botnames[n]));
		Q_CleanStr(removeBotsMenuInfo.botnames[n]);
	}
}

static void UI_RemoveBotsMenu_DeleteEvent(void *ptr, int event)
{
	if (event != QM_ACTIVATED) {
		return;
	}

	trap_Cmd_ExecuteText(EXEC_APPEND, va("clientkick %i\n", removeBotsMenuInfo.botClientNums[removeBotsMenuInfo.baseBotNum + removeBotsMenuInfo.selectedBotNum]));
}

static void UI_RemoveBotsMenu_BotEvent(void *ptr, int event)
{
	if (event != QM_ACTIVATED) {
		return;
	}

	removeBotsMenuInfo.bots[removeBotsMenuInfo.selectedBotNum].color = colorOrange;
	removeBotsMenuInfo.selectedBotNum = ((menucommon_s*)ptr)->id - ID_BOTNAME0;
	removeBotsMenuInfo.bots[removeBotsMenuInfo.selectedBotNum].color = colorWhite;
}

static void UI_RemoveBotsMenu_BackEvent(void *ptr, int event)
{
	if (event != QM_ACTIVATED) {
		return;
	}
	UI_PopMenu();
}

static void UI_RemoveBotsMenu_UpEvent(void *ptr, int event)
{
	if (event != QM_ACTIVATED) {
		return;
	}

	if (removeBotsMenuInfo.baseBotNum > 0) {
		removeBotsMenuInfo.baseBotNum--;
		UI_RemoveBotsMenu_SetBotNames();
	}
}

static void UI_RemoveBotsMenu_DownEvent(void *ptr, int event)
{
	if (event != QM_ACTIVATED) {
		return;
	}

	if (removeBotsMenuInfo.baseBotNum + 7 < removeBotsMenuInfo.numBots) {
		removeBotsMenuInfo.baseBotNum++;
		UI_RemoveBotsMenu_SetBotNames();
	}
}

static void UI_RemoveBotsMenu_GetBots(void)
{
	int		numPlayers;
	int		isBot;
	int		n;
	char	info[MAX_INFO_STRING];

	trap_GetConfigString(CS_SERVERINFO, info, sizeof(info));
	numPlayers = atoi(Info_ValueForKey(info, "sv_maxclients"));
	removeBotsMenuInfo.numBots = 0;

	for (n = 0; n < numPlayers; n++) {
		trap_GetConfigString(CS_PLAYERS + n, info, MAX_INFO_STRING);

		isBot = atoi(Info_ValueForKey(info, "skill"));
		if (!isBot) {
			continue;
		}

		removeBotsMenuInfo.botClientNums[removeBotsMenuInfo.numBots] = n;
		removeBotsMenuInfo.numBots++;
	}
}

void UI_RemoveBots_Cache(void)
{
	trap_R_RegisterShaderNoMip(ART_BACKGROUND);
}

static void UI_RemoveBotsMenu_Init(void)
{
	int		n;
	int		count;
	int		y;

	memset(&removeBotsMenuInfo, 0, sizeof removeBotsMenuInfo);
	removeBotsMenuInfo.menu.fullscreen = qfalse;
	removeBotsMenuInfo.menu.wrapAround = qtrue;

	UI_RemoveBots_Cache();

	UI_RemoveBotsMenu_GetBots();
	UI_RemoveBotsMenu_SetBotNames();
	count = removeBotsMenuInfo.numBots < 7 ? removeBotsMenuInfo.numBots : 7;

	removeBotsMenuInfo.banner.generic.type		= MTYPE_BTEXT;
	removeBotsMenuInfo.banner.generic.x			= 320;
	removeBotsMenuInfo.banner.generic.y			= 16;
	removeBotsMenuInfo.banner.string			= "REMOVE BOTS";
	removeBotsMenuInfo.banner.color				= colorBanner;
	removeBotsMenuInfo.banner.style				= FONT_CENTER | FONT_SHADOW;

	removeBotsMenuInfo.background.generic.type	= MTYPE_BITMAP;
	removeBotsMenuInfo.background.generic.name	= ART_BACKGROUND;
	removeBotsMenuInfo.background.generic.flags	= QMF_INACTIVE;
	removeBotsMenuInfo.background.generic.x		= 320-233;
	removeBotsMenuInfo.background.generic.y		= 240-166;
	removeBotsMenuInfo.background.width			= 466;
	removeBotsMenuInfo.background.height		= 332;

	removeBotsMenuInfo.arrows.generic.type		= MTYPE_BITMAP;
	removeBotsMenuInfo.arrows.generic.name		= ART_ARROWS;
	removeBotsMenuInfo.arrows.generic.flags		= QMF_INACTIVE;
	removeBotsMenuInfo.arrows.generic.x			= 200;
	removeBotsMenuInfo.arrows.generic.y			= 128;
	removeBotsMenuInfo.arrows.width				= 64;
	removeBotsMenuInfo.arrows.height			= 128;

	removeBotsMenuInfo.up.generic.type			= MTYPE_BITMAP;
	removeBotsMenuInfo.up.generic.flags			= QMF_LEFT_JUSTIFY|QMF_PULSEIFFOCUS;
	removeBotsMenuInfo.up.generic.x				= 200;
	removeBotsMenuInfo.up.generic.y				= 128;
	removeBotsMenuInfo.up.generic.id			= ID_UP;
	removeBotsMenuInfo.up.generic.callback		= UI_RemoveBotsMenu_UpEvent;
	removeBotsMenuInfo.up.width					= 64;
	removeBotsMenuInfo.up.height				= 64;
	removeBotsMenuInfo.up.focuspic				= ART_ARROWUP;

	removeBotsMenuInfo.down.generic.type		= MTYPE_BITMAP;
	removeBotsMenuInfo.down.generic.flags		= QMF_LEFT_JUSTIFY|QMF_PULSEIFFOCUS;
	removeBotsMenuInfo.down.generic.x			= 200;
	removeBotsMenuInfo.down.generic.y			= 128+64;
	removeBotsMenuInfo.down.generic.id			= ID_DOWN;
	removeBotsMenuInfo.down.generic.callback	= UI_RemoveBotsMenu_DownEvent;
	removeBotsMenuInfo.down.width				= 64;
	removeBotsMenuInfo.down.height				= 64;
	removeBotsMenuInfo.down.focuspic			= ART_ARROWDOWN;

	for (n = 0, y = 120; n < count; n++, y += 20) {
		removeBotsMenuInfo.bots[n].generic.type		= MTYPE_PTEXT;
		removeBotsMenuInfo.bots[n].generic.flags	= QMF_LEFT_JUSTIFY|QMF_PULSEIFFOCUS;
		removeBotsMenuInfo.bots[n].generic.id		= ID_BOTNAME0 + n;
		removeBotsMenuInfo.bots[n].generic.x		= 320 - 56;
		removeBotsMenuInfo.bots[n].generic.y		= y;
		removeBotsMenuInfo.bots[n].generic.callback	= UI_RemoveBotsMenu_BotEvent;
		removeBotsMenuInfo.bots[n].string			= removeBotsMenuInfo.botnames[n];
		removeBotsMenuInfo.bots[n].color			= colorOrange;
		removeBotsMenuInfo.bots[n].style			= FONT_SMALL;
	}

	removeBotsMenuInfo.delete.generic.type		= MTYPE_BITMAP;
	removeBotsMenuInfo.delete.generic.flags		= QMF_LEFT_JUSTIFY;
	removeBotsMenuInfo.delete.generic.id		= ID_DELETE;
	removeBotsMenuInfo.delete.generic.callback	= UI_RemoveBotsMenu_DeleteEvent;
	removeBotsMenuInfo.delete.generic.x			= 320+128-128;
	removeBotsMenuInfo.delete.generic.y			= 256+128-64;
	removeBotsMenuInfo.delete.width				= 128;
	removeBotsMenuInfo.delete.height			= 64;
	removeBotsMenuInfo.delete.string			= "Remove";

	removeBotsMenuInfo.back.generic.type		= MTYPE_BUTTON;
	removeBotsMenuInfo.back.generic.flags		= QMF_LEFT_JUSTIFY;
	removeBotsMenuInfo.back.generic.id			= ID_BACK;
	removeBotsMenuInfo.back.generic.callback	= UI_RemoveBotsMenu_BackEvent;
	removeBotsMenuInfo.back.generic.x			= 320-128;
	removeBotsMenuInfo.back.generic.y			= 256+128-64;
	removeBotsMenuInfo.back.width				= 128;
	removeBotsMenuInfo.back.height				= 64;
	removeBotsMenuInfo.back.string				= "Back";

	Menu_AddItem(&removeBotsMenuInfo.menu, &removeBotsMenuInfo.background);
	Menu_AddItem(&removeBotsMenuInfo.menu, &removeBotsMenuInfo.banner);
	Menu_AddItem(&removeBotsMenuInfo.menu, &removeBotsMenuInfo.arrows);
	Menu_AddItem(&removeBotsMenuInfo.menu, &removeBotsMenuInfo.up);
	Menu_AddItem(&removeBotsMenuInfo.menu, &removeBotsMenuInfo.down);
	for (n = 0; n < count; n++) {
		Menu_AddItem(&removeBotsMenuInfo.menu, &removeBotsMenuInfo.bots[n]);
	}
	Menu_AddItem(&removeBotsMenuInfo.menu, &removeBotsMenuInfo.delete);
	Menu_AddItem(&removeBotsMenuInfo.menu, &removeBotsMenuInfo.back);

	removeBotsMenuInfo.baseBotNum = 0;
	removeBotsMenuInfo.selectedBotNum = 0;
	removeBotsMenuInfo.bots[0].color = colorWhite;
}

void UI_RemoveBotsMenu(void)
{
	UI_RemoveBotsMenu_Init();
	UI_PushMenu(&removeBotsMenuInfo.menu);
}


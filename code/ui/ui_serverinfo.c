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
// ui_serverinfo.c

#include "ui_local.h"

enum {
	ID_ADD = 100,
	ID_BACK
};

typedef struct {
	menuframework_s	menu;
	menutext_s		banner;
	menubutton_s	back;
	menutext_s		add;
	char			info[MAX_INFO_STRING];
	int				numlines;
} serverinfo_t;

static serverinfo_t	s_serverinfo;

/**
Add current server to favorites
*/
void Favorites_Add(void)
{
	char	adrstr[128];
	char	serverbuff[128];
	int		i;
	int		best;

	trap_Cvar_VariableStringBuffer("cl_currentServerAddress", serverbuff, sizeof serverbuff);
	if (!serverbuff[0])
		return;

	best = 0;
	for (i = 0; i < MAX_FAVORITESERVERS; i++) {
		trap_Cvar_VariableStringBuffer(va("server%d", i + 1), adrstr, sizeof adrstr);
		if (!Q_stricmp(serverbuff, adrstr)) {
			// already in list
			return;
		}

		// use first empty available slot
		if (!adrstr[0] && !best) {
			best = i + 1;
		}
	}

	if (best) {
		trap_Cvar_Set(va("server%d", best), serverbuff);
	}
}

static void ServerInfo_Event(void *ptr, int event)
{
	if (event != QM_ACTIVATED) {
		return;
	}

	switch (((menucommon_s *) ptr)->id) {
	case ID_ADD:
		Favorites_Add();
		UI_PopMenu();
		break;
	case ID_BACK:
		UI_PopMenu();
		break;
	}
}

static void ServerInfo_MenuDraw(void)
{
	const char		*s;
	char			key[MAX_INFO_KEY];
	char			value[MAX_INFO_VALUE];
	int				i = 0, y;

	y = SCREEN_HEIGHT / 2 - s_serverinfo.numlines * SMALLCHAR_SIZE / 2 - 20;
	s = s_serverinfo.info;
	while (s && i < s_serverinfo.numlines) {
		Info_NextPair(&s, key, value);
		if (!key[0]) {
			break;
		}

		Q_strcat(key, MAX_INFO_KEY, ":");

		SCR_DrawString(SCREEN_WIDTH / 2 - 8, y, key, SMALLCHAR_SIZE, FONT_RIGHT, colorRed);
		SCR_DrawString(SCREEN_WIDTH / 2 + 8, y, value, SMALLCHAR_SIZE, 0, colorTextNormal);

		y += SMALLCHAR_SIZE;
		i++;
	}

	Menu_Draw(&s_serverinfo.menu);
}

static sfxHandle_t ServerInfo_MenuKey(int key)
{
	return (Menu_DefaultKey(&s_serverinfo.menu, key));
}

void ServerInfo_Cache(void) { }

void UI_ServerInfoMenu(void)
{
	const char		*s;
	char			key[MAX_INFO_KEY];
	char			value[MAX_INFO_VALUE];

	// zero set all our globals
	memset(&s_serverinfo, 0, sizeof (serverinfo_t));

	ServerInfo_Cache();

	s_serverinfo.menu.draw			= ServerInfo_MenuDraw;
	s_serverinfo.menu.key			= ServerInfo_MenuKey;
	s_serverinfo.menu.wrapAround	= qtrue;
	s_serverinfo.menu.fullscreen	= qtrue;

	s_serverinfo.banner.generic.type	= MTYPE_BTEXT;
	s_serverinfo.banner.generic.x		= 320;
	s_serverinfo.banner.generic.y		= 16;
	s_serverinfo.banner.string			= "SERVER INFO";
	s_serverinfo.banner.color			= colorBanner;
	s_serverinfo.banner.style			= FONT_CENTER | FONT_SHADOW;

	s_serverinfo.add.generic.type		= MTYPE_PTEXT;
	s_serverinfo.add.generic.flags		= QMF_CENTER_JUSTIFY|QMF_PULSEIFFOCUS;
	s_serverinfo.add.generic.callback	= ServerInfo_Event;
	s_serverinfo.add.generic.id			= ID_ADD;
	s_serverinfo.add.generic.x			= 320;
	s_serverinfo.add.generic.y			= 371;
	s_serverinfo.add.string				= "ADD TO FAVORITES";
	s_serverinfo.add.style				= FONT_CENTER | FONT_SMALL;
	s_serverinfo.add.color				= colorRed;
	if (trap_Cvar_VariableValue("sv_running")) {
		s_serverinfo.add.generic.flags |= QMF_GRAYED;
	}

	s_serverinfo.back.generic.type		= MTYPE_BUTTON;
	s_serverinfo.back.generic.flags		= QMF_LEFT_JUSTIFY;
	s_serverinfo.back.generic.callback	= ServerInfo_Event;
	s_serverinfo.back.generic.id		= ID_BACK;
	s_serverinfo.back.generic.x			= 0;
	s_serverinfo.back.generic.y			= 480-64;
	s_serverinfo.back.width				= 128;
	s_serverinfo.back.height			= 64;
	s_serverinfo.back.string			= "Back";

	trap_GetConfigString(CS_SERVERINFO, s_serverinfo.info, MAX_INFO_STRING);

	s_serverinfo.numlines = 0;
	s = s_serverinfo.info;
	while (s) {
		Info_NextPair(&s, key, value);
		if (!key[0]) {
			break;
		}
		s_serverinfo.numlines++;
	}

	if (s_serverinfo.numlines > 16)
		s_serverinfo.numlines = 16;

	Menu_AddItem(&s_serverinfo.menu, (void *) &s_serverinfo.banner);
	Menu_AddItem(&s_serverinfo.menu, (void *) &s_serverinfo.add);
	Menu_AddItem(&s_serverinfo.menu, (void *) &s_serverinfo.back);

	UI_PushMenu(&s_serverinfo.menu);
}


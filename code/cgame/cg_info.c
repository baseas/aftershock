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
// cg_info.c -- display information while data is being loading

#include "cg_local.h"

#define MAX_LOADING_PLAYER_ICONS	16
#define MAX_LOADING_ITEM_ICONS		26

static int			loadingItemIconCount;
static qhandle_t	loadingItemIcons[MAX_LOADING_ITEM_ICONS];

static void CG_DrawLoadingIcons(void)
{
	int		n;
	int		x, y;

	for (n = 0; n < loadingItemIconCount; n++) {
		y = 400-40;
		if (n >= 13) {
			y += 40;
		}
		x = 16 + n % 13 * 48;
		CG_DrawAdjustPic(x, y, 32, 32, loadingItemIcons[n]);
	}
}

void CG_LoadingItem(int itemNum)
{
	gitem_t		*item;

	item = &bg_itemlist[itemNum];
	
	if (item->icon && loadingItemIconCount < MAX_LOADING_ITEM_ICONS) {
		loadingItemIcons[loadingItemIconCount++] = trap_R_RegisterShaderNoMip(item->icon);
	}
	trap_UpdateScreen();
}

/**
Draw all the status / pacifier stuff during level loading
*/
void CG_DrawInformation(void)
{
	const char	*s;
	const char	*info;
	const char	*sysInfo;
	int			y;
	int			value;
	qhandle_t	background;
	char		buf[1024];

	if (!cg.showInfo && !cg.showInfoScreen) {
		return;
	}

	info = CG_ConfigString(CS_SERVERINFO);
	sysInfo = CG_ConfigString(CS_SYSTEMINFO);

	if (!cg.showInfo) {
		background = trap_R_RegisterShaderNoMip("menuback");
		CG_DrawPic(0, 0, SCREEN_WIDTH, SCREEN_HEIGHT, background);

		// draw the icons of things as they are loaded
		CG_DrawLoadingIcons();
	}

	// the first 150 rows are reserved for the client connection
	// screen to write into
	if (cg.showInfoScreen) {
		SCR_DrawPropString(320, 128 - 32, "Loading... ", FONT_CENTER | FONT_SHADOW, colorWhite);
	} else if (!cg.showInfo) {
		SCR_DrawPropString(320, 128 - 32, "Awaiting snapshot...", FONT_CENTER | FONT_SHADOW, colorWhite);
	}

	// draw info string information

	y = 180-32;

	// don't print server lines if playing a local game
	trap_Cvar_VariableStringBuffer("sv_running", buf, sizeof(buf));
	if (!atoi(buf)) {
		// server hostname
		Q_strncpyz(buf, Info_ValueForKey(info, "sv_hostname"), 1024);
		Q_CleanStr(buf);
		SCR_DrawPropString(320, y, buf, FONT_CENTER | FONT_SHADOW, colorWhite);
		y += PROP_HEIGHT;

		// pure server
		s = Info_ValueForKey(sysInfo, "sv_pure");
		if (s[0] == '1') {
			SCR_DrawPropString(320, y, "Pure Server", FONT_CENTER | FONT_SHADOW, colorWhite);
			y += PROP_HEIGHT;
		}

		// server-specific message of the day
		s = CG_ConfigString(CS_MOTD);
		if (s[0]) {
			SCR_DrawPropString(320, y, s, FONT_CENTER | FONT_SHADOW, colorWhite);
			y += PROP_HEIGHT;
		}

		// some extra space after hostname and motd
		y += 10;
	}

	// map-specific message (long map name)
	s = CG_ConfigString(CS_MESSAGE);
	if (s[0]) {
		SCR_DrawPropString(320, y, s, FONT_CENTER | FONT_SHADOW, colorYellow);
		y += PROP_HEIGHT;
	}

	// cheats warning
	s = Info_ValueForKey(sysInfo, "sv_cheats");
	if (s[0] == '1') {
		SCR_DrawPropString(320, y, "CHEATS ARE ENABLED", FONT_CENTER | FONT_SHADOW, colorWhite);
		y += PROP_HEIGHT;
	}

	SCR_DrawPropString(320, y, cgs.gametypeName, FONT_CENTER | FONT_SHADOW, colorWhite);
	y += PROP_HEIGHT;

	value = atoi(Info_ValueForKey(info, "timelimit"));
	if (value) {
		SCR_DrawPropString(320, y, "timelimit %d", FONT_CENTER | FONT_SHADOW, colorWhite);
		y += PROP_HEIGHT;
	}

	if (cgs.gametype == GT_FFA) {
		value = atoi(Info_ValueForKey(info, "fraglimit"));
		if (value) {
			SCR_DrawPropString(320, y, va("fraglimit %d", value), FONT_CENTER | FONT_SHADOW, colorWhite);
			y += PROP_HEIGHT;
		}
	}

	if (cgs.gametype >= GT_CTF) {
		value = atoi(Info_ValueForKey(info, "capturelimit"));
		if (value) {
			SCR_DrawPropString(320, y, va("capturelimit %d", value), FONT_CENTER | FONT_SHADOW, colorWhite);
			y += PROP_HEIGHT;
		}
	}
}


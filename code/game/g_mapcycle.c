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
// g_mapcycle.c

#include "g_local.h"

typedef struct {
	char	name[64];
	int		minPlayers;
	int		maxPlayers;
	int		gametype;
} mapEntry_t;

static int			mapCount;
static mapEntry_t	maps[128];

void G_MapCycleRead(void)
{
	fileHandle_t	fp;
	iniSection_t	section;
	mapEntry_t		*map;
	char			*value;

	mapCount = 0;

	trap_FS_FOpenFile("mapcycle.ini", &fp, FS_READ);
	if (!fp) {
		return;
	} else {
		G_LogPrintf("Reading map cycle...\n");
	}

	while (!trap_Ini_Section(&section, fp)) {
		if (strcmp(section.label, "map")) {
			G_Printf("Map cycle: Invalid section in mapcycle.ini.\n");
			continue;
		}

		if (mapCount >= ARRAY_LEN(maps) - 1) {
			G_Printf("Map cycle: Too many custom votes.\n");
			break;
		}

		map = &maps[mapCount];

		value = Ini_GetValue(&section, "name");
		if (!value) {
			G_Printf("Map cycle: No map name specified.\n");
			continue;
		}
		Q_strncpyz(map->name, value, sizeof map->name);

		value = Ini_GetValue(&section, "minPlayers");
		if (value) {
			map->minPlayers = atoi(value);
		}

		value = Ini_GetValue(&section, "maxPlayers");
		if (value) {
			map->maxPlayers = atoi(value);
		}

		value = Ini_GetValue(&section, "gametype");
		if (value) {
			char	*token;
			while ((token = COM_Parse(&value))) {
				map->gametype |= 1 << atoi(token);
			}
		}

		mapCount++;
	}

	trap_FS_FCloseFile(fp);
}

void G_MapCycleNextmap(void)
{
}

void G_MapCycleList(gentity_t *ent)
{
}

void G_MapCycleAdd(gentity_t *ent)
{
}

void G_MapCycleAdjust(gentity_t *ent)
{
}

void G_MapCycleDel(gentity_t *ent)
{
}

void Cmd_MapCycle_f(gentity_t *ent)
{
	const char	*cmd;

	cmd = BG_Argv(1);

	if (!strcmp(cmd, "list")) {
		G_MapCycleList(ent);
	} else if (!strcmp(cmd, "add")) {
		G_MapCycleAdd(ent);
	} else if (!strcmp(cmd, "adjust")) {
		G_MapCycleAdjust(ent);
	} else if (!strcmp(cmd, "del")) {
		G_MapCycleDel(ent);
	} else {
		ClientPrint(ent, "Usage: mapcycle (list | add | adjust | del) [...]");
	}
}


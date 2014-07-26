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
			while (*(token = COM_Parse(&value))) {
				map->gametype |= 1 << atoi(token);
			}
		}

		mapCount++;
	}

	trap_FS_FCloseFile(fp);
}

static void G_MapCycleWrite(void)
{
	int				i;
	const int		pad = 10;
	fileHandle_t	fp;

	trap_FS_FOpenFile("mapcycle.ini", &fp, FS_WRITE);

	for (i = 0; i < mapCount; ++i) {
		Ini_WriteLabel("map", i == 0, fp);
		Ini_WriteString("name", maps[i].name, pad, fp);
		if (maps[i].minPlayers) {
			Ini_WriteNumber("minPlayers", maps[i].minPlayers, pad, fp);
		}
		if (maps[i].maxPlayers) {
			Ini_WriteNumber("maxPlayers", maps[i].minPlayers, pad, fp);
		}
	}

	trap_FS_Write("\n", 1, fp);
	trap_FS_FCloseFile(fp);
}

static void G_MapCycleList(gentity_t *ent)
{
	int	i;

	ClientPrint(ent, "Map cycle:");
	for (i = 0; i < MIN(20, mapCount); ++i) {
		ClientPrint(ent, "%s ", maps[i].name);
	}

	if (i < mapCount) {
		ClientPrint(ent, "...");
	}
}

static void G_MapCycleAdd(gentity_t *ent)
{
	if (mapCount == ARRAY_LEN(maps)) {
		ClientPrint(ent, "Too many maps.");
		return;
	}

	if (trap_Argc() != 3) {
		ClientPrint(ent, "Usage: mapcycle add <map>");
		return;
	}

	Q_strncpyz(maps[mapCount].name, BG_Argv(2), sizeof maps[0].name);
	mapCount++;
	G_MapCycleWrite();
}

static int G_MapId(gentity_t *ent, const char *id)
{
	int		i;

	for (i = 0; i < mapCount; ++i) {
		if (!Q_stricmp(id, maps[i].name)) {
			return i;
		}
	}

	ClientPrint(ent, "This map is not in the cycle.");
	return -1;
}

static void G_MapCycleAdjust(gentity_t *ent)
{
	int		mapid;
	char	field[MAX_TOKEN_CHARS], value[MAX_TOKEN_CHARS];

	if (trap_Argc() < 3) {
		ClientPrint(ent, "Usage: mapcycle adjust <map> (minplayers | maxplayers | gametype) <value>");
		return;
	}

	if ((mapid = G_MapId(ent, BG_Argv(1))) < 0) {
		return;
	}

	trap_Argv(3, field, sizeof field);
	trap_Argv(4, value, sizeof value);

	if (!strcmp(field, "minplayers") || !strcmp(field, "maxplayers")) {
		int	number;
		if (!Q_isanumber(value) || !Q_isintegral(atof(value))) {
			ClientPrint(ent, "Specify a number.");
			return;
		}
		number = atoi(value);
		if (number < 0 || number > MAX_CLIENTS) {
			ClientPrint(ent, "Specify a number from zero to %d.", MAX_CLIENTS);
			return;
		}
	}

	if (!strcmp(field, "minplayers")) {
		maps[mapid].minPlayers = atoi(value);
	} else if (!strcmp(field, "maxplayers")) {
		maps[mapid].maxPlayers = atoi(value);
	} else if (!strcmp(field, "gametype")) {
		// TODO
	} else {
		ClientPrint(ent, "Valid fields are: minplayers, maxplayers, gametype.");
		return;
	}

	G_MapCycleWrite();
}

void G_MapCycleDel(gentity_t *ent)
{
	int			i, k;
	const char	*map;

	if (trap_Argc() != 3) {
		ClientPrint(ent, "Usage: mapcycle del <map>");
		return;
	}

	map = BG_Argv(2);

	for (i = 0, k = 0; k < mapCount; ++i, ++k) {
		if (!Q_stricmp(map, maps[i].name)) {
			k++;
		}
		maps[i] = maps[k];
	}

	mapCount--;
	G_MapCycleWrite();
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

void G_MapCycleNextmap(void)
{
	if (mapCount == 0) {
		trap_Cvar_Set("nextmap", "map_restart");
		return;
	}
}

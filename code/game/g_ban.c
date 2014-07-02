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
// g_ban.c

#include "g_local.h"

typedef struct {
	char	name[MAX_NETNAME];	// name of banned player
	char	id[64];				// ip or guid
	char	reason[64];
	int		date;				// time when ban has started
	char	durationString[32];
	int		duration;			// in seconds
} ban_t;

static int		banCount;
static ban_t	bans[32];

/**
Convert a duration string to seconds.
*/
static long G_BanDurationToLong(const char *duration)
{
	int	sec = 0;
	int	total = 0;

	while (*duration) {
		if (isdigit(*duration)) {
			sec += 10 * sec + *duration - '0';
			continue;
		}

		switch (*duration) {
		case 'w': sec *= 7;
		case 'd': sec *= 24;
		case 'h': sec *= 60;
		case 'm': sec *= 60;
		case 's': break;
		default:  return -1;
		}

		total += sec;
		sec = 0;
	}

	return total;
}

void G_BanList(void)
{
	int	i;

	G_Printf("Bans:\n");
	G_Printf("nr    name          date        duration    reason        \n");
	G_Printf("----------------------------------------------------------\n");
	for (i = 0; i < banCount; ++i) {
//		G_Printf("#%d %s %s %s %s\n");
	}
}

void G_BanWrite(void)
{
	int				i;
	const int		pad = 10;
	fileHandle_t	fp;

	trap_FS_FOpenFile("ban.ini", &fp, FS_WRITE);

	for (i = 0; i < banCount; ++i) {
		// check if ban expired
		if (bans[i].date + bans[i].duration < trap_RealTime(NULL)) {
			continue;
		}

		Ini_WriteLabel("ban", i == 0, fp);
		Ini_WriteString("name", bans[i].name, pad, fp);
		Ini_WriteString("reason", bans[i].reason, pad, fp);
		Ini_WriteString("date", va("%d", bans[i].date), pad, fp);
		Ini_WriteString("duration", bans[i].durationString, pad, fp);
	}

	trap_FS_Write("\n", 1, fp);
	trap_FS_FCloseFile(fp);
}

/**
Remove all bans.
*/
void G_BanFlush(void)
{
	banCount = 0;
	G_BanWrite();
}


void G_BanKick(gclient_t *client, const char *duration, const char *reason)
{
	long	dur;

	dur = G_BanDurationToLong(duration);
	if (!dur) {
		ClientPrint(&g_entities[client - level.clients],
			"Luckily for you, a ban attempt has failed.");
		return;
	}

	if (banCount == ARRAY_LEN(bans) - 1) {
		G_Printf("Too many bans.\n");
		return;
	}

	Q_strncpyz(bans[banCount].name, client->pers.netname, sizeof bans[0].name);
	Q_strncpyz(bans[banCount].durationString, duration, sizeof bans[0].durationString);
	bans[banCount].duration = dur;
	bans[banCount].date = trap_RealTime(NULL);

	G_BanWrite();
	banCount++;

	trap_DropClient(client - level.clients, va("Banned for %s: %s\n", duration, reason));
}

void G_BanRead(void)
{
	fileHandle_t	fp;
	iniSection_t	section;
	ban_t			*ban;
	char			*value;

	banCount = 0;

	trap_FS_FOpenFile("ban.ini", &fp, FS_READ);
	if (!fp) {
		return;
	} else {
		G_LogPrintf("Reading ban file ...\n");
	}

	while (!trap_Ini_Section(&section, fp)) {
		if (strcmp(section.label, "ban")) {
			G_Printf("Ban file: Invalid section in ban.ini.\n");
			continue;
		}

		if (banCount >= ARRAY_LEN(bans) - 1) {
			G_Printf("Ban file: Too many bans.\n");
			break;
		}

		ban = &bans[banCount];

		value = Ini_GetValue(&section, "name");
		if (!value) {
			G_Printf("Ban file: No map name specified.\n");
			continue;
		}
		Q_strncpyz(bans->name, value, sizeof bans->name);

		ban++;
	}

	trap_FS_FCloseFile(fp);
}


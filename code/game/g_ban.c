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
	char		name[MAX_NETNAME];	// name of banned player
	char		id[64];				// ip or guid
	char		reason[64];
	int			date;				// time when ban has started
	char		durationString[32];
	int			duration;			// in seconds

	unsigned	mask;				// for ip ban
	unsigned	compare;
} ban_t;

static int		banCount;
static ban_t	bans[32];

/**
Convert a duration string to seconds.
*/
static int G_BanDuration(const char *duration)
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

static void G_BanList(gentity_t *ent)
{
	int	i;

	ClientPrint(ent, "Bans:\n");
	ClientPrint(ent, "nr    name          date        duration    reason        \n");
	ClientPrint(ent, "----------------------------------------------------------\n");
	for (i = 0; i < banCount; ++i) {
//		G_Printf("#%d %s %s %s %s\n");
	}
}

static void G_BanWrite(void)
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
static void G_BanFlush(gentity_t *ent)
{
	banCount = 0;
	G_BanWrite();
	ClientPrint(ent, "All bans have been removed.");
}

static void G_BanAdd(gentity_t *ent)
{
	int			duration;
	gclient_t	*client;

	if (trap_Argc() != 5) {
		ClientPrint(ent, "Usage: ban add <player or ip> <duration> <reason>");
		return;
	}

	client = ClientFromString(BG_Argv(2));
	if (!client) {
		ClientPrint(ent, "Player not found.");
		return;
	}

	duration = G_BanDuration(BG_Argv(3));
	if (!duration) {
		ClientPrint(&g_entities[client - level.clients],
			"Luckily for you, a ban attempt has failed.");
		return;
	}

	if (banCount == ARRAY_LEN(bans) - 1) {
		G_Printf("Too many bans.\n");
		return;
	}

	Q_strncpyz(bans[banCount].name, client->pers.netname, sizeof bans[0].name);
	Q_strncpyz(bans[banCount].durationString, BG_Argv(2), sizeof bans[0].durationString);
	bans[banCount].duration = duration;
	bans[banCount].date = trap_RealTime(NULL);

	G_BanWrite();
	banCount++;

	trap_DropClient(client - level.clients, va("Banned by %s for %s: %s\n",
		G_UserName(ent), BG_Argv(3), BG_Argv(4)));
}

static void G_BanAdjust(gentity_t *ent)
{
	int			banid;
	int			duration;
	const char	*reason;

	if (trap_Argc() != 4 && trap_Argc() != 5) {
		ClientPrint(ent, "Usage: ban adjust <banid> <duration> [reason]");
		return;
	}

	if (Q_isanumber(BG_Argv(1))) {
		ClientPrint(ent, "Invalid ban id.");
		return;
	}

	banid = atoi(BG_Argv(1));
	if (banid < 0 || banid > banCount) {
		ClientPrint(ent, "Invalid ban id.");
		return;
	}

	duration = G_BanDuration(BG_Argv(3));
	if (!duration) {
		ClientPrint(ent, "Invalid ban duration.");
		return;
	}

	reason = BG_Argv(5);
	if (*reason) {
		Q_strncpyz(bans[banid].reason, reason, sizeof bans[0].reason);
	}

	Q_strncpyz(bans[banid].durationString, BG_Argv(3), sizeof bans[0].duration);
	bans[banid].duration = duration;

	ClientPrint(ent, "The ban has been adjusted.");
}

static void G_BanDel(gentity_t *ent)
{
	int			i;
	int			banid;

	if (trap_Argc() != 3) {
		ClientPrint(ent, "Usage: ban del <id>");
		return;
	}

	if (!Q_isanumber(BG_Argv(2))) {
		ClientPrint(ent, "Invalid ban id.");
		return;
	}

	banid = atoi(BG_Argv(2));
	if (banid < 0) {
		ClientPrint(ent, "Invalid ban id.");
		return;
	}

	for (i = banid; i < banCount - 1; ++i) {
		bans[i] = bans[i + 1];
	}

	ClientPrint(ent, va("Ban #%d has been deleted.", banid));
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

void Cmd_Ban_f(gentity_t *ent)
{
	const char	*cmd;

	cmd = BG_Argv(1);

	if (!strcmp(cmd, "list")) {
		G_BanList(ent);
	} else if (!strcmp(cmd, "add")) {
		G_BanAdd(ent);
	} else if (!strcmp(cmd, "del")) {
		G_BanDel(ent);
	} else if (!strcmp(cmd, "adjust")) {
		G_BanAdjust(ent);
	} else if (!strcmp(cmd, "flush")) {
		G_BanFlush(ent);
	} else {
		ClientPrint(ent, "Usage: ban (list | add | del | adjust | flush) <banid>");
	}
}


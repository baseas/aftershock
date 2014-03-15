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
// g_stats.c

#include "g_local.h"

static void G_StatsList(char *buffer, int size, int *list, int listlen)
{
	int	i;
	buffer[0] = '\0';
	for (i = 0; i < listlen; ++i) {
		Q_strcat(buffer, size, va("%d", list[i]));
		if (i < listlen - 1) {
			Q_strcat(buffer, size, " ");
		}
	}
}

static void G_StatsPlayer(gclient_t *cl, qboolean disconnected, fileHandle_t fp)
{
	const int	pad = 13;
	char		buf[128];

	Ini_WriteLabel("player", qfalse, fp);
	Ini_WriteString("name", cl->pers.netname, pad, fp);

	if (disconnected) {
		Ini_WriteString("team", "disconnected", pad, fp);
	} else {
		switch (cl->sess.sessionTeam) {
		case TEAM_RED:
			Ini_WriteString("team", "red", pad, fp);
			break;
		case TEAM_BLUE:
			Ini_WriteString("team", "blue", pad, fp);
			break;
		case TEAM_FREE:
			Ini_WriteString("team", "free", pad, fp);
			break;
		case TEAM_SPECTATOR:
			Ini_WriteString("team", "spectator", pad, fp);
			break;
		default:
			break;
		}
	}

	Ini_WriteNumber("score", cl->pers.score, pad, fp);
	Ini_WriteNumber("damageDone", cl->pers.stats.miscStats[MSTAT_DAMAGE_DONE], pad, fp);
	Ini_WriteNumber("damageTaken", cl->pers.stats.miscStats[MSTAT_DAMAGE_TAKEN], pad, fp);
	if (g_entities[cl->ps.clientNum].r.svFlags & SVF_BOT) {
		Ini_WriteString("isBot", "true", pad, fp);
	} else {
		Ini_WriteString("isBot", "false", pad, fp);
	}

	G_StatsList(buf, sizeof buf, &cl->pers.stats.kills[WP_GAUNTLET], WP_NUM_WEAPONS - WP_GAUNTLET);
	Ini_WriteString("kills", buf, pad, fp);

	G_StatsList(buf, sizeof buf, &cl->pers.stats.deaths[WP_GAUNTLET], WP_NUM_WEAPONS - WP_GAUNTLET);
	Ini_WriteString("deaths", buf, pad, fp);

	G_StatsList(buf, sizeof buf, &cl->pers.stats.shots[WP_GAUNTLET], WP_NUM_WEAPONS - WP_GAUNTLET);
	Ini_WriteString("shots", buf, pad, fp);

	G_StatsList(buf, sizeof buf, &cl->pers.stats.teamHits[WP_GAUNTLET], WP_NUM_WEAPONS - WP_GAUNTLET);
	Ini_WriteString("teamHits", buf, pad, fp);

	G_StatsList(buf, sizeof buf, &cl->pers.stats.enemyHits[WP_GAUNTLET], WP_NUM_WEAPONS - WP_GAUNTLET);
	Ini_WriteString("enemyHits", buf, pad, fp);

	G_StatsList(buf, sizeof buf, cl->pers.stats.rewards, ARRAY_LEN(cl->pers.stats.rewards));
	Ini_WriteString("rewards", buf, pad, fp);
}

void G_StatsWrite(void)
{
	int				i;
	fileHandle_t	fp;
	char			datetime[64];
	char			mapname[64];
	qtime_t			now;
	const int		pad = 13;

	if (!g_writeStats.integer) {
		return;
	}

	trap_RealTime(&now);
	Com_sprintf(datetime, sizeof datetime, "%i-%02i-%02i %02i:%02i:%02i",
		1900 + now.tm_year, 1 + now.tm_mon, now.tm_mday, now.tm_hour, now.tm_min, now.tm_sec);
	trap_Cvar_VariableStringBuffer("mapname", mapname, sizeof mapname);

	trap_FS_FOpenFile(va("statistics/%s %s.ini", datetime, mapname), &fp, FS_WRITE);

	Ini_WriteLabel("match", qtrue, fp);

	Ini_WriteNumber("gametype", g_gametype.integer, pad, fp);
	Ini_WriteString("mapname", mapname, pad, fp);
	Ini_WriteNumber("duration", level.time, pad, fp);
	Ini_WriteString("datetime", datetime, pad, fp);
//	Ini_WriteString("demopath", "", pad, fp);

	for (i = 0; i < level.maxclients; ++i) {
		if (level.clients[i].pers.connected != CON_CONNECTED) {
			continue;
		}
		G_StatsPlayer(&level.clients[i], qfalse, fp);
	}

	for (i = 0; i < level.numDisconnectedClients; ++i) {
		G_StatsPlayer(&level.disconnectedClients[i], qtrue, fp);
	}

	trap_FS_Write("\n", 1, fp);
	trap_FS_FCloseFile(fp);
}


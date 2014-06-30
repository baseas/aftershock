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
// g_bot.c

#include "g_local.h"

static int		g_numBots;
static char		*g_botInfos[MAX_BOTS];

int				g_numArenas;
static char		*g_arenaInfos[MAX_ARENAS];

#define BOT_BEGIN_DELAY_BASE		2000
#define BOT_BEGIN_DELAY_INCREMENT	1500

#define BOT_SPAWN_QUEUE_DEPTH	16

vmCvar_t	bot_minplayers;

float trap_Cvar_VariableValue(const char *var_name)
{
	char buf[128];

	trap_Cvar_VariableStringBuffer(var_name, buf, sizeof(buf));
	return atof(buf);
}

int G_ParseInfos(char *buf, int max, char *infos[])
{
	char	*token;
	int		count;
	char	key[MAX_TOKEN_CHARS];
	char	info[MAX_INFO_STRING];

	count = 0;

	while (1) {
		token = COM_Parse(&buf);
		if (!token[0]) {
			break;
		}
		if (strcmp(token, "{")) {
			Com_Printf("Missing { in info file\n");
			break;
		}

		if (count == max) {
			Com_Printf("Max infos exceeded\n");
			break;
		}

		info[0] = '\0';
		while (1) {
			token = COM_ParseExt(&buf, qtrue);
			if (!token[0]) {
				Com_Printf("Unexpected end of info file\n");
				break;
			}
			if (!strcmp(token, "}")) {
				break;
			}
			Q_strncpyz(key, token, sizeof(key));

			token = COM_ParseExt(&buf, qfalse);
			if (!token[0]) {
				strcpy(token, "<NULL>");
			}
			Info_SetValueForKey(info, key, token);
		}
		//NOTE: extra space for arena number
		infos[count] = G_Alloc(strlen(info) + strlen("\\num\\") + strlen(va("%d", MAX_ARENAS)) + 1);
		if (infos[count]) {
			strcpy(infos[count], info);
			count++;
		}
	}
	return count;
}

static void G_LoadArenasFromFile(char *filename)
{
	int				len;
	fileHandle_t	f;
	char			buf[MAX_ARENAS_TEXT];

	len = trap_FS_FOpenFile(filename, &f, FS_READ);
	if (!f) {
		trap_Print(va(S_COLOR_RED "file not found: %s\n", filename));
		return;
	}
	if (len >= MAX_ARENAS_TEXT) {
		trap_FS_FCloseFile(f);
		trap_Print(va(S_COLOR_RED "file too large: %s is %i, max allowed is %i\n", filename, len, MAX_ARENAS_TEXT));
		return;
	}

	trap_FS_Read(buf, len, f);
	buf[len] = 0;
	trap_FS_FCloseFile(f);

	g_numArenas += G_ParseInfos(buf, MAX_ARENAS - g_numArenas, &g_arenaInfos[g_numArenas]);
}

static void G_LoadArenas(void)
{
	int			numdirs;
	char		filename[128];
	char		dirlist[1024];
	char*		dirptr;
	int			i, n;
	int			dirlen;

	g_numArenas = 0;

	// get all arenas from .arena files
	numdirs = trap_FS_GetFileList("scripts", ".arena", dirlist, 1024);
	dirptr  = dirlist;
	for (i = 0; i < numdirs; i++, dirptr += dirlen+1) {
		dirlen = strlen(dirptr);
		strcpy(filename, "scripts/");
		strcat(filename, dirptr);
		G_LoadArenasFromFile(filename);
	}

	for (n = 0; n < g_numArenas; n++) {
		Info_SetValueForKey(g_arenaInfos[n], "num", va("%i", n));
	}
}

const char *G_GetArenaInfoByMap(const char *map)
{
	int			n;

	for (n = 0; n < g_numArenas; n++) {
		if (Q_stricmp(Info_ValueForKey(g_arenaInfos[n], "map"), map) == 0) {
			return g_arenaInfos[n];
		}
	}

	return NULL;
}

void G_AddRandomBot(int team)
{
	int		i, n, num;
	float	skill;
	char	*value, netname[36], *teamstr;
	gclient_t	*cl;

	num = 0;
	for (n = 0; n < g_numBots; n++) {
		value = Info_ValueForKey(g_botInfos[n], "name");
		for (i = 0; i < g_maxclients.integer; i++) {
			cl = level.clients + i;
			if (cl->pers.connected != CON_CONNECTED) {
				continue;
			}
			if (!(g_entities[i].r.svFlags & SVF_BOT)) {
				continue;
			}
			if (team >= 0 && cl->sess.sessionTeam != team) {
				continue;
			}
			if (!Q_stricmp(value, cl->pers.netname)) {
				break;
			}
		}
		if (i >= g_maxclients.integer) {
			num++;
		}
	}
	num = random() * num;
	for (n = 0; n < g_numBots; n++) {
		value = Info_ValueForKey(g_botInfos[n], "name");
		for (i = 0; i < g_maxclients.integer; i++) {
			cl = level.clients + i;
			if (cl->pers.connected != CON_CONNECTED) {
				continue;
			}
			if (!(g_entities[i].r.svFlags & SVF_BOT)) {
				continue;
			}
			if (team >= 0 && cl->sess.sessionTeam != team) {
				continue;
			}
			if (!Q_stricmp(value, cl->pers.netname)) {
				break;
			}
		}
		if (i >= g_maxclients.integer) {
			num--;
			if (num <= 0) {
				skill = trap_Cvar_VariableValue("g_spSkill");
				if (team == TEAM_RED) teamstr = "red";
				else if (team == TEAM_BLUE) teamstr = "blue";
				else teamstr = "";
				Q_strncpyz(netname, value, sizeof(netname));
				Q_CleanStr(netname);
				trap_SendConsoleCommand(EXEC_INSERT, va("addbot %s %f %s\n", netname, skill, teamstr));
				return;
			}
		}
	}
}

int G_RemoveRandomBot(int team)
{
	int i;
	gclient_t	*cl;

	for (i = 0; i < g_maxclients.integer; i++) {
		cl = level.clients + i;
		if (cl->pers.connected != CON_CONNECTED) {
			continue;
		}
		if (!(g_entities[i].r.svFlags & SVF_BOT)) {
			continue;
		}
		if (team >= 0 && cl->sess.sessionTeam != team) {
			continue;
		}
		trap_SendConsoleCommand(EXEC_INSERT, va("clientkick %d\n", i));
		return qtrue;
	}
	return qfalse;
}

int G_CountHumanPlayers(int team)
{
	int			i, num;
	gclient_t	*cl;

	num = 0;
	for (i = 0; i < g_maxclients.integer; i++) {
		cl = level.clients + i;
		if (cl->pers.connected != CON_CONNECTED) {
			continue;
		}
		if (g_entities[i].r.svFlags & SVF_BOT) {
			continue;
		}
		if (team >= 0 && cl->sess.sessionTeam != team) {
			continue;
		}
		num++;
	}
	return num;
}

int G_CountBotPlayers(int team)
{
	int			i, num;
	gclient_t	*cl;

	num = 0;
	for (i = 0; i < g_maxclients.integer; i++) {
		cl = level.clients + i;
		if (cl->pers.connected != CON_CONNECTED) {
			continue;
		}
		if (!(g_entities[i].r.svFlags & SVF_BOT)) {
			continue;
		}
		if (team >= 0 && cl->sess.sessionTeam != team) {
			continue;
		}
		num++;
	}
	return num;
}

void G_CheckMinimumPlayers(void)
{
	int minplayers;
	int humanplayers, botplayers;
	static int checkminimumplayers_time;

	if (level.intermissiontime) return;
	//only check once each 10 seconds
	if (checkminimumplayers_time > level.time - 10000) {
		return;
	}
	checkminimumplayers_time = level.time;
	trap_Cvar_Update(&bot_minplayers);
	minplayers = bot_minplayers.integer;
	if (minplayers <= 0) return;

	if (g_gametype.integer >= GT_TEAM) {
		if (minplayers >= g_maxclients.integer / 2) {
			minplayers = (g_maxclients.integer / 2) -1;
		}

		humanplayers = G_CountHumanPlayers(TEAM_RED);
		botplayers = G_CountBotPlayers(	TEAM_RED);
		//
		if (humanplayers + botplayers < minplayers) {
			G_AddRandomBot(TEAM_RED);
		} else if (humanplayers + botplayers > minplayers && botplayers) {
			G_RemoveRandomBot(TEAM_RED);
		}
		//
		humanplayers = G_CountHumanPlayers(TEAM_BLUE);
		botplayers = G_CountBotPlayers(TEAM_BLUE);
		//
		if (humanplayers + botplayers < minplayers) {
			G_AddRandomBot(TEAM_BLUE);
		} else if (humanplayers + botplayers > minplayers && botplayers) {
			G_RemoveRandomBot(TEAM_BLUE);
		}
	}
	else if (g_gametype.integer == GT_TOURNAMENT) {
		if (minplayers >= g_maxclients.integer) {
			minplayers = g_maxclients.integer-1;
		}
		humanplayers = G_CountHumanPlayers(-1);
		botplayers = G_CountBotPlayers(-1);
		//
		if (humanplayers + botplayers < minplayers) {
			G_AddRandomBot(TEAM_FREE);
		} else if (humanplayers + botplayers > minplayers && botplayers) {
			// try to remove spectators first
			if (!G_RemoveRandomBot(TEAM_SPECTATOR)) {
				// just remove the bot that is playing
				G_RemoveRandomBot(-1);
			}
		}
	}
	else if (g_gametype.integer == GT_FFA) {
		if (minplayers >= g_maxclients.integer) {
			minplayers = g_maxclients.integer-1;
		}
		humanplayers = G_CountHumanPlayers(TEAM_FREE);
		botplayers = G_CountBotPlayers(TEAM_FREE);
		//
		if (humanplayers + botplayers < minplayers) {
			G_AddRandomBot(TEAM_FREE);
		} else if (humanplayers + botplayers > minplayers && botplayers) {
			G_RemoveRandomBot(TEAM_FREE);
		}
	}
}

qboolean G_BotConnect(int clientNum, qboolean restart)
{
	bot_settings_t	settings;
	char			userinfo[MAX_INFO_STRING];

	trap_GetUserinfo(clientNum, userinfo, sizeof(userinfo));

	Q_strncpyz(settings.characterfile, Info_ValueForKey(userinfo, "characterfile"), sizeof(settings.characterfile));
	settings.skill = atof(Info_ValueForKey(userinfo, "skill"));
	Q_strncpyz(settings.team, Info_ValueForKey(userinfo, "team"), sizeof(settings.team));

	if (!BotAISetupClient(clientNum, &settings, restart)) {
		trap_DropClient(clientNum, "BotAISetupClient failed");
		return qfalse;
	}

	return qtrue;
}

static void G_AddBot(const char *name, float skill, const char *team, char *altname)
{
	int				clientNum;
	char			*botinfo;
	char			*key;
	char			*s;
	char			*botname;
	char			userinfo[MAX_INFO_STRING];

	// have the server allocate a client slot
	clientNum = trap_BotAllocateClient();
	if (clientNum == -1) {
		G_Printf(S_COLOR_RED "Unable to add bot. All player slots are in use.\n");
		G_Printf(S_COLOR_RED "Start server with more 'open' slots (or check setting of sv_maxclients cvar).\n");
		return;
	}

	// get the botinfo from bots.txt
	botinfo = G_GetBotInfoByName(name);
	if (!botinfo) {
		G_Printf(S_COLOR_RED "Error: Bot '%s' not defined\n", name);
		trap_BotFreeClient(clientNum);
		return;
	}

	// create the bot's userinfo
	userinfo[0] = '\0';

	botname = Info_ValueForKey(botinfo, "funname");
	if (!botname[0]) {
		botname = Info_ValueForKey(botinfo, "name");
	}
	// check for an alternative name
	if (altname && altname[0]) {
		botname = altname;
	}
	Info_SetValueForKey(userinfo, "name", botname);
	Info_SetValueForKey(userinfo, "rate", "25000");
	Info_SetValueForKey(userinfo, "snaps", "40");
	Info_SetValueForKey(userinfo, "skill", va("%.2f", skill));

	key = "gender";
	s = Info_ValueForKey(botinfo, key);
	if (!*s) {
		s = "male";
	}
	Info_SetValueForKey(userinfo, "sex", s);

	s = Info_ValueForKey(botinfo, "aifile");
	if (!*s) {
		trap_Print(S_COLOR_RED "Error: bot has no aifile specified\n");
		trap_BotFreeClient(clientNum);
		return;
	}
	Info_SetValueForKey(userinfo, "characterfile", s);

	if (!team || !*team) {
		if (g_gametype.integer >= GT_TEAM) {
			if (PickTeam(clientNum) == TEAM_RED) {
				team = "red";
			}
			else {
				team = "blue";
			}
		}
		else {
			team = "red";
		}
	}
	Info_SetValueForKey(userinfo, "team", team);

	// register the userinfo
	trap_SetUserinfo(clientNum, userinfo);

	// have it connect to the game as a normal client
	if (ClientConnect(clientNum, qtrue, qtrue)) {
		return;
	}

	ClientBegin(clientNum);
	return;
}

void Svcmd_AddBot_f(void)
{
	float			skill;
	char			name[MAX_TOKEN_CHARS];
	char			altname[MAX_TOKEN_CHARS];
	char			string[MAX_TOKEN_CHARS];
	char			team[MAX_TOKEN_CHARS];

	// are bots enabled?
	if (!trap_Cvar_VariableIntegerValue("bot_enable")) {
		trap_Print("Bots are not enabled.\n");
		return;
	}

	if (g_gametype.integer == GT_DEFRAG) {
		trap_Print("Bots cannot play DeFRaG.\n");
		return;
	}

	// name
	trap_Argv(1, name, sizeof(name));
	if (!name[0]) {
		trap_Print("Usage: addbot <botname> [skill 1-5] [team] [altname]\n");
		return;
	}

	// skill
	trap_Argv(2, string, sizeof(string));
	if (!string[0]) {
		skill = 4;
	}
	else {
		skill = atof(string);
	}

	// team
	trap_Argv(3, team, sizeof(team));

	// alternative name
	trap_Argv(5, altname, sizeof(altname));

	G_AddBot(name, skill, team, altname);
}

void Svcmd_BotList_f(void)
{
	int i;
	char name[MAX_TOKEN_CHARS];
	char funname[MAX_TOKEN_CHARS];
	char aifile[MAX_TOKEN_CHARS];

	trap_Print("^1name            aifile              funname\n");
	for (i = 0; i < g_numBots; i++) {
		strcpy(name, Info_ValueForKey(g_botInfos[i], "name"));
		if (!*name) {
			strcpy(name, "UnnamedPlayer");
		}
		strcpy(funname, Info_ValueForKey(g_botInfos[i], "funname"));
		if (!*funname) {
			strcpy(funname, "");
		}
		strcpy(aifile, Info_ValueForKey(g_botInfos[i], "aifile"));
		if (!*aifile) {
			strcpy(aifile, "bots/default_c.c");
		}
		trap_Print(va("%-16s %-20s %-20s\n", name, aifile, funname));
	}
}

static void G_LoadBotsFromFile(char *filename)
{
	int				len;
	fileHandle_t	f;
	char			buf[MAX_BOTS_TEXT];

	len = trap_FS_FOpenFile(filename, &f, FS_READ);
	if (!f) {
		trap_Print(va(S_COLOR_RED "file not found: %s\n", filename));
		return;
	}
	if (len >= MAX_BOTS_TEXT) {
		trap_Print(va(S_COLOR_RED "file too large: %s is %i, max allowed is %i\n", filename, len, MAX_BOTS_TEXT));
		trap_FS_FCloseFile(f);
		return;
	}

	trap_FS_Read(buf, len, f);
	buf[len] = 0;
	trap_FS_FCloseFile(f);

	g_numBots += G_ParseInfos(buf, MAX_BOTS - g_numBots, &g_botInfos[g_numBots]);
}

static void G_LoadBots(void)
{
	vmCvar_t	botsFile;
	int			numdirs;
	char		filename[128];
	char		dirlist[1024];
	char*		dirptr;
	int			i;
	int			dirlen;

	if (!trap_Cvar_VariableIntegerValue("bot_enable")) {
		return;
	}

	g_numBots = 0;

	trap_Cvar_Register(&botsFile, "g_botsFile", "", CVAR_INIT|CVAR_ROM);
	if (*botsFile.string) {
		G_LoadBotsFromFile(botsFile.string);
	}
	else {
		G_LoadBotsFromFile("scripts/bots.txt");
	}

	// get all bots from .bot files
	numdirs = trap_FS_GetFileList("scripts", ".bot", dirlist, 1024);
	dirptr  = dirlist;
	for (i = 0; i < numdirs; i++, dirptr += dirlen+1) {
		dirlen = strlen(dirptr);
		strcpy(filename, "scripts/");
		strcat(filename, dirptr);
		G_LoadBotsFromFile(filename);
	}
}

char *G_GetBotInfoByNumber(int num)
{
	if (num < 0 || num >= g_numBots) {
		trap_Print(va(S_COLOR_RED "Invalid bot number: %i\n", num));
		return NULL;
	}
	return g_botInfos[num];
}

char *G_GetBotInfoByName(const char *name)
{
	int		n;
	char	*value;

	for (n = 0; n < g_numBots; n++) {
		value = Info_ValueForKey(g_botInfos[n], "name");
		if (!Q_stricmp(value, name)) {
			return g_botInfos[n];
		}
	}

	return NULL;
}

void G_InitBots(qboolean restart)
{
	G_LoadBots();
	G_LoadArenas();

	trap_Cvar_Register(&bot_minplayers, "bot_minplayers", "0", CVAR_SERVERINFO);
}


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
// g_vote.c

#include "g_local.h"

typedef struct {
	char	name[32];
	char	command[64];
	char	display[32];
} customVote_t;

static int			customVoteCount;
static customVote_t	customVotes[32];

static qboolean AllowedVote(gentity_t *ent, const char *cmd)
{
	return qtrue;
}

static int Vote_Gametype(gentity_t *ent)
{
	int gt;
	const char *str;
	const char *gameNames[] = {
		"Free For All",
		"Duel",
		"Defrag",
		"Team Deathmatch",
		"Capture the Flag",
		"Elimination"
	};

	str = BG_Argv(2);
	if (!str[0]) {
		ClientPrint(ent, "Invalid gametype.");
		return 1;
	}

	if (!Q_stricmp(str, "ffa")) {
		gt = 0;
	} else if (!Q_stricmp(str, "duel")) {
		gt = 1;
	} else if (!Q_stricmp(str, "defrag")) {
		gt = 2;
	} else if (!Q_stricmp(str, "tdm")) {
		gt = 3;
	} else if (!Q_stricmp(str, "ctf")) {
		gt = 4;
	} else if (!Q_stricmp(str, "ca") || !Q_stricmp(str, "elimination")) {
		gt = 5;
	} else if (Q_isanumber(str)) {
		gt = atoi(BG_Argv(1));
	} else {
		gt = -1;
	}

	if (gt < 0 || gt >= GT_MAX_GAME_TYPE) {
		ClientPrint(ent, "Invalid gametype.");
		return 1;
	}

	Com_sprintf(level.voteString, sizeof level.voteString, "g_gametype %d; map_restart", gt);
	Com_sprintf(level.voteDisplay, sizeof level.voteDisplay, "Gametype %s?", gameNames[gt]);
	return 0;
}

static int Vote_MapRestart(gentity_t *ent)
{
	Com_sprintf(level.voteString, sizeof level.voteString, "map_restart");
	Com_sprintf(level.voteDisplay, sizeof level.voteDisplay, "Restart map?");
	return 0;
}

static int Vote_Map(gentity_t *ent)
{
	// special case for map changes, we want to reset the nextmap setting
	// this allows a player to change maps, but not upset the map rotation
	char		nextmap[MAX_STRING_CHARS];
	const char	*map;

	map = BG_Argv(2);

	if (!trap_FS_FOpenFile(va("maps/%s.bsp", map), NULL, FS_READ)) {
		ClientPrint(ent, "Invalid map name.");
		return 1;
	}

	trap_Cvar_VariableStringBuffer("nextmap", nextmap, sizeof nextmap);
	if (*nextmap) {
		Com_sprintf(level.voteString, sizeof level.voteString, "map %s; set nextmap \"%s\"",
			BG_Argv(2), nextmap);
	} else {
		Com_sprintf(level.voteString, sizeof level.voteString, "map %s", map);
	}
	Com_sprintf(level.voteDisplay, sizeof level.voteDisplay, "Next map %s", map);
	return 0;
}

static int Vote_Nextmap(gentity_t *ent)
{
	char	s[MAX_STRING_CHARS];

	trap_Cvar_VariableStringBuffer("nextmap", s, sizeof(s));
	if (!*s) {
		ClientPrint(ent, "nextmap not set.");
		return 1;
	}
	Com_sprintf(level.voteString, sizeof level.voteString, "vstr nextmap");
	Com_sprintf(level.voteDisplay, sizeof level.voteDisplay, "Next map?");
	return 0;
}

static int Vote_Kick(gentity_t *ent)
{
	gclient_t	*cl;

	cl = ClientFromString(BG_Argv(2));
	if (!cl) {
		ClientPrint(ent, "Player not found.");
		return 1;
	}

	Com_sprintf(level.voteString, sizeof level.voteString, "kick %ld", cl - level.clients);
	Com_sprintf(level.voteDisplay, sizeof level.voteDisplay, "Kick %s^7?", cl->pers.netname);
	return 0;
}

static int Vote_Shuffle(gentity_t *ent)
{
	if (g_gametype.integer < GT_TEAM) {
		ClientPrint(ent, "Can only be used in team games.");
		return 1;
	}
	Com_sprintf(level.voteString, sizeof level.voteString, "shuffle");
	Com_sprintf(level.voteDisplay, sizeof level.voteDisplay, "Shuffle teams?");
	return 0;
}

static int Vote_ForceTeam(gentity_t *ent)
{
	gclient_t	*cl;
	const char	*team;

	cl = ClientFromString(BG_Argv(2));
	if (!cl) {
		ClientPrint(ent, "Player not found.");
		return 1;
	}

	team = BG_Argv(3);

	if (!team[0]) {
		ClientPrint(ent, "Invalid team.");
		return 1;
	}

	if (!Q_stricmp(team, "speconly") || !Q_stricmp(team, "so")) {
		team = "speconly";
	} else if (!Q_stricmp(team, "spectator") || !Q_stricmp(team, "s")) {
		team = "spectator";
	} else if (!Q_stricmp(team, "red") || !Q_stricmp(team, "r")) {
		team = "red";
	} else if (!Q_stricmp(team, "blue") || !Q_stricmp(team, "b")) {
		team = "blue";
	} else if (!Q_stricmp(team, "free") || !Q_stricmp(team, "f")) {
		team = "free";
	} else {
		ClientPrint(ent, "Invalid team.");
		return 1;
	}

	Com_sprintf(level.voteString, sizeof level.voteString, "forceteam %ld %s",
		cl - level.clients, team);
	Com_sprintf(level.voteDisplay, sizeof level.voteDisplay,
		"Force %s ^7to team %s?", cl->pers.netname, team);
	return 0;
}

static int Vote_Mute(gentity_t *ent)
{
	gclient_t	*cl;

	cl = ClientFromString(BG_Argv(2));
	if (!cl) {
		ClientPrint(ent, "Player not found.");
		return 1;
	}

	Com_sprintf(level.voteString, sizeof level.voteString, "mute %ld", cl - level.clients);
	Com_sprintf(level.voteDisplay, sizeof level.voteDisplay, "Mute %s^7?", cl->pers.netname);
	return 0;
}

static int Vote_Custom(gentity_t *ent, customVote_t *vote)
{
	Q_strncpyz(level.voteString, vote->command, sizeof level.voteString);

	if (*vote->display) {
		Q_strncpyz(level.voteDisplay, vote->display, sizeof level.voteDisplay);
	} else {
		Q_strncpyz(level.voteDisplay, vote->command, sizeof level.voteDisplay);
	}
	return 0;
}

struct {
	const char	*cmd;
	int			(*func)(gentity_t *ent);
} voteCommands[] = {
	{ "map_restart", Vote_MapRestart },
	{ "nextmap", Vote_Nextmap },
	{ "map", Vote_Map },
	{ "gametype", Vote_Gametype },
	{ "kick", Vote_Kick },
	{ "shuffle", Vote_Shuffle },
	{ "forceteam", Vote_ForceTeam },
	{ "mute", Vote_Mute },
	{ NULL, 0 }
};

static void G_VotePrintCommands(gentity_t *ent)
{
	int		i;
	char	buffer[512];

	strcpy(buffer, "Vote commands are: ");
	for (i = 0; voteCommands[i].cmd; ++i) {
		Q_strcat(buffer, sizeof buffer, voteCommands[i].cmd);
		Q_strcat(buffer, sizeof buffer, ", ");
	}

	buffer[strlen(buffer) - 2] = '\0';

	if (customVoteCount > 0) {
		strcat(buffer, "\nCustom votes are: ");
		for (i = 0; i < customVoteCount; ++i) {
			Q_strcat(buffer, sizeof buffer, customVotes[i].name);
			Q_strcat(buffer, sizeof buffer, ", ");
		}
	}

	buffer[strlen(buffer) - 2] = '\0';
	ClientPrint(ent, "%s", buffer);
}

void G_VoteRead(void)
{
	fileHandle_t	fp;
	iniSection_t	section;
	customVote_t	*vote;
	const char		*value;

	customVoteCount = 0;

	trap_FS_FOpenFile("vote.ini", &fp, FS_READ);
	if (!fp) {
		return;
	} else {
		G_LogPrintf("Reading custom votes ...\n");
	}

	while (!trap_Ini_Section(&section, fp)) {
		if (strcmp(section.label, "vote")) {
			G_Printf("Custom vote: Invalid section in vote.ini.\n");
			continue;
		}

		if (customVoteCount >= ARRAY_LEN(customVotes) - 1) {
			G_Printf("Custom vote: Too many custom votes.\n");
			break;
		}

		vote = &customVotes[customVoteCount];

		value = Ini_GetValue(&section, "name");
		if (!value) {
			G_Printf("Custom vote: No vote name specified.\n");
			continue;
		}
		Q_strncpyz(vote->name, value, sizeof vote->name);

		value = Ini_GetValue(&section, "cmd");
		if (!value) {
			G_Printf("Custom vote: No command specified for vote '%s'\n", vote->name);
			continue;
		}
		Q_strncpyz(vote->command, value, sizeof vote->command);

		value = Ini_GetValue(&section, "display");
		if (value) {
			Q_strncpyz(vote->display, value, sizeof vote->display);
		} else {
			Q_strncpyz(vote->display, vote->command, sizeof vote->display);
		}

		++customVoteCount;
	}

	trap_FS_FCloseFile(fp);
}

int G_VoteCall(gentity_t *ent)
{
	int			i;
	const char	*cmd;

	cmd = BG_Argv(1);
	if (!*cmd) {
		G_VotePrintCommands(ent);
		return 1;
	}

	if (strchr(cmd, ';') || strchr(cmd, '\n') || strchr(cmd, '\r')) {
		ClientPrint(ent, "Invalid vote string.");
		return 1;
	}

	if (!AllowedVote(ent, cmd)) {
		ClientPrint(ent, "This vote is not allowed.");
		return 1;
	}

	for (i = 0; voteCommands[i].cmd; ++i) {
		if (!Q_stricmp(cmd, voteCommands[i].cmd)) {
			if (!G_UserAllowed(ent, va("callvote_%s", voteCommands[i].cmd))) {
				ClientPrint(ent, "You are not allowed to call this vote.");
				return 1;
			}
			return voteCommands[i].func(ent);
		}
	}

	for (i = 0; i < customVoteCount; ++i) {
		if (!Q_stricmp(cmd, customVotes[i].name)) {
			if (!G_UserAllowed(ent, va("callvote_%s", customVotes[i].name))) {
				ClientPrint(ent, "You are not allowed to call this vote.");
				return 1;
			}
			return Vote_Custom(ent, &customVotes[i]);
		}
	}

	G_VotePrintCommands(ent);

	return 1;
}

void G_VoteCheck(void)
{
	if (level.voteExecuteTime && level.voteExecuteTime < level.time) {
		level.voteExecuteTime = 0;
		trap_SendConsoleCommand(EXEC_APPEND, va("%s\n", level.voteString));
	}

	if (!level.voteTime) {
		return;
	}

	if (level.time - level.voteTime >= VOTE_TIME) {
		if (level.voteYes > level.voteNo * 2) {
			// let pass if there was at least twice as many for as against
			trap_SetConfigstring(CS_VOTE_TIME, "passed");
			G_LogPrintf("Vote passed. At least 2 of 3 voted yes.\n");
			level.voteExecuteTime = level.time + 3000;
		} else if (level.voteYes > level.voteNo && level.voteYes >= 2 && (level.voteYes*10)>=(level.numVotingClients*3)) {
			// let pass if there is more yes than no and at least 2 yes votes and at
			// least 30% yes of all on the server
			trap_SetConfigstring(CS_VOTE_TIME, "passed");
			G_LogPrintf("Vote passed.\n");
			level.voteExecuteTime = level.time + 3000;
		} else {
			trap_SetConfigstring(CS_VOTE_TIME, "failed");
			G_LogPrintf("Vote failed.\n");
		}
	} else {
		// ATVI Q3 1.32 Patch #9, WNF
		if (2 * level.voteYes > level.numVotingClients) {
			// execute the command, then remove the vote
			trap_SetConfigstring(CS_VOTE_TIME, "passed");
			G_LogPrintf("Vote passed.\n");
			level.voteExecuteTime = level.time + 3000;
		} else if (2 * level.voteNo >= level.numVotingClients) {
			// same behavior as a timeout
			trap_SetConfigstring(CS_VOTE_TIME, "failed");
			G_LogPrintf("Vote failed.\n");
		} else {
			// still waiting for a majority
			return;
		}
	}

	level.voteTime = 0;
}

/**
Iterates through all the clients and counts the votes
*/
void G_VoteUpdateCount(void)
{
	int i;
	int yes, no;

	yes = 0;
	no = 0;
	level.numVotingClients = 0;

	for (i = 0; i < level.maxclients; i++) {
		if (level.clients[i].pers.connected != CON_CONNECTED) {
			continue;
		}

		if (level.clients[i].sess.sessionTeam == TEAM_SPECTATOR) {
			continue; // don't count spectators
		}

		if (g_entities[i].r.svFlags & SVF_BOT) {
			continue;
		}

		// the client can vote
		level.numVotingClients++;

		// did the client vote yes?
		if (level.clients[i].pers.vote > 0) {
			yes++;
		}

		// did the client vote no?
		if (level.clients[i].pers.vote < 0) {
			no++;
		}
	}

	if (level.voteYes != yes) {
		level.voteYes = yes;
		trap_SetConfigstring(CS_VOTE_YES, va("%i", level.voteYes));
	}

	if (level.voteNo != no) {
		level.voteNo = no;
		trap_SetConfigstring(CS_VOTE_NO, va("%i", level.voteNo));
	}
}


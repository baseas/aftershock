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
// g_main.c

#include "g_local.h"

#define PING_UPDATE_INTERVAL	2000
#define SCOREBOARD_INTERVAL		2000
#define STATS_INTERVAL			2000

typedef struct {
	vmCvar_t	*vmCvar;
	char		*cvarName;
	char		*defaultString;
	int			cvarFlags;
	int			modificationCount;	// for tracking changes
	qboolean	trackChange;		// track this variable, and announce if changed
	qboolean 	teamShader;			// track and if changed, update shader state
} cvarTable_t;

svPlayerState_t	svps[MAX_CLIENTS];
level_locals_t	level;
gentity_t		g_entities[MAX_GENTITIES];

vmCvar_t	g_gametype;
vmCvar_t	g_dmflags;
vmCvar_t	g_fraglimit;
vmCvar_t	g_timelimit;
vmCvar_t	g_capturelimit;
vmCvar_t	g_friendlyFire;
vmCvar_t	g_password;
vmCvar_t	g_needpass;
vmCvar_t	g_maxclients;
vmCvar_t	g_maxGameClients;
vmCvar_t	g_dedicated;
vmCvar_t	g_speed;
vmCvar_t	g_gravity;
vmCvar_t	g_cheats;
vmCvar_t	g_knockback;
vmCvar_t	g_quadfactor;
vmCvar_t	g_inactivity;
vmCvar_t	g_debugMove;
vmCvar_t	g_debugDamage;
vmCvar_t	g_debugAlloc;
vmCvar_t	g_weaponRespawn;
vmCvar_t	g_weaponTeamRespawn;
vmCvar_t	g_motd;
vmCvar_t	g_warmupTime;
vmCvar_t	g_restarted;
vmCvar_t	g_logfile;
vmCvar_t	g_logfileSync;
vmCvar_t	g_allowVote;
vmCvar_t	g_teamAutoJoin;
vmCvar_t	g_teamForceBalance;
vmCvar_t	g_banIPs;
vmCvar_t	g_filterBan;
vmCvar_t	g_smoothClients;
vmCvar_t	pmove_fixed;
vmCvar_t	pmove_msec;
vmCvar_t	g_listEntity;
vmCvar_t	g_newItemHeight;
vmCvar_t	g_instantgib;
vmCvar_t	g_instantgibGauntlet;
vmCvar_t	g_instantgibRailjump;
vmCvar_t	g_rockets;
vmCvar_t	g_selfDamage;
vmCvar_t	g_itemDrop;
vmCvar_t	g_startWhenReady;
vmCvar_t	g_autoReady;
vmCvar_t	g_overtime;
vmCvar_t	g_friendsThroughWalls;
vmCvar_t	g_teamLock;
vmCvar_t	g_redLocked;
vmCvar_t	g_blueLocked;
vmCvar_t	g_writeStats;
vmCvar_t	g_roundWarmup;
vmCvar_t	g_roundTimelimit;
vmCvar_t	g_allowRespawnTimer;
vmCvar_t	sv_fps;

static	cvarTable_t		gameCvarTable[] = {
	// don't override the cheat state set by the system
	{ &g_cheats, "sv_cheats", "", 0, 0, qfalse },

	// noset vars
	{ NULL, "gamename", GAMEVERSION , CVAR_SERVERINFO | CVAR_ROM, 0, qfalse  },
	{ NULL, "gamedate", __DATE__ , CVAR_ROM, 0, qfalse  },
	{ &g_restarted, "g_restarted", "0", CVAR_ROM, 0, qfalse  },

	// latched vars
	{ &g_gametype, "g_gametype", "0", CVAR_SERVERINFO | CVAR_ARCHIVE | CVAR_USERINFO | CVAR_LATCH, 0, qfalse  },
	{ &g_maxclients, "sv_maxclients", "8", CVAR_SERVERINFO | CVAR_LATCH | CVAR_ARCHIVE, 0, qfalse  },
	{ &g_maxGameClients, "g_maxGameClients", "0", CVAR_SERVERINFO | CVAR_LATCH | CVAR_ARCHIVE, 0, qfalse  },

	// change anytime vars
	{ &g_dmflags, "dmflags", "0", CVAR_SERVERINFO | CVAR_ARCHIVE, 0, qtrue  },
	{ &g_fraglimit, "g_fraglimit", "20", CVAR_SERVERINFO | CVAR_ARCHIVE | CVAR_NORESTART, 0, qtrue },
	{ &g_timelimit, "g_timelimit", "0", CVAR_SERVERINFO | CVAR_ARCHIVE | CVAR_NORESTART, 0, qtrue },
	{ &g_capturelimit, "g_capturelimit", "8", CVAR_SERVERINFO | CVAR_ARCHIVE | CVAR_NORESTART, 0, qtrue },

	{ &g_friendlyFire, "g_friendlyFire", "0", CVAR_ARCHIVE, 0, qtrue  },
	{ &g_teamAutoJoin, "g_teamAutoJoin", "0", CVAR_ARCHIVE  },
	{ &g_teamForceBalance, "g_teamForceBalance", "0", CVAR_ARCHIVE  },

	{ &g_warmupTime, "g_warmupTime", "20", CVAR_ARCHIVE, 0, qtrue  },
	{ &g_logfile, "g_logfile", "games.log", CVAR_ARCHIVE, 0, qfalse  },
	{ &g_logfileSync, "g_logsync", "0", CVAR_ARCHIVE, 0, qfalse  },

	{ &g_password, "g_password", "", CVAR_USERINFO, 0, qfalse  },
	{ &g_banIPs, "g_banIPs", "", CVAR_ARCHIVE, 0, qfalse  },
	{ &g_filterBan, "g_filterBan", "1", CVAR_ARCHIVE, 0, qfalse  },
	{ &g_needpass, "g_needpass", "0", CVAR_SERVERINFO | CVAR_ROM, 0, qfalse },
	{ &g_dedicated, "dedicated", "0", 0, 0, qfalse  },

	{ &g_speed, "g_speed", "320", 0, 0, qtrue  },
	{ &g_gravity, "g_gravity", "800", 0, 0, qtrue  },
	{ &g_knockback, "g_knockback", "1000", 0, 0, qtrue  },
	{ &g_quadfactor, "g_quadfactor", "3", 0, 0, qtrue  },
	{ &g_weaponRespawn, "g_weaponrespawn", "5", 0, 0, qtrue  },
	{ &g_weaponTeamRespawn, "g_weaponTeamRespawn", "30", 0, 0, qtrue },
	{ &g_inactivity, "g_inactivity", "0", 0, 0, qtrue },
	{ &g_debugMove, "g_debugMove", "0", 0, 0, qfalse },
	{ &g_debugDamage, "g_debugDamage", "0", 0, 0, qfalse },
	{ &g_debugAlloc, "g_debugAlloc", "0", 0, 0, qfalse },
	{ &g_motd, "g_motd", "", 0, 0, qfalse },

	{ &g_allowVote, "g_allowVote", "1", CVAR_ARCHIVE, 0, qfalse },
	{ &g_listEntity, "g_listEntity", "0", 0, 0, qfalse },

	{ &g_smoothClients, "g_smoothClients", "1", 0, 0, qfalse},
	{ &pmove_fixed, "pmove_fixed", "1", CVAR_SYSTEMINFO, 0, qfalse},
	{ &pmove_msec, "pmove_msec", "8", CVAR_SYSTEMINFO, 0, qfalse},

	{ &g_newItemHeight, "g_newItemHeight", "1", CVAR_SERVERINFO | CVAR_ARCHIVE, 0, qfalse },
	{ &g_instantgib, "g_instantgib", "0", CVAR_LATCH, 0, qfalse },
	{ &g_instantgibGauntlet, "g_instantgibGauntlet", "1", CVAR_LATCH, 0, qfalse },
	{ &g_instantgibRailjump, "g_instantgibRailjump", "1", 0, 0, qfalse },
	{ &g_rockets, "g_rockets", "0", CVAR_LATCH, 0, qfalse },
	{ &g_selfDamage, "g_selfDamage", "1", 0, 0, qfalse },
	{ &g_itemDrop, "g_itemDrop", "1", CVAR_SERVERINFO | CVAR_ARCHIVE, 0, qfalse },
	{ &g_startWhenReady, "g_startWhenReady", "1", CVAR_SERVERINFO | CVAR_ARCHIVE, 0, qfalse },
	{ &g_autoReady, "g_autoReady", "0", CVAR_ARCHIVE, 0, qfalse },
	{ &g_overtime, "g_overtime", "120", CVAR_SERVERINFO | CVAR_ARCHIVE, 0, qfalse },
	{ &g_friendsThroughWalls, "g_friendsThroughWalls", "0", CVAR_SERVERINFO | CVAR_ARCHIVE, 0, qfalse },
	{ &g_teamLock, "g_teamLock", "0", CVAR_ARCHIVE | CVAR_SERVERINFO | CVAR_NORESTART, 0, qfalse },
	{ &g_redLocked, "g_redLocked", "0", CVAR_SERVERINFO | CVAR_NORESTART, 0, qfalse },
	{ &g_blueLocked, "g_blueLocked", "0", CVAR_SERVERINFO | CVAR_NORESTART, 0, qfalse },
	{ &g_writeStats, "g_writeStats", "1", CVAR_ARCHIVE, 0, qfalse },
	{ &g_roundWarmup, "g_roundWarmup", "7", CVAR_ARCHIVE, 0, qfalse },
	{ &g_roundTimelimit, "g_roundTimelimit", "300", CVAR_ARCHIVE, 0, qfalse },
	{ &g_allowRespawnTimer, "g_allowRespawnTimer", "1", CVAR_ARCHIVE, 0, qfalse },
	{ &sv_fps, "sv_fps", "40", CVAR_SYSTEMINFO | CVAR_SERVERINFO, 0, qfalse }
};

static	int		gameCvarTableSize = ARRAY_LEN(gameCvarTable);

void	G_InitGame(int levelTime, int randomSeed, int restart);
void	G_RunFrame(int levelTime);
void	G_ShutdownGame(int restart);
void	CheckExitRules(void);

/**
This is the only way control passes into the module.
This must be the very first function compiled into the .q3vm file
*/
Q_EXPORT intptr_t vmMain(int command, int arg0, int arg1, int arg2, int arg3, int arg4,
	int arg5, int arg6, int arg7, int arg8, int arg9, int arg10, int arg11)
{
	switch (command) {
	case GAME_INIT:
		G_InitGame(arg0, arg1, arg2);
		return 0;
	case GAME_SHUTDOWN:
		G_ShutdownGame(arg0);
		return 0;
	case GAME_CLIENT_CONNECT:
		return (intptr_t)ClientConnect(arg0, arg1, arg2);
	case GAME_CLIENT_THINK:
		ClientThink(arg0);
		return 0;
	case GAME_CLIENT_USERINFO_CHANGED:
		ClientUserinfoChanged(arg0);
		return 0;
	case GAME_CLIENT_DISCONNECT:
		ClientDisconnect(arg0);
		return 0;
	case GAME_CLIENT_BEGIN:
		ClientBegin(arg0);
		return 0;
	case GAME_CLIENT_COMMAND:
		ClientCommand(arg0);
		return 0;
	case GAME_RUN_FRAME:
		G_RunFrame(arg0);
		return 0;
	case GAME_CONSOLE_COMMAND:
		return ConsoleCommand();
	case BOTAI_START_FRAME:
		return BotAIStartFrame(arg0);
	}

	return -1;
}

void QDECL G_Printf(const char *fmt, ...)
{
	va_list		argptr;
	char		text[1024];

	va_start (argptr, fmt);
	Q_vsnprintf(text, sizeof(text), fmt, argptr);
	va_end (argptr);

	trap_Print(text);
}

void QDECL G_Error(const char *fmt, ...)
{
	va_list		argptr;
	char		text[1024];

	va_start (argptr, fmt);
	Q_vsnprintf(text, sizeof(text), fmt, argptr);
	va_end (argptr);

	trap_Error(text);
}

/**
Chain together all entities with a matching team field.
Entity teams are used for item groups and multi-entity mover groups.

All but the first will have the FL_TEAMSLAVE flag set and teammaster field set
All but the last will have the teamchain field set to the next one
*/
void G_FindTeams(void)
{
	gentity_t	*e, *e2;
	int		i, j;
	int		c, c2;

	c = 0;
	c2 = 0;
	for (i=1, e=g_entities+i; i < level.num_entities; i++,e++) {
		if (!e->inuse)
			continue;
		if (!e->team)
			continue;
		if (e->flags & FL_TEAMSLAVE)
			continue;
		e->teammaster = e;
		c++;
		c2++;
		for (j=i+1, e2=e+1; j < level.num_entities; j++,e2++) {
			if (!e2->inuse)
				continue;
			if (!e2->team)
				continue;
			if (e2->flags & FL_TEAMSLAVE)
				continue;
			if (!strcmp(e->team, e2->team)) {
				c2++;
				e2->teamchain = e->teamchain;
				e->teamchain = e2;
				e2->teammaster = e;
				e2->flags |= FL_TEAMSLAVE;

				// make sure that targets only point at the master
				if (e2->targetname) {
					e->targetname = e2->targetname;
					e2->targetname = NULL;
				}
			}
		}
	}
}

void G_RegisterCvars(void)
{
	int			i;
	cvarTable_t	*cv;

	for (i = 0, cv = gameCvarTable; i < gameCvarTableSize; i++, cv++) {
		trap_Cvar_Register(cv->vmCvar, cv->cvarName,
			cv->defaultString, cv->cvarFlags);
		if (cv->vmCvar)
			cv->modificationCount = cv->vmCvar->modificationCount;
	}

	// check some things
	if (g_gametype.integer < 0 || g_gametype.integer >= GT_MAX_GAME_TYPE) {
		G_Printf("g_gametype %i is out of range, defaulting to 0\n", g_gametype.integer);
		trap_Cvar_Set("g_gametype", "0");
		trap_Cvar_Update(&g_gametype);
	}

	level.warmupModificationCount = g_warmupTime.modificationCount;
}

void G_UpdateCvars(void)
{
	int			i;
	cvarTable_t	*cv;

	for (i = 0, cv = gameCvarTable; i < gameCvarTableSize; i++, cv++) {
		if (!cv->vmCvar) {
			continue;
		}

		trap_Cvar_Update(cv->vmCvar);

		if (cv->modificationCount != cv->vmCvar->modificationCount) {
			cv->modificationCount = cv->vmCvar->modificationCount;

			if (cv->trackChange) {
				trap_SendServerCommand(-1, va("print \"Server: %s changed to %s\n\"",
					cv->cvarName, cv->vmCvar->string));
			}
		}
	}
}

static void G_TeamSpawnpoints(char *buffer, int size, team_t team)
{
	gentity_t	*spot;
	char		*label;

	if (size < 1) {
		return;
	}

	switch (team) {
	case TEAM_RED:
		label = "team_CTF_redspawn";
		break;
	case TEAM_BLUE:
		label = "team_CTF_bluespawn";
		break;
	case TEAM_FREE:
		label = "info_player_deathmatch";
		break;
	default:
		return;
	}

	spot = NULL;
	buffer[0] = '\0';

	while ((spot = G_Find(spot, FOFS(classname), label)) != NULL) {
		trace_t	tr;
		vec3_t	dest;
		char	entry[64];

		VectorSet(dest, spot->s.origin[0], spot->s.origin[1], spot->s.origin[2] - 4096);
		trap_Trace(&tr, spot->s.origin, NULL, NULL, dest, spot->s.number, MASK_SOLID);

		Com_sprintf(entry, sizeof entry, "%i %i %i %i %i %i",
			(int) spot->s.origin[0], (int) spot->s.origin[1], (int) (tr.endpos[2] + 1),
			(int) spot->s.angles[0], (int) spot->s.angles[1], (int) spot->s.angles[2]);

		if (*buffer) {
			Q_strcat(buffer, size, " ");
		}
		Q_strcat(buffer, size, entry);
	}
}

static void G_SetSpawnpoints(void)
{
	if (g_gametype.integer == GT_CTF) {
		char	red[MAX_STRING_CHARS], blue[MAX_STRING_CHARS];
		G_TeamSpawnpoints(red, sizeof red, TEAM_RED);
		G_TeamSpawnpoints(blue, sizeof blue, TEAM_BLUE);
		trap_SetConfigstring(CS_SPAWNPOINTS, va("red %s blue %s", red, blue));
	} else {
		char	free[MAX_STRING_CHARS];
		G_TeamSpawnpoints(free, sizeof free, TEAM_FREE);
		trap_SetConfigstring(CS_SPAWNPOINTS, free);
	}
}

void G_InitGame(int levelTime, int randomSeed, int restart)
{
	int					i;

	srand(randomSeed);

	G_RegisterCvars();

	G_ProcessIPBans();

	G_InitMemory();

	if (!restart) {
		trap_Cvar_Set("g_redLocked", "0");
		trap_Cvar_Set("g_blueLocked", "0");
	}

	// set some level globals
	memset(&level, 0, sizeof(level));
	level.time = levelTime;
	level.startTime = levelTime;
	level.timeComplete = levelTime;

	level.snd_fry = G_SoundIndex("sound/player/fry.wav");	// FIXME standing in lava / slime

	if (g_logfile.string[0]) {
		if (g_logfileSync.integer) {
			trap_FS_FOpenFile(g_logfile.string, &level.logFile, FS_APPEND_SYNC);
		} else {
			trap_FS_FOpenFile(g_logfile.string, &level.logFile, FS_APPEND);
		}
		if (!level.logFile) {
			G_Printf("WARNING: Couldn't open logfile: %s\n", g_logfile.string);
		} else {
			char	serverinfo[MAX_INFO_STRING];

			trap_GetServerinfo(serverinfo, sizeof(serverinfo));

			G_LogPrintf("------------------------------------------------------------\n");
			G_LogPrintf("InitGame: %s\n", serverinfo);
		}
	} else {
		G_Printf("Not logging to disk.\n");
	}

	G_InitWorldSession();

	// initialize all entities for this game
	memset(g_entities, 0, MAX_GENTITIES * sizeof(g_entities[0]));
	level.gentities = g_entities;

	// initialize all clients for this game
	level.maxclients = g_maxclients.integer;
	memset(level.clients, 0, sizeof level.clients);
	memset(level.disconnectedClients, 0, sizeof level.clients);
	memset(svps, 0, sizeof svps);

	// set client fields on player ents
	for (i = 0; i<level.maxclients; i++) {
		g_entities[i].client = level.clients + i;
	}

	// always leave room for the max number of clients,
	// even if they aren't all used, so numbers inside that
	// range are NEVER anything but clients
	level.num_entities = MAX_CLIENTS;

	for (i = 0; i < MAX_CLIENTS; i++) {
		g_entities[i].classname = "clientslot";
	}

	// let the server system know where the entites are
	trap_LocateGameData(level.gentities, level.num_entities, sizeof (gentity_t),
		&level.clients[0].ps, sizeof level.clients[0], svps);

	// reserve some spots for dead player bodies
	InitBodyQue();

	ClearRegisteredItems();

	// parse the key/value pairs and spawn gentities
	G_SpawnEntitiesFromString();

	// send spawnpoint positions to clients
	G_SetSpawnpoints();

	// general initialization
	G_FindTeams();

	// make sure we have flags for CTF, etc
	if (g_gametype.integer >= GT_TEAM) {
		G_CheckTeamItems();
	}

	SaveRegisteredItems();

	G_Vote_ReadCustom();

	if (g_gametype.integer != GT_DEFRAG && trap_Cvar_VariableIntegerValue("bot_enable")) {
		BotAISetup(restart);
		BotAILoadMap(restart);
		G_InitBots(restart);
	}
}

void G_ShutdownGame(int restart)
{
	if (level.logFile) {
		G_LogPrintf("ShutdownGame:\n");
		G_LogPrintf("------------------------------------------------------------\n");
		trap_FS_FCloseFile(level.logFile);
		level.logFile = 0;
	}

	// write all the client session data so we can get it back
	G_WriteSessionData();

	if (g_gametype.integer != GT_DEFRAG && trap_Cvar_VariableIntegerValue("bot_enable")) {
		BotAIShutdown(restart);
	}
}

void QDECL Com_Error (int level, const char *error, ...)
{
	va_list		argptr;
	char		text[1024];

	va_start (argptr, error);
	Q_vsnprintf(text, sizeof(text), error, argptr);
	va_end (argptr);

	trap_Error(text);
}

void QDECL Com_Printf(const char *msg, ...)
{
	va_list		argptr;
	char		text[1024];

	va_start (argptr, msg);
	Q_vsnprintf(text, sizeof(text), msg, argptr);
	va_end (argptr);

	trap_Print(text);
}

/* PLAYER COUNTING / SCORE SORTING */

/**
If there are less than two tournament players, put a
spectator in the game and restart
*/
void AddTournamentPlayer(void)
{
	int			i;
	gclient_t	*client;
	gclient_t	*nextInLine;

	if (level.numPlayingClients >= 2) {
		return;
	}

	// never change during intermission
	if (level.intermissiontime) {
		return;
	}

	nextInLine = NULL;

	for (i = 0; i < level.maxclients; i++) {
		client = &level.clients[i];
		if (client->pers.connected != CON_CONNECTED) {
			continue;
		}
		if (client->sess.sessionTeam != TEAM_SPECTATOR) {
			continue;
		}
		// never select the dedicated follow or scoreboard clients
		if (client->sess.spectatorState == SPECTATOR_SCOREBOARD ||
			client->sess.spectatorClient < 0) {
			continue;
		}
		if (client->sess.specOnly) {
			continue;
		}
		if (!nextInLine || client->sess.spectatorNum > nextInLine->sess.spectatorNum) {
			nextInLine = client;
		}
	}

	if (!nextInLine) {
		return;
	}

	level.warmupTime = -1;

	// set them to free-for-all team
	SetTeam(&g_entities[ nextInLine - level.clients ], "f");
}

/**
Add client to end of tournament queue
*/
void AddTournamentQueue(gclient_t *client)
{
	int index;
	gclient_t *curclient;

	for (index = 0; index < level.maxclients; index++) {
		curclient = &level.clients[index];

		if (curclient->pers.connected != CON_DISCONNECTED) {
			if (curclient == client) {
				curclient->sess.spectatorNum = 0;
			}
			else if (curclient->sess.sessionTeam == TEAM_SPECTATOR) {
				curclient->sess.spectatorNum++;
			}
		}
	}
}

/**
Make the loser a spectator at the back of the line
*/
void RemoveTournamentLoser(void)
{
	int			clientNum;

	if (level.numPlayingClients != 2) {
		return;
	}

	clientNum = level.sortedClients[1];

	if (level.clients[ clientNum ].pers.connected != CON_CONNECTED) {
		return;
	}

	// make them a spectator
	SetTeam(&g_entities[ clientNum ], "s");
}

void RemoveTournamentWinner(void)
{
	int			clientNum;

	if (level.numPlayingClients != 2) {
		return;
	}

	clientNum = level.sortedClients[0];

	if (level.clients[ clientNum ].pers.connected != CON_CONNECTED) {
		return;
	}

	// make them a spectator
	SetTeam(&g_entities[ clientNum ], "s");
}

void AdjustTournamentScores(void)
{
	int			clientNum;

	clientNum = level.sortedClients[0];
	if (level.clients[ clientNum ].pers.connected == CON_CONNECTED) {
		level.clients[ clientNum ].sess.wins++;
		ClientUserinfoChanged(clientNum);
	}

	clientNum = level.sortedClients[1];
	if (level.clients[ clientNum ].pers.connected == CON_CONNECTED) {
		level.clients[ clientNum ].sess.losses++;
		ClientUserinfoChanged(clientNum);
	}

}

int QDECL SortRanks(const void *a, const void *b)
{
	gclient_t	*ca, *cb;

	ca = &level.clients[*(int *)a];
	cb = &level.clients[*(int *)b];

	// sort special clients last
	if (ca->sess.spectatorState == SPECTATOR_SCOREBOARD || ca->sess.spectatorClient < 0) {
		return 1;
	}
	if (cb->sess.spectatorState == SPECTATOR_SCOREBOARD || cb->sess.spectatorClient < 0) {
		return -1;
	}

	// then connecting clients
	if (ca->pers.connected == CON_CONNECTING) {
		return 1;
	}
	if (cb->pers.connected == CON_CONNECTING) {
		return -1;
	}

	// then spectators
	if (ca->sess.sessionTeam == TEAM_SPECTATOR && cb->sess.sessionTeam == TEAM_SPECTATOR) {
		if (!ca->sess.specOnly && !cb->sess.specOnly) {
			if (ca->sess.spectatorNum > cb->sess.spectatorNum) {
				return -1;
			}
			if (ca->sess.spectatorNum < cb->sess.spectatorNum) {
				return 1;
			}
		}
		if (ca->sess.specOnly && ca->sess.specOnly) {
			if (ca->pers.enterTime > cb->pers.enterTime) {
				return 1;
			}
			if (ca->pers.enterTime < cb->pers.enterTime) {
				return -1;
			}
		}
		if (ca->sess.specOnly) {
			return -1;
		}
		if (cb->sess.specOnly) {
			return 1;
		}
		return 0;
	}
	if (ca->sess.sessionTeam == TEAM_SPECTATOR) {
		return 1;
	}
	if (cb->sess.sessionTeam == TEAM_SPECTATOR) {
		return -1;
	}

	if (g_gametype.integer == GT_ELIMINATION) {
		if (ca->ps.persistant[PERS_DAMAGE_DONE] > cb->ps.persistant[PERS_DAMAGE_DONE]) {
			return -1;
		}
		if (ca->ps.persistant[PERS_DAMAGE_DONE] < cb->ps.persistant[PERS_DAMAGE_DONE]) {
			return 1;
		}
	}

	if (g_gametype.integer == GT_DEFRAG) {
		if (ca->pers.score < cb->pers.score) {
			return -1;
		}
		if (ca->pers.score > cb->pers.score) {
			return 1;
		}
		return 0;
	}

	// then sort by score
	if (ca->pers.score > cb->pers.score) {
		return -1;
	}
	if (ca->pers.score < cb->pers.score) {
		return 1;
	}
	return 0;
}

/**
Recalculates the score ranks of all players
This will be called on every client connect, begin, disconnect, death,
and team change.
*/
void CalculateRanks(void)
{
	int		i;
	int		rank;
	int		score;
	int		newScore;
	gclient_t	*cl;

	level.follow1 = -1;
	level.follow2 = -1;
	level.numConnectedClients = 0;
	level.numNonSpectatorClients = 0;
	level.numPlayingClients = 0;
	level.numVotingClients = 0;		// don't count bots

	for (i = 0; i < ARRAY_LEN(level.numteamVotingClients); i++)
		level.numteamVotingClients[i] = 0;

	for (i = 0; i < level.maxclients; i++) {
		if (level.clients[i].pers.connected != CON_DISCONNECTED) {
			level.sortedClients[level.numConnectedClients] = i;
			level.numConnectedClients++;

			if (level.clients[i].sess.sessionTeam != TEAM_SPECTATOR) {
				level.numNonSpectatorClients++;

				// decide if this should be auto-followed
				if (level.clients[i].pers.connected == CON_CONNECTED) {
					level.numPlayingClients++;
					if (!(g_entities[i].r.svFlags & SVF_BOT)) {
						level.numVotingClients++;
						if (level.clients[i].sess.sessionTeam == TEAM_RED)
							level.numteamVotingClients[0]++;
						else if (level.clients[i].sess.sessionTeam == TEAM_BLUE)
							level.numteamVotingClients[1]++;
					}
					if (level.follow1 == -1) {
						level.follow1 = i;
					} else if (level.follow2 == -1) {
						level.follow2 = i;
					}
				}
			}
		}
	}

	qsort(level.sortedClients, level.numConnectedClients,
		sizeof(level.sortedClients[0]), SortRanks);

	for (i = 0; i < level.maxclients; ++i) {
		g_entities[level.sortedClients[i]].client->ps.persistant[PERS_RANK] = i;
	}

	// set the rank value for all clients that are connected and not spectators
	if (g_gametype.integer >= GT_TEAM) {
		// in team games, rank is just the order of the teams, 0=red, 1=blue, 2=tied
		for (i = 0;  i < level.numConnectedClients; i++) {
			cl = &level.clients[ level.sortedClients[i] ];
			if (level.teamScores[TEAM_RED] == level.teamScores[TEAM_BLUE]) {
				cl->ps.persistant[PERS_RANK] = 2;
			} else if (level.teamScores[TEAM_RED] > level.teamScores[TEAM_BLUE]) {
				cl->ps.persistant[PERS_RANK] = 0;
			} else {
				cl->ps.persistant[PERS_RANK] = 1;
			}
		}
	} else {
		rank = -1;
		score = 0;
		for (i = 0;  i < level.numPlayingClients; i++) {
			cl = &level.clients[ level.sortedClients[i] ];
			newScore = cl->pers.score;
			if (i == 0 || newScore != score) {
				rank = i;
				// assume we aren't tied until the next client is checked
				level.clients[ level.sortedClients[i] ].ps.persistant[PERS_RANK] = rank;
			} else {
				// we are tied with the previous client
				level.clients[ level.sortedClients[i-1] ].ps.persistant[PERS_RANK] = rank | RANK_TIED_FLAG;
				level.clients[ level.sortedClients[i] ].ps.persistant[PERS_RANK] = rank | RANK_TIED_FLAG;
			}
			score = newScore;
		}
	}

	// set the CS_SCORES1/2 configstrings, which will be visible to everyone
	if (g_gametype.integer >= GT_TEAM) {
		trap_SetConfigstring(CS_SCORES1, va("%i", level.teamScores[TEAM_RED]));
		trap_SetConfigstring(CS_SCORES2, va("%i", level.teamScores[TEAM_BLUE]));
	} else {
		if (level.numConnectedClients == 0) {
			trap_SetConfigstring(CS_SCORES1, va("%i", SCORE_NOT_PRESENT));
			trap_SetConfigstring(CS_SCORES2, va("%i", SCORE_NOT_PRESENT));
		} else if (level.numConnectedClients == 1) {
			trap_SetConfigstring(CS_SCORES1, va("%i", level.clients[ level.sortedClients[0] ].pers.score));
			trap_SetConfigstring(CS_SCORES2, va("%i", SCORE_NOT_PRESENT));
		} else {
			// in tourney: SCORES1 corresponds to the client with the lower clientNum
			int	min, max;
			min = MIN(level.sortedClients[0], level.sortedClients[1]);
			max = MAX(level.sortedClients[0], level.sortedClients[1]);
			trap_SetConfigstring(CS_SCORES1, va("%i", level.clients[min].pers.score));
			trap_SetConfigstring(CS_SCORES2, va("%i", level.clients[max].pers.score));
		}
	}

	// see if it is time to end the level
	CheckExitRules();
}

// MAP CHANGING

/**
When the intermission starts, this will be called for all players.
If a new client connects, this will be called after the spawn function.
*/
void MoveClientToIntermission(gentity_t *ent)
{
	// take out of follow mode if needed
	if (ent->client->sess.spectatorState == SPECTATOR_FOLLOW) {
		StopFollowing(ent);
	}

	FindIntermissionPoint();
	// move to the spot
	VectorCopy(level.intermission_origin, ent->s.origin);
	VectorCopy(level.intermission_origin, ent->client->ps.origin);
	VectorCopy(level.intermission_angle, ent->client->ps.viewangles);
	ent->client->ps.pm_type = PM_INTERMISSION;

	// clean up powerup info
	memset(ent->client->ps.powerups, 0, sizeof(ent->client->ps.powerups));

	ent->client->ps.eFlags = 0;
	ent->s.eFlags = 0;
	ent->s.eType = ET_GENERAL;
	ent->s.modelindex = 0;
	ent->s.loopSound = 0;
	ent->s.event = 0;
	ent->r.contents = 0;
}

/**
This is also used for spectator spawns
*/
void FindIntermissionPoint(void)
{
	gentity_t	*ent, *target;
	vec3_t		dir;

	// find the intermission spot
	ent = G_Find(NULL, FOFS(classname), "info_player_intermission");
	if (!ent) {	// the map creator forgot to put in an intermission point...
		SelectSpawnPoint (vec3_origin, level.intermission_origin, level.intermission_angle, qfalse);
	} else {
		VectorCopy(ent->s.origin, level.intermission_origin);
		VectorCopy(ent->s.angles, level.intermission_angle);
		// if it has a target, look towards it
		if (ent->target) {
			target = G_PickTarget(ent->target);
			if (target) {
				VectorSubtract(target->s.origin, level.intermission_origin, dir);
				vectoangles(dir, level.intermission_angle);
			}
		}
	}
}

static void G_SendStats(gclient_t *client)
{
	int			i;
	int			*data;
	char		diffmsg[512];
	char		fullmsg[512];
	qboolean	diffempty, fullempty;
	playerStats_t	*oldstats, *newstats;

	diffmsg[0] = '\0';
	fullmsg[0] = '\0';
	oldstats = &client->pers.oldstats;
	newstats = &client->pers.stats;

	diffempty = qtrue;
	fullempty = qtrue;

	i = 0;
	while ((data = BG_StatsData(newstats, i))) {
		if (*data != 0) {
			Q_strcat(fullmsg, sizeof fullmsg, va(" %i %i", i, *data));
			fullempty = qfalse;
		}

		if (*data != *BG_StatsData(oldstats, i)) {
			Q_strcat(diffmsg, sizeof diffmsg, va(" %i %i", i, *data));
			diffempty = qfalse;
		}
		++i;
	}

	*oldstats = *newstats;

	for (i = 0; i < level.maxclients; ++i) {
		gclient_t	*cl = &level.clients[i];
		if (cl->pers.connected != CON_CONNECTED) {
			continue;
		}

		if (g_entities[i].r.svFlags & SVF_BOT) {
			continue;
		}

		if (cl == client
			|| (g_gametype.integer == GT_TOURNAMENT && cl->sess.sessionTeam == TEAM_SPECTATOR)
			|| (g_gametype.integer == GT_TOURNAMENT && level.intermissiontime)
			|| (cl->sess.spectatorState == SPECTATOR_FOLLOW
			&& cl->sess.spectatorClient == client - level.clients))
		{
			if (!cl->sess.fullStatsSent && !fullempty) {
				trap_SendServerCommand(i, va("stats %ld %s", client - level.clients, fullmsg));
				cl->sess.fullStatsSent = qtrue;
			} else if (!diffempty) {
				trap_SendServerCommand(i, va("stats %ld %s", client - level.clients, diffmsg));
			}
		}
	}
}

void BeginIntermission(void)
{
	int			i;
	gentity_t	*ent;

	if (level.intermissiontime) {
		return;		// already active
	}

	// if in tournement mode, change the wins / losses
	if (g_gametype.integer == GT_TOURNAMENT) {
		AdjustTournamentScores();
	}

	G_StatsWrite();

	level.intermissiontime = level.time;
	// move all clients to the intermission point
	for (i = 0; i< level.maxclients; i++) {
		ent = &g_entities[i];
		if (!ent->inuse)
			continue;
		// respawn if dead
		if (ent->health <= 0) {
			ClientRespawn(ent);
		}
		MoveClientToIntermission(ent);
		G_SendStats(ent->client);
	}

	G_SendScoreboard(NULL);
}

/**
When the intermission has been exited, the server is either killed
or moved to a new level based on the "nextmap" cvar
*/
void ExitLevel (void)
{
	int		i;
	gclient_t *cl;
	char nextmap[MAX_STRING_CHARS];
	char d1[MAX_STRING_CHARS];

	//bot interbreeding
	BotInterbreedEndMatch();

	// if we are running a tournement map, kick the loser to spectator status,
	// which will automatically grab the next spectator and restart
	if (g_gametype.integer == GT_TOURNAMENT) {
		if (!level.restarted) {
			RemoveTournamentLoser();
			trap_SendConsoleCommand(EXEC_APPEND, "map_restart\n");
			level.restarted = qtrue;
			level.changemap = NULL;
			level.intermissiontime = 0;
		}
		return;
	}

	trap_Cvar_VariableStringBuffer("nextmap", nextmap, sizeof(nextmap));
	trap_Cvar_VariableStringBuffer("d1", d1, sizeof(d1));

	if (!Q_stricmp(nextmap, "map_restart") && Q_stricmp(d1, "")) {
		trap_Cvar_Set("nextmap", "vstr d2");
		trap_SendConsoleCommand(EXEC_APPEND, "vstr d1\n");
	} else {
		trap_SendConsoleCommand(EXEC_APPEND, "vstr nextmap\n");
	}

	level.changemap = NULL;
	level.intermissiontime = 0;

	// reset all the scores so we don't enter the intermission again
	level.teamScores[TEAM_RED] = 0;
	level.teamScores[TEAM_BLUE] = 0;
	for (i = 0; i< g_maxclients.integer; i++) {
		cl = level.clients + i;
		if (cl->pers.connected != CON_CONNECTED) {
			continue;
		}
		cl->pers.score = 0;
	}

	// we need to do this here before changing to CON_CONNECTING
	G_WriteSessionData();

	// change all client states to connecting, so the early players into the
	// next level will know the others aren't done reconnecting
	for (i = 0; i< g_maxclients.integer; i++) {
		if (level.clients[i].pers.connected == CON_CONNECTED) {
			level.clients[i].pers.connected = CON_CONNECTING;
		}
	}

}

/**
Print to the logfile with a time stamp if it is open
*/
void QDECL G_LogPrintf(const char *fmt, ...)
{
	va_list		argptr;
	char		string[1024];
	int			min, tens, sec;

	sec = (level.time - level.startTime) / 1000;

	min = sec / 60;
	sec -= min * 60;
	tens = sec / 10;
	sec -= tens * 10;

	Com_sprintf(string, sizeof(string), "%3i:%i%i ", min, tens, sec);

	va_start(argptr, fmt);
	Q_vsnprintf(string + 7, sizeof(string) - 7, fmt, argptr);
	va_end(argptr);

	if (g_dedicated.integer) {
		G_Printf("%s", string + 7);
	}

	if (!level.logFile) {
		return;
	}

	trap_FS_Write(string, strlen(string), level.logFile);
}

/**
Append information about this game to the log file
*/
void LogExit(const char *string)
{
	int				i;
	gclient_t		*cl;
	G_LogPrintf("Exit: %s\n", string);

	level.intermissionQueued = level.time;

	// this will keep the clients from playing any voice sounds
	// that will get cut off when the queued intermission starts
	trap_SetConfigstring(CS_INTERMISSION, "1");

	if (g_gametype.integer >= GT_TEAM) {
		G_LogPrintf("red:%i  blue:%i\n",
			level.teamScores[TEAM_RED], level.teamScores[TEAM_BLUE]);
	}

	for (i = 0; i < level.numConnectedClients; i++) {
		int		ping;

		cl = &level.clients[level.sortedClients[i]];

		if (cl->sess.sessionTeam == TEAM_SPECTATOR) {
			continue;
		}
		if (cl->pers.connected == CON_CONNECTING) {
			continue;
		}

		ping = (cl->pers.ping < 999 ? cl->pers.ping : 999);

		G_LogPrintf("score: %i  ping: %i  client: %i %s\n", cl->pers.score,
			ping, level.sortedClients[i], cl->pers.netname);
	}
}

/**
The level will stay at the intermission for a minimum of 5 seconds
If all players wish to continue, the level will then exit.
If one or more players have not acknowledged the continue, the game will
wait 10 seconds before going on.
*/
void CheckIntermissionExit(void)
{
	int			ready, notReady, playerCount;
	int			i;
	gclient_t	*cl;

	// see which players are ready
	ready = 0;
	notReady = 0;
	playerCount = 0;
	for (i = 0; i< g_maxclients.integer; i++) {
		cl = level.clients + i;
		if (cl->pers.connected != CON_CONNECTED) {
			continue;
		}
		if (g_entities[i].r.svFlags & SVF_BOT) {
			continue;
		}

		playerCount++;
		if (cl->pers.ready) {
			ready++;
		} else {
			notReady++;
		}
	}

	// never exit in less than five seconds
	if (level.time < level.intermissiontime + 5000) {
		return;
	}

	// only test ready status when there are real players present
	if (playerCount > 0) {
		// if nobody wants to go, clear timer
		if (!ready) {
			level.readyToExit = qfalse;
			return;
		}

		// if everyone wants to go, go now
		if (!notReady) {
			ExitLevel();
			return;
		}
	}

	// the first person to ready starts the ten second timeout
	if (!level.readyToExit) {
		level.readyToExit = qtrue;
		level.exitTime = level.time;
	}

	// if we have waited ten seconds since at least one player
	// wanted to exit, go ahead
	if (level.time < level.exitTime + 10000) {
		return;
	}

	ExitLevel();
}

qboolean ScoreIsTied(void)
{
	int		a, b;

	if (level.numPlayingClients < 2) {
		return qfalse;
	}

	if (g_gametype.integer >= GT_TEAM) {
		return level.teamScores[TEAM_RED] == level.teamScores[TEAM_BLUE];
	}

	a = level.clients[level.sortedClients[0]].pers.score;
	b = level.clients[level.sortedClients[1]].pers.score;

	return a == b;
}

static void G_RoundStats(void)
{
	int			i;
	gclient_t	*cl, *maxDmgClient, *minDmgClient, *maxKillsClient;

	maxDmgClient = NULL;
	minDmgClient = NULL;
	maxKillsClient = NULL;

	for (i = 0; i < level.maxclients; ++i) {
		cl = &level.clients[level.sortedClients[i]];

		if (cl->pers.connected != CON_CONNECTED) {
			continue;
		}

		if (cl->sess.sessionTeam == TEAM_SPECTATOR) {
			continue;
		}

		if (!maxDmgClient || cl->roundDamageDone > maxDmgClient->roundDamageDone) {
			maxDmgClient = cl;
		}

		if (!minDmgClient || cl->roundDamageTaken < minDmgClient->roundDamageTaken) {
			minDmgClient = cl;
		}

		if (!maxKillsClient || cl->roundKills > maxKillsClient->roundKills
			|| (cl->roundKills == maxKillsClient->roundKills
			&& cl->roundDamageDone > maxKillsClient->roundDamageDone))
		{
			maxKillsClient = cl;
		}
	}

	ClientPrint(NULL, va("Max damage done: %s ^2%i",
		maxDmgClient->pers.netname, maxDmgClient->roundDamageDone));
	ClientPrint(NULL, va("Min damage taken: %s ^2%i",
		minDmgClient->pers.netname, minDmgClient->roundDamageTaken));
	ClientPrint(NULL, va("Max kills: %s ^2%i",
		maxKillsClient->pers.netname, maxKillsClient->roundKills));
}

/**
There will be a delay between the time the exit is qualified for
and the time everyone is moved to the intermission spot, so you
can see the last frag.
*/
void CheckExitRules(void)
{
	int			i;
	gclient_t	*cl;

	if (g_gametype.integer == GT_DEFRAG) {
		return;
	}

	// if at the intermission, wait for all non-bots to
	// signal ready, then go to next level
	if (level.intermissiontime) {
		CheckIntermissionExit();
		return;
	}

	if (level.intermissionQueued) {
		if (level.time - level.intermissionQueued >= INTERMISSION_DELAY_TIME) {
			level.intermissionQueued = 0;
			BeginIntermission();
		}

		return;
	}

	if (g_gametype.integer == GT_ELIMINATION) {
		if (level.teamScores[TEAM_RED] == g_capturelimit.integer) {
			ClientPrint(NULL, "Red wins.");
			LogExit("Red wins.");
			return;
		} else if (level.teamScores[TEAM_BLUE] == g_capturelimit.integer) {
			ClientPrint(NULL, "Blue wins.");
			LogExit("Blue wins.");
			return;
		}
	}

	// check for sudden death
	if (ScoreIsTied() && !g_overtime.integer) {
		// always wait for sudden death
		return;
	}

	if (g_timelimit.integer && !level.warmupTime
		&& (level.time - level.startTime >= g_timelimit.integer * 60000
		|| (level.overtimeStart && level.time - level.overtimeStart >= g_overtime.integer * 60000)))
	{
		if (ScoreIsTied() && g_overtime.integer) {
			level.overtimeStart = level.time;
			trap_SetConfigstring(CS_OVERTIME, va("%d", level.overtimeStart));
		} else if (!g_overtime.integer || level.overtimeStart) {
			ClientPrint(NULL, "Timelimit hit.");
			LogExit("Timelimit hit.");
		} else {
			ClientPrint(NULL, "Overtime complete.");
			LogExit("Overtime complete.");
		}
		return;
	}

	if (g_gametype.integer < GT_CTF && g_fraglimit.integer) {
		if (level.teamScores[TEAM_RED] >= g_fraglimit.integer) {
			trap_SendServerCommand(-1, "print \"Red hit the fraglimit.\n\"");
			LogExit("Fraglimit hit.");
			return;
		}

		if (level.teamScores[TEAM_BLUE] >= g_fraglimit.integer) {
			trap_SendServerCommand(-1, "print \"Blue hit the fraglimit.\n\"");
			LogExit("Fraglimit hit.");
			return;
		}

		for (i = 0; i< g_maxclients.integer; i++) {
			cl = level.clients + i;
			if (cl->pers.connected != CON_CONNECTED) {
				continue;
			}
			if (cl->sess.sessionTeam != TEAM_FREE) {
				continue;
			}

			if (cl->pers.score >= g_fraglimit.integer) {
				LogExit("Fraglimit hit.");
				trap_SendServerCommand(-1, va("print \"%s" S_COLOR_WHITE " hit the fraglimit.\n\"",
					cl->pers.netname));
				return;
			}
		}
	}

	if (g_gametype.integer >= GT_CTF && g_capturelimit.integer) {

		if (level.teamScores[TEAM_RED] >= g_capturelimit.integer) {
			trap_SendServerCommand(-1, "print \"Red hit the capturelimit.\n\"");
			LogExit("Capturelimit hit.");
			return;
		}

		if (level.teamScores[TEAM_BLUE] >= g_capturelimit.integer) {
			trap_SendServerCommand(-1, "print \"Blue hit the capturelimit.\n\"");
			LogExit("Capturelimit hit.");
			return;
		}
	}
}

// FUNCTIONS CALLED EVERY FRAME

void CheckPings(qboolean forceSend)
{
	int			i;
	gclient_t	*cl;
	qboolean	empty;
	char		pings[1024];

	if (!forceSend && (level.time - level.lastPingTime < PING_UPDATE_INTERVAL)) {
		return;
	}

	strcpy(pings, "pings");
	empty = qtrue;

	for (i = 0; i < g_maxclients.integer; ++i) {
		cl = &level.clients[i];
		if (g_entities[i].r.svFlags & SVF_BOT) {
			continue;
		}

		if (cl->pers.connected == CON_CONNECTING) {
			Q_strcat(pings, sizeof pings, " -1");
		} else if (cl->pers.connected == CON_CONNECTED) {
			Q_strcat(pings, sizeof pings, va(" %i", cl->pers.ping));
		} else {
			continue;
		}

		empty = qfalse;
	}

	level.lastPingTime = level.time;

	if (!empty) {
		trap_SendServerCommand(-1, pings);
	}
}

static void CheckScoreboard(void)
{
	if (level.time - level.lastScoreboardTime > SCOREBOARD_INTERVAL) {
		G_SendScoreboard(NULL);
		level.lastScoreboardTime = level.time;
	}
}

static void CheckStats(void)
{
	int	i;
	if (level.time - level.lastStatsTime < STATS_INTERVAL) {
		return;
	}

	for (i = 0; i < level.maxclients; ++i) {
		gclient_t	*cl = &level.clients[i];
		if (cl->pers.connected != CON_CONNECTED || cl->sess.sessionTeam == TEAM_SPECTATOR) {
			continue;
		}
		G_SendStats(cl);
	}
	level.lastStatsTime = level.time;
}

static qboolean EnoughReady(void)
{
	int	i, clientsReady, humanPlayers;

	if (g_gametype.integer > GT_TEAM) {
		if (TeamCount(-1, TEAM_BLUE) < 1 || TeamCount(-1, TEAM_RED) < 1) {
			return qfalse;
		}
	} else if (level.numPlayingClients < 2) {
		level.timeComplete = level.time;
		return qfalse;
	}

	if (!g_startWhenReady.integer) {
		return qtrue;
	}

	if (g_autoReady.integer > 0 && g_autoReady.integer * 1000 < level.time - level.timeComplete) {
		return qtrue;
	}

	for (i = 0, clientsReady = 0, humanPlayers = 0; i < level.numPlayingClients; i++) {
		gentity_t	*ent;
		ent = &g_entities[level.sortedClients[i]];
		if (ent->inuse && !(ent->r.svFlags & SVF_BOT)) {
			humanPlayers++;
			if (ent->client->pers.ready) {
				clientsReady++;
			}
		}
	}

	if (g_startWhenReady.integer == 1 && clientsReady < ceil(humanPlayers / 2.0f)) {
		return qfalse;
	}

	if (g_startWhenReady.integer == 2 && clientsReady < humanPlayers) {
		return qfalse;
	}

	return qtrue;
}

void CheckElimination(void)
{
	int	i;
	int	livingRed, livingBlue;
	static qboolean	roundRespawned = qfalse;

	if (g_gametype.integer != GT_ELIMINATION) {
		return;
	}

	if (level.numConnectedClients == 0 || level.warmupTime != 0) {
		return;
	}

	livingRed = TeamLivingCount(TEAM_RED);
	livingBlue = TeamLivingCount(TEAM_BLUE);

	if (level.roundStarted && livingRed == 0) {
		ClientPrint(NULL, "Red team eliminated.");
		AddTeamScore(level.intermission_origin, TEAM_BLUE, 1);
		Team_ForceGesture(TEAM_RED);
	} else if (level.roundStarted && livingBlue == 0) {
		ClientPrint(NULL, "Blue team eliminated.");
		AddTeamScore(level.intermission_origin, TEAM_RED, 1);
		Team_ForceGesture(TEAM_BLUE);
	}

	// round ends
	if (level.roundStarted && (livingRed == 0 || livingBlue == 0)) {
		CalculateRanks();
		G_RoundStats();

		roundRespawned = qfalse;
		level.roundStarted = qfalse;

		// set roundStartTime to round end time to calculate a two seconds delay to respawn
		level.roundStartTime = level.time;
		return;
	} else if (level.time > level.roundStartTime + 1000 * g_roundTimelimit.integer) {
		ClientPrint(NULL, "Round draw.");

		roundRespawned = qfalse;
		level.roundStarted = qfalse;

		level.roundStartTime = level.time;
		return;
	}

	// respawn clients for next round
	if (!roundRespawned && level.time > level.roundStartTime + 1700) {
		gclient_t	*cl;
		roundRespawned = qtrue;
		for (i = 0, cl = level.clients; i < level.maxclients; ++i, ++cl) {
			if (cl->pers.connected != CON_CONNECTED) {
				continue;
			}
			if (cl->sess.sessionTeam == TEAM_SPECTATOR) {
				continue;
			}

			// respawn if this is not the first round
			if (level.roundStartTime) {
				ClientSpawn(&g_entities[i]);
			}

			cl->ps.pm_flags |= PMF_ELIM_WARMUP;
		}
		level.roundStartTime = level.time + 1000 * g_roundWarmup.integer;
		trap_SetConfigstring(CS_LIVING_COUNT, va("%d %d", TeamLivingCount(TEAM_RED),
			TeamLivingCount(TEAM_BLUE)));
		trap_SetConfigstring(CS_ROUND_START, va("%d", level.roundStartTime));
		return;
	}

	// start the round
	if (roundRespawned && !level.roundStarted && level.time > level.roundStartTime) {
		gclient_t	*cl;
		for (i = 0, cl = level.clients; i < level.maxclients; ++i, ++cl) {
			if (cl->pers.connected != CON_CONNECTED) {
				continue;
			}
			if (cl->sess.sessionTeam == TEAM_SPECTATOR) {
				continue;
			}

			if (g_entities[i].client->eliminated) {
				ClientSpawn(&g_entities[i]);
			}

			cl->roundDamageDone = 0;
			cl->roundDamageTaken = 0;
			cl->roundKills = 0;
			cl->ps.pm_flags &= ~PMF_ELIM_WARMUP;
		}
		level.roundStarted = qtrue;
	}
}

/**
Once a frame, check for changes in tournement player state
*/
void CheckWarmup(void)
{
	if (g_gametype.integer == GT_DEFRAG) {
		return;
	}

	/// unlock empty teams
	if (level.time - level.startTime > 1000) {
		if (g_redLocked.integer && TeamCount(-1, TEAM_RED) == 0) {
			trap_Cvar_Set("g_redLocked", "0");
		}

		if (g_blueLocked.integer && TeamCount(-1, TEAM_BLUE) == 0) {
			trap_Cvar_Set("g_blueLocked", "0");
		}
	}

	// check because we run 3 game frames before calling Connect and/or ClientBegin
	// for clients on a map_restart
	if (level.numPlayingClients == 0) {
		return;
	}

	if (level.warmupTime == 0) {
		return;
	}

	// if the warmup is changed at the console, restart it
	if (g_warmupTime.modificationCount != level.warmupModificationCount) {
		level.warmupModificationCount = g_warmupTime.modificationCount;
		level.warmupTime = -1;
	}

	if (g_gametype.integer == GT_TOURNAMENT) {
		// pull in a spectator if needed
		if (level.numPlayingClients < 2) {
			level.timeComplete = level.time;
			AddTournamentPlayer();
		}

		// if we don't have two players, go back to "waiting for players"
		if (level.numPlayingClients != 2) {
			if (level.warmupTime != -1) {
				level.warmupTime = -1;
				trap_SetConfigstring(CS_WARMUP, va("%i", level.warmupTime));
				G_LogPrintf("Warmup:\n");
			}
			return;
		}

		// if the warmup is changed at the console, restart it
		if (g_warmupTime.modificationCount != level.warmupModificationCount) {
			level.warmupModificationCount = g_warmupTime.modificationCount;
			level.warmupTime = -1;
		}

		// if all players have arrived, start the countdown
		if (level.warmupTime < 0 && !EnoughReady()) {
			return;
		}
	} else if (level.warmupTime != 0) {
		if (!EnoughReady()) {
			if (level.warmupTime != -1) {
				level.warmupTime = -1;
				trap_SetConfigstring(CS_WARMUP, va("%i", level.warmupTime));
				G_LogPrintf("Warmup:\n");
			}
			return; // still waiting for team members
		}
	}

	// if all players have arrived, start the countdown
	if (level.warmupTime < 0) {
		if (g_warmupTime.integer < 3) {
			g_warmupTime.integer = 3;
		}

		level.warmupTime = level.time + (g_warmupTime.integer - 1) * 1000;
		trap_SetConfigstring(CS_WARMUP, va("%i", level.warmupTime));
		return;
	}

	// if the warmup time has counted down, restart
	if (level.time > level.warmupTime) {
		level.warmupTime += 10000;
		trap_Cvar_Set("g_restarted", "1");
		trap_SendConsoleCommand(EXEC_APPEND, "map_restart\n");
		level.restarted = qtrue;
		return;
	}
}

void PrintTeam(int team, char *message)
{
	int i;

	for (i = 0; i < level.maxclients; i++) {
		if (level.clients[i].sess.sessionTeam != team)
			continue;
		trap_SendServerCommand(i, message);
	}
}

void SetLeader(int team, int client)
{
	int		i;

	if (level.clients[client].pers.connected == CON_DISCONNECTED) {
		PrintTeam(team, va("print \"%s is not connected\n\"", level.clients[client].pers.netname));
		return;
	}

	if (level.clients[client].sess.sessionTeam != team) {
		PrintTeam(team, va("print \"%s is not on the team anymore\n\"", level.clients[client].pers.netname));
		return;
	}

	for (i = 0; i < level.maxclients; i++) {
		if (level.clients[i].sess.sessionTeam != team)
			continue;
		if (level.clients[i].sess.teamLeader) {
			level.clients[i].sess.teamLeader = qfalse;
			ClientUserinfoChanged(i);
		}
	}

	level.clients[client].sess.teamLeader = qtrue;
	ClientUserinfoChanged(client);
	PrintTeam(team, va("print \"%s is the new team leader\n\"", level.clients[client].pers.netname));
}

void CheckTeamLeader(int team)
{
	int		i;

	for (i = 0; i < level.maxclients; i++) {
		if (level.clients[i].sess.sessionTeam != team)
			continue;
		if (level.clients[i].sess.teamLeader)
			break;
	}

	if (i >= level.maxclients) {
		for (i = 0; i < level.maxclients; i++) {
			if (level.clients[i].sess.sessionTeam != team)
				continue;
			if (!(g_entities[i].r.svFlags & SVF_BOT)) {
				level.clients[i].sess.teamLeader = qtrue;
				break;
			}
		}

		if (i >= level.maxclients) {
			for (i = 0; i < level.maxclients; i++) {
				if (level.clients[i].sess.sessionTeam != team)
					continue;
				level.clients[i].sess.teamLeader = qtrue;
				break;
			}
		}
	}
}

void CheckTeamVote(int team)
{
	int		cs_offset;

	if (team == TEAM_RED) {
		cs_offset = 0;
	} else if (team == TEAM_BLUE) {
		cs_offset = 1;
	} else {
		return;
	}

	if (!level.teamVoteTime[cs_offset]) {
		return;
	}
	if (level.time - level.teamVoteTime[cs_offset] >= VOTE_TIME) {
		trap_SetConfigstring(CS_TEAMVOTE_TIME + cs_offset, "failed");
		G_LogPrintf("Team vote failed.\n");
	} else {
		if (level.teamVoteYes[cs_offset] > level.numteamVotingClients[cs_offset]/2) {
			// execute the command, then remove the vote
			trap_SetConfigstring(CS_TEAMVOTE_TIME + cs_offset, "passed");
			G_LogPrintf("Team vote passed.\n");

			if (!Q_strncmp("leader", level.teamVoteString[cs_offset], 6)) {
				//set the team leader
				SetLeader(team, atoi(level.teamVoteString[cs_offset] + 7));
			}
			else {
				trap_SendConsoleCommand(EXEC_APPEND, va("%s\n", level.teamVoteString[cs_offset]));
			}
		} else if (level.teamVoteNo[cs_offset] >= level.numteamVotingClients[cs_offset]/2) {
			// same behavior as a timeout
			trap_SetConfigstring(CS_TEAMVOTE_TIME + cs_offset, "failed");
			G_LogPrintf("Team vote failed.\n");
		} else {
			// still waiting for a majority
			return;
		}
	}

	level.teamVoteTime[cs_offset] = 0;
}

void CheckCvars(void)
{
	static int lastMod = -1;

	if (g_password.modificationCount != lastMod) {
		lastMod = g_password.modificationCount;
		if (*g_password.string && Q_stricmp(g_password.string, "none")) {
			trap_Cvar_Set("g_needpass", "1");
		} else {
			trap_Cvar_Set("g_needpass", "0");
		}
	}
}

/**
Runs thinking code for this frame if necessary
*/
void G_RunThink(gentity_t *ent)
{
	float	thinktime;

	thinktime = ent->nextthink;
	if (thinktime <= 0) {
		return;
	}

	if (thinktime > level.time) {
		return;
	}

	ent->nextthink = 0;
	if (!ent->think) {
		G_Error("NULL ent->think");
	}

	ent->think(ent);
}

/**
Advances the non-player objects in the world
*/
void G_RunFrame(int levelTime)
{
	int			i;
	gentity_t	*ent;

	// if we are waiting for the level to restart, do nothing
	if (level.restarted) {
		return;
	}

	level.framenum++;
	level.previousTime = level.time;
	level.time = levelTime;

	// get any cvar changes
	G_UpdateCvars();

	//
	// go through all allocated objects
	//
	ent = &g_entities[0];
	for (i = 0; i<level.num_entities; i++, ent++) {
		if (!ent->inuse) {
			continue;
		}

		// clear events that are too old
		if (level.time - ent->eventTime > EVENT_VALID_MSEC) {
			if (ent->s.event) {
				ent->s.event = 0;	// &= EV_EVENT_BITS;
				if (ent->client) {
					ent->client->ps.externalEvent = 0;
					// predicted events should never be set to zero
					//ent->client->ps.events[0] = 0;
					//ent->client->ps.events[1] = 0;
				}
			}
			if (ent->freeAfterEvent) {
				// tempEntities or dropped items completely go away after their event
				G_FreeEntity(ent);
				continue;
			} else if (ent->unlinkAfterEvent) {
				// items that will respawn will hide themselves after their pickup event
				ent->unlinkAfterEvent = qfalse;
				trap_UnlinkEntity(ent);
			}
		}

		// temporary entities don't think
		if (ent->freeAfterEvent) {
			continue;
		}

		if (!ent->r.linked && ent->neverFree) {
			continue;
		}

		if (ent->s.eType == ET_ITEM || ent->physicsObject) {
			G_RunItem(ent);
			continue;
		}

		if (ent->s.eType == ET_MOVER) {
			G_RunMover(ent);
			continue;
		}

		if (i < MAX_CLIENTS) {
			G_RunClient(ent);
			continue;
		}

		G_RunThink(ent);
	}

	// unlagged - backward reconciliation #2
	// NOW run the missiles, with all players backward-reconciled
	// to the positions they were in exactly 50ms ago, at the end
	// of the last server frame

	G_TimeShiftAllClients( level.previousTime, NULL );
	ent = &g_entities[0];
	for (i = 0; i<level.num_entities; i++, ent++) {
		if (ent->s.eType == ET_MISSILE) {
			G_RunMissile(ent);
			continue;
		}
	}
	G_UnTimeShiftAllClients(NULL);

	// end unlagged

	// perform final fixups on the players
	ent = &g_entities[0];
	for (i = 0; i < level.maxclients; i++, ent++) {
		if (ent->inuse) {
			ClientEndFrame(ent);
		}
	}

	// update game data that is sent to server
	for (i = 0; i < MAX_CLIENTS; ++i) {
		svps[i].score = level.clients[i].pers.score;
		svps[i].ping = level.clients[i].pers.ping;
		svps[i].team = level.clients[i].sess.sessionTeam;
	}

	CheckWarmup();

	CheckElimination();

	// see if it is time to end the level
	CheckExitRules();

	// update to team status?
	CheckTeamStatus();

	// cancel vote if timed out
	G_Vote_Check();

	// check team votes
	CheckTeamVote(TEAM_RED);
	CheckTeamVote(TEAM_BLUE);

	// for tracking changes
	CheckCvars();

	CheckPings(qfalse);

	CheckScoreboard();

	CheckStats();

	if (g_listEntity.integer) {
		for (i = 0; i < MAX_GENTITIES; i++) {
			G_Printf("%4i: %s\n", i, g_entities[i].classname);
		}
		trap_Cvar_Set("g_listEntity", "0");
	}

	// unlagged - backward reconciliation #4
	// record the time at the end of this frame - it should be about
	// the time the next frame begins - when the server starts
	// accepting commands from connected clients
	level.frameStartTime = trap_Milliseconds();
}

static void G_ScoreboardLine(char *buffer, int length, gclient_t *cl, qboolean cache)
{
	int	clientNum, powerups;

	clientNum = cl - level.clients;
	powerups = g_entities[cl - level.clients].s.powerups;

	switch (g_gametype.integer) {
	case GT_FFA:
		Com_sprintf(buffer, length, " %i %i %i", clientNum, cl->pers.score, powerups);
		break;
	case GT_TOURNAMENT:
		Com_sprintf(buffer, length, " %i %i %i %i %i %i", clientNum, cl->pers.score,
			cl->pers.stats.miscStats[MSTAT_YA], cl->pers.stats.miscStats[MSTAT_RA],
			cl->pers.stats.miscStats[MSTAT_MH], cl->pers.stats.miscStats[MSTAT_DAMAGE_DONE]);
		break;
	case GT_DEFRAG:
		Com_sprintf(buffer, length, " %i %i", clientNum, cl->pers.score);
		break;
	case GT_TEAM:
		Com_sprintf(buffer, length, " %i %i %i", clientNum, cl->pers.score, powerups);
		break;
	case GT_CTF:
		Com_sprintf(buffer, length, " %i %i %i", clientNum, cl->pers.score, powerups);
		break;
	case GT_ELIMINATION:
		Com_sprintf(buffer, length, " %i %i %i %i %i", clientNum, cl->eliminated,
			cl->pers.score, cl->pers.stats.miscStats[MSTAT_DAMAGE_DONE],
			cl->pers.stats.miscStats[MSTAT_DAMAGE_TAKEN]);
		break;
	default:
		return;
	}

	// scores did not change since last update
	if (!strcmp(buffer, cl->pers.scoreboardLine)) {
		buffer[0] = '\0';
		return;
	}

	if (cache) {
		Q_strncpyz(cl->pers.scoreboardLine, buffer, sizeof cl->pers.scoreboardLine);
	}
}

/**
Send the difference to the last sent scoreboard to the clients.
If client is NULL, send scores to all clients and write to cache.
Otherwise, send to specified client and to its followers and do not cache the scoreboard update.
*/
void G_SendScoreboard(gclient_t *client)
{
	int			i;
	qboolean	empty;
	char		line[sizeof client->pers.scoreboardLine];
	char		msg[2 + MAX_CLIENTS * sizeof line];
	gclient_t	*cl;

	empty = qtrue;
	strcpy(msg, "s");
	for (i = 0; i < level.maxclients; i++) {
		cl = &level.clients[i];
		if (cl->pers.connected != CON_CONNECTED || cl->sess.sessionTeam == TEAM_SPECTATOR) {
			continue;
		}

		G_ScoreboardLine(line, sizeof line, cl, !client);
		Q_strcat(msg, sizeof msg, line);
		if (*line) {
			empty = qfalse;
		}
	}

	if (empty) {
		return;
	}

	if (!client) {
		trap_SendServerCommand(-1, msg);
		return;
	}

	trap_SendServerCommand(client - level.clients, msg);

	// send to spectators following client
	for (i = 0; i < level.maxclients; ++i) {
		cl = &level.clients[i];
		if (cl->pers.connected != CON_CONNECTED) {
			continue;
		}
		if (cl->sess.spectatorState == SPECTATOR_FOLLOW
			&& cl->sess.spectatorClient == client - level.clients)
		{
			trap_SendServerCommand(i, msg);
		}
	}
}


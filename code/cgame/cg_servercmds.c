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
// cg_servercmds.c -- reliably sequenced text commands sent by the server
// these are processed at snapshot transition time, so there will definately
// be a valid snapshot this frame

#include "cg_local.h"

static void CG_ParseRespawnTimer(int index, const char *str)
{
	cgs.respawnTimer[index].item = atoi(COM_Parse((char **) &str));
	cgs.respawnTimer[index].nearestItem = atoi(COM_Parse((char **) &str));
	cgs.respawnTimer[index].time = 1000 * atoi(COM_Parse((char **) &str));
	cgs.respawnTimer[index].taker = atoi(COM_Parse((char **) &str));
	cgs.respawnTimer[index].ctfTeam = atoi(COM_Parse((char **) &str));
}

static void CG_ParseTeamInfo(void)
{
	int				i, argc;
	int				clientNum;
	clientInfo_t	*ci;

	argc = trap_Argc() - 1;

	i = 1;
	while (i < argc) {
		clientNum = atoi(BG_Argv(i++));
		if (clientNum < 0 || clientNum > MAX_CLIENTS - 1) {
			CG_Printf(S_COLOR_YELLOW "Invalid clientNum in tinfo.\n");
			continue;
		}
		ci = &cgs.clientinfo[clientNum];
		ci->location = atoi(BG_Argv(i++));
		ci->health = atoi(BG_Argv(i++));
		ci->armor = atoi(BG_Argv(i++));
		ci->weapon = atoi(BG_Argv(i++));
	}
}

static void CG_ParsePings(void)
{
	int	i, k;
	int	numClients;

	numClients = trap_Argc() - 1;
	if (numClients > MAX_CLIENTS) {
		numClients = MAX_CLIENTS;
	}

	for (i = 0, k = 0; i < numClients; ++i) {
		if (!cgs.clientinfo[k].infoValid) {
			continue;
		}
		if (cgs.clientinfo[k].botSkill) {
			cgs.clientinfo[k].ping = 0;
			continue;
		}
		cgs.clientinfo[k++].ping = atoi(BG_Argv(i + 1));
	}
}

static int CG_ComparePlayerNums(const void *num1, const void *num2)
{
	clientInfo_t	*a, *b;

	a = &cgs.clientinfo[*((const int *) num1)];
	b = &cgs.clientinfo[*((const int *) num2)];

	if (!a->infoValid) {
		return 1;
	} else if (!b->infoValid) {
		return -1;
	}

	if (!a->botSkill && a->ping == -1) {
		return -1;
	}
	if (!b->botSkill && b->ping == -1) {
		return 1;
	}

	if (a->team == TEAM_SPECTATOR && b->team == TEAM_SPECTATOR) {
		if (a->specOnly == b->specOnly) {
			if (a->enterTime > b->enterTime) {
				return -1;
			}
			if (a->enterTime < b->enterTime) {
				return 1;
			}

			// compare clientNum for correct queue number
			if (a > b) {
				return -1;
			} else if (b < a) {
				return 1;
			}
		}
		if (a->specOnly) {
			return 1;
		}
		if (b->specOnly) {
			return -1;
		}
		return 0;
	}

	if (cgs.gametype == GT_ELIMINATION) {
		if (a->damageDone > b->damageDone) {
			return -1;
		}
		if (a->damageDone < b->damageDone) {
			return 1;
		}
	}

	if (a->score > b->score) {
		return -1;
	}
	if (a->score < b->score) {
		return 1;
	}
	return 0;
}

static void CG_ParseScores(void)
{
	int				i, argc;
	int				clientNum;
	clientInfo_t	*ci;

	argc = trap_Argc() - 1;

	i = 1;
	while (i < argc) {
		clientNum = atoi(BG_Argv(i++));
		if (clientNum < 0 || clientNum > MAX_CLIENTS - 1) {
			CG_Printf(S_COLOR_YELLOW "Invalid score table.\n");
			continue;
		}
		ci = &cgs.clientinfo[clientNum];

		switch (cgs.gametype) {
		case GT_FFA:
			ci->score = atoi(BG_Argv(i++));
			ci->powerups = atoi(BG_Argv(i++));
			break;
		case GT_TOURNAMENT:
			ci->score = atoi(BG_Argv(i++));
			ci->yellowArmor = atoi(BG_Argv(i++));
			ci->redArmor = atoi(BG_Argv(i++));
			ci->megaHealth = atoi(BG_Argv(i++));
			ci->damageDone = atoi(BG_Argv(i++));
			break;
		case GT_DEFRAG:
			ci->score = atoi(BG_Argv(i++));
			break;
		case GT_TEAM:
			ci->score = atoi(BG_Argv(i++));
			ci->powerups = atoi(BG_Argv(i++));
			break;
		case GT_CTF:
			ci->score = atoi(BG_Argv(i++));
			ci->powerups = atoi(BG_Argv(i++));
			break;
		case GT_ELIMINATION:
			ci->eliminated = atoi(BG_Argv(i++));
			ci->score = atoi(BG_Argv(i++));
			ci->damageDone = atoi(BG_Argv(i++));
			ci->damageTaken = atoi(BG_Argv(i++));
			break;
		default:
			return;
		}
	}

	qsort(cg.sortedClients, MAX_CLIENTS, sizeof cg.sortedClients[0], CG_ComparePlayerNums);
}

static void CG_ParseStats(void)
{
	int				i, argc;
	int				*data;
	int				clientNum;
	playerStats_t	*stats;

	argc = trap_Argc() - 1;

	clientNum = atoi(BG_Argv(1));
	if (clientNum == cg.clientNum) {
		stats = &cg.statsOwn;
	} else if (clientNum == cg.snap->ps.clientNum) {
		stats = &cg.statsFollow;
	} else {
		stats = &cg.statsEnemy;
	}

	i = 1;
	while (i < argc) {
		data = BG_StatsData(stats, atoi(BG_Argv(++i)));
		if (!data) {
			CG_Printf("Invalid statstics table.\n");
			continue;
		}
		*data = atoi(BG_Argv(++i));
	}
}

static void CG_SetWarmup(int warmup)
{
	cg.warmupCount = -1;

	if (warmup > 0 && cg.warmup <= 0) {
		trap_S_StartLocalSound(cgs.media.countPrepareSound, CHAN_ANNOUNCER);
	}

	cg.warmup = warmup;
}

static void CG_ConfigStringModified(void)
{
	const char	*str;
	int		num;

	num = atoi(BG_Argv(1));

	// get the gamestate from the client system, which will have the
	// new configstring already integrated
	trap_GetGameState(&cgs.gameState);

	// look up the individual string that was modified
	str = CG_ConfigString(num);

	// do something with it if necessary
	if (num == CS_SERVERINFO) {
		CG_ParseServerinfo();
	} else if (num == CS_WARMUP) {
		CG_SetWarmup(atoi(str));
	} else if (num == CS_SCORES1) {
		cgs.scores1 = atoi(str);
	} else if (num == CS_SCORES2) {
		cgs.scores2 = atoi(str);
	} else if (num == CS_LEVEL_START_TIME) {
		cgs.levelStartTime = atoi(str);
	} else if (num == CS_VOTE_TIME) {
		if (!strcmp(str, "failed")) {
			cgs.voteTime = 0;
			CG_Printf("Vote failed.\n");
			CG_AddBufferedSound(cgs.media.voteFailed);
		} else if (!strcmp(str, "passed")) {
			cgs.voteTime = 0;
			CG_Printf("Vote passed.\n");
			CG_AddBufferedSound(cgs.media.votePassed);
		} else if (*str) {
			cgs.voteTime = atoi(str);
			CG_Printf("Vote cast.\n");
			CG_AddBufferedSound(cgs.media.voteNow);
		}
	} else if (num == CS_VOTE_YES) {
		cgs.voteYes = atoi(str);
		trap_S_StartLocalSound(cgs.media.talkSound, CHAN_LOCAL_SOUND);
	} else if (num == CS_VOTE_NO) {
		cgs.voteNo = atoi(str);
		trap_S_StartLocalSound(cgs.media.talkSound, CHAN_LOCAL_SOUND);
	} else if (num == CS_VOTE_STRING) {
		Q_strncpyz(cgs.voteString, str, sizeof(cgs.voteString));
	} else if (num >= CS_TEAMVOTE_TIME && num <= CS_TEAMVOTE_TIME + 1) {
		if (!strcmp(str, "failed")) {
			cgs.teamVoteTime[num - CS_TEAMVOTE_TIME] = 0;
			CG_Printf("Team vote failed.\n");
			CG_AddBufferedSound(cgs.media.voteFailed);
		} else if (!strcmp(str, "passed")) {
			cgs.teamVoteTime[num - CS_TEAMVOTE_TIME] = 0;
			CG_Printf("Team vote passed.\n");
			CG_AddBufferedSound(cgs.media.votePassed);
		} else if (*str) {
			cgs.teamVoteTime[num - CS_TEAMVOTE_TIME] = atoi(str);
			CG_Printf("Team vote cast.\n");
			CG_AddBufferedSound(cgs.media.voteNow);
		}
	} else if (num >= CS_TEAMVOTE_YES && num <= CS_TEAMVOTE_YES + 1) {
		cgs.teamVoteYes[num - CS_TEAMVOTE_YES] = atoi(str);
		trap_S_StartLocalSound(cgs.media.talkSound, CHAN_LOCAL_SOUND);
	} else if (num >= CS_TEAMVOTE_NO && num <= CS_TEAMVOTE_NO + 1) {
		cgs.teamVoteNo[num - CS_TEAMVOTE_NO] = atoi(str);
		trap_S_StartLocalSound(cgs.media.talkSound, CHAN_LOCAL_SOUND);
	} else if (num >= CS_TEAMVOTE_STRING && num <= CS_TEAMVOTE_STRING + 1) {
		Q_strncpyz(cgs.teamVoteString[num-CS_TEAMVOTE_STRING], str, sizeof(cgs.teamVoteString));
	} else if (num == CS_INTERMISSION) {
		cg.intermissionStarted = atoi(str);
	} else if (num >= CS_MODELS && num < CS_MODELS+MAX_MODELS) {
		cgs.gameModels[num-CS_MODELS] = trap_R_RegisterModel(str);
	} else if (num >= CS_SOUNDS && num < CS_SOUNDS+MAX_SOUNDS) {
		if (str[0] != '*') {	// player specific sounds don't register here
			cgs.gameSounds[num-CS_SOUNDS] = trap_S_RegisterSound(str, qfalse);
		}
	} else if (num >= CS_PLAYERS && num < CS_PLAYERS+MAX_CLIENTS) {
		CG_NewClientInfo(num - CS_PLAYERS);
	} else if (num == CS_FLAGSTATUS) {
		// format is rb where its red/blue, 0 is at base, 1 is taken, 2 is dropped
		if (cgs.gametype == GT_CTF) {
			team_t	team;
			team = cgs.clientinfo[cg.clientNum].team;
			if ((cgs.redflag == 1 && str[0] - '0' == 2 && team == TEAM_RED)
				|| (cgs.blueflag == 1 && str[1] - '0' == 2 && team == TEAM_BLUE))
			{
				CG_CenterPrint("^2The enemy dropped your flag");
			} else if ((cgs.redflag == 1 && str[0] - '0' == 2 && team == TEAM_BLUE)
				|| (cgs.blueflag == 1 && str[1] - '0' == 2 && team == TEAM_RED))
			{
				CG_CenterPrint("^1You team dropped the flag");
			}

			cgs.redflag = str[0] - '0';
			cgs.blueflag = str[1] - '0';
		}
	} else if (num == CS_SHADERSTATE) {
		CG_ShaderStateChanged();
	} else if (num == CS_ROUND_START) {
		cg.warmupCount = -1;
		cgs.roundStartTime = atoi(str);
	} else if (num == CS_LIVING_COUNT) {
		int		newRed, newBlue;

		newRed = atoi(COM_Parse((char **) &str));
		newBlue = atoi(COM_Parse((char **) &str));

		if (!cg.warmup && cg.time >= cgs.roundStartTime) {
			team_t	team;
			team = cg.snap->ps.persistant[PERS_TEAM];
			if ((newRed == 1 && cgs.redLivingCount != 1 && team == TEAM_RED)
				|| (newBlue == 1 && cgs.blueLivingCount != 1 && team == TEAM_BLUE))
			{
				CG_CenterPrint("You are the last in your team");
			}
		}

		cgs.redLivingCount = newRed;
		cgs.blueLivingCount = newBlue;
	} else if (num == CS_OVERTIME) {
		cgs.overtimeStart = atoi(str);
		trap_S_StartLocalSound(cgs.media.protectSound, CHAN_ANNOUNCER);
		CG_CenterPrint(va("^3OVERTIME^7 - %d seconds added", cgs.overtimeLimit));
	} else if (num >= CS_RESPAWN_TIMERS && num < CS_RESPAWN_TIMERS + MAX_RESPAWN_TIMERS) {
		CG_ParseRespawnTimer(num - CS_RESPAWN_TIMERS, str);
	}
}

static void CG_AddChatMessage(msgItem_t list[CHAT_HEIGHT + 1], const char *str)
{
	int	i;
	for (i = CHAT_HEIGHT - 1; i > 0; --i) {
		list[i] = list[i - 1];
	}
	list[i].time = cg.time;
	Q_strncpyz(list[i].message, str, sizeof list[i].message);
}

/**
The server has issued a map_restart, so the next snapshot
is completely new and should not be interpolated to.

A tournament restart will clear everything, but doesn't
require a reload of all the media
*/
static void CG_MapRestart(void)
{
	if (cg_showmiss.integer) {
		CG_Printf("CG_MapRestart\n");
	}

	CG_InitLocalEntities();
	CG_InitMarkPolys();
	CG_ClearParticles();

	// make sure the "3 frags left" warnings play again
	cg.fraglimitWarnings = 0;

	cg.timelimitWarnings = 0;
	cg.rewardTime = 0;
	cg.rewardStack = 0;
	cg.intermissionStarted = qfalse;
	cg.levelShot = qfalse;

	memset(&cg.statsOwn, 0, sizeof cg.statsOwn);
	memset(&cg.statsFollow, 0, sizeof cg.statsFollow);
	memset(&cg.statsEnemy, 0, sizeof cg.statsEnemy);

	cgs.voteTime = 0;

	cg.mapRestart = qtrue;

	trap_S_ClearLoopingSounds(qtrue);

	// we really should clear more parts of cg here and stop sounds

	// play the "fight" sound if this is a restart without warmup
	if (cg.warmup == 0) {
		trap_S_StartLocalSound(cgs.media.countFightSound, CHAN_ANNOUNCER);
		CG_CenterPrint("Fight!");
	}

	trap_Cvar_Set("cg_thirdPerson", "0");
}

static void CG_RemoveChatEscapeChar(char *text)
{
	int i, l;

	l = 0;
	for (i = 0; text[i]; i++) {
		if (text[i] == '\x19')
			continue;
		text[l++] = text[i];
	}
	text[l] = '\0';
}

/**
The string has been tokenized and can be retrieved with
Cmd_Argc() / Cmd_Argv()
*/
static void CG_ServerCommand(void)
{
	const char	*cmd;
	char		text[MAX_SAY_TEXT];

	cmd = BG_Argv(0);

	if (!cmd[0]) {
		// server claimed the command
		return;
	}

	if (!strcmp(cmd, "cp")) {
		CG_CenterPrint(BG_Argv(1));
		return;
	}

	if (!strcmp(cmd, "cs")) {
		CG_ConfigStringModified();
		return;
	}

	if (!strcmp(cmd, "pings")) {
		CG_ParsePings();
		return;
	}

	if (!strcmp(cmd, "tinfo")) {
		CG_ParseTeamInfo();
	}

	if (!strcmp(cmd, "print")) {
		CG_Printf("%s", BG_Argv(1));
		return;
	}

	if (!strcmp(cmd, "s")) {
		CG_ParseScores();
		return;
	}

	if (!strcmp(cmd, "stats")) {
		CG_ParseStats();
		return;
	}

	if (!strcmp(cmd, "chat")) {
		if (cg_teamChatsOnly.integer) {
			return;
		}
		trap_S_StartLocalSound(cgs.media.talkSound, CHAN_LOCAL_SOUND);
		Q_strncpyz(text, BG_Argv(1), MAX_SAY_TEXT);
		CG_RemoveChatEscapeChar(text);
		CG_AddChatMessage(cgs.chatMessages, text);
		CG_Printf("%s\n", text);
		return;
	}

	if (!strcmp(cmd, "tchat")) {
		trap_S_StartLocalSound(cgs.media.talkSound, CHAN_LOCAL_SOUND);
		Q_strncpyz(text, BG_Argv(1), MAX_SAY_TEXT);
		CG_RemoveChatEscapeChar(text);
		CG_AddChatMessage(cgs.teamChatMessages, text);
		CG_Printf("%s\n", text);
		return;
	}

	if (!strcmp(cmd, "map_restart")) {
		CG_MapRestart();
		return;
	}

	if (!strcmp(cmd, "remapShader") == 0) {
		if (trap_Argc() == 4) {
			char shader1[MAX_QPATH];
			char shader2[MAX_QPATH];
			char shader3[MAX_QPATH];

			Q_strncpyz(shader1, BG_Argv(1), sizeof(shader1));
			Q_strncpyz(shader2, BG_Argv(2), sizeof(shader2));
			Q_strncpyz(shader3, BG_Argv(3), sizeof(shader3));

			trap_R_RemapShader(shader1, shader2, shader3);
		}

		return;
	}

	// clientLevelShot is sent before taking a special screenshot for
	// the menu system during development
	if (!strcmp(cmd, "clientLevelShot")) {
		cg.levelShot = qtrue;
		return;
	}

	CG_Printf("Unknown client game command: %s\n", cmd);
}

/**
This is called explicitly when the gamestate is first received,
and whenever the server updates any serverinfo flagged cvars
*/
void CG_ParseServerinfo(void)
{
	const char	*info;
	char		*mapname;

	info = CG_ConfigString(CS_SERVERINFO);
	cgs.gametype = atoi(Info_ValueForKey(info, "g_gametype"));
	trap_Cvar_Set("g_gametype", va("%i", cgs.gametype));
	cgs.dmflags = atoi(Info_ValueForKey(info, "dmflags"));
	cgs.teamflags = atoi(Info_ValueForKey(info, "teamflags"));
	cgs.fraglimit = atoi(Info_ValueForKey(info, "g_fraglimit"));
	cgs.capturelimit = atoi(Info_ValueForKey(info, "g_capturelimit"));
	cgs.timelimit = atoi(Info_ValueForKey(info, "g_timelimit"));
	cgs.roundTimelimit = atoi(Info_ValueForKey(info, "g_roundTimelimit"));
	cgs.maxclients = atoi(Info_ValueForKey(info, "sv_maxclients"));
	cgs.newItemHeight = atoi(Info_ValueForKey(info, "g_newItemHeight"));
	cgs.startWhenReady = atoi(Info_ValueForKey(info, "g_startWhenReady"));
	cgs.redLocked = atoi(Info_ValueForKey(info, "g_redLocked"));
	cgs.blueLocked = atoi(Info_ValueForKey(info, "g_blueLocked"));
	cgs.friendsThroughWalls = atoi(Info_ValueForKey(info, "g_friendsThroughWalls"));
	cgs.overtimeLimit = atoi(Info_ValueForKey(info, "g_overtime"));
	cgs.allowRespawnTimer = atoi(Info_ValueForKey(info, "g_allowRespawnTimer"));

	mapname = Info_ValueForKey(info, "mapname");
	Com_sprintf(cgs.mapname, sizeof cgs.mapname, "maps/%s.bsp", mapname);

	switch (cgs.gametype) {
	case GT_FFA:
		cgs.gametypeName = "Free For All";
		cgs.gametypeShortName = "FFA";
		break;
	case GT_TOURNAMENT:
		cgs.gametypeName = "Tournament";
		cgs.gametypeShortName = "1v1";
		break;
	case GT_DEFRAG:
		cgs.gametypeName = "Defrag";
		cgs.gametypeShortName = "DF";
		break;
	case GT_TEAM:
		cgs.gametypeName = "Team Deathmatch";
		cgs.gametypeShortName = "TDM";
		break;
	case GT_CTF:
		cgs.gametypeName = "Capture The Flag";
		cgs.gametypeShortName = "CTF";
		break;
	case GT_ELIMINATION:
		cgs.gametypeName = "Elimination";
		cgs.gametypeShortName = "CA";
		break;
	default:
		cgs.gametypeName = "Unknown gametype";
		cgs.gametypeShortName = "???";
	}
}

/**
Called on load to set the initial values from configure strings
*/
void CG_SetConfigValues(void)
{
	int i;
	const char *s;

	cgs.scores1 = atoi(CG_ConfigString(CS_SCORES1));
	cgs.scores2 = atoi(CG_ConfigString(CS_SCORES2));
	cgs.levelStartTime = atoi(CG_ConfigString(CS_LEVEL_START_TIME));
	if (cgs.gametype == GT_CTF) {
		s = CG_ConfigString(CS_FLAGSTATUS);
		cgs.redflag = s[0] - '0';
		cgs.blueflag = s[1] - '0';
	}

	cg.warmup = atoi(CG_ConfigString(CS_WARMUP));

	for (i = 0; i < MAX_RESPAWN_TIMERS; ++i) {
		CG_ParseRespawnTimer(i, CG_ConfigString(CS_RESPAWN_TIMERS + i));
	}
}

void CG_ShaderStateChanged(void)
{
	char originalShader[MAX_QPATH];
	char newShader[MAX_QPATH];
	char timeOffset[16];
	const char *o;
	char *n,*t;

	o = CG_ConfigString(CS_SHADERSTATE);
	while (o && *o) {
		n = strstr(o, "=");
		if (n && *n) {
			strncpy(originalShader, o, n-o);
			originalShader[n-o] = 0;
			n++;
			t = strstr(n, ":");
			if (t && *t) {
				strncpy(newShader, n, t-n);
				newShader[t-n] = 0;
			} else {
				break;
			}
			t++;
			o = strstr(t, "@");
			if (o) {
				strncpy(timeOffset, t, o-t);
				timeOffset[o-t] = 0;
				o++;
				trap_R_RemapShader(originalShader, newShader, timeOffset);
			}
		} else {
			break;
		}
	}
}

/**
Execute all of the server commands that were received along
with this this snapshot.
*/
void CG_ExecuteNewServerCommands(int latestSequence)
{
	while (cgs.serverCommandSequence < latestSequence) {
		if (trap_GetServerCommand(++cgs.serverCommandSequence)) {
			CG_ServerCommand();
		}
	}
}


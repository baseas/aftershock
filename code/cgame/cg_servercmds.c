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
		cgs.clientinfo[k++].ping = atoi(CG_Argv(i + 1));
	}
}

static void CG_ParseWarmup(void)
{
	int			warmup;

	warmup = atoi(CG_ConfigString(CS_WARMUP));
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

	num = atoi(CG_Argv(1));

	// get the gamestate from the client system, which will have the
	// new configstring already integrated
	trap_GetGameState(&cgs.gameState);

	// look up the individual string that was modified
	str = CG_ConfigString(num);

	// do something with it if necessary
	if (num == CS_SERVERINFO) {
		CG_ParseServerinfo();
	} else if (num == CS_WARMUP) {
		CG_ParseWarmup();
	} else if (num == CS_SCORES1) {
		cgs.scores1 = atoi(str);
	} else if (num == CS_SCORES2) {
		cgs.scores2 = atoi(str);
	} else if (num == CS_LEVEL_START_TIME) {
		cgs.levelStartTime = atoi(str);
	} else if (num == CS_VOTE_TIME) {
		cgs.voteTime = atoi(str);
		cgs.voteModified = qtrue;
	} else if (num == CS_VOTE_YES) {
		cgs.voteYes = atoi(str);
		cgs.voteModified = qtrue;
	} else if (num == CS_VOTE_NO) {
		cgs.voteNo = atoi(str);
		cgs.voteModified = qtrue;
	} else if (num == CS_VOTE_STRING) {
		Q_strncpyz(cgs.voteString, str, sizeof(cgs.voteString));
	} else if (num >= CS_TEAMVOTE_TIME && num <= CS_TEAMVOTE_TIME + 1) {
		cgs.teamVoteTime[num-CS_TEAMVOTE_TIME] = atoi(str);
		cgs.teamVoteModified[num-CS_TEAMVOTE_TIME] = qtrue;
	} else if (num >= CS_TEAMVOTE_YES && num <= CS_TEAMVOTE_YES + 1) {
		cgs.teamVoteYes[num-CS_TEAMVOTE_YES] = atoi(str);
		cgs.teamVoteModified[num-CS_TEAMVOTE_YES] = qtrue;
	} else if (num >= CS_TEAMVOTE_NO && num <= CS_TEAMVOTE_NO + 1) {
		cgs.teamVoteNo[num-CS_TEAMVOTE_NO] = atoi(str);
		cgs.teamVoteModified[num-CS_TEAMVOTE_NO] = qtrue;
	} else if (num >= CS_TEAMVOTE_STRING && num <= CS_TEAMVOTE_STRING + 1) {
		Q_strncpyz(cgs.teamVoteString[num-CS_TEAMVOTE_STRING], str, sizeof(cgs.teamVoteString));
	} else if (num == CS_INTERMISSION) {
		cg.intermissionStarted = atoi(str);
	} else if (num >= CS_MODELS && num < CS_MODELS+MAX_MODELS) {
		cgs.gameModels[ num-CS_MODELS ] = trap_R_RegisterModel(str);
	} else if (num >= CS_SOUNDS && num < CS_SOUNDS+MAX_SOUNDS) {
		if (str[0] != '*') {	// player specific sounds don't register here
			cgs.gameSounds[ num-CS_SOUNDS] = trap_S_RegisterSound(str, qfalse);
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
	CG_ClearParticles ();

	// make sure the "3 frags left" warnings play again
	cg.fraglimitWarnings = 0;

	cg.timelimitWarnings = 0;
	cg.rewardTime = 0;
	cg.rewardStack = 0;
	cg.intermissionStarted = qfalse;
	cg.levelShot = qfalse;

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

	cmd = CG_Argv(0);

	if (!cmd[0]) {
		// server claimed the command
		return;
	}

	if (!strcmp(cmd, "cp")) {
		CG_CenterPrint(CG_Argv(1));
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

	if (!strcmp(cmd, "print")) {
		CG_Printf("%s", CG_Argv(1));
		return;
	}

	if (!strcmp(cmd, "chat")) {
		if (cg_teamChatsOnly.integer) {
			return;
		}
		trap_S_StartLocalSound(cgs.media.talkSound, CHAN_LOCAL_SOUND);
		Q_strncpyz(text, CG_Argv(1), MAX_SAY_TEXT);
		CG_RemoveChatEscapeChar(text);
		CG_AddChatMessage(cgs.chatMessages, text);
		CG_Printf("%s\n", text);
		return;
	}

	if (!strcmp(cmd, "tchat")) {
		trap_S_StartLocalSound(cgs.media.talkSound, CHAN_LOCAL_SOUND);
		Q_strncpyz(text, CG_Argv(1), MAX_SAY_TEXT);
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

			Q_strncpyz(shader1, CG_Argv(1), sizeof(shader1));
			Q_strncpyz(shader2, CG_Argv(2), sizeof(shader2));
			Q_strncpyz(shader3, CG_Argv(3), sizeof(shader3));

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
	cgs.fraglimit = atoi(Info_ValueForKey(info, "fraglimit"));
	cgs.capturelimit = atoi(Info_ValueForKey(info, "capturelimit"));
	cgs.timelimit = atoi(Info_ValueForKey(info, "timelimit"));
	cgs.roundTimelimit = atoi(Info_ValueForKey(info, "g_roundTimelimit"));
	cgs.maxclients = atoi(Info_ValueForKey(info, "sv_maxclients"));
	cgs.newItemHeight = atoi(Info_ValueForKey(info, "g_newItemHeight"));
	cgs.startWhenReady = atoi(Info_ValueForKey(info, "g_startWhenReady"));
	cgs.redLocked = atoi(Info_ValueForKey(info, "g_redLocked"));
	cgs.blueLocked = atoi(Info_ValueForKey(info, "g_blueLocked"));

	mapname = Info_ValueForKey(info, "mapname");
	Com_sprintf(cgs.mapname, sizeof cgs.mapname, "maps/%s.bsp", mapname);

	switch (cgs.gametype) {
	case GT_FFA:
		cgs.gametypeName = "Free For All";
		cgs.gametypeShortName = "FFA";
		break;
	case GT_SINGLE_PLAYER:
		cgs.gametypeName = "Single Player";
		cgs.gametypeShortName = "SP";
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


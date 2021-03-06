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
// cg_consolecmds.c -- text commands typed in at the local console, or
// executed by a key binding

#include "cg_local.h"

typedef struct {
	char	*cmd;
	void	(*function)(void);
} consoleCommand_t;

/**
Debugging command to print the current position
*/
static void CG_Viewpos_f(void)
{
	CG_Printf("(%g %g %g) : %g\n", cg.refdef.vieworg[0], cg.refdef.vieworg[1],
		cg.refdef.vieworg[2], cg.refdefViewAngles[YAW]);
}

static void CG_ScoresDown_f(void)
{
	cg.showScores = qtrue;
	cg.showInfo = qfalse;
}

static void CG_ScoresUp_f(void)
{
	cg.showScores = qfalse;
}

static void CG_InfoDown_f(void)
{
	cg.showInfo = qtrue;
}

static void CG_InfoUp_f(void)
{
	cg.showInfo = qfalse;
}

static void CG_StatsDown_f(void)
{
	cg.showStats = qtrue;
}

static void CG_StatsUp_f(void)
{
	cg.showStats = qfalse;
}

static void CG_ChatUp_f(void)
{
	cg.showChat = qtrue;
}

static void CG_ChatDown_f(void)
{
	cg.showChat = qfalse;
}

static void CG_TellTarget_f(void)
{
	int		clientNum;
	char	command[128];
	char	message[128];

	clientNum = CG_CrosshairPlayer();
	if (clientNum == -1) {
		return;
	}

	trap_Args(message, 128);
	Com_sprintf(command, 128, "tell %i %s", clientNum, message);
	trap_SendClientCommand(command);
}

static void CG_TellAttacker_f(void)
{
	int		clientNum;
	char	command[128];
	char	message[128];

	clientNum = CG_LastAttacker();
	if (clientNum == -1) {
		return;
	}

	trap_Args(message, 128);
	Com_sprintf(command, 128, "tell %i %s", clientNum, message);
	trap_SendClientCommand(command);
}

static void CG_StartOrbit_f(void)
{
	char var[MAX_TOKEN_CHARS];

	trap_Cvar_VariableStringBuffer("developer", var, sizeof(var));
	if (!atoi(var)) {
		return;
	}
	if (cg_cameraOrbit.value != 0) {
		trap_Cvar_Set ("cg_cameraOrbit", "0");
		trap_Cvar_Set("cg_thirdPerson", "0");
	} else {
		trap_Cvar_Set("cg_cameraOrbit", "5");
		trap_Cvar_Set("cg_thirdPerson", "1");
		trap_Cvar_Set("cg_thirdPersonAngle", "0");
		trap_Cvar_Set("cg_thirdPersonRange", "100");
	}
}

/**
Create a cvar that teleports to the current position when executed.
Example: savepos pos1; vstr pos1;
*/
static void CG_Savepos_f(void)
{
	if (trap_Argc() != 2) {
		CG_Printf("Usage: savepos <cvar name>\n");
		return;
	}

	trap_Cvar_Set(BG_Argv(1), va("setviewpos %g %g %g %g", cg.predictedPlayerState.origin[0],
		cg.predictedPlayerState.origin[1], cg.predictedPlayerState.origin[2],
		cg.predictedPlayerState.viewangles[YAW]));
}

static void CG_MessageMode_f(void)
{
	cgs.activeChat = SAY_ALL;
	MField_Clear(&cgs.chatField);
	trap_Key_SetCatcher(KEYCATCH_CGAME);
}

static void CG_MessageMode2_f(void)
{
	cgs.activeChat = SAY_TEAM;
	MField_Clear(&cgs.chatField);
	trap_Key_SetCatcher(KEYCATCH_CGAME);
}

static void CG_MessageMode3_f(void)
{
	cgs.activeChat = SAY_TELL;
	cgs.chatPlayerNum = CG_CrosshairPlayer();
	MField_Clear(&cgs.chatField);
	trap_Key_SetCatcher(KEYCATCH_CGAME);
}

static void CG_MessageMode4_f(void)
{
	cgs.activeChat = SAY_TELL;
	cgs.chatPlayerNum = CG_LastAttacker();
	MField_Clear(&cgs.chatField);
	trap_Key_SetCatcher(KEYCATCH_CGAME);
}

static consoleCommand_t	commands[] = {
	{ "testgun", CG_TestGun_f },
	{ "testmodel", CG_TestModel_f },
	{ "nextframe", CG_TestModelNextFrame_f },
	{ "prevframe", CG_TestModelPrevFrame_f },
	{ "nextskin", CG_TestModelNextSkin_f },
	{ "prevskin", CG_TestModelPrevSkin_f },
	{ "viewpos", CG_Viewpos_f },
	{ "+scores", CG_ScoresDown_f },
	{ "-scores", CG_ScoresUp_f },
	{ "+info", CG_InfoDown_f },
	{ "-info", CG_InfoUp_f },
	{ "+stats", CG_StatsDown_f },
	{ "-stats", CG_StatsUp_f },
	{ "+zoom", CG_ZoomDown_f },
	{ "-zoom", CG_ZoomUp_f },
	{ "+chat", CG_ChatUp_f },
	{ "-chat", CG_ChatDown_f },
	{ "weapnext", CG_NextWeapon_f },
	{ "weapprev", CG_PrevWeapon_f },
	{ "weapon", CG_Weapon_f },
	{ "tell_target", CG_TellTarget_f },
	{ "tell_attacker", CG_TellAttacker_f },
	{ "startOrbit", CG_StartOrbit_f },
	{ "hud", CG_Hud_f },
	{ "savepos", CG_Savepos_f },
	{ "messagemode", CG_MessageMode_f },
	{ "messagemode2", CG_MessageMode2_f },
	{ "messagemode3", CG_MessageMode3_f },
	{ "messagemode4", CG_MessageMode4_f }
};

/**
The string has been tokenized and can be retrieved with Cmd_Argc() / Cmd_Argv()
*/
qboolean CG_ConsoleCommand(void)
{
	const char	*cmd;
	int			i;

	cmd = BG_Argv(0);

	for (i = 0; i < ARRAY_LEN(commands); i++) {
		if (!Q_stricmp(cmd, commands[i].cmd)) {
			commands[i].function();
			return qtrue;
		}
	}

	return qfalse;
}

/**
Let the client system know about all of our commands
so it can perform tab completion
*/
void CG_InitConsoleCommands(void)
{
	int		i;

	for (i = 0; i < ARRAY_LEN(commands); i++) {
		trap_AddCommand(commands[i].cmd);
	}

	//
	// the game server will interpret these commands, which will be automatically
	// forwarded to the server after they are not recognized locally
	//
	trap_AddCommand("kill");
	trap_AddCommand("say");
	trap_AddCommand("say_team");
	trap_AddCommand("tell");
	trap_AddCommand("give");
	trap_AddCommand("god");
	trap_AddCommand("notarget");
	trap_AddCommand("noclip");
	trap_AddCommand("where");
	trap_AddCommand("team");
	trap_AddCommand("follow");
	trap_AddCommand("follownext");
	trap_AddCommand("followprev");
	trap_AddCommand("levelshot");
	trap_AddCommand("setviewpos");
	trap_AddCommand("callvote");
	trap_AddCommand("vote");
	trap_AddCommand("teamtask");
	trap_AddCommand("dropammo");
	trap_AddCommand("droparmor");
	trap_AddCommand("drophealth");
	trap_AddCommand("dropweapon");
	trap_AddCommand("dropflag");
	trap_AddCommand("drop");
	trap_AddCommand("ready");
	trap_AddCommand("lock");
	trap_AddCommand("unlock");
	trap_AddCommand("forfeit");
	trap_AddCommand("block");
	trap_AddCommand("unblock");
	trap_AddCommand("gamedata");
	trap_AddCommand("login");
	trap_AddCommand("rename");
	trap_AddCommand("kick");
	trap_AddCommand("ban");
	trap_AddCommand("timein");
	trap_AddCommand("timeout");
	trap_AddCommand("pause");
	trap_AddCommand("unpause");
	trap_AddCommand("put");
	trap_AddCommand("mute");
	trap_AddCommand("unmute");
	trap_AddCommand("addbot");
	trap_AddCommand("botlist");
	trap_AddCommand("allready");
	trap_AddCommand("usertest");
	trap_AddCommand("user");
	trap_AddCommand("mapcycle");
}


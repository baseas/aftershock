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
// cg_main.c -- initialization and primary entry point for cgame

#include "cg_local.h"

#define FLT_MIN		-1e5
#define FLT_MAX		+1e5

typedef enum {
	RANGE_TYPE_ALL,
	RANGE_TYPE_INT,
	RANGE_TYPE_FLOAT,
	RANGE_TYPE_COLOR
} rangeType_t;

typedef struct {
	vmCvar_t	*vmCvar;
	char		*cvarName;
	char		*defaultString;
	int			cvarFlags;
	int			min;
	int			max;
	rangeType_t	rangeType;
} cvarTable_t;

#define RANGE_ALL				0, 0, RANGE_TYPE_ALL
#define RANGE_BOOL				0, 1, RANGE_TYPE_INT
#define RANGE_INT(min, max)		min, max, RANGE_TYPE_INT
#define RANGE_FLOAT(min, max)	min, max, RANGE_TYPE_FLOAT
#define RANGE_COLOR				0, 0, RANGE_TYPE_COLOR

static void	CG_Init(int serverMessageNum, int serverCommandSequence, int clientNum);
static void	CG_Shutdown(void);
static void	CG_KeyEvent(int key, qboolean down);
static void	CG_MouseEvent(int x, int y);

/**
This is the only way control passes into the module.
This must be the very first function compiled into the .q3vm file
*/
Q_EXPORT intptr_t vmMain(int command, int arg0, int arg1, int arg2, int arg3, int arg4,
	int arg5, int arg6, int arg7, int arg8, int arg9, int arg10, int arg11)
{
	switch (command) {
	case CG_INIT:
		CG_Init(arg0, arg1, arg2);
		return 0;
	case CG_SHUTDOWN:
		CG_Shutdown();
		return 0;
	case CG_CONSOLE_COMMAND:
		return CG_ConsoleCommand();
	case CG_DRAW_ACTIVE_FRAME:
		CG_DrawActiveFrame(arg0, arg1, arg2);
		return 0;
	case CG_CROSSHAIR_PLAYER:
		return CG_CrosshairPlayer();
	case CG_LAST_ATTACKER:
		return CG_LastAttacker();
	case CG_KEY_EVENT:
		CG_KeyEvent(arg0, arg1);
		return 0;
	case CG_MOUSE_EVENT:
		CG_MouseEvent(arg0, arg1);
		return 0;
	case CG_EVENT_HANDLING:
		CG_EventHandling(arg0);
		return 0;
	default:
		CG_Error("vmMain: unknown command %i", command);
		break;
	}

	return -1;
}

cg_t			cg;
cgs_t			cgs;
centity_t		cg_entities[MAX_GENTITIES];
weaponInfo_t	cg_weapons[MAX_WEAPONS];
itemInfo_t		cg_items[MAX_ITEMS];

vmCvar_t	cg_railTrailTime;
vmCvar_t	cg_centertime;
vmCvar_t	cg_swingSpeed;
vmCvar_t	cg_shadows;
vmCvar_t	cg_gibs;
vmCvar_t	cg_drawTimer;
vmCvar_t	cg_drawFPS;
vmCvar_t	cg_drawSnapshot;
vmCvar_t	cg_draw3dIcons;
vmCvar_t	cg_drawIcons;
vmCvar_t	cg_drawAmmoWarning;
vmCvar_t	cg_drawCrosshair;
vmCvar_t	cg_drawCrosshairNames;
vmCvar_t	cg_drawRewards;
vmCvar_t	cg_crosshairSize;
vmCvar_t	cg_crosshairX;
vmCvar_t	cg_crosshairY;
vmCvar_t	cg_crosshairHealth;
vmCvar_t	cg_draw2D;
vmCvar_t	cg_drawStatus;
vmCvar_t	cg_animSpeed;
vmCvar_t	cg_debugAnim;
vmCvar_t	cg_debugPosition;
vmCvar_t	cg_debugEvents;
vmCvar_t	cg_errorDecay;
vmCvar_t	cg_nopredict;
vmCvar_t	cg_noPlayerAnims;
vmCvar_t	cg_showmiss;
vmCvar_t	cg_footsteps;
vmCvar_t	cg_addMarks;
vmCvar_t	cg_brassTime;
vmCvar_t	cg_viewsize;
vmCvar_t	cg_drawGun;
vmCvar_t	cg_gun_frame;
vmCvar_t	cg_gun_x;
vmCvar_t	cg_gun_y;
vmCvar_t	cg_gun_z;
vmCvar_t	cg_tracerChance;
vmCvar_t	cg_tracerWidth;
vmCvar_t	cg_tracerLength;
vmCvar_t	cg_autoswitch;
vmCvar_t	cg_ignore;
vmCvar_t	cg_simpleItems;
vmCvar_t	cg_fov;
vmCvar_t	cg_zoomFov;
vmCvar_t	cg_thirdPerson;
vmCvar_t	cg_thirdPersonRange;
vmCvar_t	cg_thirdPersonAngle;
vmCvar_t	cg_lagometer;
vmCvar_t	cg_drawAttacker;
vmCvar_t	cg_synchronousClients;
vmCvar_t	cg_teamChatTime;
vmCvar_t	cg_teamChatHeight;
vmCvar_t	cg_stats;
vmCvar_t	cg_buildScript;
vmCvar_t	cg_paused;
vmCvar_t	cg_blood;
vmCvar_t	cg_predictItems;
vmCvar_t	cg_drawTeamOverlay;
vmCvar_t	cg_teamOverlayUserinfo;
vmCvar_t	cg_drawFriend;
vmCvar_t	cg_teamChatsOnly;
vmCvar_t	cg_hudFiles;
vmCvar_t	cg_scorePlum;
vmCvar_t	cg_smoothClients;
vmCvar_t	pmove_fixed;
vmCvar_t	pmove_msec;
vmCvar_t	cg_cameraMode;
vmCvar_t	cg_cameraOrbit;
vmCvar_t	cg_cameraOrbitDelay;
vmCvar_t	cg_timescaleFadeEnd;
vmCvar_t	cg_timescaleFadeSpeed;
vmCvar_t	cg_timescale;
vmCvar_t	cg_smallFont;
vmCvar_t	cg_bigFont;
vmCvar_t	cg_noTaunt;
vmCvar_t	cg_noProjectileTrail;
vmCvar_t	cg_trueLightning;
vmCvar_t	cg_crosshairColor;
vmCvar_t	cg_teamModel;
vmCvar_t	cg_teamSoundModel;
vmCvar_t	cg_enemyModel;
vmCvar_t	cg_enemySoundModel;
vmCvar_t	cg_redTeamModel;
vmCvar_t	cg_redTeamSoundModel;
vmCvar_t	cg_blueTeamModel;
vmCvar_t	cg_blueTeamSoundModel;
vmCvar_t	cg_forceTeamModels;
vmCvar_t	cg_deadBodyDarken;
vmCvar_t	cg_deadBodyColor;
vmCvar_t	cg_teamHeadColor;
vmCvar_t	cg_teamTorsoColor;
vmCvar_t	cg_teamLegsColor;
vmCvar_t	cg_enemyHeadColor;
vmCvar_t	cg_enemyTorsoColor;
vmCvar_t	cg_enemyLegsColor;
vmCvar_t	cg_redHeadColor;
vmCvar_t	cg_redTorsoColor;
vmCvar_t	cg_redLegsColor;
vmCvar_t	cg_blueHeadColor;
vmCvar_t	cg_blueTorsoColor;
vmCvar_t	cg_blueLegsColor;
vmCvar_t	cg_zoomToggle;
vmCvar_t	cg_zoomOutOnDeath;
vmCvar_t	cg_zoomScaling;
vmCvar_t	cg_zoomSensitivity;
vmCvar_t	cg_weaponConfig;
vmCvar_t	cg_weaponConfig_g;
vmCvar_t	cg_weaponConfig_mg;
vmCvar_t	cg_weaponConfig_sg;
vmCvar_t	cg_weaponConfig_gl;
vmCvar_t	cg_weaponConfig_rl;
vmCvar_t	cg_weaponConfig_lg;
vmCvar_t	cg_weaponConfig_rg;
vmCvar_t	cg_weaponConfig_pg;
vmCvar_t	cg_weaponConfig_bfg;
vmCvar_t	cg_weaponConfig_gh;

static cvarTable_t cvarTable[] = {
	{ &cg_ignore, "cg_ignore", "0", 0, RANGE_BOOL },	// used for debugging
	{ &cg_autoswitch, "cg_autoswitch", "1", CVAR_ARCHIVE, RANGE_BOOL },
	{ &cg_drawGun, "cg_drawGun", "1", CVAR_ARCHIVE, RANGE_INT(0, 3) },
	{ &cg_zoomFov, "cg_zoomFov", "22.5", CVAR_ARCHIVE, RANGE_FLOAT(1, 160) },
	{ &cg_fov, "cg_fov", "90", CVAR_ARCHIVE, RANGE_FLOAT(1, 160) },
	{ &cg_viewsize, "cg_viewsize", "100", CVAR_ARCHIVE, RANGE_INT(30, 100) },
	{ &cg_shadows, "cg_shadows", "1", CVAR_ARCHIVE, RANGE_INT(0, 3) },
	{ &cg_gibs, "cg_gibs", "1", CVAR_ARCHIVE, RANGE_BOOL },
	{ &cg_draw2D, "cg_draw2D", "1", CVAR_ARCHIVE, RANGE_BOOL },
	{ &cg_drawStatus, "cg_drawStatus", "1", CVAR_ARCHIVE, RANGE_BOOL },
	{ &cg_drawTimer, "cg_drawTimer", "0", CVAR_ARCHIVE, RANGE_BOOL },
	{ &cg_drawFPS, "cg_drawFPS", "0", CVAR_ARCHIVE, RANGE_BOOL },
	{ &cg_drawSnapshot, "cg_drawSnapshot", "0", CVAR_ARCHIVE, RANGE_BOOL },
	{ &cg_draw3dIcons, "cg_draw3dIcons", "1", CVAR_ARCHIVE, RANGE_BOOL },
	{ &cg_drawIcons, "cg_drawIcons", "1", CVAR_ARCHIVE, RANGE_BOOL },
	{ &cg_drawAmmoWarning, "cg_drawAmmoWarning", "1", CVAR_ARCHIVE, RANGE_BOOL },
	{ &cg_drawAttacker, "cg_drawAttacker", "1", CVAR_ARCHIVE, RANGE_BOOL },
	{ &cg_drawCrosshair, "cg_drawCrosshair", "4", CVAR_ARCHIVE, RANGE_INT(0, INT_MAX) },
	{ &cg_drawCrosshairNames, "cg_drawCrosshairNames", "1", CVAR_ARCHIVE, RANGE_BOOL },
	{ &cg_drawRewards, "cg_drawRewards", "1", CVAR_ARCHIVE, RANGE_BOOL },
	{ &cg_crosshairSize, "cg_crosshairSize", "24", CVAR_ARCHIVE, RANGE_INT(0, INT_MAX) },
	{ &cg_crosshairHealth, "cg_crosshairHealth", "1", CVAR_ARCHIVE, RANGE_BOOL },
	{ &cg_crosshairX, "cg_crosshairX", "0", CVAR_ARCHIVE, RANGE_INT(INT_MIN, INT_MAX) },
	{ &cg_crosshairY, "cg_crosshairY", "0", CVAR_ARCHIVE, RANGE_INT(INT_MIN, INT_MAX) },
	{ &cg_brassTime, "cg_brassTime", "2500", CVAR_ARCHIVE, RANGE_INT(0, INT_MAX) },
	{ &cg_simpleItems, "cg_simpleItems", "0", CVAR_ARCHIVE, RANGE_BOOL },
	{ &cg_addMarks, "cg_marks", "1", CVAR_ARCHIVE, RANGE_BOOL },
	{ &cg_lagometer, "cg_lagometer", "1", CVAR_ARCHIVE, RANGE_BOOL },
	{ &cg_railTrailTime, "cg_railTrailTime", "400", CVAR_ARCHIVE, RANGE_INT(0, INT_MAX) },
	{ &cg_gun_x, "cg_gunX", "0", 0, RANGE_BOOL },
	{ &cg_gun_y, "cg_gunY", "0", 0, RANGE_BOOL },
	{ &cg_gun_z, "cg_gunZ", "0", 0, RANGE_BOOL },
	{ &cg_centertime, "cg_centertime", "3", CVAR_CHEAT, RANGE_FLOAT(0, FLT_MAX) },
	{ &cg_swingSpeed, "cg_swingSpeed", "0.3", CVAR_CHEAT, RANGE_FLOAT(0, FLT_MAX) },
	{ &cg_animSpeed, "cg_animSpeed", "1", CVAR_CHEAT, RANGE_BOOL },
	{ &cg_debugAnim, "cg_debugAnim", "0", CVAR_CHEAT, RANGE_BOOL },
	{ &cg_debugPosition, "cg_debugPosition", "0", CVAR_CHEAT, RANGE_BOOL },
	{ &cg_debugEvents, "cg_debugEvents", "0", CVAR_CHEAT, RANGE_BOOL },
	{ &cg_errorDecay, "cg_errorDecay", "100", 0, RANGE_FLOAT(FLT_MIN, FLT_MAX) },
	{ &cg_nopredict, "cg_nopredict", "0", 0, RANGE_BOOL },
	{ &cg_noPlayerAnims, "cg_noPlayerAnims", "0", CVAR_CHEAT, RANGE_BOOL },
	{ &cg_showmiss, "cg_showmiss", "0", RANGE_BOOL },
	{ &cg_footsteps, "cg_footsteps", "1", CVAR_CHEAT, RANGE_BOOL },
	{ &cg_tracerChance, "cg_tracerchance", "0.4", CVAR_CHEAT, RANGE_FLOAT(FLT_MIN, FLT_MAX) },
	{ &cg_tracerWidth, "cg_tracerwidth", "1", CVAR_CHEAT, RANGE_FLOAT(0, FLT_MAX) },
	{ &cg_tracerLength, "cg_tracerlength", "100", CVAR_CHEAT, RANGE_FLOAT(0, FLT_MAX) },
	{ &cg_thirdPersonRange, "cg_thirdPersonRange", "40", 0, RANGE_FLOAT(FLT_MIN, FLT_MAX) },
	{ &cg_thirdPersonAngle, "cg_thirdPersonAngle", "0", 0, RANGE_FLOAT(FLT_MIN, FLT_MAX) },
	{ &cg_thirdPerson, "cg_thirdPerson", "0", CVAR_CHEAT, RANGE_BOOL },
	{ &cg_teamChatTime, "cg_teamChatTime", "3000", CVAR_ARCHIVE, RANGE_INT(0, INT_MAX) },
	{ &cg_teamChatHeight, "cg_teamChatHeight", "0", CVAR_ARCHIVE, RANGE_INT(0, INT_MAX) },
	{ &cg_predictItems, "cg_predictItems", "1", CVAR_ARCHIVE, RANGE_BOOL },
	{ &cg_drawTeamOverlay, "cg_drawTeamOverlay", "0", CVAR_ARCHIVE, RANGE_INT(0, 3) },
	{ &cg_teamOverlayUserinfo, "teamoverlay", "0", CVAR_ROM | CVAR_USERINFO, RANGE_BOOL },
	{ &cg_stats, "cg_stats", "0", 0, RANGE_BOOL },
	{ &cg_drawFriend, "cg_drawFriend", "1", CVAR_ARCHIVE, RANGE_BOOL },
	{ &cg_teamChatsOnly, "cg_teamChatsOnly", "0", CVAR_ARCHIVE, RANGE_BOOL },
	// the following variables are created in other parts of the system,
	// but we also reference them here
	{ &cg_buildScript, "com_buildScript", "0", 0, RANGE_BOOL },	// force loading of all possible data amd error on failures
	{ &cg_paused, "cl_paused", "0", CVAR_ROM, RANGE_BOOL },
	{ &cg_blood, "com_blood", "1", CVAR_ARCHIVE, RANGE_BOOL },
	{ &cg_synchronousClients, "g_synchronousClients", "0", CVAR_SYSTEMINFO, RANGE_BOOL },
	{ &cg_cameraOrbit, "cg_cameraOrbit", "0", CVAR_CHEAT, RANGE_FLOAT(FLT_MIN, FLT_MAX) },
	{ &cg_cameraOrbitDelay, "cg_cameraOrbitDelay", "50", CVAR_ARCHIVE, RANGE_FLOAT(FLT_MIN, FLT_MAX) },
	{ &cg_timescaleFadeEnd, "cg_timescaleFadeEnd", "1", 0, RANGE_FLOAT(FLT_MIN, FLT_MAX) },
	{ &cg_timescaleFadeSpeed, "cg_timescaleFadeSpeed", "0", RANGE_FLOAT(FLT_MIN, FLT_MAX) },
	{ &cg_timescale, "timescale", "1", 0, RANGE_FLOAT(FLT_MIN, FLT_MAX) },
	{ &cg_scorePlum, "cg_scorePlums", "1", CVAR_USERINFO | CVAR_ARCHIVE, RANGE_BOOL },
	{ &cg_smoothClients, "cg_smoothClients", "0", CVAR_USERINFO | CVAR_ARCHIVE, RANGE_BOOL },
	{ &cg_cameraMode, "com_cameraMode", "0", CVAR_CHEAT, RANGE_BOOL },

	{ &pmove_fixed, "pmove_fixed", "0", CVAR_SYSTEMINFO, RANGE_BOOL },
	{ &pmove_msec, "pmove_msec", "8", CVAR_SYSTEMINFO, RANGE_INT(8, 33) },
	{ &cg_noTaunt, "cg_noTaunt", "0", CVAR_ARCHIVE, RANGE_BOOL },
	{ &cg_noProjectileTrail, "cg_noProjectileTrail", "0", CVAR_ARCHIVE, RANGE_BOOL },
	{ &cg_smallFont, "ui_smallFont", "0.25", CVAR_ARCHIVE, RANGE_FLOAT(0, FLT_MAX) },
	{ &cg_bigFont, "ui_bigFont", "0.4", CVAR_ARCHIVE, RANGE_FLOAT(0, FLT_MAX) },
	{ &cg_trueLightning, "cg_trueLightning", "0.0", CVAR_ARCHIVE, RANGE_FLOAT(0, FLT_MAX) },
	{ &cg_crosshairColor, "cg_crosshairColor", "7", CVAR_ARCHIVE, RANGE_COLOR },

	{ &cg_teamModel, "cg_teamModel", "major/pm", CVAR_ARCHIVE, RANGE_ALL },
	{ &cg_teamSoundModel, "cg_teamSoundModel", "", CVAR_ARCHIVE, RANGE_ALL },
	{ &cg_enemyModel, "cg_enemyModel", "smarine/pm", CVAR_ARCHIVE, RANGE_ALL },
	{ &cg_enemySoundModel, "cg_enemySoundModel", "", CVAR_ARCHIVE, RANGE_ALL },
	{ &cg_redTeamModel, "cg_redTeamModel", "smarine/pm", CVAR_ARCHIVE, RANGE_ALL },
	{ &cg_redTeamSoundModel, "cg_redTeamSoundModel", "", CVAR_ARCHIVE, RANGE_ALL },
	{ &cg_blueTeamModel, "cg_blueTeamModel", "smarine/pm", CVAR_ARCHIVE, RANGE_ALL },
	{ &cg_blueTeamSoundModel, "cg_blueTeamSoundModel", "", CVAR_ARCHIVE, RANGE_ALL },
	{ &cg_forceTeamModels, "cg_forceTeamModels", "0", CVAR_ARCHIVE, RANGE_INT(0, 2) },
	{ &cg_deadBodyDarken, "cg_deadBodyDarken", "1", CVAR_ARCHIVE, RANGE_ALL },
	{ &cg_deadBodyColor, "cg_deadBodyColor", "0x323232FF", CVAR_ARCHIVE, RANGE_COLOR },
	{ &cg_teamHeadColor, "cg_teamHeadColor", "white", CVAR_ARCHIVE, RANGE_COLOR },
	{ &cg_teamTorsoColor, "cg_teamTorsoColor", "white", CVAR_ARCHIVE, RANGE_COLOR },
	{ &cg_teamLegsColor, "cg_teamLegsColor", "white", CVAR_ARCHIVE, RANGE_COLOR },
	{ &cg_enemyHeadColor, "cg_enemyHeadColor", "green", CVAR_ARCHIVE, RANGE_COLOR },
	{ &cg_enemyTorsoColor, "cg_enemyTorsoColor", "green", CVAR_ARCHIVE, RANGE_COLOR },
	{ &cg_enemyLegsColor, "cg_enemyLegsColor", "white", CVAR_ARCHIVE, RANGE_COLOR },
	{ &cg_redHeadColor, "cg_redHeadColor", "red", CVAR_ARCHIVE, RANGE_COLOR },
	{ &cg_redTorsoColor, "cg_redTorsoColor", "red", CVAR_ARCHIVE, RANGE_COLOR },
	{ &cg_redLegsColor, "cg_redLegsColor", "red", CVAR_ARCHIVE, RANGE_COLOR },
	{ &cg_blueHeadColor, "cg_blueHeadColor", "blue", CVAR_ARCHIVE, RANGE_COLOR },
	{ &cg_blueTorsoColor, "cg_blueTorsoColor", "blue", CVAR_ARCHIVE, RANGE_COLOR },
	{ &cg_blueLegsColor, "cg_blueLegsColor", "blue", CVAR_ARCHIVE, RANGE_COLOR },

	{ &cg_zoomToggle, "cg_zoomToggle", "0", CVAR_ARCHIVE, RANGE_BOOL },
	{ &cg_zoomOutOnDeath, "cg_zoomOutOnDeath", "1", CVAR_ARCHIVE, RANGE_BOOL },
	{ &cg_zoomScaling, "cg_zoomScaling", "1", CVAR_ARCHIVE, RANGE_FLOAT(0, FLT_MAX) },
	{ &cg_zoomSensitivity, "cg_zoomSensitivity", "1", CVAR_ARCHIVE, RANGE_FLOAT(0, FLT_MAX) },

	{ &cg_weaponConfig, "cg_weaponConfig", "", CVAR_ARCHIVE, RANGE_ALL },
	{ &cg_weaponConfig_g, "cg_weaponConfig_g", "", CVAR_ARCHIVE, RANGE_ALL },
	{ &cg_weaponConfig_mg, "cg_weaponConfig_mg", "", CVAR_ARCHIVE, RANGE_ALL },
	{ &cg_weaponConfig_sg, "cg_weaponConfig_sg", "", CVAR_ARCHIVE, RANGE_ALL },
	{ &cg_weaponConfig_gl, "cg_weaponConfig_gl", "", CVAR_ARCHIVE, RANGE_ALL },
	{ &cg_weaponConfig_rl, "cg_weaponConfig_rl", "", CVAR_ARCHIVE, RANGE_ALL },
	{ &cg_weaponConfig_lg, "cg_weaponConfig_lg", "", CVAR_ARCHIVE, RANGE_ALL },
	{ &cg_weaponConfig_rg, "cg_weaponConfig_rg", "", CVAR_ARCHIVE, RANGE_ALL },
	{ &cg_weaponConfig_pg, "cg_weaponConfig_pg", "", CVAR_ARCHIVE, RANGE_ALL },
	{ &cg_weaponConfig_bfg, "cg_weaponConfig_bfg", "", CVAR_ARCHIVE, RANGE_ALL },
	{ &cg_weaponConfig_gh, "cg_weaponConfig_gh", "", CVAR_ARCHIVE, RANGE_ALL }
};

static int	cvarTableSize = ARRAY_LEN(cvarTable);

/**
This function may execute for a couple of minutes with a slow disk.
*/
static void CG_RegisterGraphics(void)
{
	int			i;
	char		items[MAX_ITEMS+1];
	static char	*sb_nums[11] = {
		"gfx/2d/numbers/zero_32b",
		"gfx/2d/numbers/one_32b",
		"gfx/2d/numbers/two_32b",
		"gfx/2d/numbers/three_32b",
		"gfx/2d/numbers/four_32b",
		"gfx/2d/numbers/five_32b",
		"gfx/2d/numbers/six_32b",
		"gfx/2d/numbers/seven_32b",
		"gfx/2d/numbers/eight_32b",
		"gfx/2d/numbers/nine_32b",
		"gfx/2d/numbers/minus_32b",
	};

	// clear any references to old media
	memset(&cg.refdef, 0, sizeof(cg.refdef));
	trap_R_ClearScene();

	CG_LoadingString(cgs.mapname);

	trap_R_LoadWorldMap(cgs.mapname);

	// precache status bar pics
	CG_LoadingString("game media");

	for (i = 0; i<11; i++) {
		cgs.media.numberShaders[i] = trap_R_RegisterShader(sb_nums[i]);
	}

	cgs.media.botSkillShaders[0] = trap_R_RegisterShader("menu/art/skill1.tga");
	cgs.media.botSkillShaders[1] = trap_R_RegisterShader("menu/art/skill2.tga");
	cgs.media.botSkillShaders[2] = trap_R_RegisterShader("menu/art/skill3.tga");
	cgs.media.botSkillShaders[3] = trap_R_RegisterShader("menu/art/skill4.tga");
	cgs.media.botSkillShaders[4] = trap_R_RegisterShader("menu/art/skill5.tga");

	cgs.media.viewBloodShader = trap_R_RegisterShader("viewBloodBlend");

	cgs.media.deferShader = trap_R_RegisterShader("gfx/2d/defer.tga");

	cgs.media.scoreboardName = trap_R_RegisterShaderNoMip("menu/tab/name.tga");
	cgs.media.scoreboardPing = trap_R_RegisterShaderNoMip("menu/tab/ping.tga");
	cgs.media.scoreboardScore = trap_R_RegisterShaderNoMip("menu/tab/score.tga");
	cgs.media.scoreboardTime = trap_R_RegisterShaderNoMip("menu/tab/time.tga");

	cgs.media.smokePuffShader = trap_R_RegisterShader("smokePuff");
	cgs.media.smokePuffRageProShader = trap_R_RegisterShader("smokePuffRagePro");
	cgs.media.shotgunSmokePuffShader = trap_R_RegisterShader("shotgunSmokePuff");
	cgs.media.plasmaBallShader = trap_R_RegisterShader("sprites/plasma1");
	cgs.media.bloodTrailShader = trap_R_RegisterShader("bloodTrail");
	cgs.media.lagometerShader = trap_R_RegisterShader("lagometer");
	cgs.media.connectionShader = trap_R_RegisterShader("disconnected");

	cgs.media.waterBubbleShader = trap_R_RegisterShader("waterBubble");

	cgs.media.tracerShader = trap_R_RegisterShader("gfx/misc/tracer");
	cgs.media.selectShader = trap_R_RegisterShader("gfx/2d/select");

	for (i = 0; i < NUM_CROSSHAIRS; i++) {
		cgs.media.crosshairShader[i] = trap_R_RegisterShader(va("gfx/2d/crosshair%c", 'a'+i));
	}

	cgs.media.backTileShader = trap_R_RegisterShader("gfx/2d/backtile");
	cgs.media.noammoShader = trap_R_RegisterShader("icons/noammo");

	// powerup shaders
	cgs.media.quadShader = trap_R_RegisterShader("powerups/quad");
	cgs.media.quadWeaponShader = trap_R_RegisterShader("powerups/quadWeapon");
	cgs.media.battleSuitShader = trap_R_RegisterShader("powerups/battleSuit");
	cgs.media.battleWeaponShader = trap_R_RegisterShader("powerups/battleWeapon");
	cgs.media.regenShader = trap_R_RegisterShader("powerups/regen");
	cgs.media.hastePuffShader = trap_R_RegisterShader("hasteSmokePuff");

	if (cgs.gametype == GT_CTF || cg_buildScript.integer) {
		cgs.media.redFlagModel = trap_R_RegisterModel("models/flags/r_flag.md3");
		cgs.media.blueFlagModel = trap_R_RegisterModel("models/flags/b_flag.md3");
		cgs.media.redFlagShader[0] = trap_R_RegisterShaderNoMip("icons/iconf_red1");
		cgs.media.redFlagShader[1] = trap_R_RegisterShaderNoMip("icons/iconf_red2");
		cgs.media.redFlagShader[2] = trap_R_RegisterShaderNoMip("icons/iconf_red3");
		cgs.media.blueFlagShader[0] = trap_R_RegisterShaderNoMip("icons/iconf_blu1");
		cgs.media.blueFlagShader[1] = trap_R_RegisterShaderNoMip("icons/iconf_blu2");
		cgs.media.blueFlagShader[2] = trap_R_RegisterShaderNoMip("icons/iconf_blu3");
	}

	if (cgs.gametype >= GT_TEAM || cg_buildScript.integer) {
		cgs.media.friendShader = trap_R_RegisterShader("sprites/foe");
		cgs.media.redQuadShader = trap_R_RegisterShader("powerups/blueflag");
		cgs.media.teamStatusBar = trap_R_RegisterShader("gfx/2d/colorbar.tga");
	}

	cgs.media.armorModel = trap_R_RegisterModel("models/powerups/armor/armor_yel.md3");
	cgs.media.armorIcon  = trap_R_RegisterShaderNoMip("icons/iconr_yellow");

	cgs.media.machinegunBrassModel = trap_R_RegisterModel("models/weapons2/shells/m_shell.md3");
	cgs.media.shotgunBrassModel = trap_R_RegisterModel("models/weapons2/shells/s_shell.md3");

	cgs.media.gibAbdomen = trap_R_RegisterModel("models/gibs/abdomen.md3");
	cgs.media.gibArm = trap_R_RegisterModel("models/gibs/arm.md3");
	cgs.media.gibChest = trap_R_RegisterModel("models/gibs/chest.md3");
	cgs.media.gibFist = trap_R_RegisterModel("models/gibs/fist.md3");
	cgs.media.gibFoot = trap_R_RegisterModel("models/gibs/foot.md3");
	cgs.media.gibForearm = trap_R_RegisterModel("models/gibs/forearm.md3");
	cgs.media.gibIntestine = trap_R_RegisterModel("models/gibs/intestine.md3");
	cgs.media.gibLeg = trap_R_RegisterModel("models/gibs/leg.md3");
	cgs.media.gibSkull = trap_R_RegisterModel("models/gibs/skull.md3");
	cgs.media.gibBrain = trap_R_RegisterModel("models/gibs/brain.md3");

	cgs.media.smoke2 = trap_R_RegisterModel("models/weapons2/shells/s_shell.md3");

	cgs.media.balloonShader = trap_R_RegisterShader("sprites/balloon3");

	cgs.media.bloodExplosionShader = trap_R_RegisterShader("bloodExplosion");

	cgs.media.bulletFlashModel = trap_R_RegisterModel("models/weaphits/bullet.md3");
	cgs.media.ringFlashModel = trap_R_RegisterModel("models/weaphits/ring02.md3");
	cgs.media.dishFlashModel = trap_R_RegisterModel("models/weaphits/boom01.md3");
	cgs.media.teleportEffectModel = trap_R_RegisterModel("models/misc/telep.md3");
	cgs.media.teleportEffectShader = trap_R_RegisterShader("teleportEffect");
	cgs.media.medalImpressive = trap_R_RegisterShaderNoMip("medal_impressive");
	cgs.media.medalExcellent = trap_R_RegisterShaderNoMip("medal_excellent");
	cgs.media.medalGauntlet = trap_R_RegisterShaderNoMip("medal_gauntlet");
	cgs.media.medalDefend = trap_R_RegisterShaderNoMip("medal_defend");
	cgs.media.medalAssist = trap_R_RegisterShaderNoMip("medal_assist");
	cgs.media.medalCapture = trap_R_RegisterShaderNoMip("medal_capture");


	memset(cg_items, 0, sizeof(cg_items));
	memset(cg_weapons, 0, sizeof(cg_weapons));

	// only register the items that the server says we need
	Q_strncpyz(items, CG_ConfigString(CS_ITEMS), sizeof(items));

	for (i = 1; i < bg_numItems; i++) {
		if (items[ i ] == '1' || cg_buildScript.integer) {
			CG_LoadingItem(i);
			CG_RegisterItemVisuals(i);
		}
	}

	// wall marks
	cgs.media.bulletMarkShader = trap_R_RegisterShader("gfx/damage/bullet_mrk");
	cgs.media.burnMarkShader = trap_R_RegisterShader("gfx/damage/burn_med_mrk");
	cgs.media.holeMarkShader = trap_R_RegisterShader("gfx/damage/hole_lg_mrk");
	cgs.media.energyMarkShader = trap_R_RegisterShader("gfx/damage/plasma_mrk");
	cgs.media.shadowMarkShader = trap_R_RegisterShader("markShadow");
	cgs.media.wakeMarkShader = trap_R_RegisterShader("wake");
	cgs.media.bloodMarkShader = trap_R_RegisterShader("bloodMark");

	// register the inline models
	cgs.numInlineModels = trap_CM_NumInlineModels();
	for (i = 1; i < cgs.numInlineModels; i++) {
		char	name[10];
		vec3_t			mins, maxs;
		int				j;

		Com_sprintf(name, sizeof(name), "*%i", i);
		cgs.inlineDrawModel[i] = trap_R_RegisterModel(name);
		trap_R_ModelBounds(cgs.inlineDrawModel[i], mins, maxs);
		for (j = 0; j < 3; j++) {
			cgs.inlineModelMidpoints[i][j] = mins[j] + 0.5 * (maxs[j] - mins[j]);
		}
	}

	// register all the server specified models
	for (i=1; i < MAX_MODELS; i++) {
		const char		*modelName;

		modelName = CG_ConfigString(CS_MODELS+i);
		if (!modelName[0]) {
			break;
		}
		cgs.gameModels[i] = trap_R_RegisterModel(modelName);
	}

	CG_ClearParticles ();
}

/**
The server says this item is used on this level
*/
static void CG_RegisterItemSounds(int itemNum)
{
	gitem_t			*item;
	char			data[MAX_QPATH];
	char			*s, *start;
	int				len;

	item = &bg_itemlist[ itemNum ];

	if (item->pickup_sound) {
		trap_S_RegisterSound(item->pickup_sound, qfalse);
	}

	// parse the space seperated precache string for other media
	s = item->sounds;
	if (!s || !s[0])
		return;

	while (*s) {
		start = s;
		while (*s && *s != ' ') {
			s++;
		}

		len = s - start;
		if (len >= MAX_QPATH || len < 5) {
			CG_Error("PrecacheItem: %s has bad precache string", item->classname);
			return;
		}

		memcpy (data, start, len);
		data[len] = 0;
		if (*s) {
			s++;
		}

		if (!strcmp(data + len - 3, "wav")) {
			trap_S_RegisterSound(data, qfalse);
		}
	}
}

/**
Called during a precache command
*/
static void CG_RegisterSounds(void)
{
	int		i;
	char	items[MAX_ITEMS+1];
	char	name[MAX_QPATH];
	const char	*soundName;

	// voice commands
	cgs.media.oneMinuteSound = trap_S_RegisterSound("sound/feedback/1_minute.wav", qtrue);
	cgs.media.fiveMinuteSound = trap_S_RegisterSound("sound/feedback/5_minute.wav", qtrue);
	cgs.media.suddenDeathSound = trap_S_RegisterSound("sound/feedback/sudden_death.wav", qtrue);
	cgs.media.oneFragSound = trap_S_RegisterSound("sound/feedback/1_frag.wav", qtrue);
	cgs.media.twoFragSound = trap_S_RegisterSound("sound/feedback/2_frags.wav", qtrue);
	cgs.media.threeFragSound = trap_S_RegisterSound("sound/feedback/3_frags.wav", qtrue);
	cgs.media.count3Sound = trap_S_RegisterSound("sound/feedback/three.wav", qtrue);
	cgs.media.count2Sound = trap_S_RegisterSound("sound/feedback/two.wav", qtrue);
	cgs.media.count1Sound = trap_S_RegisterSound("sound/feedback/one.wav", qtrue);
	cgs.media.countFightSound = trap_S_RegisterSound("sound/feedback/fight.wav", qtrue);
	cgs.media.countPrepareSound = trap_S_RegisterSound("sound/feedback/prepare.wav", qtrue);

	if (cgs.gametype >= GT_TEAM || cg_buildScript.integer) {
		cgs.media.captureAwardSound = trap_S_RegisterSound("sound/teamplay/flagcapture_yourteam.wav", qtrue);
		cgs.media.redLeadsSound = trap_S_RegisterSound("sound/feedback/redleads.wav", qtrue);
		cgs.media.blueLeadsSound = trap_S_RegisterSound("sound/feedback/blueleads.wav", qtrue);
		cgs.media.teamsTiedSound = trap_S_RegisterSound("sound/feedback/teamstied.wav", qtrue);
		cgs.media.hitTeamSound = trap_S_RegisterSound("sound/feedback/hit_teammate.wav", qtrue);

		cgs.media.redScoredSound = trap_S_RegisterSound("sound/teamplay/voc_red_scores.wav", qtrue);
		cgs.media.blueScoredSound = trap_S_RegisterSound("sound/teamplay/voc_blue_scores.wav", qtrue);

		cgs.media.captureYourTeamSound = trap_S_RegisterSound("sound/teamplay/flagcapture_yourteam.wav", qtrue);
		cgs.media.captureOpponentSound = trap_S_RegisterSound("sound/teamplay/flagcapture_opponent.wav", qtrue);

		cgs.media.returnYourTeamSound = trap_S_RegisterSound("sound/teamplay/flagreturn_yourteam.wav", qtrue);
		cgs.media.returnOpponentSound = trap_S_RegisterSound("sound/teamplay/flagreturn_opponent.wav", qtrue);

		cgs.media.takenYourTeamSound = trap_S_RegisterSound("sound/teamplay/flagtaken_yourteam.wav", qtrue);
		cgs.media.takenOpponentSound = trap_S_RegisterSound("sound/teamplay/flagtaken_opponent.wav", qtrue);

		if (cgs.gametype == GT_CTF || cg_buildScript.integer) {
			cgs.media.redFlagReturnedSound = trap_S_RegisterSound("sound/teamplay/voc_red_returned.wav", qtrue);
			cgs.media.blueFlagReturnedSound = trap_S_RegisterSound("sound/teamplay/voc_blue_returned.wav", qtrue);
			cgs.media.enemyTookYourFlagSound = trap_S_RegisterSound("sound/teamplay/voc_enemy_flag.wav", qtrue);
			cgs.media.yourTeamTookEnemyFlagSound = trap_S_RegisterSound("sound/teamplay/voc_team_flag.wav", qtrue);
		}

		cgs.media.youHaveFlagSound = trap_S_RegisterSound("sound/teamplay/voc_you_flag.wav", qtrue);
		cgs.media.holyShitSound = trap_S_RegisterSound("sound/feedback/voc_holyshit.wav", qtrue);
	}

	cgs.media.tracerSound = trap_S_RegisterSound("sound/weapons/machinegun/buletby1.wav", qfalse);
	cgs.media.selectSound = trap_S_RegisterSound("sound/weapons/change.wav", qfalse);
	cgs.media.wearOffSound = trap_S_RegisterSound("sound/items/wearoff.wav", qfalse);
	cgs.media.useNothingSound = trap_S_RegisterSound("sound/items/use_nothing.wav", qfalse);
	cgs.media.gibSound = trap_S_RegisterSound("sound/player/gibsplt1.wav", qfalse);
	cgs.media.gibBounce1Sound = trap_S_RegisterSound("sound/player/gibimp1.wav", qfalse);
	cgs.media.gibBounce2Sound = trap_S_RegisterSound("sound/player/gibimp2.wav", qfalse);
	cgs.media.gibBounce3Sound = trap_S_RegisterSound("sound/player/gibimp3.wav", qfalse);

	cgs.media.teleInSound = trap_S_RegisterSound("sound/world/telein.wav", qfalse);
	cgs.media.teleOutSound = trap_S_RegisterSound("sound/world/teleout.wav", qfalse);
	cgs.media.respawnSound = trap_S_RegisterSound("sound/items/respawn1.wav", qfalse);

	cgs.media.talkSound = trap_S_RegisterSound("sound/player/talk.wav", qfalse);
	cgs.media.landSound = trap_S_RegisterSound("sound/player/land1.wav", qfalse);

	cgs.media.hitSound = trap_S_RegisterSound("sound/feedback/hit.wav", qfalse);
	cgs.media.noAmmoSound = trap_S_RegisterSound("sound/weapons/noammo.wav", qfalse);

	cgs.media.impressiveSound = trap_S_RegisterSound("sound/feedback/impressive.wav", qtrue);
	cgs.media.excellentSound = trap_S_RegisterSound("sound/feedback/excellent.wav", qtrue);
	cgs.media.deniedSound = trap_S_RegisterSound("sound/feedback/denied.wav", qtrue);
	cgs.media.humiliationSound = trap_S_RegisterSound("sound/feedback/humiliation.wav", qtrue);
	cgs.media.assistSound = trap_S_RegisterSound("sound/feedback/assist.wav", qtrue);
	cgs.media.defendSound = trap_S_RegisterSound("sound/feedback/defense.wav", qtrue);

	cgs.media.takenLeadSound = trap_S_RegisterSound("sound/feedback/takenlead.wav", qtrue);
	cgs.media.tiedLeadSound = trap_S_RegisterSound("sound/feedback/tiedlead.wav", qtrue);
	cgs.media.lostLeadSound = trap_S_RegisterSound("sound/feedback/lostlead.wav", qtrue);

	cgs.media.watrInSound = trap_S_RegisterSound("sound/player/watr_in.wav", qfalse);
	cgs.media.watrOutSound = trap_S_RegisterSound("sound/player/watr_out.wav", qfalse);
	cgs.media.watrUnSound = trap_S_RegisterSound("sound/player/watr_un.wav", qfalse);

	cgs.media.jumpPadSound = trap_S_RegisterSound ("sound/world/jumppad.wav", qfalse);

	for (i = 0; i<4; i++) {
		Com_sprintf (name, sizeof(name), "sound/player/footsteps/step%i.wav", i+1);
		cgs.media.footsteps[FOOTSTEP_NORMAL][i] = trap_S_RegisterSound (name, qfalse);

		Com_sprintf (name, sizeof(name), "sound/player/footsteps/boot%i.wav", i+1);
		cgs.media.footsteps[FOOTSTEP_BOOT][i] = trap_S_RegisterSound (name, qfalse);

		Com_sprintf (name, sizeof(name), "sound/player/footsteps/flesh%i.wav", i+1);
		cgs.media.footsteps[FOOTSTEP_FLESH][i] = trap_S_RegisterSound (name, qfalse);

		Com_sprintf (name, sizeof(name), "sound/player/footsteps/mech%i.wav", i+1);
		cgs.media.footsteps[FOOTSTEP_MECH][i] = trap_S_RegisterSound (name, qfalse);

		Com_sprintf (name, sizeof(name), "sound/player/footsteps/energy%i.wav", i+1);
		cgs.media.footsteps[FOOTSTEP_ENERGY][i] = trap_S_RegisterSound (name, qfalse);

		Com_sprintf (name, sizeof(name), "sound/player/footsteps/splash%i.wav", i+1);
		cgs.media.footsteps[FOOTSTEP_SPLASH][i] = trap_S_RegisterSound (name, qfalse);

		Com_sprintf (name, sizeof(name), "sound/player/footsteps/clank%i.wav", i+1);
		cgs.media.footsteps[FOOTSTEP_METAL][i] = trap_S_RegisterSound (name, qfalse);
	}

	// only register the items that the server says we need
	Q_strncpyz(items, CG_ConfigString(CS_ITEMS), sizeof(items));

	for (i = 1; i < bg_numItems; i++) {
//		if (items[ i ] == '1' || cg_buildScript.integer) {
			CG_RegisterItemSounds(i);
//		}
	}

	for (i = 1; i < MAX_SOUNDS; i++) {
		soundName = CG_ConfigString(CS_SOUNDS+i);
		if (!soundName[0]) {
			break;
		}
		if (soundName[0] == '*') {
			continue;	// custom sound
		}
		cgs.gameSounds[i] = trap_S_RegisterSound(soundName, qfalse);
	}

	// FIXME: only needed with item
	cgs.media.flightSound = trap_S_RegisterSound("sound/items/flight.wav", qfalse);
	cgs.media.medkitSound = trap_S_RegisterSound ("sound/items/use_medkit.wav", qfalse);
	cgs.media.quadSound = trap_S_RegisterSound("sound/items/damage3.wav", qfalse);
	cgs.media.sfx_ric1 = trap_S_RegisterSound ("sound/weapons/machinegun/ric1.wav", qfalse);
	cgs.media.sfx_ric2 = trap_S_RegisterSound ("sound/weapons/machinegun/ric2.wav", qfalse);
	cgs.media.sfx_ric3 = trap_S_RegisterSound ("sound/weapons/machinegun/ric3.wav", qfalse);
	//cgs.media.sfx_railg = trap_S_RegisterSound ("sound/weapons/railgun/railgf1a.wav", qfalse);
	cgs.media.sfx_rockexp = trap_S_RegisterSound ("sound/weapons/rocket/rocklx1a.wav", qfalse);
	cgs.media.sfx_plasmaexp = trap_S_RegisterSound ("sound/weapons/plasma/plasmx1a.wav", qfalse);

	cgs.media.regenSound = trap_S_RegisterSound("sound/items/regen.wav", qfalse);
	cgs.media.protectSound = trap_S_RegisterSound("sound/items/protect3.wav", qfalse);
	cgs.media.n_healthSound = trap_S_RegisterSound("sound/items/n_health.wav", qfalse);
	cgs.media.hgrenb1aSound = trap_S_RegisterSound("sound/weapons/grenade/hgrenb1a.wav", qfalse);
	cgs.media.hgrenb2aSound = trap_S_RegisterSound("sound/weapons/grenade/hgrenb2a.wav", qfalse);
}

static void CG_RegisterModels(void)
{
	CG_LoadCvarModel("cg_teamModel", &cg_teamModel);
	CG_LoadCvarModel("cg_enemyModel", &cg_enemyModel);
	CG_LoadCvarModel("cg_redTeamModel", &cg_redTeamModel);
	CG_LoadCvarModel("cg_blueTeamModel", &cg_blueTeamModel);
	CG_LoadCvarModel("cg_teamSoundModel", &cg_teamSoundModel);
	CG_LoadCvarModel("cg_enemySoundModel", &cg_enemySoundModel);
	CG_LoadCvarModel("cg_redTeamSoundModel", &cg_redTeamSoundModel);
	CG_LoadCvarModel("cg_blueTeamSoundModel", &cg_blueTeamSoundModel);
}

static void CG_RegisterClients(void)
{
	int		i;

	CG_LoadingClient(cg.clientNum);
	CG_NewClientInfo(cg.clientNum);

	for (i = 0; i < MAX_CLIENTS; i++) {
		const char		*clientInfo;

		if (cg.clientNum == i) {
			continue;
		}

		clientInfo = CG_ConfigString(CS_PLAYERS+i);
		if (!clientInfo[0]) {
			continue;
		}

		CG_LoadingClient(i);
		CG_NewClientInfo(i);
	}

	CG_BuildSpectatorString();
}

static void CG_ValidateCvar(cvarTable_t *cv)
{
	if (cv->rangeType == RANGE_TYPE_COLOR) {
		byte color[4];
		if (CG_SetRGBA(color, cv->vmCvar->string)) {
			trap_Cvar_Set(cv->cvarName, cv->defaultString);
			Com_Printf("WARNING: Invalid color string, setting to 7 (white)\n");
		}
	}
}

void CG_RegisterCvars(void)
{
	int			i;
	cvarTable_t	*cv;
	char		var[MAX_TOKEN_CHARS];

	for (i = 0, cv = cvarTable; i < cvarTableSize; i++, cv++) {
		trap_Cvar_Register(cv->vmCvar, cv->cvarName,
			cv->defaultString, cv->cvarFlags);
		if (cv->rangeType == RANGE_TYPE_INT || cv->rangeType == RANGE_TYPE_FLOAT) {
			trap_Cvar_CheckRange(cv->cvarName, cv->min, cv->max, cv->rangeType == RANGE_TYPE_INT);
		}
	}

	// see if we are also running the server on this machine
	trap_Cvar_VariableStringBuffer("sv_running", var, sizeof(var));
	cgs.localServer = atoi(var);
}

void CG_BuildSpectatorString(void)
{
	int i;
	cg.spectatorList[0] = 0;
	for (i = 0; i < MAX_CLIENTS; i++) {
		if (cgs.clientinfo[i].infoValid && cgs.clientinfo[i].team == TEAM_SPECTATOR) {
			Q_strcat(cg.spectatorList, sizeof(cg.spectatorList), va("%s     ", cgs.clientinfo[i].name));
		}
	}

	i = strlen(cg.spectatorList);
	if (i != cg.spectatorLen) {
		cg.spectatorLen = i;
		cg.spectatorWidth = -1;
	}
}

const char *CG_ConfigString(int index)
{
	if (index < 0 || index >= MAX_CONFIGSTRINGS) {
		CG_Error("CG_ConfigString: bad index: %i", index);
	}

	return cgs.gameState.stringData + cgs.gameState.stringOffsets[ index ];
}

void CG_StartMusic(void)
{
	char	*s;
	char	parm1[MAX_QPATH], parm2[MAX_QPATH];

	// start the background music
	s = (char *)CG_ConfigString(CS_MUSIC);
	Q_strncpyz(parm1, COM_Parse(&s), sizeof(parm1));
	Q_strncpyz(parm2, COM_Parse(&s), sizeof(parm2));

	trap_S_StartBackgroundTrack(parm1, parm2);
}

/**
Called after every level change or subsystem restart
Will perform callbacks to make the loading info screen update.
*/
void CG_Init(int serverMessageNum, int serverCommandSequence, int clientNum)
{
	const char	*s;

	// clear everything
	memset(&cgs, 0, sizeof(cgs));
	memset(&cg, 0, sizeof(cg));
	memset(cg_entities, 0, sizeof(cg_entities));
	memset(cg_weapons, 0, sizeof(cg_weapons));
	memset(cg_items, 0, sizeof(cg_items));

	cg.clientNum = clientNum;

	cgs.processedSnapshotNum = serverMessageNum;
	cgs.serverCommandSequence = serverCommandSequence;

	// load a few needed things before we do any screen updates
	cgs.media.charsetShader		= trap_R_RegisterShader("gfx/2d/bigchars");
	cgs.media.whiteShader		= trap_R_RegisterShader("white");
	cgs.media.charsetProp		= trap_R_RegisterShaderNoMip("menu/art/font1_prop.tga");
	cgs.media.charsetPropGlow	= trap_R_RegisterShaderNoMip("menu/art/font1_prop_glo.tga");
	cgs.media.charsetPropB		= trap_R_RegisterShaderNoMip("menu/art/font2_prop.tga");

	CG_RegisterCvars();

	CG_InitConsoleCommands();

	cgs.redflag = cgs.blueflag = -1; // For compatibily, default to unset for
	cgs.flagStatus = -1;
	// old servers

	// get the rendering configuration from the client system
	trap_GetGlconfig(&cgs.glconfig);
	cgs.screenXScale = cgs.glconfig.vidWidth / 640.0;
	cgs.screenYScale = cgs.glconfig.vidHeight / 480.0;

	// get the gamestate from the client system
	trap_GetGameState(&cgs.gameState);

	// check version
	s = CG_ConfigString(CS_GAME_VERSION);
	if (strcmp(s, GAME_VERSION)) {
		CG_Error("Client/Server game mismatch: %s/%s", GAME_VERSION, s);
	}

	s = CG_ConfigString(CS_LEVEL_START_TIME);
	cgs.levelStartTime = atoi(s);

	CG_ParseServerinfo();

	// load the new map
	CG_LoadingString("collision map");

	trap_CM_LoadMap(cgs.mapname);

	CG_LoadingString("sounds");

	CG_RegisterSounds();

	CG_LoadingString("graphics");

	CG_RegisterGraphics();

	CG_LoadingString("models");

	CG_RegisterModels();

	CG_LoadingString("clients");

	CG_RegisterClients();

	CG_InitLocalEntities();

	CG_InitMarkPolys();

	// remove the last loading update
	cg.infoScreenText[0] = 0;

	// Make sure we have update values (scores)
	CG_SetConfigValues();

	CG_StartMusic();

	CG_LoadingString("");

	CG_ShaderStateChanged();

	trap_S_ClearLoopingSounds(qtrue);
}

/**
Called before every level change or subsystem restart
*/
void CG_Shutdown(void)
{
	// some mods may need to do cleanup work here,
	// like closing files or archiving session data
}

/**
type 0 - no event handling
     1 - team menu
     2 - hud editor
*/
void CG_EventHandling(int type) { }

void CG_KeyEvent(int key, qboolean down) { }

void CG_MouseEvent(int x, int y) { }

void CG_UpdateCvars(void)
{
	int			i;
	cvarTable_t	*cv;
	int			modCount;

	for (i = 0, cv = cvarTable; i < cvarTableSize; i++, cv++) {
		modCount = cv->vmCvar->modificationCount;
		trap_Cvar_Update(cv->vmCvar);
		if (modCount == cv->vmCvar->modificationCount) {
			continue;
		}

		CG_ValidateCvar(cv);

		if (!CG_LoadCvarModel(cv->cvarName, cv->vmCvar)) {
			continue;
		}

		if (cv->vmCvar == &cg_forceTeamModels) {
			CG_ForceModelChange();
		}

		// If team overlay is on, ask for updates from the server.  If it's off,
		// let the server know so we don't receive it
		if (cv->vmCvar == &cg_drawTeamOverlay && cv->vmCvar->integer) {
			trap_Cvar_Set("teamoverlay", "1");
		} else {
			trap_Cvar_Set("teamoverlay", "0");
		}
	}
}

int CG_CrosshairPlayer(void)
{
	if (cg.time > (cg.crosshairClientTime + 1000)) {
		return -1;
	}
	return cg.crosshairClientNum;
}

int CG_LastAttacker(void) {
	if (!cg.attackerTime) {
		return -1;
	}
	return cg.snap->ps.persistant[PERS_ATTACKER];
}

void QDECL CG_Printf(const char *msg, ...)
{
	va_list		argptr;
	char		text[1024];

	va_start (argptr, msg);
	Q_vsnprintf (text, sizeof(text), msg, argptr);
	va_end (argptr);

	trap_Print(text);
}

void QDECL CG_Error(const char *msg, ...)
{
	va_list		argptr;
	char		text[1024];

	va_start(argptr, msg);
	Q_vsnprintf(text, sizeof(text), msg, argptr);
	va_end(argptr);

	trap_Error(text);
}

void QDECL Com_Error(int level, const char *error, ...)
{
	va_list		argptr;
	char		text[1024];

	va_start(argptr, error);
	Q_vsnprintf(text, sizeof(text), error, argptr);
	va_end(argptr);

	trap_Error(text);
}

void QDECL Com_Printf(const char *msg, ...)
{
	va_list		argptr;
	char		text[1024];

	va_start(argptr, msg);
	Q_vsnprintf(text, sizeof(text), msg, argptr);
	va_end(argptr);

	trap_Print(text);
}

const char *CG_Argv(int arg)
{
	static char	buffer[MAX_STRING_CHARS];
	trap_Argv(arg, buffer, sizeof(buffer));
	return buffer;
}


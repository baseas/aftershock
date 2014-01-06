
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
// cg_hud.c

#include "cg_local.h"

#define FPS_FRAMES	4

#define LAG_SAMPLES			128
#define MAX_LAGOMETER_PING	900
#define MAX_LAGOMETER_RANGE	300

#define ICON_BLEND_TIME		3000

typedef struct {
	int		frameSamples[LAG_SAMPLES];
	int		frameCount;
	int		snapshotFlags[LAG_SAMPLES];
	int		snapshotSamples[LAG_SAMPLES];
	int		snapshotCount;
} lagometer_t;

lagometer_t		lagometer;

char	systemChat[256];
char	teamChat1[256];
char	teamChat2[256];

static void CG_DrawHudIcon(int hudnumber, qboolean override, qhandle_t hShader)
{
	hudElement_t	*hudelement;
	vec4_t			color;

	hudelement = &cgs.hud[hudnumber];

	if (!hudelement->inuse)
		return;

	if (cgs.gametype >= GT_TEAM && hudelement->teamBgColor == 1) {
		if (cg.snap->ps.persistant[PERS_TEAM] == TEAM_RED) {
			color[0] = 1;
			color[1] = 0;
			color[2] = 0;
		} else if (cg.snap->ps.persistant[PERS_TEAM] == TEAM_BLUE) {
			color[0] = 0;
			color[1] = 0;
			color[2] = 1;
		}
	} else if (cgs.gametype >= GT_TEAM && hudelement->teamBgColor == 2) {
		if (cg.snap->ps.persistant[PERS_TEAM] == TEAM_BLUE) {
			color[0] = 1;
			color[1] = 0;
			color[2] = 0;
		} else if (cg.snap->ps.persistant[PERS_TEAM] == TEAM_RED) {
			color[0] = 0;
			color[1] = 0;
			color[2] = 1;
		}
	}
	else {
		color[0] = hudelement->bgcolor[0];
		color[1] = hudelement->bgcolor[1];
		color[2] = hudelement->bgcolor[2];
	}
	color[3] = hudelement->bgcolor[3];

	if (hudelement->fill) {
		CG_FillRect(hudelement->xpos, hudelement->ypos, hudelement->width, hudelement->height, color);
	} else{
		CG_DrawRect(hudelement->xpos, hudelement->ypos, hudelement->width, hudelement->height, 1.0f, color);
	}

	if (cgs.gametype >= GT_TEAM && hudelement->teamColor == 1) {
		if (cg.snap->ps.persistant[PERS_TEAM] == TEAM_RED) {
			color[0] = 1;
			color[1] = 0;
			color[2] = 0;
		} else if (cg.snap->ps.persistant[PERS_TEAM] == TEAM_BLUE) {
			color[0] = 0;
			color[1] = 0;
			color[2] = 1;
		}
	}
	else if (cgs.gametype >= GT_TEAM && hudelement->teamColor == 2) {
		if (cg.snap->ps.persistant[PERS_TEAM] == TEAM_BLUE) {
			color[0] = 1;
			color[1] = 0;
			color[2] = 0;
		} else if (cg.snap->ps.persistant[PERS_TEAM] == TEAM_RED) {
			color[0] = 0;
			color[1] = 0;
			color[2] = 1;
		}
	} else {
		color[0] = hudelement->color[0];
		color[1] = hudelement->color[1];
		color[2] = hudelement->color[2];
	}
	color[3] = hudelement->color[3];

	trap_R_SetColor(color);

	if (hudelement->imageHandle && override) {
		CG_DrawAdjustPic(hudelement->xpos, hudelement->ypos, hudelement->width,
			hudelement->height, hudelement->imageHandle);
		return;
	}
	if (hShader) {
		CG_DrawAdjustPic(hudelement->xpos, hudelement->ypos, hudelement->width,
			hudelement->height, hShader);
	}
	trap_R_SetColor(NULL);
}

static void CG_DrawHudString(int hudnumber, qboolean colorize, const char* text)
{
	hudElement_t	*hudelement;
	int				w, x;
	vec4_t			color;
	qboolean		shadow;

	hudelement = &cgs.hud[hudnumber];
	shadow = (hudelement->textstyle & 1);

	if (!hudelement->inuse) {
		return;
	}

	CG_DrawHudIcon(hudnumber, qtrue, hudelement->imageHandle);

	w = CG_DrawStrlen(text) * CG_AdjustWidth(hudelement->fontWidth);

	if (hudelement->textAlign == 0)
		x = hudelement->xpos;
	else if (hudelement->textAlign == 2)
		x = hudelement->xpos + hudelement->width - w;
	else
		x = hudelement->xpos + hudelement->width/2 - w/2;

	if (colorize) {
		if (cgs.gametype >= GT_TEAM && hudelement->teamColor == 1) {
			if (cg.snap->ps.persistant[PERS_TEAM] == TEAM_RED) {
				color[0] = 1;
				color[1] = 0;
				color[2] = 0;
			} else if (cg.snap->ps.persistant[PERS_TEAM] == TEAM_BLUE) {
				color[0] = 0;
				color[1] = 0;
				color[2] = 1;
			}
		} else if (cgs.gametype >= GT_TEAM && hudelement->teamColor == 2) {
			if (cg.snap->ps.persistant[PERS_TEAM] == TEAM_BLUE) {
				color[0] = 1;
				color[1] = 0;
				color[2] = 0;
			} else if (cg.snap->ps.persistant[PERS_TEAM] == TEAM_RED) {
				color[0] = 0;
				color[1] = 0;
				color[2] = 1;
			}
		}
		else {
			Vector4Copy(hudelement->color, color);
		}
	}
	else {
		color[0] = 1.0f;
		color[1] = 1.0f;
		color[2] = 1.0f;
		color[3] = 1.0f;
	}

	CG_DrawStringExt(x, hudelement->ypos, text, color, qfalse,
		shadow, hudelement->fontWidth, hudelement->fontHeight, 0);
}

static void CG_DrawFieldFontsize(int x, int y, int width, int value, int fontWidth, int fontHeight)
{
	char	num[8], *ptr;
	int		l;
	int		frame;

	if (width < 1) {
		return;
	}

	// draw number string
	if (width > 5) {
		width = 5;
	}

	switch (width) {
	case 1:
		value = value > 9 ? 9 : value;
		value = value < 0 ? 0 : value;
		break;
	case 2:
		value = value > 99 ? 99 : value;
		value = value < -9 ? -9 : value;
		break;
	case 3:
		value = value > 999 ? 999 : value;
		value = value < -99 ? -99 : value;
		break;
	case 4:
		value = value > 9999 ? 9999 : value;
		value = value < -999 ? -999 : value;
		break;
	}

	Com_sprintf(num, sizeof num, "%i", value);
	l = strlen(num);
	if (l > width) {
		l = width;
	}

	x += CG_AdjustWidth(fontWidth) * (width - l);

	ptr = num;
	while (*ptr && l) {
		if (*ptr == '-') {
			frame = STAT_MINUS;
		} else {
			frame = *ptr -'0';
		}

		CG_DrawAdjustPic(x, y, fontWidth, fontHeight, cgs.media.numberShaders[frame]);
		x += CG_AdjustWidth(fontWidth);
		ptr++;
		l--;
	}
}

/**
Draws large numbers for status bar and powerups
*/
static void CG_DrawHudField(int hudnumber, int value)
{
	hudElement_t	*hudelement;
	int				fontWidth;
	int				digits;

	hudelement = &cgs.hud[hudnumber];
	fontWidth = CG_AdjustWidth(hudelement->fontWidth);

	if (hudelement->color[0] == 0 && hudelement->color[1] == 0 && hudelement->color[2] == 0) {
		trap_R_SetColor(hudelement->color);
	}

	if (value >= 100) {
		digits = 3;
	} else if (value >= 10) {
		digits = 2;
	} else {
		digits = 1;
	}

	switch (hudelement->textAlign) {
	case 0:
		CG_DrawFieldFontsize(hudelement->xpos - (3 - digits) * fontWidth, hudelement->ypos, 3,
			value, hudelement->fontWidth, hudelement->fontHeight);
		break;
	case 1:
		CG_DrawFieldFontsize(hudelement->xpos + hudelement->width/2 - (6 - digits) * fontWidth/2,
			hudelement->ypos, 3, value, hudelement->fontWidth, hudelement->fontHeight);
		break;
	case 2:
		CG_DrawFieldFontsize(hudelement->xpos + hudelement->width - 3*fontWidth, hudelement->ypos,
			3, value, hudelement->fontWidth, hudelement->fontHeight);
		break;
	}
}

static void CG_ScanForCrosshairEntity(void)
{
	trace_t		trace;
	vec3_t		start, end;
	int			content;

	VectorCopy(cg.refdef.vieworg, start);
	VectorMA(start, 131072, cg.refdef.viewaxis[0], end);

	CG_Trace(&trace, start, vec3_origin, vec3_origin, end,
		cg.snap->ps.clientNum, CONTENTS_SOLID|CONTENTS_BODY);
	if (trace.entityNum >= MAX_CLIENTS) {
		return;
	}

	// if the player is in fog, don't show it
	content = CG_PointContents(trace.endpos, 0);
	if (content & CONTENTS_FOG) {
		return;
	}

	// if the player is invisible, don't show it
	if (cg_entities[ trace.entityNum ].currentState.powerups & (1 << PW_INVIS)) {
		return;
	}

	// update the fade timer
	cg.crosshairClientNum = trace.entityNum;
	cg.crosshairClientTime = cg.time;
}

static void Hud_HealthIcon(int hudnumber)
{
	switch (cg.snap->ps.persistant[PERS_TEAM]) {
	case TEAM_RED:
		CG_DrawHudIcon(hudnumber, qtrue, cgs.media.healthRed);
		break;
	case TEAM_BLUE:
		CG_DrawHudIcon(hudnumber, qtrue, cgs.media.healthBlue);
		break;
	default:
		CG_DrawHudIcon(hudnumber, qtrue, cgs.media.healthYellow);
	}
}

static void Hud_Health(int hudnumber)
{
	int	value, color;
	value = cg.snap->ps.stats[STAT_HEALTH];

	if (value >= 100) {
		color = 7;
	} else if (value > 25) {
		color = 3;
	} else if (value > 0) {
		color = (cg.time >> 8) ? 3 : 4;	// flash
	} else {
		color = 1;
	}

	trap_R_SetColor(g_color_table[color]);
	CG_DrawHudField(hudnumber, value);
	trap_R_SetColor(NULL);
}

static void Hud_Ammo(int hudnumber)
{
	int			value, color;
	centity_t	*cent;

	cent = &cg_entities[cg.snap->ps.clientNum];
	if (!cent->currentState.weapon) {
		return;
	}

	value = cg.snap->ps.ammo[cent->currentState.weapon];
	if (value == -1) {
		return;
	}

	if (cg.predictedPlayerState.weaponstate == WEAPON_FIRING
		&& cg.predictedPlayerState.weaponTime > 100)
	{
		// draw as dark grey when reloading
		color = 9;
	} else if (value >= 0) {
		color = 3;
	} else {
		color = 1;
	}

	trap_R_SetColor(g_color_table[color]);
	CG_DrawHudField(hudnumber, value);
	trap_R_SetColor(NULL);
}

static void Hud_AmmoIcon(int hudnumber)
{
	if (!cg_drawIcons.integer) {
		return;
	}

	qhandle_t	icon;

	icon = cg_weapons[cg.predictedPlayerState.weapon].ammoIcon;
	if (icon) {
		CG_DrawHudIcon(hudnumber, qtrue, icon);
	}
}

static void Hud_Armor(int hudnumber)
{
	int	value, color;
	value = cg.snap->ps.stats[STAT_ARMOR];
	if (value <= 0) {
		return;
	}

	if (value >= 100) {
		color = 7;
	} else {
		color = 3;
	}

	trap_R_SetColor(g_color_table[color]);
	CG_DrawHudField(hudnumber, value);
	trap_R_SetColor(NULL);
}

static void Hud_ArmorIcon(int hudnumber)
{
	switch (cg.snap->ps.persistant[PERS_TEAM]) {
	case TEAM_RED:
		CG_DrawHudIcon(hudnumber, qtrue, cgs.media.armorRed);
		break;
	case TEAM_BLUE:
		CG_DrawHudIcon(hudnumber, qtrue, cgs.media.armorBlue);
		break;
	default:
		CG_DrawHudIcon(hudnumber, qtrue, cgs.media.armorYellow);
	}
}

static void Hud_FPS(int hudnumber)
{
	char		*s;
	static int	previousTimes[FPS_FRAMES];
	static int	index;
	int		i, total;
	int		fps;
	static	int	previous;
	int		t, frameTime;

	// don't use serverTime, because that will be drifting to
	// correct for internet lag changes, timescales, timedemos, etc
	t = trap_Milliseconds();
	frameTime = t - previous;
	previous = t;

	previousTimes[index % FPS_FRAMES] = frameTime;
	index++;
	if (index > FPS_FRAMES) {
		// average multiple frames together to smooth changes out a bit
		total = 0;
		for (i = 0; i < FPS_FRAMES; i++) {
			total += previousTimes[i];
		}
		if (!total) {
			total = 1;
		}
		fps = 1000 * FPS_FRAMES / total;

		s = va("%i fps", fps);

		CG_DrawHudString(hudnumber, qtrue, s);
	}
}

static void Hud_Gametime(int hudnumber)
{
	char		*s;
	int			mins, seconds, tens;
	int			msec;

	msec = cg.time - cgs.levelStartTime;

	seconds = msec / 1000;
	mins = seconds / 60;
	seconds -= mins * 60;
	tens = seconds / 10;
	seconds -= tens * 10;

	s = va("%i:%i%i", mins, tens, seconds);

	CG_DrawHudString(hudnumber, qtrue, s);
}

static void Hud_ItemPickupIcon(int hudnumber)
{
	if (!cg.itemPickup || cg.snap->ps.stats[STAT_HEALTH] <= 0) {
		return;
	}

	if (cg.time - cg.itemPickupBlendTime > ICON_BLEND_TIME) {
		return;
	}

	CG_DrawHudIcon(hudnumber, qfalse, cg_items[cg.itemPickup].icon);
}

static void Hud_ItemPickupName(int hudnumber)
{
	if (!cg.itemPickup || cg.snap->ps.stats[STAT_HEALTH] <= 0) {
		return;
	}

	if (cg.time - cg.itemPickupBlendTime > ICON_BLEND_TIME) {
		return;
	}

	CG_DrawHudString(hudnumber, qtrue, bg_itemlist[cg.itemPickup].pickup_name);
}

static void Hud_ItemPickupTime(int hudnumber)
{
	int min, ten, second, msecs;

	if (cg.time - cg.itemPickupBlendTime > ICON_BLEND_TIME) {
		return;
	}

	msecs = cg.itemPickupBlendTime - cgs.levelStartTime;
	second = msecs / 1000;
	min = second / 60;
	second -= min * 60;
	ten = second / 10;
	second -= ten * 10;
	CG_DrawHudString(hudnumber, qtrue, va("%i:%i%i", min, ten, second));
}

static void Hud_AmmoWarning(int hudnumber)
{
	if (cg.snap->ps.stats[STAT_HEALTH] <= 0) {
		return;
	}

	if (!cg.lowAmmoWarning) {
		return;
	}

	if (cg.lowAmmoWarning == 2) {
		CG_DrawHudString(hudnumber, qtrue, "OUT OF AMMO");
	} else {
		CG_DrawHudString(hudnumber, qtrue, "LOW AMMO WARNING");
	}
}

static void Hud_AttackerName(int hudnumber)
{
	int	clientNum, t;

	if (cg.predictedPlayerState.stats[STAT_HEALTH] <= 0) {
		return;
	}

	if (!cg.attackerTime) {
		return;
	}

	clientNum = cg.predictedPlayerState.persistant[PERS_ATTACKER];
	if (clientNum < 0 || clientNum >= MAX_CLIENTS || clientNum == cg.snap->ps.clientNum) {
		return;
	}

	t = cg.time - cg.attackerTime;
	if (t > ATTACKER_HEAD_TIME) {
		cg.attackerTime = 0;
		return;
	}

	CG_DrawHudString(hudnumber, qtrue, cgs.clientinfo[clientNum].name);
}

static void Hud_AttackerIcon(int hudnumber)
{
	int	clientNum, t;

	if (cg.predictedPlayerState.stats[STAT_HEALTH] <= 0) {
		return;
	}

	if (!cg.attackerTime) {
		return;
	}

	clientNum = cg.predictedPlayerState.persistant[PERS_ATTACKER];
	if (clientNum < 0 || clientNum >= MAX_CLIENTS || clientNum == cg.snap->ps.clientNum) {
		return;
	}

	t = cg.time - cg.attackerTime;
	if (t > ATTACKER_HEAD_TIME) {
		cg.attackerTime = 0;
		return;
	}

	CG_DrawHudIcon(hudnumber, qtrue, cgs.clientinfo[clientNum].model->modelIcon);
}

static void Hud_Speed(int hudnumber)
{
	CG_DrawHudString(hudnumber, qtrue, va("%i ups", (int) cg.xyspeed));
}

static void Hud_TargetName(int hudnumber)
{
	char	*name;

	if (cg.renderingThirdPerson || !cg_drawCrosshair.integer || !cg_drawCrosshairNames.integer) {
		return;
	}

	CG_ScanForCrosshairEntity();

	if (cg.time > cg.crosshairClientTime + cgs.hud[hudnumber].time) {
		return;
	}

	name = cgs.clientinfo[cg.crosshairClientNum].name;
	CG_DrawHudString(hudnumber, qfalse, name);
}

static void Hud_TargetStatus(int hudnumber)
{
	char			statusBuf[16];
	int				*stats;
	int				healthColor, armorColor;

	if (cg.renderingThirdPerson || !cg_drawCrosshair.integer || !cg_drawCrosshairNames.integer) {
		return;
	}

	CG_ScanForCrosshairEntity();

	if (cgs.clientinfo[cg.clientNum].team == TEAM_SPECTATOR) {
		return;
	}

	if (cg.snap->ps.persistant[PERS_TEAM] != TEAM_RED
		&& cg.snap->ps.persistant[PERS_TEAM] != TEAM_BLUE)
	{
		return;
	}

	if (cg.snap->ps.persistant[PERS_TEAM] != cgs.clientinfo[cg.crosshairClientNum].team) {
		return;
	}

	stats = cg_entities[cg.crosshairClientNum].currentState.pubStats;
	if (stats[PUBSTAT_HEALTH] >= 100) {
		healthColor = 2;
	} else if (stats[PUBSTAT_ARMOR] >= 50) {
		healthColor = 3;
	} else {
		healthColor = 1;
	}

	if (stats[PUBSTAT_ARMOR] >= 100) {
		armorColor = 2;
	} else if (stats[PUBSTAT_ARMOR] >= 50) {
		armorColor = 3;
	} else {
		armorColor = 1;
	}

	Com_sprintf(statusBuf, sizeof statusBuf, "(^%i%i^7|^%i%i^7)",
		healthColor, stats[PUBSTAT_HEALTH], armorColor, stats[PUBSTAT_ARMOR]);
	CG_DrawHudString(hudnumber, qfalse, statusBuf);
}

static void Hud_Warmup(int hudnumber)
{
	if (cg.warmup >= 0) {
		return;
	}
	CG_DrawHudString(HUD_WARMUP, qtrue, "Warmup");
}

static void Hud_Gametype(int hudnumber)
{
	clientInfo_t	*ci1, *ci2;
	int				i;

	if (cgs.gametype != GT_TOURNAMENT) {
		CG_DrawHudString(hudnumber, qtrue, cgs.gametypeName);
		return;
	}

	ci1 = NULL;
	ci2 = NULL;
	for (i = 0; i < cgs.maxclients; i++) {
		if (cgs.clientinfo[i].infoValid && cgs.clientinfo[i].team != TEAM_FREE) {
			continue;
		}

		if (!ci1) {
			ci1 = &cgs.clientinfo[i];
		} else {
			ci2 = &cgs.clientinfo[i];
		}
	}

	if (ci1 && ci2) {
		const char	*s;
		s = va("%s ^7vs %s", ci1->name, ci2->name);
		CG_DrawHudString(hudnumber, qtrue, s);
	}
}

static void Hud_Countdown(int hudnumber)
{
	int			sec;
	const char	*s;

	sec = (cg.warmup - cg.time) / 1000;
	if (sec < 0) {
		sec = 0;
	}

	s = va("Starts in: %i", sec + 1);
	CG_DrawHudString(hudnumber, qtrue, s);
}

static void Hud_WeaponList(int hudnumber)
{
	hudElement_t	*hudelement;
	int			height, width, xpos, ypos, textalign;
	qboolean	horizontal;
	int			boxWidth, boxHeight, x,y;
	int			i;
	int			iconsize;
	float		fontWidth;
	int			icon_xrel, icon_yrel, text_xrel, text_yrel, text_step;
	char		*s;
	int			w;
	vec4_t		charColor;
	int			ammoPack;
	vec4_t		bgcolor;
	int			count;

	hudelement = &cgs.hud[hudnumber];
	height = hudelement->height;
	width = hudelement->width;
	xpos = hudelement->xpos;
	ypos = hudelement->ypos;
	textalign = hudelement->textAlign;
	fontWidth = CG_AdjustWidth(hudelement->fontWidth);

	horizontal = height < width;

	count = 0;
	for (i = WP_MACHINEGUN; i <= WP_BFG; i++) {
		if (cg_weapons[i].weaponIcon) {
			count++;
		}
	}

	if (hudelement->bgcolor[3] != 0) {
		if (cgs.gametype >= GT_TEAM && hudelement->teamBgColor == 1) {
			if (cg.snap->ps.persistant[PERS_TEAM] == TEAM_RED) {
				bgcolor[0] = 1;
				bgcolor[1] = 0;
				bgcolor[2] = 0;
			} else if (cg.snap->ps.persistant[PERS_TEAM] == TEAM_BLUE) {
				bgcolor[0] = 0;
				bgcolor[1] = 0;
				bgcolor[2] = 1;
			}
		} else if (cgs.gametype >= GT_TEAM && hudelement->teamBgColor == 2) {
			if (cg.snap->ps.persistant[PERS_TEAM] == TEAM_BLUE) {
				bgcolor[0] = 1;
				bgcolor[1] = 0;
				bgcolor[2] = 0;
			} else if (cg.snap->ps.persistant[PERS_TEAM] == TEAM_RED) {
				bgcolor[0] = 0;
				bgcolor[1] = 0;
				bgcolor[2] = 1;
			}
		} else {
			bgcolor[0] = hudelement->bgcolor[0];
			bgcolor[1] = hudelement->bgcolor[1];
			bgcolor[2] = hudelement->bgcolor[2];
		}
		bgcolor[3] = hudelement->bgcolor[3];
	} else {
		bgcolor[0] = 0;
		bgcolor[1] = 0;
		bgcolor[2] = 0;
		bgcolor[3] = 0;
	}

	if (horizontal) {
		boxHeight = height;
		boxWidth = width/8;
		x = xpos + width/2 - (boxWidth*count)/2;
		y = ypos;
		CG_FillRect(x, y, count*boxWidth, boxHeight, bgcolor);
	} else {
		boxHeight = height/8;
		boxWidth = width;
		x = xpos;
		y = ypos + height/2 - (boxHeight*count)/2;
		CG_FillRect(x, y, boxWidth, count*boxHeight, bgcolor);
	}

	if (textalign == 0) {
		if (boxHeight < (boxWidth - 3*fontWidth - 6))
			iconsize = boxHeight;
		else
			iconsize = (boxWidth - 3*fontWidth - 6);

		icon_yrel = boxHeight/2 - iconsize/2;
		icon_xrel = boxWidth - iconsize - 2;

		text_yrel = boxHeight/2 - hudelement->fontHeight/2;
		text_xrel = 2;
		text_step = 0;
	} else if (textalign == 2) {
		if (boxHeight < (boxWidth - 3*fontWidth - 6))
			iconsize = boxHeight;
		else
			iconsize = (boxWidth - 3*fontWidth - 6);

		icon_yrel = boxHeight/2 - iconsize/2;
		icon_xrel = 2;

		text_yrel = boxHeight/2 - hudelement->fontHeight/2;
		text_xrel = boxWidth - 3*fontWidth - 2;
		text_step = fontWidth;
	} else {
		if (boxWidth < (boxHeight - hudelement->fontHeight - 6))
			iconsize = boxWidth;
		else
			iconsize = (boxHeight - hudelement->fontHeight - 6);

		icon_xrel = boxWidth/2 - iconsize/2;
		icon_yrel = 2;

		text_xrel = boxWidth/2 - 3*fontWidth/2;
		text_yrel = boxHeight - hudelement->fontHeight - 2;
		text_step = fontWidth/2;
	}

	if (iconsize < 0)
		iconsize = 0;

	for (i = WP_MACHINEGUN; i <= WP_BFG; i++) {
		if (!cg_weapons[i].weaponIcon) {
			continue;
		}

		switch (i) {
			case WP_MACHINEGUN:
				ammoPack = 50;
				break;
			case WP_SHOTGUN:
				ammoPack = 10;
				break;
			case WP_GRENADE_LAUNCHER:
				ammoPack = 5;
				break;
			case WP_ROCKET_LAUNCHER:
				ammoPack = 5;
				break;
			case WP_LIGHTNING:
				ammoPack = 60;
				break;
			case WP_RAILGUN:
				ammoPack = 5;
				break;
			case WP_PLASMAGUN:
				ammoPack = 30;
				break;
			case WP_BFG:
				ammoPack = 15;
				break;
			default:
				ammoPack = 200;
				break;
		}

		if (cg.snap->ps.ammo[i] < ammoPack/2+1) {
			charColor[0] = 1.0;
			charColor[1] = 0.0;
			charColor[2] = 0.0;
		} else if (cg.snap->ps.ammo[i] < ammoPack) {
			charColor[0] = 1.0;
			charColor[1] = 1.0;
			charColor[2] = 0.0;
		} else {
			charColor[0] = 1.0;
			charColor[1] = 1.0;
			charColor[2] = 1.0;
		}

		if ((i == cg.weaponSelect && !(cg.snap->ps.pm_flags & PMF_FOLLOW)) || ((i == cg_entities[cg.snap->ps.clientNum].currentState.weapon) && (cg.snap->ps.pm_flags & PMF_FOLLOW))) {
			if (hudelement->imageHandle)
				CG_DrawPic(x, y, boxWidth, boxHeight, hudelement->imageHandle);
			else if (hudelement->fill)
				CG_FillRect(x, y, boxWidth, boxHeight, hudelement->color);
			else
				CG_DrawRect(x, y, boxWidth, boxHeight, 2, hudelement->color);
		}

		CG_DrawAdjustPic(x + icon_xrel, y + icon_yrel, iconsize, iconsize, cg_weapons[i].weaponIcon);

		if (cg.snap->ps.stats[STAT_WEAPONS] & (1 << i)) {
			if (cg.snap->ps.ammo[i] == 0) {
				CG_DrawAdjustPic(x + icon_xrel, y + icon_yrel, iconsize, iconsize, cgs.media.noammoShader);
			}

			/** Draw Weapon Ammo **/
			if (cg.snap->ps.ammo[i] != -1) {
				s = va("%i", cg.snap->ps.ammo[i]);
				w = CG_DrawStrlen(s);
				CG_DrawStringExt(x + text_xrel + (3 - w)*text_step, y + text_yrel, s, charColor,
					qfalse, hudelement->textstyle & 1, hudelement->fontWidth, hudelement->fontHeight, 0);
			}
		}

		if (horizontal)
			x += boxWidth;
		else
			y += boxHeight;
	}
}

static void Hud_Follow(int hudnumber)
{
	const char	*name;
	if (!(cg.snap->ps.pm_flags & PMF_FOLLOW)) {
		return;
	}
	name = cgs.clientinfo[cg.snap->ps.clientNum].name;
	CG_DrawHudString(hudnumber, qtrue, va("following %s", name));
}

void CG_DrawHud()
{
	int	i;

	struct {
		int hudnumber;
		void (*func)(int);
	} hudCallbacks[] = {
		{ HUD_HEALTHICON, &Hud_HealthIcon },
		{ HUD_HEALTHCOUNT, &Hud_Health },
		{ HUD_ARMORICON, &Hud_ArmorIcon },
		{ HUD_ARMORCOUNT, &Hud_Armor },
		{ HUD_AMMOICON, &Hud_AmmoIcon },
		{ HUD_AMMOCOUNT, &Hud_Ammo },
		{ HUD_FPS, &Hud_FPS },
		{ HUD_GAMETIME, &Hud_Gametime },
		{ HUD_ITEMPICKUPICON, &Hud_ItemPickupIcon },
		{ HUD_ITEMPICKUPNAME, &Hud_ItemPickupName },
		{ HUD_ITEMPICKUPTIME, &Hud_ItemPickupTime },
		{ HUD_AMMOWARNING, &Hud_AmmoWarning },
		{ HUD_ATTACKERICON, &Hud_AttackerIcon },
		{ HUD_ATTACKERNAME, &Hud_AttackerName },
		{ HUD_SPEED, &Hud_Speed },
		{ HUD_TARGETNAME, &Hud_TargetName },
		{ HUD_TARGETSTATUS, &Hud_TargetStatus },
		{ HUD_WARMUP, Hud_Warmup },
		{ HUD_GAMETYPE, Hud_Gametype },
		{ HUD_COUNTDOWN, Hud_Countdown },
		{ HUD_WEAPONLIST, Hud_WeaponList },
		{ HUD_FOLLOW, Hud_Follow },
		{ HUD_MAX, NULL }
	};

	for (i = 0; hudCallbacks[i].func; ++i) {
		if (!cgs.hud[hudCallbacks[i].hudnumber].inuse) {
			continue;
		}
		hudCallbacks[i].func(hudCallbacks[i].hudnumber);
	}
}


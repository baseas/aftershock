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

#define FPS_FRAMES			4
#define MAX_LAGOMETER_PING	900
#define MAX_LAGOMETER_RANGE	300
#define ICON_BLEND_TIME		3000

/**
Set team color and team background color, does not change alpha channel.
*/
static void CG_SetElementColors(void)
{
	int				i;
	hudElement_t	*element;
	team_t			team;

	for (i = 0; i < HUD_MAX; ++i) {
		element = &cgs.hud[i];

		team = cg.snap->ps.persistant[PERS_TEAM];

		if (element->teamBgColor == 1) {
			VectorCopy(CG_TeamColor(team), element->bgcolor);
		}

		if (element->teamColor == 1) {
			VectorCopy(CG_TeamColor(team), element->color);
		}

		if (team == TEAM_RED) {
			team = TEAM_BLUE;
		} else if (team == TEAM_BLUE) {
			team = TEAM_RED;
		}

		if (element->teamBgColor == 2) {
			VectorCopy(CG_TeamColor(team), element->bgcolor);
		}

		if (element->teamColor == 2) {
			VectorCopy(CG_TeamColor(team), element->color);
		}
	}
}

static void CG_DrawHudIcon(int hudnumber, qhandle_t hShader)
{
	hudElement_t	*hudelement;

	hudelement = &cgs.hud[hudnumber];
	if (!hudelement->inuse) {
		return;
	}

	if (hudelement->fill) {
		CG_FillRect(hudelement->xpos, hudelement->ypos, hudelement->width, hudelement->height,
			hudelement->color);
	}

	trap_R_SetColor(hudelement->color);

	if (hShader) {
		CG_DrawAdjustPic(hudelement->xpos, hudelement->ypos, hudelement->width,
			hudelement->height, hShader);
	}

	trap_R_SetColor(NULL);
}

static void CG_DrawHudString(int hudnumber, qboolean colorize, const char *text)
{
	hudElement_t	*hudelement;
	int				w, x;
	vec4_t			color;
	qboolean		shadow;

	hudelement = &cgs.hud[hudnumber];
	shadow = (hudelement->textStyle & 1);

	if (!hudelement->inuse) {
		return;
	}

	w = CG_StringWidth(hudelement->fontWidth, text);

	if (hudelement->textAlign == 0)
		x = hudelement->xpos;
	else if (hudelement->textAlign == 2)
		x = hudelement->xpos + hudelement->width - w;
	else
		x = hudelement->xpos + hudelement->width/2 - w/2;

	if (colorize) {
		Vector4Copy(hudelement->color, color);
	}
	else {
		Vector4Copy(colorWhite, color);
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

	if (hudelement->color[0] != 1 || hudelement->color[1] != 1 || hudelement->color[2] != 1) {
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

/**
Draw the powerup whose position in the powerup list is 'index'.
Powerups are sorted by their time at which they disappear.
*/
static powerup_t CG_GetActivePowerup(int index)
{
	int		validPowerups[MAX_POWERUPS];
	int		i, j, k;
	int		smallerTime;
	playerState_t	*ps;

	ps = &cg.snap->ps;

	if (ps->stats[STAT_HEALTH] <= 0) {
		return PW_NONE;
	}

	// CTF flags have unlimited time (999 seconds)
	for (i = 0, j = 0; i < MAX_POWERUPS; ++i) {
		if (!ps->powerups[i]) {
			continue;
		}
		if (ps->powerups[i] < cg.time || ps->powerups[i] - cg.time >= 999000) {
			continue;
		}
		validPowerups[j++] = i;
	}

	if (j <= index) {
		return PW_NONE;
	}

	for (i = 0; i < j; ++i) {
		// Find number of powerups that run out before this powerup
		smallerTime = 0;

		for (k = 0; k < j; ++k) {
			if (i != k && ps->powerups[validPowerups[k]] <= ps->powerups[validPowerups[i]]) {
				++smallerTime;
			}
		}
		if (smallerTime == index) {
			break;
		}
	}
	return validPowerups[i];
}

static void CG_DrawHudPowerup(int hudnumber, powerup_t powerup)
{
	CG_DrawHudField(hudnumber, (cg.snap->ps.powerups[powerup] - cg.time) / 1000);
}

static void CG_DrawHudPowerupIcon(int hudnumber, powerup_t powerup)
{
	gitem_t		*item;
	item = BG_FindItemForPowerup(powerup);
	if (!item) {
		return;
	}
	CG_DrawHudIcon(hudnumber, cg_items[ITEM_INDEX(item)].icon);
}

static void CG_DrawHudChat(int hudnumber, msgItem_t list[CHAT_HEIGHT], int duration, int index)
{
	int	i;

	// find the index of the oldest chat message to display
	for (i = 0; i < CHAT_HEIGHT; ++i) {
		if (list[i].time == 0) {
			break;
		}
		if (!cg.showChat && (cg.time - list[i].time > duration)) {
			break;
		}
	}

	index = i - index - 1;
	if (index < 0) {
		return;
	}

	CG_DrawHudString(hudnumber, qfalse, list[index].message);
}

static void CG_DrawHudDeathNotice(int hudnumber, int index)
{
	int		i;
	float	x;
	float	y;
	float	width;
	hudElement_t	*hudelement;
	deathNotice_t	*notice;

	for (i = 0; i < DEATHNOTICE_HEIGHT; ++i) {
		if (cgs.deathNotices[i].time == 0) {
			break;
		}
		if (cg.time - cgs.deathNotices[i].time > cg_deathNoticeTime.integer) {
			break;
		}
	}

	index = i - index - 1;
	if (index < 0) {
		return;
	}

	notice = &cgs.deathNotices[index];

	hudelement = &cgs.hud[hudnumber];
	y = hudelement->ypos;

	width = CG_StringWidth(hudelement->fontWidth, notice->target);
	width += CG_StringWidth(hudelement->fontWidth, notice->attacker);
	width += (notice->directHit ? 2 : 1) * (CG_AdjustWidth(hudelement->fontHeight) + 2);

	if (hudelement->textAlign == 0) {
		x = hudelement->xpos;
	} else if (hudelement->textAlign == 2) {
		x = hudelement->xpos + hudelement->width - width;
	} else {
		x = hudelement->xpos + hudelement->width / 2 - width / 2;
	}

	CG_DrawStringExt(x, y, notice->attacker, hudelement->color, qfalse, qfalse,
		hudelement->fontWidth, hudelement->fontHeight, 0);

	x += CG_StringWidth(hudelement->fontWidth, notice->attacker);

	if (notice->directHit) {
		CG_DrawAdjustPic(x + 1 , y, hudelement->fontHeight, hudelement->fontHeight, cgs.media.directHit);
		x += CG_AdjustWidth(hudelement->fontHeight) + 2;
	}

	CG_DrawAdjustPic(x + 1, y, hudelement->fontHeight, hudelement->fontHeight, notice->icon);
	x += CG_AdjustWidth(hudelement->fontHeight) + 2;
	CG_DrawStringExt(x, y, notice->target, hudelement->color, qfalse, qfalse,
		hudelement->fontWidth, hudelement->fontHeight, 0);
}

static void CG_DrawHudScores(int hudnumber, int score)
{
	hudElement_t	*hudelement;
	vec4_t			color;
	int				x, y, w;
	qboolean		shadow;
	const char		*text;
	qboolean		spec;

	spec = (cg.snap->ps.persistant[PERS_TEAM] == TEAM_SPECTATOR);

	if (score == SCORE_NOT_PRESENT) {
		text = "-";
	} else {
		text = va("%i", score);
	}

	hudelement = &cgs.hud[hudnumber];
	shadow = hudelement->textStyle & 1;
	if (!spec) {
		Vector4Copy(hudelement->bgcolor, color);
	} else {
		color[0] = color[1] = color[2] = color[3] = 0.5f;
	}

	CG_FillRect(hudelement->xpos, hudelement->ypos, hudelement->width, hudelement->height, color);
	y = hudelement->ypos + hudelement->height / 2 - hudelement->fontHeight / 2;
	w = CG_DrawStrlen(text) * hudelement->fontWidth;

	if (hudelement->textAlign == 0) {
		x = hudelement->xpos;
	} else if (hudelement->textAlign == 2) {
		x = hudelement->xpos + hudelement->width - w;
	} else {
		x = hudelement->xpos + hudelement->width / 2 - w / 2;
	}

	Vector4Copy(hudelement->color, color);

	CG_DrawStringExt(x, y, text, color, qfalse, shadow,
		hudelement->fontWidth, hudelement->fontHeight, 0);
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
	if (cg_entities[trace.entityNum].currentState.powerups & (1 << PW_INVIS)) {
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
		CG_DrawHudIcon(hudnumber, cgs.media.healthRed);
		break;
	case TEAM_BLUE:
		CG_DrawHudIcon(hudnumber, cgs.media.healthBlue);
		break;
	case TEAM_FREE:
		CG_DrawHudIcon(hudnumber, cgs.media.healthYellow);
	}
}

static void Hud_Health(int hudnumber)
{
	int	value, color;

	if (cg.snap->ps.persistant[PERS_TEAM] == TEAM_SPECTATOR) {
		return;
	}

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

	if (cg.snap->ps.persistant[PERS_TEAM] == TEAM_SPECTATOR) {
		return;
	}

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
	qhandle_t	icon;

	icon = cg_weapons[cg.predictedPlayerState.weapon].ammoIcon;
	if (icon) {
		CG_DrawHudIcon(hudnumber, icon);
	}
}

static void Hud_Armor(int hudnumber)
{
	int	value, color;

	if (cg.snap->ps.persistant[PERS_TEAM] == TEAM_SPECTATOR) {
		return;
	}

	value = cg.snap->ps.stats[STAT_ARMOR];

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
		CG_DrawHudIcon(hudnumber, cgs.media.armorRed);
		break;
	case TEAM_BLUE:
		CG_DrawHudIcon(hudnumber, cgs.media.armorBlue);
		break;
	case TEAM_FREE:
		CG_DrawHudIcon(hudnumber, cgs.media.armorYellow);
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

static void Hud_GameTime(int hudnumber)
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

static void Hud_RoundTime(int hudnumber)
{
	const char	*str;
	int			mins, seconds, tens;
	int			msec;

	if (cgs.gametype != GT_ELIMINATION || cg.warmup || cg.time < cgs.roundStartTime) {
		return;
	}

	msec = cgs.roundTimelimit * 1000 - (cg.time - cgs.roundStartTime);
	msec += 1000;

	seconds = msec / 1000;
	mins = seconds / 60;
	seconds -= mins * 60;
	tens = seconds / 10;
	seconds -= tens * 10;

	str = va("%i:%i%i", mins, tens, seconds);
	CG_DrawHudString(hudnumber, qtrue, str);
}

static void Hud_RealTime(int hudnumber)
{
	qtime_t		now;
	const char	*str;

	trap_RealTime(&now);
	str = va("%02i:%02i", now.tm_hour, now.tm_min);
	CG_DrawHudString(hudnumber, qtrue, str);
}

static void Hud_ItemPickupIcon(int hudnumber)
{
	if (!cg.itemPickup) {
		return;
	}

	if (cg.time - cg.itemPickupBlendTime > ICON_BLEND_TIME) {
		return;
	}

	CG_DrawHudIcon(hudnumber, cg_items[cg.itemPickup].icon);
}

static void Hud_ItemPickupName(int hudnumber)
{
	if (!cg.itemPickup) {
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

	CG_DrawHudIcon(hudnumber, cgs.clientinfo[clientNum].model->modelIcon);
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
	clientInfo_t	*ci;
	int				healthColor, armorColor;

	if (cg.renderingThirdPerson || !cg_drawCrosshair.integer || !cg_drawCrosshairNames.integer) {
		return;
	}

	if (cg.snap->ps.persistant[PERS_TEAM] != TEAM_RED
		&& cg.snap->ps.persistant[PERS_TEAM] != TEAM_BLUE)
	{
		return;
	}

	CG_ScanForCrosshairEntity();

	if (cg.snap->ps.persistant[PERS_TEAM] != cgs.clientinfo[cg.crosshairClientNum].team) {
		return;
	}

	if (cg.time - cg.crosshairClientTime > cgs.hud[hudnumber].time) {
		return;
	}

	ci = &cgs.clientinfo[cg.crosshairClientNum];

	if (ci->health >= 100) {
		healthColor = 2;
	} else if (ci->health >= 50) {
		healthColor = 3;
	} else {
		healthColor = 1;
	}

	if (ci->armor >= 100) {
		armorColor = 2;
	} else if (ci->armor >= 50) {
		armorColor = 3;
	} else {
		armorColor = 1;
	}

	Com_sprintf(statusBuf, sizeof statusBuf, "(^%i%i^7|^%i%i^7)",
		healthColor, ci->health, armorColor, ci->health);
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

	if (cg.warmup <= 0) {
		return;
	}

	if (cgs.gametype != GT_TOURNAMENT) {
		CG_DrawHudString(hudnumber, qtrue, cgs.gametypeName);
		return;
	}

	ci1 = NULL;
	ci2 = NULL;
	for (i = 0; i < cgs.maxclients; i++) {
		if (!cgs.clientinfo[i].infoValid || cgs.clientinfo[i].team != TEAM_FREE) {
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
	int		sec;
	char	*label;

	if (cgs.gametype == GT_ELIMINATION && cg.warmup == 0 && cg.time < cgs.roundStartTime) {
		sec = (cgs.roundStartTime - cg.time) / 1000;
		label = "Round in";
	} else if (cg.warmup > 0) {
		sec = (cg.warmup - cg.time) / 1000;
		label = "Start in";
	} else {
		return;
	}

	if (sec < 0) {
		sec = 0;
	}

	CG_DrawHudString(hudnumber, qtrue, va("%s: %i", label, sec + 1));
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

	if (cg.snap->ps.persistant[PERS_TEAM] == TEAM_SPECTATOR) {
		return;
	}

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
		Vector4Copy(hudelement->bgcolor, bgcolor);
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

		if (cg.snap->ps.ammo[i] == -1) {
			charColor[0] = 1.0;
			charColor[1] = 1.0;
			charColor[2] = 1.0;
			charColor[3] = 1.0;
		} else if (cg.snap->ps.ammo[i] < ammoPack/2+1) {
			charColor[0] = 1.0;
			charColor[1] = 0.0;
			charColor[2] = 0.0;
			charColor[3] = 1.0;
		} else if (cg.snap->ps.ammo[i] < ammoPack) {
			charColor[0] = 1.0;
			charColor[1] = 1.0;
			charColor[2] = 0.0;
			charColor[3] = 1.0;
		} else {
			charColor[0] = 1.0;
			charColor[1] = 1.0;
			charColor[2] = 1.0;
			charColor[3] = 1.0;
		}

		if ((i == cg.weaponSelect && !(cg.snap->ps.pm_flags & PMF_FOLLOW)) || ((i == cg_entities[cg.snap->ps.clientNum].currentState.weapon) && (cg.snap->ps.pm_flags & PMF_FOLLOW))) {
			if (hudelement->fill) {
				CG_FillRect(x, y, boxWidth, boxHeight, hudelement->color);
			} else {
				CG_DrawRect(x, y, boxWidth, boxHeight, 2, hudelement->color);
			}
		}

		CG_DrawAdjustPic(x + icon_xrel, y + icon_yrel, iconsize, iconsize, cg_weapons[i].weaponIcon);

		if (cg.snap->ps.stats[STAT_WEAPONS] & (1 << i)) {
			if (cg.snap->ps.ammo[i] == 0) {
				CG_DrawAdjustPic(x + icon_xrel, y + icon_yrel, iconsize, iconsize, cgs.media.noammoShader);
			}

			/** Draw Weapon Ammo **/
			if (cg.snap->ps.ammo[i] == -1) {
				s = "inf";
			} else {
				s = va("%i", cg.snap->ps.ammo[i]);
			}

			w = CG_DrawStrlen(s);
			CG_DrawStringExt(x + text_xrel + (3 - w)*text_step, y + text_yrel, s, charColor,
				qfalse, hudelement->textStyle & 1, hudelement->fontWidth, hudelement->fontHeight, 0);
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

	if (cg.snap->ps.pm_flags & PMF_FOLLOW) {
		name = cgs.clientinfo[cg.snap->ps.clientNum].name;
		CG_DrawHudString(hudnumber, qtrue, va("following %s", name));
	} else if (cgs.clientinfo[cg.clientNum].team == TEAM_SPECTATOR) {
		CG_DrawHudString(hudnumber, qtrue, "SPECTATOR");
	}
}

static void Hud_NetgraphPing(int hudnumber)
{
	if (cgs.localServer) {
		return;
	}
	CG_DrawHudString(hudnumber, qtrue, va("%i ms", cg.snap->ping));
}

static void Hud_Netgraph(int hudnumber)
{
	int		a, i;
	float	v;
	float	ax, ay, aw, ah, mid, range;
	int		color;
	float	vscale;

	ax = cgs.hud[hudnumber].xpos;
	ay = cgs.hud[hudnumber].ypos;
	aw = cgs.hud[hudnumber].width;
	ah = cgs.hud[hudnumber].height;
	CG_AdjustFrom640(&ax, &ay, &aw, &ah);

	if (cg.drawDisconnect) {
		// blink the icon
		if ((cg.time >> 9) & 1) {
			return;
		}
		trap_R_DrawStretchPic(ax, ay, aw, ah, 0, 0, 1, 1, cgs.media.netgraph);
		return;
	}

	if (cgs.localServer) {
		return;
	}

	trap_R_SetColor(NULL);
	CG_DrawHudIcon(hudnumber, 0);

	color = -1;
	range = ah / 3;
	mid = ay + range;

	vscale = range / MAX_LAGOMETER_RANGE;

	// draw the frame interpoalte / extrapolate graph
	for (a = 0; a < aw; a++) {
		i = (cg.lagometer.frameCount - 1 - a) & (LAG_SAMPLES - 1);
		v = cg.lagometer.frameSamples[i];
		v *= vscale;
		if (v > 0) {
			if (color != 1) {
				color = 1;
				trap_R_SetColor(g_color_table[ColorIndex(COLOR_YELLOW)]);
			}
			if (v > range) {
				v = range;
			}
			trap_R_DrawStretchPic (ax + aw - a, mid - v, 1, v, 0, 0, 0, 0, cgs.media.whiteShader);
		} else if (v < 0) {
			if (color != 2) {
				color = 2;
				trap_R_SetColor(g_color_table[ColorIndex(COLOR_BLUE)]);
			}
			v = -v;
			if (v > range) {
				v = range;
			}
			trap_R_DrawStretchPic(ax + aw - a, mid, 1, v, 0, 0, 0, 0, cgs.media.whiteShader);
		}
	}

	// draw the snapshot latency / drop graph
	range = ah / 2;
	vscale = range / MAX_LAGOMETER_PING;

	for (a = 0; a < aw; a++) {
		i = (cg.lagometer.snapshotCount - 1 - a) & (LAG_SAMPLES - 1);
		v = cg.lagometer.snapshotSamples[i];
		if (v > 0) {
			if (cg.lagometer.snapshotFlags[i] & SNAPFLAG_RATE_DELAYED) {
				if (color != 5) {
					color = 5;	// YELLOW for rate delay
					trap_R_SetColor(g_color_table[ColorIndex(COLOR_YELLOW)]);
				}
			} else {
				if (color != 3) {
					color = 3;
					trap_R_SetColor(g_color_table[ColorIndex(COLOR_GREEN)]);
				}
			}
			v = v * vscale;
			if (v > range) {
				v = range;
			}
			trap_R_DrawStretchPic(ax + aw - a, ay + ah - v, 1, v,
				0, 0, 0, 0, cgs.media.whiteShader);
		} else if (v < 0) {
			if (color != 4) {
				color = 4;		// RED for dropped snapshots
				trap_R_SetColor(g_color_table[ColorIndex(COLOR_RED)]);
			}
			trap_R_DrawStretchPic(ax + aw - a, ay + ah - range, 1, range,
				0, 0, 0, 0, cgs.media.whiteShader);
		}
	}

	trap_R_SetColor(NULL);

	if (cg_nopredict.integer || cg_synchronousClients.integer) {
		CG_DrawBigString(ax, ay, "snc", 1.0);
	}
}

static void Hud_ScoreOwn(int hudnumber)
{
	int			score;

	if (cgs.gametype >= GT_TEAM) {
		if (cg.snap->ps.persistant[PERS_TEAM] == TEAM_RED) {
			score = cgs.scores1;
		} else {
			score = cgs.scores2;
		}
	} else {
		score = cg.snap->ps.persistant[PERS_SCORE];
	}

	CG_DrawHudScores(hudnumber, score);
}

static void Hud_ScoreNme(int hudnumber)
{
	int			score;

	if (cgs.gametype >= GT_TEAM) {
		if (cg.snap->ps.persistant[PERS_TEAM] == TEAM_RED) {
			score = cgs.scores2;
		} else {
			score = cgs.scores1;
		}
	} else {
		score = cgs.scores1;
	}

	CG_DrawHudScores(hudnumber, score);
}

static void Hud_FlagStatus(int hudnumber)
{
	int		team;
	gitem_t	*item;

	if (cgs.gametype != GT_CTF) {
		return;
	}

	team = cg.snap->ps.persistant[PERS_TEAM];
	if (team == TEAM_RED) {
		item = BG_FindItemForPowerup(PW_REDFLAG);
	} else {
		item = BG_FindItemForPowerup(PW_BLUEFLAG);
	}

	if (!item) {
		return;
	}

	if (team == TEAM_RED && cgs.redflag >= 0 && cgs.redflag <= 2) {
		CG_DrawHudIcon(hudnumber, cgs.media.redFlagShader[cgs.redflag]);
	} else if (team == TEAM_BLUE && cgs.blueflag >= 0 && cgs.blueflag <= 2) {
		CG_DrawHudIcon(hudnumber, cgs.media.blueFlagShader[cgs.blueflag]);
	}
}

static void Hud_ScoreLimit(int hudnumber)
{
	int	limit;

	if (cgs.gametype == GT_CTF || cgs.gametype == GT_ELIMINATION) {
		limit = cgs.capturelimit;
	} else {
		limit = cgs.fraglimit;
	}

	if (limit) {
		CG_DrawHudScores(hudnumber, limit);
	}
}

static void Hud_Reward(int hudnumber)
{
	if (!cg_drawRewards.integer || cg.rewardStack <= 0) {
		return;
	}
	if (cg.time - cg.rewardTime > REWARD_TIME) {
		return;
	}
	CG_DrawHudIcon(hudnumber, cg.rewardShader[0]);
}

static void Hud_RewardCount(int hudnumber)
{
	if (!cg_drawRewards.integer || cg.rewardStack <= 0) {
		return;
	}
	if (cg.time - cg.rewardTime > REWARD_TIME) {
		return;
	}
	CG_DrawHudString(hudnumber, qtrue, va("%i", cg.rewardCount[0]));
}

static void Hud_Vote(int hudnumber)
{
	char	*str;
	int		sec;

	if (!cgs.voteTime) {
		return;
	}

	sec = (VOTE_TIME - (cg.time - cgs.voteTime)) / 1000;
	if (sec < 0) {
		sec = 0;
	}

	str = va("VOTE(%i): %s yes:%i no:%i", sec, cgs.voteString, cgs.voteYes, cgs.voteNo);
	CG_DrawHudString(hudnumber, qtrue, str);
}

static void Hud_Holdable(int hudnumber)
{
	int	index;

	index = cg.snap->ps.stats[STAT_HOLDABLE_ITEM];
	if (index <= 0 || index >= MAX_ITEMS) {
		return;
	}

	CG_DrawHudIcon(hudnumber, cg_items[index].icon);
}

static void Hud_Help(int hudnumber)
{
	if (cg.snap->ps.persistant[PERS_TEAM] == TEAM_SPECTATOR) {
		CG_DrawHudString(hudnumber, qtrue, "Press ESCAPE and use the JOIN menu to play");
		return;
	}

	if (cgs.startWhenReady && cg.warmup < 0) {
		CG_DrawHudString(hudnumber, qtrue, "Press F3 to get ready");
		return;
	}
}

static void Hud_TeamCountOwn(int hudnumber)
{
	if (cgs.gametype != GT_ELIMINATION) {
		return;
	}

	if (cg.snap->ps.persistant[PERS_TEAM] == TEAM_RED) {
		CG_DrawHudScores(hudnumber, cgs.redLivingCount);
	} else {
		CG_DrawHudScores(hudnumber, cgs.blueLivingCount);
	}
}

static void Hud_TeamCountNme(int hudnumber)
{
	if (cgs.gametype != GT_ELIMINATION) {
		return;
	}

	if (cg.snap->ps.persistant[PERS_TEAM] == TEAM_RED) {
		CG_DrawHudScores(hudnumber, cgs.blueLivingCount);
	} else {
		CG_DrawHudScores(hudnumber, cgs.redLivingCount);
	}
}

void CG_DrawHud()
{
	int	i;

	struct {
		int			hudnumber;
		void		(*func)(int);
		qboolean	drawWhenDead;
	} hudCallbacks[] = {
		{ HUD_HEALTHICON, &Hud_HealthIcon, qfalse },
		{ HUD_HEALTHCOUNT, &Hud_Health, qfalse },
		{ HUD_ARMORICON, &Hud_ArmorIcon, qfalse },
		{ HUD_ARMORCOUNT, &Hud_Armor, qfalse },
		{ HUD_AMMOICON, &Hud_AmmoIcon, qfalse },
		{ HUD_AMMOCOUNT, &Hud_Ammo, qfalse },
		{ HUD_FPS, &Hud_FPS, qtrue },
		{ HUD_GAMETIME, &Hud_GameTime, qtrue },
		{ HUD_ROUNDTIME, &Hud_RoundTime, qtrue },
		{ HUD_REALTIME, Hud_RealTime, qtrue },
		{ HUD_ITEMPICKUPICON, &Hud_ItemPickupIcon, qfalse },
		{ HUD_ITEMPICKUPNAME, &Hud_ItemPickupName, qfalse },
		{ HUD_ITEMPICKUPTIME, &Hud_ItemPickupTime, qfalse },
		{ HUD_AMMOWARNING, &Hud_AmmoWarning, qfalse },
		{ HUD_ATTACKERICON, &Hud_AttackerIcon, qtrue },
		{ HUD_ATTACKERNAME, &Hud_AttackerName, qtrue },
		{ HUD_SPEED, &Hud_Speed, qfalse },
		{ HUD_TARGETNAME, &Hud_TargetName, qfalse },
		{ HUD_TARGETSTATUS, &Hud_TargetStatus, qfalse },
		{ HUD_WARMUP, Hud_Warmup, qtrue },
		{ HUD_GAMETYPE, Hud_Gametype, qtrue },
		{ HUD_COUNTDOWN, Hud_Countdown, qtrue },
		{ HUD_WEAPONLIST, Hud_WeaponList, qfalse },
		{ HUD_FOLLOW, Hud_Follow, qtrue },
		{ HUD_NETGRAPHPING, Hud_NetgraphPing, qtrue },
		{ HUD_NETGRAPH, Hud_Netgraph, qtrue },
		{ HUD_SCOREOWN, Hud_ScoreOwn, qtrue },
		{ HUD_SCORENME, Hud_ScoreNme, qtrue },
		{ HUD_FS_OWN, Hud_FlagStatus, qtrue },
		{ HUD_FS_NME, Hud_FlagStatus, qtrue },
		{ HUD_SCORELIMIT, Hud_ScoreLimit, qtrue },
		{ HUD_REWARD, Hud_Reward, qtrue },
		{ HUD_REWARDCOUNT, Hud_RewardCount, qtrue },
		{ HUD_VOTEMSG, Hud_Vote, qtrue },
		{ HUD_HOLDABLE, Hud_Holdable, qtrue },
		{ HUD_HELP, Hud_Help, qtrue },
		{ HUD_TC_OWN, Hud_TeamCountOwn, qtrue },
		{ HUD_TC_NME, Hud_TeamCountNme, qtrue },
		{ HUD_MAX, 0, qfalse }
	};

	if (cg.showInfo || cg.scoreBoardShowing || !cg_drawHud.integer) {
		return;
	}

	CG_SetElementColors();

	for (i = 0; hudCallbacks[i].func; ++i) {
		if (!cgs.hud[hudCallbacks[i].hudnumber].inuse) {
			continue;
		} else if (!hudCallbacks[i].drawWhenDead && cg.snap->ps.stats[STAT_HEALTH] <= 0) {
			continue;
		}
		hudCallbacks[i].func(hudCallbacks[i].hudnumber);
	}

	for (i = 0; i < CHAT_HEIGHT; ++i) {
		CG_DrawHudChat(HUD_CHAT1 + i, cgs.chatMessages, cg_chatTime.integer, i);
		CG_DrawHudChat(HUD_TEAMCHAT1 + i, cgs.teamChatMessages, cg_teamChatTime.integer, i);
	}

	for (i = 0; i < DEATHNOTICE_HEIGHT; ++i) {
		CG_DrawHudDeathNotice(HUD_DEATHNOTICE1 + i, i);
	}

	for (i = 0; i < 4; ++i) {
		powerup_t	powerup;
		if (!cgs.hud[HUD_PU1 + i].inuse || !cgs.hud[HUD_PU1ICON + i].inuse) {
			continue;
		}
		powerup = CG_GetActivePowerup(i);
		if (powerup == PW_NONE) {
			continue;
		}
		CG_DrawHudPowerup(HUD_PU1 + i, powerup);
		CG_DrawHudPowerupIcon(HUD_PU1ICON + i, powerup);
	}
}



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
		CG_DrawPic(hudelement->xpos, hudelement->ypos, hudelement->width, hudelement->height, hudelement->imageHandle);
		return;
	}
	if (hShader) {
		CG_DrawPic(hudelement->xpos, hudelement->ypos, hudelement->width, hudelement->height, hShader);
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

	w = CG_DrawStrlen(text) * hudelement->fontWidth;

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

	CG_DrawStringExt(x, hudelement->ypos, text, color, qfalse, shadow, hudelement->fontWidth, hudelement->fontHeight, 0);
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

	x += fontWidth * (width - l);

	ptr = num;
	while (*ptr && l) {
		if (*ptr == '-') {
			frame = STAT_MINUS;
		} else {
			frame = *ptr -'0';
		}

		CG_DrawPic(x, y, fontWidth, fontHeight, cgs.media.numberShaders[frame]);
		x += fontWidth;
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
	hudelement = &cgs.hud[hudnumber];

	if (hudelement->color[0] == 0 && hudelement->color[1] == 0 && hudelement->color[2] == 0) {
		trap_R_SetColor(hudelement->color);
	}

	switch (hudelement->textAlign) {
	case 0:
		if (value >= 100)
			CG_DrawFieldFontsize(hudelement->xpos, hudelement->ypos, 3, value, hudelement->fontWidth, hudelement->fontHeight);
		else if (value >= 10)
			CG_DrawFieldFontsize(hudelement->xpos-hudelement->fontWidth, hudelement->ypos, 3, value, hudelement->fontWidth, hudelement->fontHeight);
		else
			CG_DrawFieldFontsize(hudelement->xpos-2*hudelement->fontWidth, hudelement->ypos, 3, value, hudelement->fontWidth, hudelement->fontHeight);
		break;
	case 1:
		if (value >= 100)
			CG_DrawFieldFontsize(hudelement->xpos+hudelement->width/2 - 3*hudelement->fontWidth/2, hudelement->ypos, 3, value, hudelement->fontWidth, hudelement->fontHeight);
		else if (value >= 10)
			CG_DrawFieldFontsize(hudelement->xpos+hudelement->width/2 - 4*hudelement->fontWidth/2, hudelement->ypos, 3, value, hudelement->fontWidth, hudelement->fontHeight);
		else
			CG_DrawFieldFontsize(hudelement->xpos+hudelement->width/2 - 5*hudelement->fontWidth/2, hudelement->ypos, 3, value, hudelement->fontWidth, hudelement->fontHeight);
		break;
	case 2:
		CG_DrawFieldFontsize(hudelement->xpos+hudelement->width - 3*hudelement->fontWidth, hudelement->ypos, 3, value, hudelement->fontWidth, hudelement->fontHeight);
		break;
	}
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
		{ 0, NULL }
	};

	for (i = 0; hudCallbacks[i].func; ++i) {
		if (!cgs.hud[hudCallbacks[i].hudnumber].inuse) {
			continue;
		}
		hudCallbacks[i].func(hudCallbacks[i].hudnumber);
	}
}


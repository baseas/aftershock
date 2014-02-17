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
// cg_draw.c -- draw all of the graphical elements during
// active (after loading) gameplay

#include "cg_local.h"

int	drawTeamOverlayModificationCount = -1;
int	sortedTeamPlayers[TEAM_MAXOVERLAY];
int	numSortedTeamPlayers;

char	systemChat[256];
char	teamChat1[256];
char	teamChat2[256];

void CG_Draw3DModel(float x, float y, float w, float h, qhandle_t model,
	qhandle_t skin, vec3_t origin, vec3_t angles)
{
	refdef_t		refdef;
	refEntity_t		ent;

	CG_AdjustFrom640(&x, &y, &w, &h);

	memset(&refdef, 0, sizeof(refdef));

	memset(&ent, 0, sizeof(ent));
	AnglesToAxis(angles, ent.axis);
	VectorCopy(origin, ent.origin);
	ent.hModel = model;
	ent.customSkin = skin;
	ent.renderfx = RF_NOSHADOW;		// no stencil shadows

	refdef.rdflags = RDF_NOWORLDMODEL;

	AxisClear(refdef.viewaxis);

	refdef.fov_x = 30;
	refdef.fov_y = 30;

	refdef.x = x;
	refdef.y = y;
	refdef.width = w;
	refdef.height = h;

	refdef.time = cg.time;

	trap_R_ClearScene();
	trap_R_AddRefEntityToScene(&ent);
	trap_R_RenderScene(&refdef);
}

/**
Used for both the status bar and the scoreboard
*/
void CG_DrawFlagModel(float x, float y, float w, float h, int team, qboolean force2D)
{
	gitem_t *item;

	if (team == TEAM_RED) {
		item = BG_FindItemForPowerup(PW_REDFLAG);
	} else if (team == TEAM_BLUE) {
		item = BG_FindItemForPowerup(PW_BLUEFLAG);
	} else {
		return;
	}
	if (item) {
	  CG_DrawAdjustPic(x, y, w, h, cg_items[ ITEM_INDEX(item) ].icon);
	}
}

/* UPPER RIGHT CORNER */

static float CG_DrawSnapshot(float y)
{
	char		*s;
	int			w;

	s = va("time:%i snap:%i cmd:%i", cg.snap->serverTime,
		cg.latestSnapshotNum, cgs.serverCommandSequence);
	w = CG_DrawStrlen(s) * BIGCHAR_WIDTH;

	CG_DrawBigString(635 - w, y + 2, s, 1.0F);

	return y + BIGCHAR_HEIGHT + 4;
}

static void CG_GetColorForHealth(int health, int armor, vec4_t hcolor)
{
	int		count;
	int		max;

	// calculate the total points of damage that can
	// be sustained at the current health / armor level
	if (health <= 0) {
		VectorClear(hcolor);	// black
		hcolor[3] = 1;
		return;
	}
	count = armor;
	max = health * ARMOR_PROTECTION / (1.0 - ARMOR_PROTECTION);
	if (max < count) {
		count = max;
	}
	health += count;

	// set the color based on health
	hcolor[0] = 1.0;
	hcolor[3] = 1.0;
	if (health >= 100) {
		hcolor[2] = 1.0;
	} else if (health < 66) {
		hcolor[2] = 0;
	} else {
		hcolor[2] = (health - 66) / 33.0;
	}

	if (health > 60) {
		hcolor[1] = 1.0;
	} else if (health < 30) {
		hcolor[1] = 0;
	} else {
		hcolor[1] = (health - 30) / 30.0;
	}
}

static float CG_DrawTeamOverlay(float y, qboolean right, qboolean upper)
{
	int x, w, h, xx;
	int i, j, len;
	const char *p;
	vec4_t		hcolor;
	int pwidth, lwidth;
	int plyrs;
	char st[16];
	clientInfo_t	*ci;
	gitem_t	*item;
	int ret_y, count;

	if (!cg_drawTeamOverlay.integer) {
		return y;
	}

	if (cg.snap->ps.persistant[PERS_TEAM] != TEAM_RED && cg.snap->ps.persistant[PERS_TEAM] != TEAM_BLUE) {
		return y; // Not on any team
	}

	plyrs = 0;

	// max player name width
	pwidth = 0;
	count = (numSortedTeamPlayers > 8) ? 8 : numSortedTeamPlayers;
	for (i = 0; i < count; i++) {
		ci = &cgs.clientinfo[sortedTeamPlayers[i]];
		if (ci->infoValid && ci->team == cg.snap->ps.persistant[PERS_TEAM]) {
			plyrs++;
			len = CG_DrawStrlen(ci->name);
			if (len > pwidth)
				pwidth = len;
		}
	}

	if (!plyrs)
		return y;

	if (pwidth > TEAM_OVERLAY_MAXNAME_WIDTH)
		pwidth = TEAM_OVERLAY_MAXNAME_WIDTH;

	// max location name width
	lwidth = 0;
	for (i = 1; i < MAX_LOCATIONS; i++) {
		p = CG_ConfigString(CS_LOCATIONS + i);
		if (p && *p) {
			len = CG_DrawStrlen(p);
			if (len > lwidth)
				lwidth = len;
		}
	}

	if (lwidth > TEAM_OVERLAY_MAXLOCATION_WIDTH)
		lwidth = TEAM_OVERLAY_MAXLOCATION_WIDTH;

	w = (pwidth + lwidth + 4 + 7) * TINYCHAR_WIDTH;

	if (right)
		x = 640 - w;
	else
		x = 0;

	h = plyrs * TINYCHAR_HEIGHT;

	if (upper) {
		ret_y = y + h;
	} else {
		y -= h;
		ret_y = y;
	}

	if (cg.snap->ps.persistant[PERS_TEAM] == TEAM_RED) {
		hcolor[0] = 1.0f;
		hcolor[1] = 0.0f;
		hcolor[2] = 0.0f;
		hcolor[3] = 0.33f;
	} else { // if (cg.snap->ps.persistant[PERS_TEAM] == TEAM_BLUE)
		hcolor[0] = 0.0f;
		hcolor[1] = 0.0f;
		hcolor[2] = 1.0f;
		hcolor[3] = 0.33f;
	}

	for (i = 0; i < count; i++) {
		ci = &cgs.clientinfo[sortedTeamPlayers[i]];
		if (ci->infoValid && ci->team == cg.snap->ps.persistant[PERS_TEAM]) {

			hcolor[0] = hcolor[1] = hcolor[2] = hcolor[3] = 1.0;

			xx = x + TINYCHAR_WIDTH;

			CG_DrawStringExt(xx, y,
				ci->name, hcolor, qfalse, qfalse,
				TINYCHAR_WIDTH, TINYCHAR_HEIGHT, TEAM_OVERLAY_MAXNAME_WIDTH);

			if (lwidth) {
				p = CG_ConfigString(CS_LOCATIONS + ci->location);
				if (!p || !*p)
					p = "unknown";
//				len = CG_DrawStrlen(p);
//				if (len > lwidth)
//					len = lwidth;

//				xx = x + TINYCHAR_WIDTH * 2 + TINYCHAR_WIDTH * pwidth +
//					((lwidth/2 - len/2) * TINYCHAR_WIDTH);
				xx = x + TINYCHAR_WIDTH * 2 + TINYCHAR_WIDTH * pwidth;
				CG_DrawStringExt(xx, y,
					p, hcolor, qfalse, qfalse, TINYCHAR_WIDTH, TINYCHAR_HEIGHT,
					TEAM_OVERLAY_MAXLOCATION_WIDTH);
			}

			CG_GetColorForHealth(ci->health, ci->armor, hcolor);

			Com_sprintf(st, sizeof(st), "%3i %3i", ci->health, ci->armor);

			xx = x + TINYCHAR_WIDTH * 3 +
				TINYCHAR_WIDTH * pwidth + TINYCHAR_WIDTH * lwidth;

			CG_DrawStringExt(xx, y,
				st, hcolor, qfalse, qfalse,
				TINYCHAR_WIDTH, TINYCHAR_HEIGHT, 0);

			// draw weapon icon
			xx += TINYCHAR_WIDTH * 3;

			if (cg_weapons[ci->weapon].weaponIcon) {
				CG_DrawAdjustPic(xx, y, TINYCHAR_WIDTH, TINYCHAR_HEIGHT,
					cg_weapons[ci->weapon].weaponIcon);
			} else {
				CG_DrawAdjustPic(xx, y, TINYCHAR_WIDTH, TINYCHAR_HEIGHT,
					cgs.media.deferShader);
			}

			// Draw powerup icons
			if (right) {
				xx = x;
			} else {
				xx = x + w - TINYCHAR_WIDTH;
			}
			for (j = 0; j <= PW_NUM_POWERUPS; j++) {
				if (ci->powerups & (1 << j)) {

					item = BG_FindItemForPowerup(j);

					if (item) {
						CG_DrawAdjustPic(xx, y, TINYCHAR_WIDTH, TINYCHAR_HEIGHT,
						trap_R_RegisterShader(item->icon));
						if (right) {
							xx -= TINYCHAR_WIDTH;
						} else {
							xx += TINYCHAR_WIDTH;
						}
					}
				}
			}

			y += TINYCHAR_HEIGHT;
		}
	}

	return ret_y;
//#endif
}

static void CG_DrawUpperRight(stereoFrame_t stereoFrame)
{
	float	y;

	y = 0;

	if (cgs.gametype >= GT_TEAM && cg_drawTeamOverlay.integer == 1) {
		y = CG_DrawTeamOverlay(y, qtrue, qtrue);
	}
	if (cg_drawSnapshot.integer) {
		y = CG_DrawSnapshot(y);
	}
}

/* LOWER RIGHT CORNER */

static void CG_DrawLowerRight(void)
{
	float	y;

	y = 480 - ICON_SIZE;

	if (cgs.gametype >= GT_TEAM && cg_drawTeamOverlay.integer == 2) {
		y = CG_DrawTeamOverlay(y, qtrue, qfalse);
	}
}

static void CG_DrawLowerLeft(void)
{
	float	y;

	y = 480 - ICON_SIZE;

	if (cgs.gametype >= GT_TEAM && cg_drawTeamOverlay.integer == 3) {
		y = CG_DrawTeamOverlay(y, qfalse, qfalse);
	}
}

/* LAGOMETER */

/**
Adds the current interpolate / extrapolate bar for this frame
*/
void CG_AddLagometerFrameInfo(void)
{
	int			offset;

	offset = cg.time - cg.latestSnapshotTime;
	cg.lagometer.frameSamples[cg.lagometer.frameCount & (LAG_SAMPLES - 1)] = offset;
	cg.lagometer.frameCount++;
}

/**
Each time a snapshot is received, log its ping time and
the number of snapshots that were dropped before it.

Pass NULL for a dropped packet.
*/
void CG_AddLagometerSnapshotInfo(snapshot_t *snap)
{
	// dropped packet
	if (!snap) {
		cg.lagometer.snapshotSamples[cg.lagometer.snapshotCount & (LAG_SAMPLES - 1)] = -1;
		cg.lagometer.snapshotCount++;
		return;
	}

	// add this snapshot's info
	cg.lagometer.snapshotSamples[cg.lagometer.snapshotCount & (LAG_SAMPLES - 1)] = snap->ping;
	cg.lagometer.snapshotFlags[cg.lagometer.snapshotCount & (LAG_SAMPLES - 1)] = snap->snapFlags;
	cg.lagometer.snapshotCount++;
}

/**
Should we draw something differnet for long lag vs no packets?
*/
static void CG_DrawDisconnect(void)
{
	int			cmdNum;
	usercmd_t	cmd;
	const char	*s;
	int			w;

	// draw the phone jack if we are completely past our buffers
	cmdNum = trap_GetCurrentCmdNumber() - CMD_BACKUP + 1;
	trap_GetUserCmd(cmdNum, &cmd);
	if (cmd.serverTime <= cg.snap->ps.commandTime || cmd.serverTime > cg.time) {
		// special check for map_restart
		cg.drawDisconnect = qfalse;
		return;
	}

	cg.drawDisconnect = qtrue;

	// also add text in center of screen
	s = "Connection Interrupted";
	w = CG_StringWidth(BIGCHAR_WIDTH, s);
	CG_DrawBigString(320 - w/2, 100, s, 1.0F);
}

/* CENTER PRINTING */

/**
Called for important messages that should stay in the center of the screen
for a few moments
*/
void CG_CenterPrint(const char *str)
{
	char	*s;

	Q_strncpyz(cg.centerPrint, str, sizeof(cg.centerPrint));

	cg.centerPrintTime = cg.time;

	// count the number of lines for centering
	cg.centerPrintLines = 1;
	s = cg.centerPrint;
	while (*s) {
		if (*s == '\n')
			cg.centerPrintLines++;
		s++;
	}
}

static void CG_DrawCenterString(void)
{
	char	*start;
	int		l;
	float	x, y;
	float	*color;

	if (!cg.centerPrintTime) {
		return;
	}

	color = CG_FadeColor(cg.centerPrintTime, 1000 * cg_centertime.value);
	if (!color) {
		return;
	}

	trap_R_SetColor(color);

	start = cg.centerPrint;

	y = SCREEN_HEIGHT / 5;

	while (1) {
		char linebuffer[1024];

		for (l = 0; l < 50; l++) {
			if (!start[l] || start[l] == '\n') {
				break;
			}
			linebuffer[l] = start[l];
		}
		linebuffer[l] = 0;

		x = (SCREEN_WIDTH - CG_StringWidth(BIGCHAR_WIDTH, linebuffer)) / 2;
		CG_DrawStringExt(x, y, linebuffer, color, qfalse, qtrue, BIGCHAR_WIDTH, BIGCHAR_HEIGHT, 0);

		y += BIGCHAR_HEIGHT;
		while (*start && (*start != '\n')) {
			start++;
		}
		if (!*start) {
			break;
		}
		start++;
	}

	trap_R_SetColor(NULL);
}

/* CROSSHAIR */

static void CG_CrosshairGetColor(float *color)
{
	if (cg_crosshairHealth.integer) {
		CG_GetColorForHealth(cg.snap->ps.stats[STAT_HEALTH], cg.snap->ps.stats[STAT_ARMOR], color);
	} else {
		Vector4Copy(cgs.media.crosshairColor, color);
	}

	if (cg.time - cg.lastHitTime >= cg_crosshairHitColorTime.integer) {
		return;
	}

	if (cg_crosshairHitColorStyle.integer == 1) {
		Vector4Copy(cgs.media.crosshairHitColor, color);
	} else if (cg_crosshairHitColorStyle.integer == 2) {
		float	factor;
		factor = cg.lastHitDamage / 200.0f + 0.5f;
		color[0] = color[0] + factor * (cgs.media.crosshairHitColor[0] - color[0]);
		color[1] = color[1] + factor * (cgs.media.crosshairHitColor[1] - color[1]);
		color[2] = color[2] + factor * (cgs.media.crosshairHitColor[2] - color[2]);
	}
}

static void CG_CrosshairGetSize(float *width, float *height)
{
	float	scale;

	*height = cg_crosshairSize.value;
	*width = cg_crosshairSize.value * cgs.screenYScale / cgs.screenXScale;

	if (cg_crosshairHitPulse.integer && cg.lastHitDamage) {
		scale = cg.time - cg.lastHitTime;
	} else if (cg_crosshairPickupPulse.integer) {
		scale = cg.time - cg.itemPickupBlendTime;
	} else {
		return;
	}

	if (scale <= 0 || scale > ITEM_BLOB_TIME) {
		return;
	}

	if (cg_crosshairHitPulse.integer && cg.lastHitDamage) {
		scale = cg.lastHitDamage * (1 - scale) / 50;
	} else if (cg_crosshairPickupPulse.integer) {
		scale /= ITEM_BLOB_TIME;
	}

	*width *= (1 + scale);
	*height *= (1 + scale);
}

static void CG_DrawCrosshair(void)
{
	float		w, h;
	qhandle_t	hShader;
	float		x, y;
	int			ca;
	vec4_t		color;

	if (!cg_drawCrosshair.integer || cg.showInfo) {
		return;
	}

	if (cg.snap->ps.persistant[PERS_TEAM] == TEAM_SPECTATOR) {
		return;
	}

	if (cg.renderingThirdPerson) {
		return;
	}

	CG_CrosshairGetColor(color);
	trap_R_SetColor(color);

	CG_CrosshairGetSize(&w, &h);

	x = cg_crosshairX.integer;
	y = cg_crosshairY.integer;
	CG_AdjustFrom640(&x, &y, &w, &h);

	ca = cg_drawCrosshair.integer;
	if (ca < 0) {
		ca = 0;
	}
	hShader = cgs.media.crosshairShader[ ca % NUM_CROSSHAIRS ];

	trap_R_DrawStretchPic(x + cg.refdef.x + 0.5 * (cg.refdef.width - w),
		y + cg.refdef.y + 0.5 * (cg.refdef.height - h),
		w, h, 0, 0, 1, 1, hShader);
}

static void CG_DrawCrosshair3D(void)
{
	float		w, h;
	qhandle_t	hShader;
	int			ca;

	trace_t trace;
	vec3_t endpos;
	float stereoSep, zProj, maxdist, xmax;
	char rendererinfos[128];
	refEntity_t ent;

	if (!cg_drawCrosshair.integer) {
		return;
	}

	if (cg.snap->ps.persistant[PERS_TEAM] == TEAM_SPECTATOR) {
		return;
	}

	if (cg.renderingThirdPerson) {
		return;
	}

	CG_CrosshairGetSize(&w, &h);

	ca = cg_drawCrosshair.integer;
	if (ca < 0) {
		ca = 0;
	}
	hShader = cgs.media.crosshairShader[ ca % NUM_CROSSHAIRS ];

	// Use a different method rendering the crosshair so players don't see two of them when
	// focusing their eyes at distant objects with high stereo separation
	// We are going to trace to the next shootable object and place the crosshair in front of it.

	// first get all the important renderer information
	trap_Cvar_VariableStringBuffer("r_zProj", rendererinfos, sizeof(rendererinfos));
	zProj = atof(rendererinfos);
	trap_Cvar_VariableStringBuffer("r_stereoSeparation", rendererinfos, sizeof(rendererinfos));
	stereoSep = zProj / atof(rendererinfos);

	xmax = zProj * tan(cg.refdef.fov_x * M_PI / 360.0f);

	// let the trace run through until a change in stereo separation of the crosshair becomes less than one pixel.
	maxdist = cgs.glconfig.vidWidth * stereoSep * zProj / (2 * xmax);
	VectorMA(cg.refdef.vieworg, maxdist, cg.refdef.viewaxis[0], endpos);
	CG_Trace(&trace, cg.refdef.vieworg, NULL, NULL, endpos, 0, MASK_SHOT);

	memset(&ent, 0, sizeof(ent));
	ent.reType = RT_SPRITE;
	ent.renderfx = RF_DEPTHHACK | RF_CROSSHAIR;

	VectorCopy(trace.endpos, ent.origin);

	// scale the crosshair so it appears the same size for all distances
	ent.radius = w / 640 * xmax * trace.fraction * maxdist / zProj;
	ent.customShader = hShader;

	trap_R_AddRefEntityToScene(&ent);
}

static void CG_DrawTeamVote(void)
{
	char	*s;
	int		sec, cs_offset;

	if (cgs.clientinfo[cg.clientNum].team == TEAM_RED)
		cs_offset = 0;
	else if (cgs.clientinfo[cg.clientNum].team == TEAM_BLUE)
		cs_offset = 1;
	else
		return;

	if (!cgs.teamVoteTime[cs_offset]) {
		return;
	}

	sec = (VOTE_TIME - (cg.time - cgs.teamVoteTime[cs_offset])) / 1000;
	if (sec < 0) {
		sec = 0;
	}
	s = va("TEAMVOTE(%i):%s yes:%i no:%i", sec, cgs.teamVoteString[cs_offset],
							cgs.teamVoteYes[cs_offset], cgs.teamVoteNo[cs_offset]);
	CG_DrawSmallString(0, 90, s, 1.0F);
}

static void CG_DrawIntermission(void)
{
	if (cg_drawScoreboard.integer) {
		cg.scoreBoardShowing = CG_DrawScoreboard();
	}
}

static void CG_DrawAcc(void)
{
	int			i;
	int			x, y, yy;
	char		*str;
	const int	iconsize = 15;

	if (!cg.showAcc) {
		return;
	}

	x = SCREEN_WIDTH - 175 - 50;
	y = (SCREEN_HEIGHT - 250) / 2;

	CG_DrawPic(x, y, 175, 250, cgs.media.accBackground);

	x += 15;

	for (i = WP_MACHINEGUN; i < WP_NUM_WEAPONS; ++i) {
		if (!cg_weapons[i].registered) {
			continue;
		}
		yy = y + (i - 1) * (iconsize + 5);
		CG_DrawAdjustPic(x, yy, iconsize, iconsize, cg_weapons[i].weaponIcon);
		str = va("-");
		CG_DrawSmallString(x + iconsize, yy, str, 1.0f);
	}
}

static void CG_Draw2D(stereoFrame_t stereoFrame)
{
	// if we are taking a levelshot for the menu, don't draw anything
	if (cg.levelShot) {
		return;
	}

	if (cg.snap->ps.pm_type == PM_INTERMISSION) {
		CG_DrawIntermission();
		return;
	}

	if (cg.snap->ps.persistant[PERS_TEAM] == TEAM_SPECTATOR) {
		if (stereoFrame == STEREO_CENTER)
			CG_DrawCrosshair();
	} else {
		// don't draw any status if dead or the scoreboard is being explicitly shown
		if (!cg.showScores && cg.snap->ps.stats[STAT_HEALTH] > 0) {
			if (stereoFrame == STEREO_CENTER) {
				CG_DrawCrosshair();
			}
		}
	}

	CG_DrawTeamVote();
	CG_DrawDisconnect();
	CG_DrawUpperRight(stereoFrame);

	CG_DrawLowerRight();
	CG_DrawLowerLeft();

	CG_DrawInformation();

	CG_DrawAcc();

	// don't draw center string if scoreboard is up
	cg.scoreBoardShowing = CG_DrawScoreboard();
	if (!cg.scoreBoardShowing && !cg.showInfo) {
		CG_DrawCenterString();
	}
}

/**
Perform all drawing needed to completely fill the screen
*/
void CG_DrawActive(stereoFrame_t stereoView)
{
	// optionally draw the info screen instead
	if (!cg.snap) {
		CG_DrawInformation();
		return;
	}

	// optionally draw the tournement scoreboard instead
	if (cg.snap->ps.persistant[PERS_TEAM] == TEAM_SPECTATOR &&
		(cg.snap->ps.pm_flags & PMF_SCOREBOARD)) {
//		CG_DrawTourneyScoreboard();
		return;
	}

	// clear around the rendered view if sized down
	CG_TileClear();

	if (stereoView != STEREO_CENTER)
		CG_DrawCrosshair3D();

	// draw 3D view
	trap_R_RenderScene(&cg.refdef);

	CG_DrawHud();

	// draw status bar and other floating elements
	CG_Draw2D(stereoView);
}


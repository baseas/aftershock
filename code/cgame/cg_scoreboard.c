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

// cg_scoreboard -- draw aftershock scoreboard on top of the game screen

#include "cg_local.h"

#define SB_MAXDISPLAY   		7
#define SB_INFOICON_SIZE		8

#define SB_CHAR_WIDTH			5
#define SB_CHAR_HEIGHT			9
#define SB_MEDCHAR_WIDTH 		6
#define SB_MEDCHAR_HEIGHT		10

#define SB_WIDTH				640
#define SB_HEIGHT				410
#define SB_XPOS 				((SCREEN_WIDTH - SB_WIDTH)/2)
#define SB_YPOS					((SCREEN_HEIGHT - SB_HEIGHT)/2)

#define SB_TEAM_WIDTH			(SB_WIDTH/2 - 40)
#define SB_TEAM_HEIGHT			20
#define SB_TEAM_RED_X			(SCREEN_WIDTH/2 - SB_TEAM_WIDTH - 20)
#define SB_TEAM_BLUE_X			(SCREEN_WIDTH/2 + 20)
#define SB_TEAM_Y				60

#define SB_INFO_WIDTH			(SB_WIDTH - 150)
#define SB_INFO_HEIGHT			20
#define SB_INFO_X				(SCREEN_WIDTH/2)
#define SB_INFO_Y				335
#define SB_INFO_MAXNUM			13

#define SB_FFA_WIDTH			(SB_WIDTH - 190)
#define SB_FFA_HEIGHT			20
#define SB_FFA_X				(SCREEN_WIDTH/2 - SB_FFA_WIDTH/2)
#define SB_FFA_Y				75

#define SB_SPEC_WIDTH			(SB_WIDTH - 340)
#define SB_SPEC_X				(SCREEN_WIDTH/2 - SB_SPEC_WIDTH/2)
#define SB_SPEC_Y				370
#define SB_SPEC_MAXCHAR			(SB_SPEC_WIDTH/SB_MEDCHAR_WIDTH)

#define SB_TOURNEY_WIDTH		(SB_WIDTH/2 - 40)
#define SB_TOURNEY_HEIGHT		16
#define SB_TOURNEY_X			(SCREEN_WIDTH/2)
#define SB_TOURNEY_Y			60
#define SB_TOURNEY_PICKUP_Y		200
#define SB_TOURNEY_AWARD_Y		340
#define SB_TOURNEY_MAX_AWARDS	8
#define SB_TOURNEY_AWARDS		4

typedef struct {
	qhandle_t	pic;
	int			val;
	qboolean	percent;
} picBar_t;

static void CG_DrawClientScore(int x, int y, int w, int h, int clientNum, float *color)
{
	clientInfo_t	*ci;
	entityState_t	*es;
	char			string[128];
	int				picSize;
	int				time;

	ci = &cgs.clientinfo[clientNum];
	es = &cg_entities[clientNum].currentState;

	// don't draw the client while he's connecting
	// FIXME need other check
	if (ci->ping == -1) {
		return;
	}

	CG_FillRect(x, y, w, h, color);

	y += h/2;

	CG_DrawStringExt(x, y - SB_MEDCHAR_HEIGHT/2, ci->name, colorWhite, qfalse, qfalse,
		SB_MEDCHAR_WIDTH, SB_MEDCHAR_HEIGHT, 31);

	Com_sprintf(string, sizeof string, "%d", es->pubStats[PUBSTAT_SCORE]);
	CG_DrawStringExt(x + w*0.7, y - SB_MEDCHAR_HEIGHT/2, string, colorWhite,
		qtrue, qfalse, SB_MEDCHAR_WIDTH, SB_MEDCHAR_HEIGHT, 0);

	Com_sprintf(string, sizeof string, "%i", ci->ping);
	CG_DrawStringExt(x + w*0.8, y - SB_MEDCHAR_HEIGHT/2, string, colorWhite,
		qtrue, qfalse, SB_MEDCHAR_WIDTH, SB_MEDCHAR_HEIGHT, 3);

	time = cg.time - ci->enterTime;
	Com_sprintf(string, sizeof string, "%i", time / 60 / 1000);
	CG_DrawStringExt(x + w*0.88, y - SB_MEDCHAR_HEIGHT/2, string, colorWhite,
		qtrue, qfalse, SB_MEDCHAR_WIDTH, SB_MEDCHAR_HEIGHT, 3);

	if (cgs.gametype == GT_ELIMINATION) {
		if (h >= SB_CHAR_HEIGHT*2) {
			Com_sprintf(string, sizeof string, "%i", es->pubStats[PUBSTAT_DAMAGE_DONE] / 100);
			CG_DrawStringExt(x + w*0.96, y - SB_CHAR_HEIGHT, string, colorGreen, qtrue, qfalse, SB_CHAR_WIDTH, SB_CHAR_HEIGHT, 0);
			Com_sprintf(string, sizeof string, "%i", es->pubStats[PUBSTAT_DAMAGE_TAKEN] / 100);
			CG_DrawStringExt(x + w*0.96, y, string, colorRed, qtrue, qfalse, SB_CHAR_WIDTH, SB_CHAR_HEIGHT, 0);
		} else {
			Com_sprintf(string, sizeof string, "^2%i^7/^1%i", es->pubStats[PUBSTAT_DAMAGE_DONE] / 100,
				es->pubStats[PUBSTAT_DAMAGE_TAKEN]);
			CG_DrawStringExt(x + w*0.96, y - SB_CHAR_HEIGHT/2, string, colorWhite,
				qfalse, qfalse, SB_CHAR_WIDTH, SB_CHAR_HEIGHT, 0);
		}
	}

	picSize = h * 0.8;

	if (ci->team == TEAM_SPECTATOR) {
		return;
	}

	if (cgs.startWhenReady && (cg.warmup < 0 || cg.intermissionStarted)) {
		if (ci->ready) {
			CG_DrawAdjustPic(x - picSize, y - picSize / 2, picSize, picSize, cgs.media.sbReady);
		} else {
			CG_DrawAdjustPic(x - picSize, y - picSize / 2, picSize, picSize, cgs.media.sbNotReady);
		}
	} else if (es->eFlags & EF_DEAD) {
		CG_DrawAdjustPic(x - picSize, y - picSize / 2, picSize, picSize, cgs.media.sbSkull);
	} else if (ci->powerups & (1 << PW_REDFLAG)) {
		CG_DrawFlagModel(x - picSize, y - picSize / 2, picSize, picSize, TEAM_RED, qfalse);
	} else if (ci->powerups & (1 << PW_BLUEFLAG)) {
		CG_DrawFlagModel(x - picSize, y - picSize / 2, picSize, picSize, TEAM_BLUE, qfalse);
	}
}

static int CG_TeamScoreboard(int x, int y, int w, int h, team_t team, float *color, int maxClients)
{
	int				i;
	clientInfo_t	*ci;
	int				count;
	float			transparent[4];
	int				sortedClients[MAX_CLIENTS];
	int				sortedCount;

	transparent[0] = 0;
	transparent[1] = 0;
	transparent[2] = 0;
	transparent[3] = 0;

	CG_DrawStringExt(x, y, "Name", colorWhite, qtrue, qfalse, SB_CHAR_WIDTH, SB_CHAR_HEIGHT, 0);
	CG_DrawStringExt(x + w*0.7, y, "Score", colorWhite, qtrue, qfalse, SB_CHAR_WIDTH, SB_CHAR_HEIGHT, 0);
	CG_DrawAdjustPic(x + w*0.8, y, SB_INFOICON_SIZE, SB_INFOICON_SIZE, cgs.media.sbPing);
	CG_DrawAdjustPic(x + w*0.88, y, SB_INFOICON_SIZE, SB_INFOICON_SIZE, cgs.media.sbClock);

	if (cgs.gametype == GT_ELIMINATION) {
		CG_DrawStringExt(x + w*0.96, y, "Dmg", colorWhite, qtrue, qfalse, SB_CHAR_WIDTH, SB_CHAR_HEIGHT, 0);
	}

	y += 20;

	// sort client numbers
	sortedCount = 0;
	for (i = 0; i < MAX_CLIENTS; ++i) {
		int	k;
		for (k = 0; k < MAX_CLIENTS; ++k) {
			if (!cgs.clientinfo[k].infoValid) {
				continue;
			}
			if (cg_entities[k].currentState.pubStats[PUBSTAT_RANK] == i) {
				sortedClients[sortedCount++] = k;
			}
		}
	}

	for (i = 0, count = 0; i < sortedCount; ++i) {
		ci = &cgs.clientinfo[sortedClients[i]];

		if (ci->team != team) {
			continue;
		}

		if (count >= maxClients) {
			break;
		}

		if (count % 2) {
			CG_DrawClientScore(x, y, w, h, sortedClients[i], transparent);
		} else {
			CG_DrawClientScore(x, y, w, h, sortedClients[i], color);
		}

		y += h + 1;
		++count;
	}

	if (count < maxClients) {
		return count;
	}

	CG_DrawStringExt(x, y, "...", colorWhite, qtrue, qfalse, SB_MEDCHAR_WIDTH, SB_MEDCHAR_HEIGHT, 0);

	y += h - 5;
	if (count % 2) {
		CG_DrawClientScore(x, y, w, h, cg.clientNum, transparent);
	} else {
		CG_DrawClientScore(x, y, w, h, cg.clientNum, color);
	}

	return maxClients;
}

static void CG_DrawSpecs(void)
{
	int				numLine;
	char			string[128];
	int				y;
	int				i;
	int				queueNumber = 1;
	clientInfo_t	*ci;

	strcpy(string, "Spectators");
	CG_DrawStringExt(SB_SPEC_X + SB_SPEC_WIDTH / 2 - CG_StringWidth(SMALLCHAR_WIDTH, string) / 2,
		SB_SPEC_Y, string, colorYellow, qtrue, qtrue, SMALLCHAR_WIDTH, SMALLCHAR_HEIGHT, 0);

	y = SB_SPEC_Y + 20;
	string[0] = '\0';
	numLine = 0;

	for (i = 0; i < MAX_CLIENTS; ++i) {
		if (!cgs.clientinfo[i].infoValid) {
			continue;
		}

		ci = &cgs.clientinfo[i];

		if (ci->team != TEAM_SPECTATOR) {
			continue;
		}

		if (CG_DrawStrlen(string) + CG_DrawStrlen(ci->name) + 3 > SB_SPEC_MAXCHAR) {
			CG_DrawStringExt(SB_SPEC_X, y, string, colorWhite, qfalse, qfalse, SB_MEDCHAR_WIDTH, SB_MEDCHAR_HEIGHT, 0);
		} else if (*string) {
			strcat(string, "   ");
		}

		if (cgs.gametype == GT_TOURNAMENT) {
			if (ci->specOnly) {
				Com_sprintf(string, sizeof string, "^7(^2%i^7/^1%i^7)%s^7(^1s^7)^7",
					ci->wins, ci->losses, ci->name);
			} else {
				Com_sprintf(string, sizeof string, "^7(^2%i^7/^1%i^7)%s^7(^2%i^7)^7",
					ci->wins, ci->losses, ci->name, queueNumber);
				queueNumber++;
			}
		} else {
			Q_strncpyz(string, ci->name, sizeof string);
		}

		y += SB_MEDCHAR_HEIGHT;
		numLine++;
		if (numLine >= 2) {
			break;
		}
	}

	// draw the local client if he hasn't been drawn yet
	if (numLine >= 2) {
		strcpy(string, "... ");
		strcat(string, cgs.clientinfo[cg.clientNum].name);
		CG_DrawStringExt(SB_SPEC_X, y, string, colorWhite, qfalse, qfalse,
			SB_MEDCHAR_WIDTH, SB_MEDCHAR_HEIGHT, 0);
	} else if (*string) {
		CG_DrawStringExt(SB_SPEC_X, y, string, colorWhite, qfalse, qfalse,
			SB_MEDCHAR_WIDTH, SB_MEDCHAR_HEIGHT, 0);
	}
}

#if 0
static void CG_DrawPicBar(picBar_t *tab, int count, int x, int y, int w, int h)
{
	int i, offset, picBarX;
	char string[16];

	offset = w/(count*2 + 1);
	for (i = 0; i<count; i++) {
		picBarX = x - w/2 + offset + i*2*offset;
		CG_DrawAdjustPic(picBarX, y - h/2, h, h, tab[i].pic);
		strcpy(string, va("%i", tab[i].val));
		if (tab[i].percent) { strcat(string, "%");
		CG_DrawStringExt(picBarX + h + 3, y - SB_MEDCHAR_HEIGHT/2, string, colorWhite, qtrue, qfalse, SB_MEDCHAR_WIDTH, SB_MEDCHAR_HEIGHT, 0);
	}
}
#endif

static void CG_DrawMapInfo(void)
{
	char	string[128];
	char	map[64];
	char	*slash;
	char	*bsp;

	slash = strchr(cgs.mapname, '/');
	if (!slash) {
		return;
	}
	Q_strncpyz(map, slash + 1, sizeof map);

	bsp = strstr(map, ".bsp");
	if (!bsp) {
		return;
	}
	bsp[0] = '\0';

	Com_sprintf(string, sizeof string, "%s // %s", map, cgs.gametypeShortName);
	CG_DrawStringExt(SB_XPOS + 10, SB_YPOS + 2, string, colorWhite, qtrue,
		qfalse, TINYCHAR_WIDTH, TINYCHAR_HEIGHT, 0);
}

qboolean CG_DrawScoreboard(void)
{
	vec4_t		color;
	char		string[128];
//	picBar_t	picBar[SB_INFO_MAXNUM];
//	int			infoCount;
//	int			i;

	// don't draw anything if the menu or console is up
	if (cg_paused.integer) {
		return qfalse;
	}

	if (cgs.gametype == GT_SINGLE_PLAYER && cg.predictedPlayerState.pm_type == PM_INTERMISSION) {
		return qfalse;
	}

	// don't draw scoreboard during death while warmup up
	if (!cg.showScores && cg.warmup) {
		return qfalse;
	}

	if (!cg.showScores && cg.predictedPlayerState.pm_type != PM_DEAD) {
		return qfalse;
	}

	CG_DrawPic(SB_XPOS, SB_YPOS, SB_WIDTH, SB_HEIGHT, cgs.media.sbBackground);

	CG_DrawMapInfo();

	if (cgs.gametype >= GT_TEAM) {
		color[0] = 1;
		color[1] = 0;
		color[2] = 0;
		color[3] = 0.25;

		// scores
		Com_sprintf(string, sizeof string, "^1%i ", cgs.scores1);
		CG_DrawStringExt(SCREEN_WIDTH / 2 - CG_StringWidth(GIANTCHAR_WIDTH, string), SB_TEAM_Y,
			string, colorWhite, qfalse, qtrue, GIANTCHAR_WIDTH, GIANTCHAR_HEIGHT, 0);

		Com_sprintf(string, sizeof string, " ^4%i", cgs.scores2);
		CG_DrawStringExt(SCREEN_WIDTH / 2, SB_TEAM_Y,
			string, colorWhite, qfalse, qtrue, GIANTCHAR_WIDTH, GIANTCHAR_HEIGHT, 0);

		strcpy(string, "to");
		CG_DrawStringExt(SCREEN_WIDTH / 2 - CG_StringWidth(SMALLCHAR_WIDTH, string) / 2,
			SB_TEAM_Y + 25, string, colorWhite, qtrue, qtrue, SMALLCHAR_WIDTH, SMALLCHAR_HEIGHT, 0);

		// team red
		strcpy(string, "Team red");
		CG_DrawStringExt(SB_TEAM_RED_X + SB_TEAM_WIDTH / 2 - CG_StringWidth(BIGCHAR_WIDTH, string) / 2,
			SB_TEAM_Y + 15, string, colorRed, qtrue, qtrue, BIGCHAR_WIDTH, BIGCHAR_HEIGHT, 0);
		CG_TeamScoreboard(SB_TEAM_RED_X, SB_TEAM_Y + 65, SB_TEAM_WIDTH, SB_TEAM_HEIGHT, TEAM_RED, color, SB_MAXDISPLAY);
		if (cgs.redLocked) {
			CG_DrawAdjustPic(0, SB_TEAM_Y, 32, 32, cgs.media.sbLocked);
		}

		color[0] = 0;
		color[1] = 0;
		color[2] = 1;
		color[3] = 0.25;

		// team blue
		strcpy(string, "Team blue");
		CG_DrawStringExt(SB_TEAM_BLUE_X + SB_TEAM_WIDTH / 2 - CG_StringWidth(BIGCHAR_WIDTH, string) / 2,
			SB_TEAM_Y + 15, string, colorBlue, qtrue, qtrue, BIGCHAR_WIDTH, BIGCHAR_HEIGHT, 0);
		CG_TeamScoreboard(SB_TEAM_BLUE_X, SB_TEAM_Y + 65, SB_TEAM_WIDTH, SB_TEAM_HEIGHT,
			TEAM_BLUE, color, SB_MAXDISPLAY);
		if (cgs.blueLocked) {
			CG_DrawAdjustPic(640-32, SB_TEAM_Y, 32, 32, cgs.media.sbLocked);
		}
	} else {
		// current rank
		if (cg.snap->ps.persistant[PERS_TEAM] != TEAM_SPECTATOR) {
			const char *place;
			place = CG_PlaceString(cg.snap->ps.persistant[PERS_RANK] + 1);
			Com_sprintf(string, sizeof string, "%s place with %i",
				place, cg.snap->ps.persistant[PERS_SCORE]);
			CG_DrawStringExt(SCREEN_WIDTH / 2 - CG_StringWidth(BIGCHAR_WIDTH, string) / 2,
				SB_FFA_Y, string, colorWhite, qfalse, qtrue, BIGCHAR_WIDTH, BIGCHAR_HEIGHT, 0);
		}

		// one team scoreboard
		color[0] = 0.7;
		color[1] = 0.7;
		color[2] = 0.7;
		color[3] = 0.25;
		CG_TeamScoreboard(SB_FFA_X, SB_FFA_Y + 50, SB_FFA_WIDTH, SB_FFA_HEIGHT, TEAM_FREE,
			color, SB_MAXDISPLAY);
	}

#if 0
	if (score != NULL) {
		infoCount = 0;
		picBar[infoCount].pic = cgs.media.medalGauntlet;
		picBar[infoCount].val = score->accuracy;
		picBar[infoCount].percent = qtrue;
		infoCount++;
		picBar[infoCount].pic = cgs.media.medalExcellent;
		picBar[infoCount].val = score->excellentCount;
		picBar[infoCount].percent = qfalse;
		infoCount++;
		picBar[infoCount].pic = cgs.media.medalImpressive;
		picBar[infoCount].val = score->impressiveCount;
		picBar[infoCount].percent = qfalse;
		infoCount++;
		picBar[infoCount].pic = cgs.media.medalGauntlet;
		picBar[infoCount].val = score->guantletCount;
		picBar[infoCount].percent = qfalse;
		infoCount++;
		picBar[infoCount].pic = cgs.media.medalAirrocket;
		picBar[infoCount].val = score->airrocketCount;
		picBar[infoCount].percent = qfalse;
		infoCount++;
		picBar[infoCount].pic = cgs.media.medalAirgrenade;
		picBar[infoCount].val = score->airgrenadeCount;
		picBar[infoCount].percent = qfalse;
		infoCount++;
		picBar[infoCount].pic = cgs.media.medalFullsg;
		picBar[infoCount].val = score->fullshotgunCount;
		picBar[infoCount].percent = qfalse;
		infoCount++;
		picBar[infoCount].pic = cgs.media.medalRocketrail;
		picBar[infoCount].val = score->rocketRailCount;
		picBar[infoCount].percent = qfalse;
		infoCount++;
		if (!cgs.nopickup && cgs.gametype != GT_ELIMINATION && cgs.gametype != GT_CTF_ELIMINATION) {
			picBar[infoCount].pic = cgs.media.medalItemdenied;
			picBar[infoCount].val = score->itemDeniedCount;
			picBar[infoCount].percent = qfalse;
			infoCount++;
		}
		if (cgs.gametype == GT_CTF) {
			picBar[infoCount].pic = cgs.media.medalCapture;
			picBar[infoCount].val = score->captures;
			picBar[infoCount].percent = qfalse;
			infoCount++;
			picBar[infoCount].pic = cgs.media.medalDefend;
			picBar[infoCount].val = score->defendCount;
			picBar[infoCount].percent = qfalse;
			infoCount++;
			picBar[infoCount].pic = cgs.media.medalAssist;
			picBar[infoCount].val = score->assistCount;
			picBar[infoCount].percent = qfalse;
			infoCount++;
		}
		picBar[infoCount].pic = cgs.media.medalSpawnkill;
		picBar[infoCount].val = score->spawnkillCount;
		picBar[infoCount].percent = qfalse;
		infoCount++;
		CG_DrawPicBar(picBar, infoCount, SB_INFO_X, SB_INFO_Y, SB_INFO_WIDTH, SB_INFO_HEIGHT);
	}
#endif

	CG_DrawSpecs();

	return qtrue;
}

#if 0
qboolean CG_DrawTourneyScoreboard(void)
{
	char		string[128];
	int			x, y, w, h;
	int			i, j, k;
	intr		side, loop, offset;
	clientInfo_t	*ci1;
	int				*stats1;
	clientInfo_t	*ci2;
	int				*stats2;
	clientInfo_t	*ci;
	gitem_t			*item;
	picBar_t		picBar[SB_TOURNEY_MAX_AWARDS];
	picBar_t		sortedPicBar[SB_TOURNEY_AWARDS];
	int				awardCount;

	// don't draw anything if the menu or console is up
	if (cg_paused.integer) {
		return qfalse;
	}

	// don't draw scoreboard during death while warmup up
	if (!cg.showScores && cg.warmup) {
		return qfalse;
	}

	if (!cg.showScores && cg.predictedPlayerState.pm_type != PM_DEAD) {
		return qfalse;
	}

	CG_DrawAdjustPic(SB_XPOS, SB_YPOS, SB_WIDTH, SB_HEIGHT, cgs.media.sbBackground);

	CG_DrawMapInfo();

	// get the two active players
	ci1 = NULL;
	ci2 = NULL;
	loop = -1; // loop is used when displaying the stats. -1 : no player, 0 : left player only, 2 : both players

	for (i = 0; i < MAX_CLIENTS; i++) {
		if (cgs.clientinfo[cg.scores[i].client].team != TEAM_FREE) {
			continue;
		}

		if (ci1 == NULL) {
			loop = 0;
			ci1 = &cgs.clientinfo[i];
			stats1 = cg_entities[i].currentState.pubStats;
		} else {
			// draw the other player only if match is over or we're spectating
			if (cg.predictedPlayerState.pm_type == PM_INTERMISSION || cgs.clientinfo[cg.clientNum].team == TEAM_SPECTATOR) {
				loop = 2;
			}
			if (i == cg.clientNum) {
				// local player always on the left side
				ci2 = ci1;
				stats2 = stats1;
				ci1 = &cgs.clientinfo[i];
				stats1 = &cg.scores[i];
			} else {
				ci2 = &cgs.clientinfo[i];
				stats2 = cg_entities[i].currentState.pubStats;
			}
		}
	}

	// scores
	x = SB_TOURNEY_X;
	y = SB_TOURNEY_Y;
	w = SB_TOURNEY_WIDTH;
	h = SB_TOURNEY_HEIGHT;

	if (ci1 != NULL && ci2 != NULL) {
		strcpy(string, va("%i %i", stats1[PUBSTAT_SCORE], stats2[PUBSTAT_SCORE]));
		CG_DrawStringExt(x - GIANTCHAR_WIDTH*CG_DrawStrlen(string)/2, y, string, colorWhite, qtrue, qtrue, GIANTCHAR_WIDTH, GIANTCHAR_HEIGHT, 0);
		strcpy(string, "to");
		CG_DrawStringExt(x - SMALLCHAR_WIDTH*CG_DrawStrlen(string)/2, y + 25, string, colorWhite, qtrue, qtrue, SMALLCHAR_WIDTH, SMALLCHAR_HEIGHT, 0);
	}

	// players headers
	// we don't use the loop var here, because we always display headers
	for (side =- 1; side < 2; side += 2) {
		offset = side < 0 ? 0 : 1;
		ci = side < 0 ? ci1 : ci2;
		stats = side < 0 ? stats1 : stats2;

		if (p == NULL) {
			break;
		}

		Com_sprintf(string, sizeof string, "^7(^2%i^7/^1%i^7) %s", ci->wins, ci->losses, ci->name);
		CG_DrawStringExt(x + side * w / 2 - SMALLCHAR_WIDTH * CG_DrawString(string) / 2, y + 15,
			string, colorWhite, qfalse, qtrue, SMALLCHAR_WIDTH, SMALLCHAR_HEIGHT, 0);
		CG_DrawAdjustPic(x + side*w*0.8 - offset * SB_INFOICON_SIZE, y + 30, SB_INFOICON_SIZE,
			SB_INFOICON_SIZE, cgs.media.sbPing);
		Com_snprintf(string, sizeof string, "%i", stats[PUBSTAT_PING]);
		CG_DrawStringExt(x + side*w*0.8 - side * SB_INFOICON_SIZE * 2 - offset * SB_MEDCHAR_WIDTH * CG_DrawStrlen(string),
			y + 30, string, colorWhite, qfalse, qtrue, SB_MEDCHAR_WIDTH, SB_MEDCHAR_HEIGHT, 0);

		if (cg.warmup < 0 && cgs.startWhenReady) {
			if (cg.readyMask & (1 << score->client)) {
				Q_strncpyz(string, "^2Ready", sizeof string);
			} else {
				Q_strncpyz(string, "^1Not ready", sizeof string);
			}
			CG_DrawStringExt(x + side*w/2 - SB_MEDCHAR_WIDTH*CG_DrawStrlen(string)/2, y + 30,
				string, colorWhite, qfalse, qtrue, SB_MEDCHAR_WIDTH, SB_MEDCHAR_HEIGHT, 0);
		}
	}

	// weapons info
	y += 60;
	for (side=-1; side<loop; side+=2) {
		Q_strncpyz(string, "Acc.", sizeof string);
		CG_DrawStringExt(x + side*0.3*w - SB_CHAR_WIDTH*CG_DrawStrlen(string)/2, y, string,
			colorWhite, qtrue, qfalse, SB_CHAR_WIDTH, SB_CHAR_HEIGHT, 0);
		Q_strncpyz(string, "Hit/Fired", sizeof string);
		CG_DrawStringExt(x + side*0.6*w - SB_CHAR_WIDTH*CG_DrawStrlen(string)/2, y, string,
			colorWhite, qtrue, qfalse, SB_CHAR_WIDTH, SB_CHAR_HEIGHT, 0);
	}

	y += 20;
	for (i = 0; i < MAX_WEAPONS; i++) {
		if (cg_weapons[i + 2].registered) {
			CG_DrawAdjustPic(x - h/2, y - h/2, h, h, cg_weapons[i + 2].weaponIcon);

			for (side=-1; side<loop; side+=2) {
				score = side < 0? p1Score : p2Score;
				// accuracy
				if (score->accuracys[i][0] > 0) {
					Com_sprintf(string, , sizeof string, "%i%%", ((int)(100*(float)score->accuracys[i][1]/(float)score->accuracys[i][0]))));
				} else {
					strcpy(string, "-%");
				}
				CG_DrawStringExt(x + side*0.3*w - SB_MEDCHAR_WIDTH*CG_DrawStrlen(string)/2, y, string, colorWhite, qtrue, qfalse, SB_MEDCHAR_WIDTH, SB_MEDCHAR_HEIGHT, 0);

				// hit/fired
				if (score->accuracys[i][0] > 0) {
					strcpy(string, va("%i/%i", score->accuracys[i][1], score->accuracys[i][0]));
				} else {
					strcpy(string, "-/-");
				}
				CG_DrawStringExt(x + side*0.6*w - SB_MEDCHAR_WIDTH*CG_DrawStrlen(string)/2, y, string, colorWhite, qtrue, qfalse, SB_MEDCHAR_WIDTH, SB_MEDCHAR_HEIGHT, 0);

			}
			y += h + 10;
		}
	}

	// picked up armor/health and dmg done
	for (side=-1; side<loop; side+=2) {

		y = SB_TOURNEY_PICKUP_Y;
		offset = side < 0? 0 : 1;
		score = side < 0? p1Score : p2Score;

		item = BG_FindItem("Mega Health");
		if (cg_items[ITEM_INDEX(item)].registered) {
			CG_DrawAdjustPic(x + side*w - offset*h, y, h, h, cg_items[ITEM_INDEX(item)].icon);
			strcpy(string, va("%i", score->megaHealth));
			CG_DrawStringExt(x + side*(w - h*2) - offset*SB_MEDCHAR_WIDTH*CG_DrawStrlen(string), y + h/2 - SB_MEDCHAR_HEIGHT/2, string, colorWhite, qtrue, qfalse, SB_MEDCHAR_WIDTH, SB_MEDCHAR_HEIGHT, 0);
			y += h + 10;
		}
		item = BG_FindItem("Red Armor");
		if (cg_items[ITEM_INDEX(item)].registered) {
			CG_DrawAdjustPic(x + side*w - offset*h, y, h, h, cg_items[ITEM_INDEX(item)].icon);
			strcpy(string, va("%i", score->redArmor));
			CG_DrawStringExt(x + side*(w - h*2) - offset*SB_MEDCHAR_WIDTH*CG_DrawStrlen(string), y + h/2 - SB_MEDCHAR_HEIGHT/2, string, colorWhite, qtrue, qfalse, SB_MEDCHAR_WIDTH, SB_MEDCHAR_HEIGHT, 0);
			y += h + 10;
		}
		item = BG_FindItem("Yellow Armor");
		if (cg_items[ITEM_INDEX(item)].registered) {
			CG_DrawAdjustPic(x + side*w - offset*h, y, h, h, cg_items[ITEM_INDEX(item)].icon);
			strcpy(string, va("%i", score->yellowArmor));
			CG_DrawStringExt(x + side*(w - h*2) - offset*SB_MEDCHAR_WIDTH*CG_DrawStrlen(string), y + h/2 - SB_MEDCHAR_HEIGHT/2, string, colorWhite, qtrue, qfalse, SB_MEDCHAR_WIDTH, SB_MEDCHAR_HEIGHT, 0);
			y += h + 10;
		}
		strcpy(string, "Dmg.");
		CG_DrawStringExt(x + side*w - offset*SB_MEDCHAR_WIDTH*CG_DrawStrlen(string), y + h/2 - SB_MEDCHAR_HEIGHT/2, string, colorWhite, qtrue, qfalse, SB_MEDCHAR_WIDTH, SB_MEDCHAR_HEIGHT, 0);
		strcpy(string, va("%i", score->dmgdone));
		CG_DrawStringExt(x + side*(w - h*2) - offset*SB_MEDCHAR_WIDTH*CG_DrawStrlen(string), y + h/2 - SB_MEDCHAR_HEIGHT/2, string, colorWhite, qtrue, qfalse, SB_MEDCHAR_WIDTH, SB_MEDCHAR_HEIGHT, 0);

	}

	// awards
	y = SB_TOURNEY_AWARD_Y;
	strcpy(string, "Awards");
	CG_DrawStringExt(x - SB_MEDCHAR_WIDTH*CG_DrawStrlen(string)/2, y - SB_MEDCHAR_HEIGHT/2, string, colorWhite, qtrue, qfalse, SB_MEDCHAR_WIDTH, SB_MEDCHAR_HEIGHT, 0);

	picBar[0].pic = cgs.media.medalExcellent;
	picBar[0].percent = qfalse;
	picBar[1].pic = cgs.media.medalImpressive;
	picBar[1].percent = qfalse;
	picBar[2].pic = cgs.media.medalGauntlet;
	picBar[2].percent = qfalse;
	picBar[3].pic = cgs.media.medalAirrocket;
	picBar[3].percent = qfalse;
	picBar[4].pic = cgs.media.medalAirgrenade;
	picBar[4].percent = qfalse;
	picBar[5].pic = cgs.media.medalFullsg;
	picBar[5].percent = qfalse;
	picBar[6].pic = cgs.media.medalRocketrail;
	picBar[6].percent = qfalse;
	picBar[7].pic = cgs.media.medalItemdenied;
	picBar[7].percent = qfalse;

	for (side=-1; side<loop; side+=2) {

		score = side < 0? p1Score : p2Score;

		picBar[0].val = score->excellentCount;
		picBar[1].val = score->impressiveCount;
		picBar[2].val = score->guantletCount;
		picBar[3].val = score->airrocketCount;
		picBar[4].val = score->airgrenadeCount;
		picBar[5].val = score->fullshotgunCount;
		picBar[6].val = score->rocketRailCount;
		picBar[7].val = score->itemDeniedCount;

		awardCount = 0;
		for (i = 0; i < SB_TOURNEY_MAX_AWARDS; i++) {
			if (picBar[i].val == 0)continue;
			for (j = 0; j < awardCount; j++) {
				if (picBar[i].val > sortedPicBar[j].val) {
					for (k=awardCount-1; k>=j; k--) {
						sortedPicBar[k + 1].pic = sortedPicBar[k].pic;
						sortedPicBar[k + 1].val = sortedPicBar[k].val;
						sortedPicBar[k + 1].percent = sortedPicBar[k].percent;
					}
					break;
				}
			}
			sortedPicBar[j].pic = picBar[i].pic;
			sortedPicBar[j].val = picBar[i].val;
			sortedPicBar[j].percent = picBar[i].percent;
			awardCount++;
			if (awardCount >= SB_TOURNEY_AWARDS)break;
		}

		CG_DrawPicBar(sortedPicBar, awardCount, x + side*w/2, y, w*0.8, h);
	}

	// spectators
	CG_DrawSpecs();

	return qtrue;
}

#endif

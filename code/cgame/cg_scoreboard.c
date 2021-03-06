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
// cg_scoreboard -- draw aftershock scoreboard on top of the game screen

#include "cg_local.h"

#define SB_MAXDISPLAY			11
#define SB_INFOICON_SIZE		8

#define SB_CHAR_SIZE			5
#define SB_MEDCHAR_SIZE 		8

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
#define SB_FFA_HEIGHT			17
#define SB_FFA_X				(SCREEN_WIDTH/2 - SB_FFA_WIDTH/2)
#define SB_FFA_Y				65

#define SB_SPEC_WIDTH			(SB_WIDTH - 340)
#define SB_SPEC_X				(SCREEN_WIDTH/2 - SB_SPEC_WIDTH/2)
#define SB_SPEC_Y				370
#define SB_SPEC_MAXCHAR			(SB_SPEC_WIDTH/SB_MEDCHAR_SIZE)

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

static void CG_DrawFlag(float x, float y, float w, float h, int team)
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
		CG_DrawAdjustPic(x, y, w, h, cg_items[ITEM_INDEX(item)].icon);
	}
}

static void CG_DrawClientScore(float x, float y, float w, float h, clientInfo_t *ci, float *color)
{
	entityState_t	*es;
	char			string[128];
	float			picSize;
	int				time;

	y += h/2;

	if (!ci) {
		SCR_DrawString(x, y - SB_MEDCHAR_SIZE / 2, "...", SB_MEDCHAR_SIZE, 0, colorWhite);
		return;
	}

	// don't draw the client while he's connecting
	if (!ci->botSkill && ci->ping == -1) {
		return;
	}

	es = &cg_entities[ci - cgs.clientinfo].currentState;

	SCR_FillRect(x, y - h / 2, w, h, color);

	SCR_DrawString(x, y - SB_MEDCHAR_SIZE / 2, ci->name, SB_MEDCHAR_SIZE, 0, colorWhite);

	if (cgs.warmup == 0) {
		Com_sprintf(string, sizeof string, "%d", ci->score);
	} else {
		Q_strncpyz(string, "-", sizeof string);
	}

	SCR_DrawString(x + w * 0.7, y - SB_MEDCHAR_SIZE / 2, string, SB_MEDCHAR_SIZE, 0, colorWhite);

	if (ci->ping == -1) {
		Com_sprintf(string, sizeof string, "-");
	} else {
		Com_sprintf(string, sizeof string, "%i", ci->ping);
	}

	SCR_DrawString(x + w * 0.8, y - SB_MEDCHAR_SIZE / 2, string, SB_MEDCHAR_SIZE, 0, colorWhite);

	time = cg.time - ci->enterTime;
	Com_sprintf(string, sizeof string, "%i", time / 60 / 1000);
	SCR_DrawString(x + w * 0.88, y - SB_MEDCHAR_SIZE / 2, string, SB_MEDCHAR_SIZE, 0, colorWhite);

	if (cgs.gametype == GT_ELIMINATION) {
		if (h >= SB_CHAR_SIZE*2) {
			Com_sprintf(string, sizeof string, "%i", ci->damageDone / 100);
			SCR_DrawString(x + w * 0.96, y - SB_CHAR_SIZE, string, SB_CHAR_SIZE, 0, colorGreen);
			Com_sprintf(string, sizeof string, "%i", ci->damageTaken / 100);
			SCR_DrawString(x + w * 0.96, y, string, SB_CHAR_SIZE, 0, colorRed);
		} else {
			Com_sprintf(string, sizeof string, "^2%i^7/^1%i",
				ci->damageDone / 100, ci->damageTaken / 100);
			SCR_DrawString(x + w * 0.96, y - SB_CHAR_SIZE / 2, string, SB_CHAR_SIZE, 0, colorWhite);
		}
	}

	picSize = h * 0.8;

	if (ci->team == TEAM_SPECTATOR) {
		return;
	}

	if (cgs.startWhenReady && (cgs.warmup < 0 || cg.intermissionStarted)) {
		if (ci->ready) {
			CG_DrawAdjustPic(x - picSize, y - picSize / 2, picSize, picSize, cgs.media.sbReady);
		} else if (!cg.intermissionStarted) {
			CG_DrawAdjustPic(x - picSize, y - picSize / 2, picSize, picSize, cgs.media.sbNotReady);
		}
	} else if ((cgs.gametype == GT_ELIMINATION && ci->eliminated)
		|| ((cgs.gametype != GT_ELIMINATION && es->eFlags & EF_DEAD)))
	{
		CG_DrawAdjustPic(x - picSize, y - picSize / 2, picSize, picSize, cgs.media.sbSkull);
	} else if (ci->powerups & (1 << PW_REDFLAG)) {
		CG_DrawFlag(x - picSize, y - picSize / 2, picSize, picSize, TEAM_RED);
	} else if (ci->powerups & (1 << PW_BLUEFLAG)) {
		CG_DrawFlag(x - picSize, y - picSize / 2, picSize, picSize, TEAM_BLUE);
	}
}

static void CG_TeamScoreboard(float x, float y, float w, float h, team_t team, float *color, int maxClients)
{
	int				i;
	clientInfo_t	*ci;
	int				count;
	qboolean		localDrawn;
	float			transparent[4];

	transparent[0] = 0;
	transparent[1] = 0;
	transparent[2] = 0;
	transparent[3] = 0;

	SCR_DrawString(x, y, "Name", SB_CHAR_SIZE, 0, colorWhite);
	SCR_DrawString(x + w * 0.7, y, "Score", SB_CHAR_SIZE, 0, colorWhite);
	CG_DrawAdjustPic(x + w*0.8, y, SB_INFOICON_SIZE, SB_INFOICON_SIZE, cgs.media.sbPing);
	CG_DrawAdjustPic(x + w*0.88, y, SB_INFOICON_SIZE, SB_INFOICON_SIZE, cgs.media.sbClock);

	if (cgs.gametype == GT_ELIMINATION) {
		SCR_DrawString(x + w * 0.96, y, "Dmg", SB_CHAR_SIZE, 0, colorWhite);
	}

	y += 20;

	localDrawn = qfalse;

	for (i = 0, count = 0; i < MAX_CLIENTS; ++i) {
		ci = &cgs.clientinfo[cg.sortedClients[i]];

		if (!ci->infoValid) {
			continue;
		}

		if (ci->team != team) {
			continue;
		}

		if (count >= maxClients) {
			break;
		}

		if (ci == &cgs.clientinfo[cg.clientNum]) {
			localDrawn = qtrue;
		}

		CG_DrawClientScore(x, y, w, h, ci, (count % 2 ? transparent : color));

		y += h + 1;
		++count;
	}

	if (count < maxClients) {
		return;
	}

	CG_DrawClientScore(x, y, w, h, NULL, (count % 2 ? transparent : color));

	if (!localDrawn) {
		y += h + 1;
		++count;
		CG_DrawClientScore(x, y, w, h, &cgs.clientinfo[cg.clientNum], transparent);
	}
}

static void CG_DrawSpecs(void)
{
	int				numLine;
	char			string[3 * SB_SPEC_MAXCHAR + 32];
	float			y;
	int				i;
	qboolean		localDrawn;
	clientInfo_t	*ci;

	strcpy(string, "Spectators");
	SCR_DrawString(SB_SPEC_X + SB_SPEC_WIDTH / 2, SB_SPEC_Y, string,
		SMALLCHAR_SIZE, FONT_CENTER, colorYellow);

	y = SB_SPEC_Y + 20;
	string[0] = '\0';
	numLine = 0;
	localDrawn = (cgs.clientinfo[cg.clientNum].team != TEAM_SPECTATOR);

	for (i = 0; i < MAX_CLIENTS; ++i) {
		ci = &cgs.clientinfo[cg.sortedClients[i]];

		if (!ci->infoValid) {
			continue;
		}

		if (ci->team != TEAM_SPECTATOR) {
			continue;
		}

		if (ci == &cgs.clientinfo[cg.clientNum]) {
			localDrawn = qtrue;
		}

		if (cgs.gametype == GT_TOURNAMENT) {
			if (ci->specOnly) {
				Q_strcat(string, sizeof string, va("^7(^2%i^7/^1%i^7)%s^7(^1s^7)^7",
					ci->wins, ci->losses, ci->name));
			} else {
				Q_strcat(string, sizeof string, va("^7(^2%i^7/^1%i^7)%s^7",
					ci->wins, ci->losses, ci->name));
			}
		} else {
			Q_strcat(string, sizeof string, ci->name);
		}

		if (SCR_Strlen(string) + SCR_Strlen(ci->name) + 3 > SB_SPEC_MAXCHAR) {
			SCR_DrawString(SB_SPEC_X, y, string, SB_MEDCHAR_SIZE, 0, colorWhite);
			string[0] = '\0';
			y += SB_MEDCHAR_SIZE;
			numLine++;
			if (numLine >= 3) {
				break;
			}
			continue;
		}

		if (*string) {
			Q_strcat(string, sizeof string, "^7   ");
		}
	}

	if (*string) {
		// draw the last spectator line
		SCR_DrawString(SB_SPEC_X, y, string, SB_MEDCHAR_SIZE, 0, colorWhite);
	} else if (numLine >= 3) {
		// draw the local client if he hasn't been drawn yet
		strcpy(string, "... ");
		if (!localDrawn) {
			Q_strcat(string, sizeof string, cgs.clientinfo[cg.clientNum].name);
		}
		SCR_DrawString(SB_SPEC_X, y, string, SB_MEDCHAR_SIZE, 0, colorWhite);
	}
}

static void CG_DrawPicBar(picBar_t *tab, int count, float x, float y, float w, float h)
{
	int		i;
	float	offset, picBarX;
	char	string[16];

	offset = w / (count * 2 + 1);
	for (i = 0; i < count; i++) {
		picBarX = x - w / 2 + offset + i * 2 * offset;
		CG_DrawAdjustPic(picBarX, y - h / 2, h, h, tab[i].pic);
		strcpy(string, va("%i", tab[i].val));
		if (tab[i].percent) {
			strcat(string, "%");
			SCR_DrawString(picBarX + h + 3, y - SB_MEDCHAR_SIZE / 2, string, SB_MEDCHAR_SIZE, 0, colorWhite);
		}
	}
}

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
	SCR_DrawString(SB_XPOS + 10, SB_YPOS + 2, string, TINYCHAR_SIZE, 0, colorWhite);
}

static void CG_DrawTeamScoreboard(void)
{
	char	string[128];
	vec4_t	color;

	color[0] = 1;
	color[1] = 0;
	color[2] = 0;
	color[3] = 0.25;

	// scores
	Com_sprintf(string, sizeof string, "^1%i ", cgs.scores1);
	SCR_DrawString(SCREEN_WIDTH / 2, SB_TEAM_Y, string, GIANTCHAR_SIZE, FONT_RIGHT, colorWhite);

	Com_sprintf(string, sizeof string, " ^4%i", cgs.scores2);
	SCR_DrawString(SCREEN_WIDTH / 2, SB_TEAM_Y, string, GIANTCHAR_SIZE, 0, colorWhite);

	strcpy(string, "to");
	SCR_DrawString(SCREEN_WIDTH / 2, SB_TEAM_Y + 25, string, SMALLCHAR_SIZE, FONT_CENTER, colorWhite);

	// team red
	strcpy(string, "Team red");
	SCR_DrawString(SB_TEAM_RED_X + SB_TEAM_WIDTH / 2,
		SB_TEAM_Y + 15, string, BIGCHAR_SIZE, FONT_CENTER, colorRed);
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
	SCR_DrawString(SB_TEAM_BLUE_X + SB_TEAM_WIDTH / 2,
		SB_TEAM_Y + 15, string, BIGCHAR_SIZE, FONT_CENTER, colorBlue);
	CG_TeamScoreboard(SB_TEAM_BLUE_X, SB_TEAM_Y + 65, SB_TEAM_WIDTH, SB_TEAM_HEIGHT,
		TEAM_BLUE, color, SB_MAXDISPLAY);
	if (cgs.blueLocked) {
		CG_DrawAdjustPic(640-32, SB_TEAM_Y, 32, 32, cgs.media.sbLocked);
	}
}

static void CG_DrawTourneyScoreboard(void)
{
	char		string[128];
	int			x, y, w, h;
	int			i, j, k;
	int			side, loop, offset;
	clientInfo_t	*ci, *ci1, *ci2;
	playerStats_t	*stats, *stats1, *stats2;
	gitem_t			*item;
	picBar_t		picBar[SB_TOURNEY_MAX_AWARDS];
	picBar_t		sortedPicBar[SB_TOURNEY_AWARDS];
	int				awardCount;

	// get the two active players
	ci1 = NULL;
	ci2 = NULL;
	loop = -1; // loop is used when displaying the stats. -1 : no player, 0 : left player only, 2 : both players

	for (i = 0; i < MAX_CLIENTS; i++) {
		if (!cgs.clientinfo[i].infoValid || cgs.clientinfo[i].team != TEAM_FREE) {
			continue;
		}

		if (ci1 == NULL) {
			loop = 0;
			ci1 = &cgs.clientinfo[i];
		} else {
			// draw the other player only if match is over or we're spectating
			if (cg.predictedPlayerState.pm_type == PM_INTERMISSION
				|| cgs.clientinfo[cg.clientNum].team == TEAM_SPECTATOR)
			{
				loop = 2;
			}

			if (i == cg.clientNum) {
				// local player always on the left side
				ci2 = ci1;
				ci1 = &cgs.clientinfo[i];
			} else {
				ci2 = &cgs.clientinfo[i];
			}
		}
	}

	if (ci1 == &cgs.clientinfo[cg.clientNum]) {
		stats1 = &cg.statsOwn;
	} else if (cg.snap->ps.pm_flags & PMF_FOLLOW && ci1 == &cgs.clientinfo[cg.snap->ps.clientNum]) {
		stats1 = &cg.statsFollow;
	} else {
		stats1 = &cg.statsEnemy;
	}

	if (ci2 == &cgs.clientinfo[cg.clientNum]) {
		stats2 = &cg.statsOwn;
	} else if (cg.snap->ps.pm_flags & PMF_FOLLOW && ci2 == &cgs.clientinfo[cg.snap->ps.clientNum]) {
		stats2 = &cg.statsFollow;
	} else {
		stats2 = &cg.statsEnemy;
	}

	// scores
	x = SB_TOURNEY_X;
	y = SB_TOURNEY_Y;
	w = SB_TOURNEY_WIDTH;
	h = SB_TOURNEY_HEIGHT;

	if (ci1 != NULL && ci2 != NULL) {
		if (ci1 < ci2) {
			strcpy(string, va("%i %i", cgs.scores1, cgs.scores2));
		} else {
			strcpy(string, va("%i %i", cgs.scores2, cgs.scores1));
		}
		SCR_DrawString(x, y, string, GIANTCHAR_SIZE, FONT_CENTER, colorWhite);
		strcpy(string, "to");
		SCR_DrawString(x, y + 25, string, SMALLCHAR_SIZE, FONT_CENTER, colorWhite);
	}

	// players headers
	// we don't use the loop var here, because we always display headers
	for (side =- 1; side < 2; side += 2) {
		if (side < 0) {
			offset = 0;
			ci = ci1;
			stats = stats1;
		} else {
			offset = 1;
			ci = ci2;
			stats = stats2;
		}

		if (ci == NULL) {
			break;
		}

		Com_sprintf(string, sizeof string, "^7(^2%i^7/^1%i^7) %s", ci->wins, ci->losses, ci->name);
		SCR_DrawString(x + side * w / 2, y + 15, string, SMALLCHAR_SIZE, FONT_CENTER, colorWhite);

		if (!ci->botSkill) {
			CG_DrawAdjustPic(x + side*w*0.8 - offset * SB_INFOICON_SIZE, y + 30, SB_INFOICON_SIZE,
				SB_INFOICON_SIZE, cgs.media.sbPing);
		}
		if (ci->botSkill) {
			Com_sprintf(string, sizeof string, "^2Bot");
		} else {
			Com_sprintf(string, sizeof string, "%i", ci->ping);
		}

		SCR_DrawString(x + side * w * 0.8 - side * SB_INFOICON_SIZE * 2 - offset * SCR_StringWidth(string, SB_MEDCHAR_SIZE),
			y + 30, string, SB_MEDCHAR_SIZE, 0, colorWhite);

		if (!ci->botSkill && cgs.warmup < 0 && cgs.startWhenReady) {
			if (ci->ready) {
				Q_strncpyz(string, "^2Ready", sizeof string);
			} else {
				Q_strncpyz(string, "^1Not ready", sizeof string);
			}
			SCR_DrawString(x + side * w / 2, y + 30, string, SB_MEDCHAR_SIZE, FONT_CENTER, colorWhite);
		}
	}

	// weapons info
	y += 60;
	for (side =- 1; side < loop; side += 2) {
		Q_strncpyz(string, "Acc.", sizeof string);
		SCR_DrawString(x + side * 0.3 * w, y, string, SB_CHAR_SIZE, FONT_CENTER, colorWhite);
		Q_strncpyz(string, "Hit/Fired", sizeof string);
		SCR_DrawString(x + side * 0.6 * w, y, string, SB_CHAR_SIZE, FONT_CENTER, colorWhite);
	}

	y += 20;
	for (i = WP_MACHINEGUN; i < WP_BFG; i++) {
		if (!cg_weapons[i].registered) {
			continue;
		}
		CG_DrawAdjustPic(x - h/2, y - h/2, h, h, cg_weapons[i].weaponIcon);

		for (side = -1; side < loop; side += 2) {
			stats = (side < 0 ? stats1 : stats2);
			// accuracy
			if (stats->shots[i] > 0) {
				int accuracy = (int) ((float) stats->enemyHits[i] / (float) stats->shots[i] * 100.0f);
				Com_sprintf(string, sizeof string, "%i %%", accuracy);
			} else {
				strcpy(string, "-%");
			}
			SCR_DrawString(x + side * 0.3 * w, y, string, SB_MEDCHAR_SIZE, FONT_CENTER, colorWhite);

			// hit/fired
			if (stats->shots[i]) {
				Com_sprintf(string, sizeof string, "%i/%i", stats->enemyHits[i], stats->shots[i]);
			} else {
				strcpy(string, "-/-");
			}
			SCR_DrawString(x + side * 0.6 * w, y, string, SB_MEDCHAR_SIZE, FONT_CENTER, colorWhite);

		}
		y += h + 10;
	}

	// picked up armor/health and dmg done
	for (side = -1; side < loop; side += 2) {
		y = SB_TOURNEY_PICKUP_Y + h/2 - SB_MEDCHAR_SIZE / 2;
		offset = (side < 0 ? 0 : 1);
		ci = (side < 0 ? ci1 : ci2);

		item = BG_FindItem("Mega Health");
		CG_DrawAdjustPic(x + side*w - offset*h, y, h, h, cg_items[ITEM_INDEX(item)].icon);
		Com_sprintf(string, sizeof string, "%i", ci->megaHealth);
		SCR_DrawString(x + side * (w - h * 2) - offset * SCR_StringWidth(string, SB_MEDCHAR_SIZE),
			y, string, SB_MEDCHAR_SIZE, 0, colorWhite);
		y += h + 10;

		item = BG_FindItem("Red Armor");
		CG_DrawAdjustPic(x + side*w - offset*h, y, h, h, cg_items[ITEM_INDEX(item)].icon);
		Com_sprintf(string, sizeof string, "%i", ci->redArmor);
		SCR_DrawString(x + side*(w - h * 2) - offset * SCR_StringWidth(string, SB_MEDCHAR_SIZE),
			y, string, SB_MEDCHAR_SIZE, 0, colorWhite);
		y += h + 10;

		item = BG_FindItem("Yellow Armor");
		CG_DrawAdjustPic(x + side*w - offset*h, y, h, h, cg_items[ITEM_INDEX(item)].icon);
		Com_sprintf(string, sizeof string, "%i", ci->yellowArmor);
		SCR_DrawString(x + side * (w - h * 2) - offset * SCR_StringWidth(string, SB_MEDCHAR_SIZE),
			y, string, SB_MEDCHAR_SIZE, 0, colorWhite);
		y += h + 10;

		strcpy(string, "Dmg.");
		SCR_DrawString(x + side * w - offset * SCR_StringWidth(string, SB_MEDCHAR_SIZE),
			y, string, SB_MEDCHAR_SIZE, 0, colorWhite);
		Com_sprintf(string, sizeof string, "%i", ci->damageDone);
		SCR_DrawString(x + side * (w - h * 2) - offset * SCR_StringWidth(string, SB_MEDCHAR_SIZE),
			y, string, SB_MEDCHAR_SIZE, 0, colorWhite);

	}

	// awards
	y = SB_TOURNEY_AWARD_Y;

	strcpy(string, "Awards");
	SCR_DrawString(x, y - SB_MEDCHAR_SIZE / 2, string, SB_MEDCHAR_SIZE, FONT_CENTER, colorWhite);

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
	picBar[5].pic = cgs.media.medalFullSg;
	picBar[5].percent = qfalse;
	picBar[6].pic = cgs.media.medalRocketrail;
	picBar[6].percent = qfalse;
	picBar[7].pic = cgs.media.medalItemdenied;
	picBar[7].percent = qfalse;

	for (side = -1; side < loop; side += 2) {
		stats = (side < 0 ? stats1 : stats2);

		picBar[0].val = stats->rewards[REWARD_EXCELLENT];
		picBar[1].val = stats->rewards[REWARD_IMPRESSIVE];
		picBar[2].val = stats->rewards[REWARD_HUMILIATION];
		picBar[3].val = stats->rewards[REWARD_AIRROCKET];
		picBar[4].val = stats->rewards[REWARD_AIRGRENADE];
		picBar[5].val = stats->rewards[REWARD_FULLSG];
		picBar[6].val = stats->rewards[REWARD_RLRG];
		picBar[7].val = stats->rewards[REWARD_ITEMDENIED];

		awardCount = 0;
		for (i = 0; i < SB_TOURNEY_MAX_AWARDS; i++) {
			if (picBar[i].val == 0) {
				continue;
			}
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
			if (awardCount >= SB_TOURNEY_AWARDS) {
				break;
			}
		}

		CG_DrawPicBar(sortedPicBar, awardCount, x + side*w/2, y, w*0.8, h);
	}
}

static void CG_DrawDefragScoreboard(void)
{

}

static void CG_DrawSingleScoreboard(void)
{
	char	string[128];
	vec4_t	color;

	// current rank
	if (cg.snap->ps.persistant[PERS_TEAM] != TEAM_SPECTATOR) {
		const char *place;
		place = CG_PlaceString(cg.snap->ps.persistant[PERS_RANK] + 1);
		Com_sprintf(string, sizeof string, "%s place with %i",
			place, cg.snap->ps.persistant[PERS_SCORE]);
		SCR_DrawString(SCREEN_WIDTH / 2, SB_FFA_Y, string, BIGCHAR_SIZE, FONT_CENTER, colorWhite);
	}

	// one team scoreboard
	color[0] = 0.7;
	color[1] = 0.7;
	color[2] = 0.7;
	color[3] = 0.25;
	CG_TeamScoreboard(SB_FFA_X, SB_FFA_Y + 40, SB_FFA_WIDTH, SB_FFA_HEIGHT, TEAM_FREE,
		color, SB_MAXDISPLAY);
}

qboolean CG_DrawScoreboard(void)
{
	// don't draw anything if the menu or console is up
	if (cg_paused.integer) {
		return qfalse;
	}

	if (cg.showInfo) {
		return qfalse;
	}

	// don't draw scoreboard during death while warmup up
	if (!cg.showScores && (cgs.warmup || !cg_drawScoreboard.integer)) {
		return qfalse;
	}

	if (cgs.gametype == GT_DEFRAG && !cg.showScores) {
		return qfalse;
	}

	if (!cg.showScores && cg.predictedPlayerState.pm_type != PM_DEAD
		&& cg.predictedPlayerState.pm_type != PM_INTERMISSION)
	{
		return qfalse;
	}

	CG_DrawPic(SB_XPOS, SB_YPOS, SB_WIDTH, SB_HEIGHT, cgs.media.sbBackground);

	CG_DrawMapInfo();

	if (cgs.gametype == GT_TOURNAMENT) {
		CG_DrawTourneyScoreboard();
	} else if (cgs.gametype >= GT_TEAM) {
		CG_DrawTeamScoreboard();
	} else if (cgs.gametype == GT_DEFRAG) {
		CG_DrawDefragScoreboard();
	} else {
		CG_DrawSingleScoreboard();
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
		picBar[infoCount].pic = cgs.media.medalFullSg;
		picBar[infoCount].val = score->fullshotgunCount;
		picBar[infoCount].percent = qfalse;
		infoCount++;
		picBar[infoCount].pic = cgs.media.medalRocketrail;
		picBar[infoCount].val = score->rocketRailCount;
		picBar[infoCount].percent = qfalse;
		infoCount++;
		if (!cgs.nopickup && cgs.gametype != GT_ELIMINATION) {
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
//		picBar[infoCount].pic = cgs.media.medalSpawnkill;
//		picBar[infoCount].val = score->spawnkillCount;
//		picBar[infoCount].percent = qfalse;
//		infoCount++;
		CG_DrawPicBar(picBar, infoCount, SB_INFO_X, SB_INFO_Y, SB_INFO_WIDTH, SB_INFO_HEIGHT);
	}
#endif
	CG_DrawSpecs();

	return qtrue;
}


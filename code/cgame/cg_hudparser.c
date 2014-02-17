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
// cg_hudparser.c

#include "cg_local.h"

static void CG_HudParseColor(const char *arg, int *teamColor, vec4_t color)
{
	char	*token1, *token2;

	token1 = COM_Parse((char **) &arg);

	if (!strcmp(token1, "team")) {
		*teamColor = 1;
	} else if (!strcmp(token1, "enemy")) {
		*teamColor = 2;
	} else {
		CG_ParseColor(color, token1);
	}

	token2 = COM_Parse((char **) &arg);

	if (*teamColor != 0 && *token2) {
		color[3] = atof(token2);
	} else if (*token2) {
		CG_Printf("HUD: Invalid color value %s %s\n", token1, token2);
	}
}

static void Prop_InusePrint(hudElement_t *element, char *buffer, int length)
{
	if (element->inuse) {
		Q_strncpyz(buffer, "true", length);
	} else {
		Q_strncpyz(buffer, "false", length);
	}
}

static void Prop_RectPrint(hudElement_t *element, char *buffer, int length)
{
	Com_sprintf(buffer, length, "%g %g %g %g",
		element->xpos, element->ypos, element->width, element->height);
}

static void Prop_BackgroundColorPrint(hudElement_t *element, char *buffer, int length)
{
	if (element->teamBgColor == 1) {
		Com_sprintf(buffer, length, "team %g", element->bgcolor[3]);
	} else if (element->teamBgColor == 2) {
		Com_sprintf(buffer, length, "enemy %g", element->bgcolor[3]);
	} else {
		Com_sprintf(buffer, length, "0x%.2x%.2x%.2x%.2x",
			(int) (element->bgcolor[0] * 255) & 0xff, (int) (element->bgcolor[1] * 255) & 0xff,
			(int) (element->bgcolor[2] * 255) & 0xff, (int) (element->bgcolor[3] * 255) & 0xff);
	}
}

static void Prop_ColorPrint(hudElement_t *element, char *buffer, int length)
{
	if (element->teamColor == 1) {
		Com_sprintf(buffer, length, "team %g", element->color[3]);
	} else if (element->teamColor == 2) {
		Com_sprintf(buffer, length, "enemy %g", element->color[3]);
	} else {
		Com_sprintf(buffer, length, "0x%.2x%.2x%.2x%.2x",
			(int) (element->color[0] * 255) & 0xff, (int) (element->color[1] * 255) & 0xff,
			(int) (element->color[2] * 255) & 0xff, (int) (element->color[3] * 255) & 0xff);
	}
}

static void Prop_FontSizePrint(hudElement_t *element, char *buffer, int length)
{
	Com_sprintf(buffer, length, "%g %g", element->fontWidth, element->fontHeight);
}

static void Prop_FillPrint(hudElement_t *element, char *buffer, int length)
{
	if (element->fill) {
		Q_strncpyz(buffer, "true", length);
	} else {
		Q_strncpyz(buffer, "false", length);
	}
}

static void Prop_TextAlignPrint(hudElement_t *element, char *buffer, int length)
{
	switch (element->textAlign) {
	case 0:
		Q_strncpyz(buffer, "L", length);
		break;
	case 1:
		Q_strncpyz(buffer, "C", length);
		break;
	case 2:
		Q_strncpyz(buffer, "R", length);
		break;
	}
}

static void Prop_TimePrint(hudElement_t *element, char *buffer, int length)
{
	Com_sprintf(buffer, length, "%i", element->time);
}

static void Prop_TextStylePrint(hudElement_t *element, char *buffer, int length)
{
	Com_sprintf(buffer, length, "%i", element->textStyle);
}

static void Prop_Inuse(hudElement_t *element, const char *arg)
{
	if (!strcmp(arg, "true")) {
		element->inuse = qtrue;
	} else if (!strcmp(arg, "false")) {
		element->inuse = qfalse;
	} else {
		Com_Printf("HUD: Invalid value '%20s' for property 'inuse'.\n", arg);
	}
}

static void Prop_Rect(hudElement_t *element, const char *arg)
{
	char	*token;

	token = COM_Parse((char **) &arg);
	element->xpos = atof(token);

	token = COM_Parse((char **) &arg);
	element->ypos = atof(token);

	token = COM_Parse((char **) &arg);
	element->width = atof(token);

	token = COM_Parse((char **) &arg);
	element->height = atof(token);
	
	if (*token == '\0') {
		CG_Printf(S_COLOR_YELLOW "HUD: rect requires four arguments.\n");
	}
}

static void Prop_BackgroundColor(hudElement_t *element, const char *arg)
{
	CG_HudParseColor(arg, &element->teamBgColor, element->bgcolor);
}

static void Prop_Color(hudElement_t *element, const char *arg)
{
	CG_HudParseColor(arg, &element->teamColor, element->color);
}

static void Prop_FontSize(hudElement_t *element, const char *arg)
{
	char	*token;

	token = COM_Parse((char **) &arg);
	element->fontWidth = atof(token);

	token = COM_Parse((char **) &arg);
	if (*token == '\0') {
		element->fontHeight = element->fontWidth;
		return;
	}
	element->fontHeight = atof(token);
}

static void Prop_Fill(hudElement_t *element, const char *arg)
{
	if (!strcmp(arg, "true")) {
		element->fill = qtrue;
	} else if (!strcmp(arg, "false")) {
		element->fill = qfalse;
	} else {
		Com_Printf("HUD: Invalid value '%20s' for property 'fill'.\n", arg);
	}
}

static void Prop_TextAlign(hudElement_t *element, const char *arg)
{
	if (!strcmp(arg, "L")) {
		element->textAlign = 0;
	} else if (!strcmp(arg, "C")) {
		element->textAlign = 1;
	} else if (!strcmp(arg, "R")) {
		element->textAlign = 2;
	} else {
		Com_Printf("HUD: Invalid value '%20s' for property 'textalign'.\n", arg);
	}
}

static void Prop_Time(hudElement_t *element, const char *arg)
{
	element->time = atoi(arg);
}

static void Prop_TextStyle(hudElement_t *element, const char *arg)
{
	element->textStyle = atoi(arg);
}

struct {
	const char	*name;
	void 		(*setFunc)(hudElement_t *element, const char *arg);
	void 		(*printFunc)(hudElement_t *element, char *buffer, int length);
} hudProperties[] = {
	{ "inuse", Prop_Inuse, Prop_InusePrint },
	{ "rect", Prop_Rect, Prop_RectPrint },
	{ "bgcolor", Prop_BackgroundColor, Prop_BackgroundColorPrint },
	{ "color", Prop_Color, Prop_ColorPrint },
	{ "fill", Prop_Fill, Prop_FillPrint },
	{ "fontsize", Prop_FontSize, Prop_FontSizePrint },
	{ "textalign", Prop_TextAlign, Prop_TextAlignPrint },
	{ "time", Prop_Time, Prop_TimePrint },
	{ "textstyle", Prop_TextStyle, Prop_TextStylePrint },
	{ NULL, NULL, NULL }
};

static const char *hudTags[] = {
	"AmmoMessage",
	"AttackerIcon",
	"AttackerName",
	"Chat1",
	"Chat2",
	"Chat3",
	"Chat4",
	"Chat5",
	"Chat6",
	"Chat7",
	"Chat8",
	"FlagStatus_OWN",
	"FlagStatus_NME",
	"FollowMessage",
	"FPS",
	"FragMessage",
	"GameTime",
	"RoundTime",
	"RealTime",
	"GameType",
	"ItemPickupName",
	"ItemPickupTime",
	"ItemPickupIcon",
	"NetGraph",
	"NetGraphPing",
	"PlayerSpeed",
	"PlayerAccel",
	"PowerUp1_Time",
	"PowerUp2_Time",
	"PowerUp3_Time",
	"PowerUp4_Time",
	"PowerUp1_Icon",
	"PowerUp2_Icon",
	"PowerUp3_Icon",
	"PowerUp4_Icon",
	"RankMessage",
	"Score_Limit",
	"Score_NME",
	"Score_OWN",
	"SpecMessage",
	"StatusBar_ArmorCount",
	"StatusBar_ArmorIcon",
	"StatusBar_AmmoCount",
	"StatusBar_AmmoIcon",
	"StatusBar_HealthCount",
	"StatusBar_HealthIcon",
	"TargetName",
	"TargetStatus",
	"TeamCount_NME",
	"TeamCount_OWN",
	"TeamIcon_NME",
	"TeamIcon_OWN",
	"Team1",
	"Team2",
	"Team3",
	"Team4",
	"Team5",
	"Team6",
	"Team7",
	"Team8",
	"VoteMessage",
	"WarmupInfo",
	"WeaponList",
	"ReadyStatus",
	"DeathNotice1",
	"DeathNotice2",
	"DeathNotice3",
	"DeathNotice4",
	"DeathNotice5",
	"Countdown",
	"RespawnTimer",
	"StatusBar_Flag",
	"TeamOverlay1",
	"TeamOverlay2",
	"TeamOverlay3",
	"TeamOverlay4",
	"TeamOverlay5",
	"TeamOverlay6",
	"TeamOverlay7",
	"TeamOverlay8",
	"Reward",
	"RewardCount",
	"Holdable",
	"Help",
	NULL
};

static int CG_HudnumberByTag(const char *tag)
{
	int	i;
	for (i = 0; hudTags[i]; ++i) {
		if (!strcmp(tag, hudTags[i])) {
			return i;
		}
	}
	return HUD_MAX;
}

static void CG_HudElementReset(hudElement_t *element)
{
	memset(element, 0, sizeof *element);
	memset(element->bgcolor, 0, sizeof *element->bgcolor);
	Vector4Copy(colorWhite, element->color);

	element->fontWidth = 8;
	element->fontHeight = 12;
}

static void CG_SetProperty(hudElement_t *element, const char *key, const char *value)
{
	int	i;
	for (i = 0; hudProperties[i].name; ++i) {
		if (!strcmp(key, hudProperties[i].name)) {
			hudProperties[i].setFunc(element, value);
		}
	}
}

static void CG_HudLoadFile(const char *hudFile, hudElement_t *hudlist)
{
	int				i;
	iniSection_t	section;
	fileHandle_t	fp;
	int				hudnumber;

	trap_FS_FOpenFile(hudFile, &fp, FS_READ);

	if (!fp) {
		CG_Printf(S_COLOR_YELLOW "hud file not found: %s\n", hudFile);
		return;
	}

	while (!trap_Ini_Section(&section, fp)) {
		hudnumber = CG_HudnumberByTag(section.label);
		if (hudnumber == HUD_MAX) {
			Com_Printf("HUD: Unknown element '%s'.\n", section.label);
			continue;
		}

		hudlist[hudnumber].inuse = qtrue;
		for (i = 0; i < section.numItems; ++i) {
			CG_SetProperty(&hudlist[hudnumber], section.keys[i], section.vals[i]);
		}
	}

	trap_FS_FCloseFile(fp);
}

/* User interface */

static void CG_PrintElements(void)
{
	int	i;
	CG_Printf("Hud element names are: ");
	for (i = 0; hudTags[i]; i++) {
		if (i % 2) {
			CG_Printf(S_COLOR_YELLOW);
		}

		CG_Printf("%s", hudTags[i]);

		if (hudTags[i + 1]) {
			CG_Printf(", ");
		}
	}
	CG_Printf("\n");
}

static void CG_PrintProperties(void)
{
	int	i;
	CG_Printf("Hud properties are: ");
	for (i = 0; hudProperties[i].name; ++i) {
		CG_Printf("%s", hudProperties[i].name);
		if (hudProperties[i + 1].name) {
			CG_Printf(", ");
		}
	}
	CG_Printf("\n");
}

static void CG_HudSaveElement(hudElement_t *a, hudElement_t *b, fileHandle_t fp)
{
	int			i, k;
	int			chardiff, padlen;
	char		prop1[64], prop2[64];
	char		line[256];
	const int	tabstop = 4;

	for (i = 0; hudProperties[i].name; ++i) {
		hudProperties[i].printFunc(a, prop1, sizeof prop1);
		hudProperties[i].printFunc(b, prop2, sizeof prop2);

		if (!strcmp(prop1, prop2)) {
			continue;
		}

		Com_sprintf(line, sizeof line, "%s:", hudProperties[i].name);

		chardiff = 3 * tabstop - strlen(hudProperties[i].name) - 1;
		padlen = chardiff / tabstop + (chardiff % tabstop ? 1 : 0);

		for (k = 0; k < padlen; ++k) {
			Q_strcat(line, sizeof line, "\t");
		}

		Q_strcat(line, sizeof line, prop1);
		Q_strcat(line, sizeof line, "\n");

		trap_FS_Write(line, strlen(line), fp);
	}

	trap_FS_Write("\n", 1, fp);
}

static qboolean CG_HudElementsDiffer(hudElement_t *a, hudElement_t *b)
{
	int		i;
	char	prop1[64], prop2[64];

	for (i = 0; hudProperties[i].name; ++i) {
		hudProperties[i].printFunc(a, prop1, sizeof prop1);
		hudProperties[i].printFunc(b, prop2, sizeof prop2);
		if (strcmp(prop1, prop2)) {
			return qtrue;
		}
	}

	return qfalse;
}

static void CG_HudSave(void)
{
	int				i;
	fileHandle_t	fp;
	hudElement_t	tmphud[HUD_MAX];
	qboolean		changed;
	char			line[64];

	for (i = 0; i < HUD_MAX; ++i) {
		CG_HudElementReset(&tmphud[i]);
	}
	CG_HudLoadFile("hud/default.ini", tmphud);

	trap_FS_FOpenFile("hud.ini", &fp, FS_WRITE);

	changed = qfalse;
	for (i = 0; i < HUD_MAX; ++i) {
		if (!CG_HudElementsDiffer(&cgs.hud[i], &tmphud[i])) {
			continue;
		}
		changed = qtrue;
		Com_sprintf(line, sizeof line, "[%s]\n", hudTags[i]);
		trap_FS_Write(line, strlen(line), fp);
		CG_HudSaveElement(&cgs.hud[i], &tmphud[i], fp);
	}

	trap_FS_FCloseFile(fp);

	if (!changed) {
		CG_Printf("HUD: No changes to save.\n");
	} else {
		CG_Printf("HUD: Changes written to 'hud.ini'.\n");
	}
}

void CG_HudInit(void)
{
	int	i;
	for (i = 0; i < HUD_MAX; ++i) {
		CG_HudElementReset(&cgs.hud[i]);
	}
	CG_HudLoadFile("hud/default.ini", cgs.hud);
	CG_HudLoadFile("hud.ini", cgs.hud);
}

static void CG_HudLoad(void)
{
	if (trap_Argc() == 2) {
		CG_Printf("usage: hud load <file>\n");
		return;
	}
	CG_HudLoadFile(CG_Argv(2), cgs.hud);
}

static void CG_HudPrintProperty(hudElement_t *element, const char *prop)
{
	int		i;
	char	buffer[64];

	for (i = 0; hudProperties[i].name; ++i) {
		if (!strcmp(CG_Argv(3), hudProperties[i].name)) {
			hudProperties[i].printFunc(element, buffer, sizeof buffer);
			CG_Printf("%s: %s\n", hudProperties[i].name, buffer);
		}
	}
}

static void CG_HudEdit(void)
{
	int	hudnumber;
	int	argc;

	argc = trap_Argc();

	if (argc == 2) {
		CG_PrintElements();
		return;
	}

	hudnumber = CG_HudnumberByTag(CG_Argv(2));
	if (hudnumber == HUD_MAX) {
		CG_Printf("Wrong hud element '%s'.\n", CG_Argv(2));
		CG_PrintElements();
		return;
	}

	if (argc == 3) {
		CG_PrintProperties();
		return;
	} else if (argc == 4) {
		CG_HudPrintProperty(&cgs.hud[hudnumber], CG_Argv(3));
		return;
	} else {
		int		i;
		char	value[64];

		value[0] = '\0';
		for (i = 0; i < argc - 4; ++i) {
			Q_strcat(value, sizeof value, CG_Argv(i + 4));
			if (i < argc - 5) {
				Q_strcat(value, sizeof value, " ");
			}
		}

		CG_SetProperty(&cgs.hud[hudnumber], CG_Argv(3), value);
		return;
	}
}

void CG_Hud_f(void)
{
	const char	*cmd;
	const char	*usage = "usage: hud <command> <arguments>\n"
		"commands are: edit, reset, save, load\n";

	if (trap_Argc() == 1) {
		CG_Printf("%s", usage);
		return;
	}

	cmd = CG_Argv(1);
	if (!strcmp(cmd, "edit")) {
		CG_HudEdit();
	} else if (!strcmp(cmd, "reset")) {
		CG_HudInit();
	} else if (!strcmp(cmd, "save")) {
		CG_HudSave();
	} else if (!strcmp(cmd, "load")) {
		CG_HudLoad();
	} else {
		CG_Printf("%s", usage);
	}
}


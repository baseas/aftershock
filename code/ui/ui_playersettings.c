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
// ui_playersettings.c

#include "ui_local.h"

#define MAX_NAMELENGTH	20

enum {
	ID_NAME = 10,
	ID_BACK
};

typedef struct {
	menuframework_s		menu;
	menutext_s			banner;
	menufield_s			name;
	menubutton_s		back;
} playersettings_t;

static playersettings_t	s_playersettings;

static void PlayerSettings_DrawName(void *self)
{
	menufield_s		*f;
	qboolean		focus;
	int				style;
	char			*txt;
	char			c;
	float			*color;
	int				n;
	int				basex, x, y;

	f = (menufield_s*)self;
	basex = f->generic.x;
	y = f->generic.y;
	focus = (f->generic.parent->cursor == f->generic.menuPosition);

	style = 0;
	color = colorTextNormal;
	if (focus) {
		style |= FONT_PULSE;
		color = colorTextHighlight;
	}

	SCR_DrawPropString(basex, y, "Name", style, color);

	// draw the actual name
	basex += 64;
	y += PROP_HEIGHT;
	txt = f->field.buffer;
	color = g_color_table[ColorIndex(COLOR_WHITE)];
	x = basex;
	while ((c = *txt) != 0) {
		if (Q_IsColorString(txt)) {
			n = ColorIndex(*(txt+1));
			if (n == 0) {
				n = 7;
			}
			color = g_color_table[n];
			if (!focus) {
				txt += 2;
				continue;
			}
		}
		SCR_DrawString(x, y, va("%c", c), SMALLCHAR_SIZE, style, color);
		txt++;
		x += BIGCHAR_SIZE;
	}

	// draw cursor if we have focus
	if (focus) {
		if (trap_Key_GetOverstrikeMode()) {
			c = 11;
		} else {
			c = 10;
		}

		style &= ~FONT_PULSE;
		style |= FONT_BLINK;

		SCR_DrawString(basex + f->field.cursor * BIGCHAR_SIZE, y,
			va("%c", c), BIGCHAR_SIZE, style, colorWhite);
	}
}

static void PlayerSettings_SaveChanges(void)
{
	// name
	trap_Cvar_Set("name", s_playersettings.name.field.buffer);
}

static sfxHandle_t PlayerSettings_MenuKey(int key)
{
	if (key == K_MOUSE2 || key == K_ESCAPE) {
		PlayerSettings_SaveChanges();
	}
	return Menu_DefaultKey(&s_playersettings.menu, key);
}

static void PlayerSettings_SetMenuItems(void)
{
	Q_strncpyz(s_playersettings.name.field.buffer, UI_Cvar_VariableString("name"),
		sizeof s_playersettings.name.field.buffer);
}

static void PlayerSettings_MenuEvent(void* ptr, int event)
{
	if (event != QM_ACTIVATED) {
		return;
	}

	switch (((menucommon_s *)ptr)->id) {
	case ID_BACK:
		PlayerSettings_SaveChanges();
		UI_PopMenu();
		break;
	}
}

static void PlayerSettings_MenuInit(void)
{
	int		y;

	memset(&s_playersettings, 0, sizeof (playersettings_t));

	PlayerSettings_Cache();

	s_playersettings.menu.key			= PlayerSettings_MenuKey;
	s_playersettings.menu.wrapAround	= qtrue;
	s_playersettings.menu.fullscreen	= qtrue;

	s_playersettings.banner.generic.type	= MTYPE_BTEXT;
	s_playersettings.banner.generic.x		= 320;
	s_playersettings.banner.generic.y		= 16;
	s_playersettings.banner.string			= "PLAYER SETTINGS";
	s_playersettings.banner.color			= colorBanner;
	s_playersettings.banner.style			= FONT_CENTER | FONT_SHADOW;

	y = 144;
	s_playersettings.name.generic.type			= MTYPE_FIELD;
	s_playersettings.name.generic.flags			= QMF_NODEFAULTINIT;
	s_playersettings.name.generic.ownerdraw		= PlayerSettings_DrawName;
	s_playersettings.name.field.widthInChars	= MAX_NAMELENGTH;
	s_playersettings.name.field.maxchars		= MAX_NAMELENGTH;
	s_playersettings.name.generic.x				= 120;
	s_playersettings.name.generic.y				= y;
	s_playersettings.name.generic.left			= 120 - 8;
	s_playersettings.name.generic.top			= y - 8;
	s_playersettings.name.generic.right			= 192 + 200;
	s_playersettings.name.generic.bottom		= y + 2 * PROP_HEIGHT;

	s_playersettings.back.generic.type			= MTYPE_BUTTON;
	s_playersettings.back.generic.flags			= QMF_LEFT_JUSTIFY;
	s_playersettings.back.generic.id			= ID_BACK;
	s_playersettings.back.generic.callback		= PlayerSettings_MenuEvent;
	s_playersettings.back.generic.x				= 0;
	s_playersettings.back.generic.y				= 480-64;
	s_playersettings.back.width					= 128;
	s_playersettings.back.height				= 64;
	s_playersettings.back.string				= "Back";

	Menu_AddItem(&s_playersettings.menu, &s_playersettings.banner);

	Menu_AddItem(&s_playersettings.menu, &s_playersettings.name);
	Menu_AddItem(&s_playersettings.menu, &s_playersettings.back);
	PlayerSettings_SetMenuItems();
}

void PlayerSettings_Cache(void) { }

void UI_PlayerSettingsMenu(void)
{
	PlayerSettings_MenuInit();
	UI_PushMenu(&s_playersettings.menu);
}


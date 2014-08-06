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
// ui_atoms.c -- User interface building blocks and support functions.

#include "ui_local.h"

uiStatic_t		uis;
qboolean		m_entersound;		// after a frame, so caching won't disrupt the sound

void QDECL Com_Error(int level, const char *error, ...)
{
	va_list		argptr;
	char		text[1024];

	va_start (argptr, error);
	Q_vsnprintf (text, sizeof(text), error, argptr);
	va_end (argptr);

	trap_Error(text);
}

void QDECL Com_Printf(const char *msg, ...)
{
	va_list		argptr;
	char		text[1024];

	va_start (argptr, msg);
	Q_vsnprintf (text, sizeof(text), msg, argptr);
	va_end (argptr);

	trap_Print(text);
}

float UI_ClampCvar(float min, float max, float value)
{
	if (value < min) return min;
	if (value > max) return max;
	return value;
}

void UI_StartDemoLoop(void)
{
	trap_Cmd_ExecuteText(EXEC_APPEND, "d1\n");
}

void UI_PushMenu(menuframework_s *menu)
{
	int				i;
	menucommon_s*	item;

	// avoid stacking menus invoked by hotkeys
	for (i = 0 ; i < uis.menusp; i++) {
		if (uis.stack[i] == menu) {
			uis.menusp = i;
			break;
		}
	}

	if (i == uis.menusp) {
		if (uis.menusp >= MAX_MENUDEPTH) {
			trap_Error("UI_PushMenu: menu stack overflow");
		}

		uis.stack[uis.menusp++] = menu;
	}

	uis.activemenu = menu;

	// default cursor position
	menu->cursor = 0;
	menu->cursor_prev = 0;

	m_entersound = qtrue;

	trap_Key_SetCatcher(KEYCATCH_UI);

	// force first available item to have focus
	for (i = 0; i<menu->nitems; i++) {
		item = (menucommon_s *)menu->items[i];
		if (!(item->flags & (QMF_GRAYED|QMF_MOUSEONLY|QMF_INACTIVE))) {
			menu->cursor_prev = -1;
			Menu_SetCursor(menu, i);
			break;
		}
	}

	uis.firstdraw = qtrue;
}

void UI_PopMenu (void)
{
	trap_S_StartLocalSound(menu_out_sound, CHAN_LOCAL_SOUND);

	uis.menusp--;

	if (uis.menusp < 0)
		trap_Error ("UI_PopMenu: menu stack underflow");

	if (uis.menusp) {
		uis.activemenu = uis.stack[uis.menusp-1];
		uis.firstdraw = qtrue;
	}
	else {
		UI_ForceMenuOff ();
	}
}

void UI_ForceMenuOff (void)
{
	uis.menusp     = 0;
	uis.activemenu = NULL;

	trap_Key_SetCatcher(trap_Key_GetCatcher() & ~KEYCATCH_UI);
	trap_Key_ClearStates();
	trap_Cvar_Set("cl_paused", "0");
}

void UI_LerpColor(vec4_t a, vec4_t b, vec4_t c, float t)
{
	int i;

	// lerp and clamp each component
	for (i = 0; i<4; i++)
	{
		c[i] = a[i] + t*(b[i]-a[i]);
		if (c[i] < 0)
			c[i] = 0;
		else if (c[i] > 1.0)
			c[i] = 1.0;
	}
}

void UI_DrawProportionalString_AutoWrapped(int x, int y, int xmax, int ystep, const char *str, int style, vec4_t color)
{
	int width;
	char *s1,*s2,*s3;
	char c_bcp;
	char buf[1024];
	float   sizeScale;

	if (!str || str[0]=='\0')
		return;

	sizeScale = (style & FONT_SMALL ? PROP_SMALL_SIZE_SCALE : 1.0f);

	Q_strncpyz(buf, str, sizeof(buf));
	s1 = s2 = s3 = buf;

	while (1) {
		do {
			s3++;
		} while (*s3!=' ' && *s3!='\0');
		c_bcp = *s3;
		*s3 = '\0';
		width = SCR_PropStringWidth(s1) * sizeScale;
		*s3 = c_bcp;
		if (width > xmax) {
			if (s1 == s2) {
				// don't have a clean cut, we'll overflow
				s2 = s3;
			}
			*s2 = '\0';
			SCR_DrawPropString(x, y, s1, style, color);
			y += ystep;
			if (c_bcp == '\0') {
				// that was the last word
				// we could start a new loop, but that wouldn't be much use
				// even if the word is too long, we would overflow it (see above)
				// so just print it now if needed
				s2++;
				if (*s2 != '\0') {
					// if we are printing an overflowing line we have s2 == s3
					SCR_DrawPropString(x, y, s2, style, color);
					break;
				}
			}
			s2++;
			s1 = s2;
			s3 = s2;
		} else {
			s2 = s3;
			if (c_bcp == '\0') {
				// we reached the end
				SCR_DrawPropString(x, y, s1, style, color);
				break;
			}
		}
	}
}

qboolean UI_IsFullscreen(void)
{
	if (uis.activemenu && (trap_Key_GetCatcher() & KEYCATCH_UI)) {
		return uis.activemenu->fullscreen;
	}

	return qfalse;
}

void UI_SetActiveMenu(uiMenuCommand_t menu)
{
	// this should be the ONLY way the menu system is brought up
	// enusure minumum menu data is cached
	Menu_Cache();

	switch (menu) {
	case UIMENU_NONE:
		UI_ForceMenuOff();
		return;
	case UIMENU_MAIN:
		UI_MainMenu();
		return;
	case UIMENU_DEMO:
		// TODO: demo menu
	case UIMENU_INGAME:
		trap_Cvar_Set("cl_paused", "1");
		UI_InGameMenu();
		return;
	default:
		Com_Printf("UI_SetActiveMenu: bad enum %d\n", menu);
		break;
	}
}

void UI_KeyEvent(int key, int down)
{
	sfxHandle_t		s;

	if (!uis.activemenu) {
		return;
	}

	if (!down) {
		return;
	}

	if (uis.activemenu->key)
		s = uis.activemenu->key(key);
	else
		s = Menu_DefaultKey(uis.activemenu, key);

	if ((s > 0) && (s != menu_null_sound))
		trap_S_StartLocalSound(s, CHAN_LOCAL_SOUND);
}

void UI_MouseEvent(int dx, int dy)
{
	int				i;
	menucommon_s*	m;

	if (!uis.activemenu) {
		return;
	}

	// update mouse screen position
	uis.cursorx += dx;
	if (uis.cursorx < -uis.bias) {
		uis.cursorx = -uis.bias;
	} else if (uis.cursorx > SCREEN_WIDTH+uis.bias) {
		uis.cursorx = SCREEN_WIDTH+uis.bias;
	}

	uis.cursory += dy;
	if (uis.cursory < 0) {
		uis.cursory = 0;
	} else if (uis.cursory > SCREEN_HEIGHT) {
		uis.cursory = SCREEN_HEIGHT;
	}

	// region test the active menu items
	for (i = 0; i<uis.activemenu->nitems; i++) {
		m = (menucommon_s*)uis.activemenu->items[i];

		if (m->flags & (QMF_GRAYED|QMF_INACTIVE)) {
			continue;
		}

		if ((uis.cursorx < m->left) || (uis.cursorx > m->right) ||
			(uis.cursory < m->top) || (uis.cursory > m->bottom))
		{
			// cursor out of item bounds
			continue;
		}

		// set focus to item at cursor
		if (uis.activemenu->cursor != i) {
			Menu_SetCursor(uis.activemenu, i);
			((menucommon_s*)(uis.activemenu->items[uis.activemenu->cursor_prev]))->flags &= ~QMF_HASMOUSEFOCUS;

			if (!(((menucommon_s*)(uis.activemenu->items[uis.activemenu->cursor]))->flags & QMF_SILENT)) {
				trap_S_StartLocalSound(menu_move_sound, CHAN_LOCAL_SOUND);
			}
		}

		((menucommon_s*)(uis.activemenu->items[uis.activemenu->cursor]))->flags |= QMF_HASMOUSEFOCUS;
		return;
	}

	if (uis.activemenu->nitems > 0) {
		// out of any region
		((menucommon_s*)(uis.activemenu->items[uis.activemenu->cursor]))->flags &= ~QMF_HASMOUSEFOCUS;
	}
}

char *UI_Argv(int arg)
{
	static char	buffer[MAX_STRING_CHARS];

	trap_Argv(arg, buffer, sizeof(buffer));

	return buffer;
}

char *UI_Cvar_VariableString(const char *var_name)
{
	static char	buffer[MAX_STRING_CHARS];

	trap_Cvar_VariableStringBuffer(var_name, buffer, sizeof(buffer));

	return buffer;
}

void UI_Cache_f(void)
{
	InGame_Cache();
	ConfirmMenu_Cache();
	PlayerModel_Cache();
	PlayerSettings_Cache();
	Controls_Cache();
	Demos_Cache();
	Preferences_Cache();
	ServerInfo_Cache();
	SpecifyServer_Cache();
	ArenaServers_Cache();
	StartServer_Cache();
	ServerOptions_Cache();
	DriverInfo_Cache();
	GraphicsOptions_Cache();
	UI_DisplayOptionsMenu_Cache();
	UI_SoundOptionsMenu_Cache();
	UI_NetworkOptionsMenu_Cache();
	TeamMain_Cache();
	UI_AddBots_Cache();
	UI_RemoveBots_Cache();
	UI_SetupMenu_Cache();
	UI_BotSelectMenu_Cache();
}

qboolean UI_ConsoleCommand(int realTime)
{
	char	*cmd;

	uis.frametime = realTime - uis.realtime;
	uis.realtime = realTime;

	cmd = UI_Argv(0);

	// ensure minimum menu data is available
	Menu_Cache();

	if (Q_stricmp (cmd, "ui_cache") == 0) {
		UI_Cache_f();
		return qtrue;
	}

	return qfalse;
}

void UI_Shutdown(void) { }

void UI_Init(void)
{
	UI_RegisterCvars();

	UI_InitGameinfo();

	// initialize the menu system
	Menu_Cache();

	uis.activemenu = NULL;
	uis.menusp     = 0;
}

void UI_DrawNamedPic(float x, float y, float width, float height, const char *picname)
{
	qhandle_t	hShader;

	hShader = trap_R_RegisterShaderNoMip(picname);
	SCR_AdjustFrom640(&x, &y, &width, &height);
	trap_R_DrawStretchPic(x, y, width, height, 0, 0, 1, 1, hShader);
}

void UI_DrawHandlePic(float x, float y, float w, float h, qhandle_t hShader)
{
	float	s0;
	float	s1;
	float	t0;
	float	t1;

	if (w < 0) {	// flip about vertical
		w  = -w;
		s0 = 1;
		s1 = 0;
	} else {
		s0 = 0;
		s1 = 1;
	}

	if (h < 0) {	// flip about horizontal
		h  = -h;
		t0 = 1;
		t1 = 0;
	} else {
		t0 = 0;
		t1 = 1;
	}
	
	SCR_AdjustFrom640(&x, &y, &w, &h);
	trap_R_DrawStretchPic(x, y, w, h, s0, t0, s1, t1, hShader);
}

void UI_UpdateScreen(void)
{
	trap_UpdateScreen();
}

void UI_Refresh(int realtime)
{
	uis.frametime = realtime - uis.realtime;
	uis.realtime  = realtime;

	trap_GetGlconfig(&uis.glconfig);

	// for 640x480 virtualized screen
	uis.xscale = uis.glconfig.vidWidth * (1.0/640.0);
	uis.yscale = uis.glconfig.vidHeight * (1.0/480.0);
	if (uis.glconfig.vidWidth * 480 > uis.glconfig.vidHeight * 640) {
		// wide screen
		uis.bias = 0.5 * (uis.glconfig.vidWidth - (uis.glconfig.vidHeight * (640.0/480.0)));
		uis.xscale = uis.yscale;
	} else {
		// no wide screen
		uis.bias = 0;
	}

	if (!(trap_Key_GetCatcher() & KEYCATCH_UI)) {
		return;
	}

	UI_UpdateCvars();

	if (uis.activemenu) {
		if (uis.activemenu->fullscreen) {
			// draw the background
			if (uis.activemenu->showlogo) {
				UI_DrawHandlePic(0, 0, SCREEN_WIDTH, SCREEN_HEIGHT, uis.menuBackShader);
			} else {
				UI_DrawHandlePic(0, 0, SCREEN_WIDTH, SCREEN_HEIGHT, uis.menuBackNoLogoShader);
			}
		}

		if (uis.activemenu->draw) {
			uis.activemenu->draw();
		} else {
			Menu_Draw(uis.activemenu);
		}

		if (uis.firstdraw) {
			UI_MouseEvent(0, 0);
			uis.firstdraw = qfalse;
		}
	}

	// draw cursor
	trap_R_SetColor(NULL);
	UI_DrawHandlePic(uis.cursorx-16, uis.cursory-16, 32, 32, uis.cursor);

#ifndef NDEBUG
	if (uis.debug) {
		// cursor coordinates
		SCR_DrawString(0, 0, va("(%d,%d)", uis.cursorx, uis.cursory), SMALLCHAR_SIZE, 0, colorRed);
	}
#endif

	// delay playing the enter sound until after the
	// menu has been drawn, to avoid delay while
	// caching images
	if (m_entersound) {
		trap_S_StartLocalSound(menu_in_sound, CHAN_LOCAL_SOUND);
		m_entersound = qfalse;
	}
}

void UI_DrawTextBox (int x, int y, int width, int lines)
{
	SCR_FillRect(x + BIGCHAR_SIZE / 2, y + BIGCHAR_SIZE / 2, (width + 1) * BIGCHAR_SIZE, (lines + 1) * BIGCHAR_SIZE, colorBlack);
	SCR_DrawRect(x + BIGCHAR_SIZE / 2, y + BIGCHAR_SIZE / 2, (width + 1) * BIGCHAR_SIZE, (lines + 1) * BIGCHAR_SIZE, 1.0f, colorWhite);
}

qboolean UI_CursorInRect (int x, int y, int width, int height)
{
	if (uis.cursorx < x || uis.cursory < y || uis.cursorx > x+width || uis.cursory > y+height) {
		return qfalse;
	}

	return qtrue;
}


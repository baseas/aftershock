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
// ui_mfield.c

#include "ui_local.h"

void MenuField_Init(menufield_s* m)
{
	int	l;
	int	w;
	int	h;

	MField_Clear(&m->field);

	if (m->generic.flags & QMF_SMALLFONT) {
		w = SMALLCHAR_SIZE;
		h = SMALLCHAR_SIZE;
	} else {
		w = BIGCHAR_SIZE;
		h = BIGCHAR_SIZE;
	}

	if (m->generic.name) {
		l = (strlen(m->generic.name)+1) * w;
	} else {
		l = 0;
	}

	m->generic.left   = m->generic.x - l;
	m->generic.top    = m->generic.y;
	m->generic.right  = m->generic.x + w + m->field.widthInChars*w;
	m->generic.bottom = m->generic.y + h;
}

void MenuField_Draw(menufield_s *f)
{
	int		x;
	int		y;
	int		w;
	int		style;
	qboolean focus;
	float	*color;

	x =	f->generic.x;
	y =	f->generic.y;

	if (f->generic.flags & QMF_SMALLFONT) {
		w = SMALLCHAR_SIZE;
		style = FONT_SMALL;
	} else {
		w = BIGCHAR_SIZE;
		style = 0;
	}

	if (Menu_ItemAtCursor(f->generic.parent) == f) {
		focus = qtrue;
		style |= FONT_PULSE;
	} else {
		focus = qfalse;
	}

	if (f->generic.flags & QMF_GRAYED) {
		color = text_color_disabled;
	} else if (focus) {
		color = text_color_highlight;
	} else {
		color = text_color_normal;
	}

	if (focus) {
		// draw cursor
		SCR_FillRect(f->generic.left, f->generic.top, f->generic.right-f->generic.left+1, f->generic.bottom-f->generic.top+1, listbar_color);
		SCR_DrawString(x, y, "\15", w, FONT_CENTER | FONT_BLINK, color);
	}

	if (f->generic.name) {
		SCR_DrawString(x - w, y, f->generic.name, w, FONT_RIGHT, color);
	}

	MField_Draw(&f->field, x + w, y, w, style, color);
}

sfxHandle_t MenuField_Key(menufield_s* m, int* key)
{
	int keycode;

	keycode = *key;

	switch (keycode) {
	case K_KP_ENTER:
	case K_ENTER:
	case K_JOY1:
	case K_JOY2:
	case K_JOY3:
	case K_JOY4:
		// have enter go to next cursor point
		*key = K_TAB;
		break;

	case K_TAB:
	case K_KP_DOWNARROW:
	case K_DOWNARROW:
	case K_KP_UPARROW:
	case K_UPARROW:
		break;

	default:
		if (keycode & K_CHAR_FLAG)
		{
			keycode &= ~K_CHAR_FLAG;

			if ((m->generic.flags & QMF_UPPERCASE) && Q_islower(keycode))
				keycode -= 'a' - 'A';
			else if ((m->generic.flags & QMF_LOWERCASE) && Q_isupper(keycode))
				keycode -= 'A' - 'a';
			else if ((m->generic.flags & QMF_NUMBERSONLY) && Q_isalpha(keycode))
				return (menu_buzz_sound);

			MField_CharEvent(&m->field, keycode);
		}
		else
			MField_KeyDownEvent(&m->field, keycode);
		break;
	}

	return 0;
}


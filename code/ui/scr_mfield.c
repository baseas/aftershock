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
// scr_mfield.c -- for ui controls and cgame chat input

#ifdef CGAME
	#include "../cgame/cg_local.h"
#else
	#include "../ui/ui_local.h"
#endif

#include "../client/keycodes.h"

static void MField_Paste(mfield_t *edit)
{
	char	pasteBuffer[64];
	int		pasteLen, i;

	trap_GetClipboardData(pasteBuffer, 64);

	// send as if typed, so insert / overstrike works properly
	pasteLen = strlen(pasteBuffer);
	for (i = 0 ; i < pasteLen ; i++) {
		MField_CharEvent(edit, pasteBuffer[i]);
	}
}

/**
Handles horizontal scrolling and cursor blinking x, y, are in pixels
*/
void MField_Draw(mfield_t *edit, int x, int y, float fontSize, int style, vec4_t color)
{
	int		len;
	int		drawLen;
	int		prestep;
	int		cursorChar;
	char	str[MAX_STRING_CHARS];
	int		fontStyle = 0;

	drawLen = edit->widthInChars;
	len = strlen(edit->buffer) + 1;

	// guarantee that cursor will be visible
	if (len <= drawLen) {
		prestep = 0;
	} else {
		if (edit->scroll + drawLen > len) {
			edit->scroll = len - drawLen;
			if (edit->scroll < 0) {
				edit->scroll = 0;
			}
		}
		prestep = edit->scroll;
	}

	if (prestep + drawLen > len) {
		drawLen = len - prestep;
	}

	// extract <drawLen> characters from the field at <prestep>
	if (drawLen >= MAX_STRING_CHARS) {
		trap_Error("drawLen >= MAX_STRING_CHARS");
	}
	memcpy(str, edit->buffer + prestep, drawLen);
	str[drawLen] = 0;

	if (style & FONT_CENTER) {
		fontStyle |= FONT_CENTER;
	} else if (style & FONT_RIGHT) {
		fontStyle |= FONT_RIGHT;
	}

	if (style & FONT_PULSE) {
		fontStyle |= FONT_PULSE;
	}
	if (style & FONT_BLINK) {
		fontStyle |= FONT_BLINK;
	}

	SCR_DrawString(x, y, str, fontSize, fontStyle, color);

	// draw the cursor
	if (!(style & FONT_PULSE)) {
		return;
	}

	if (trap_Key_GetOverstrikeMode()) {
		cursorChar = 11;
	} else {
		cursorChar = 10;
	}

	style &= ~FONT_PULSE;
	style |= FONT_BLINK;

	if (style & FONT_CENTER) {
		len = strlen(str);
		x = x - len * fontSize / 2;
	} else if (style & FONT_RIGHT) {
		len = strlen(str);
		x = x - len * fontSize;
	}

	SCR_DrawString(x + (edit->cursor - prestep) * fontSize, y, va("%c", cursorChar),
		fontSize, fontStyle & ~(FONT_CENTER | FONT_RIGHT), color);
}

/**
Performs the basic line editing functions for the console,
in-game talk, and menu fields

Key events are used for non-printable characters, others are gotten from char events.
*/
void MField_KeyDownEvent(mfield_t *edit, int key)
{
	int		len;

	// shift-insert is paste
	if (((key == K_INS) || (key == K_KP_INS)) && trap_Key_IsDown(K_SHIFT)) {
		MField_Paste(edit);
		return;
	}

	len = strlen(edit->buffer);

	if (key == K_DEL || key == K_KP_DEL) {
		if (edit->cursor < len) {
			memmove(edit->buffer + edit->cursor,
				edit->buffer + edit->cursor + 1, len - edit->cursor);
		}
		return;
	}

	if (key == K_RIGHTARROW || key == K_KP_RIGHTARROW) {
		if (edit->cursor < len) {
			edit->cursor++;
		}
		if (edit->cursor >= edit->scroll + edit->widthInChars && edit->cursor <= len) {
			edit->scroll++;
		}
		return;
	}

	if (key == K_LEFTARROW || key == K_KP_LEFTARROW) {
		if (edit->cursor > 0) {
			edit->cursor--;
		}
		if (edit->cursor < edit->scroll)
		{
			edit->scroll--;
		}
		return;
	}

	if (key == K_HOME || key == K_KP_HOME || (tolower(key) == 'a' && trap_Key_IsDown(K_CTRL))) {
		edit->cursor = 0;
		edit->scroll = 0;
		return;
	}

	if (key == K_END || key == K_KP_END || (tolower(key) == 'e' && trap_Key_IsDown(K_CTRL))) {
		edit->cursor = len;
		edit->scroll = len - edit->widthInChars + 1;
		if (edit->scroll < 0)
			edit->scroll = 0;
		return;
	}

	if (key == K_INS || key == K_KP_INS) {
		trap_Key_SetOverstrikeMode(!trap_Key_GetOverstrikeMode());
		return;
	}
}

void MField_CharEvent(mfield_t *edit, int ch)
{
	int		len;

	if (ch == 'v' - 'a' + 1) {	// ctrl-v is paste
		MField_Paste(edit);
		return;
	}

	if (ch == 'c' - 'a' + 1) {	// ctrl-c clears the field
		MField_Clear(edit);
		return;
	}

	len = strlen(edit->buffer);

	if (ch == 'h' - 'a' + 1)	{	// ctrl-h is backspace
		if (edit->cursor > 0) {
			memmove(edit->buffer + edit->cursor - 1,
				edit->buffer + edit->cursor, len + 1 - edit->cursor);
			edit->cursor--;
			if (edit->cursor < edit->scroll)
			{
				edit->scroll--;
			}
		}
		return;
	}

	if (ch == 'a' - 'a' + 1) {	// ctrl-a is home
		edit->cursor = 0;
		edit->scroll = 0;
		return;
	}

	if (ch == 'e' - 'a' + 1) {	// ctrl-e is end
		edit->cursor = len;
		edit->scroll = edit->cursor - edit->widthInChars + 1;
		if (edit->scroll < 0)
			edit->scroll = 0;
		return;
	}

	//
	// ignore any other non printable chars
	//
	if (ch < 0x20 || ch > 0x7E) {
		return;
	}

	if (trap_Key_GetOverstrikeMode()) {
		if ((edit->cursor == MAX_EDIT_LINE - 1) || (edit->maxchars && edit->cursor >= edit->maxchars)) {
			return;
		}
	} else {
		// insert mode
		if ((len == MAX_EDIT_LINE - 1) || (edit->maxchars && len >= edit->maxchars)) {
			return;
		}
		memmove(edit->buffer + edit->cursor + 1, edit->buffer + edit->cursor, len + 1 - edit->cursor);
	}

	edit->buffer[edit->cursor] = ch;
	if (!edit->maxchars || edit->cursor < edit->maxchars-1) {
		edit->cursor++;
	}

	if (edit->cursor >= edit->widthInChars) {
		edit->scroll++;
	}

	if (edit->cursor == len + 1) {
		edit->buffer[edit->cursor] = 0;
	}
}

void MField_Clear(mfield_t *edit)
{
	edit->buffer[0] = 0;
	edit->cursor = 0;
	edit->scroll = 0;
}


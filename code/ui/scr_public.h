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
// scr_public.c -- draw functions used by ui and cgame

#include "../qcommon/q_shared.h"

//
// scr_draw.c
//

#define PROP_SMALL_SIZE_SCALE	0.75

#define PROP_GAP_WIDTH		3.0f
#define PROP_SPACE_WIDTH	8.0f
#define PROP_HEIGHT			27.0f

#define PROPB_GAP_WIDTH		4.0f
#define PROPB_SPACE_WIDTH	12.0f
#define PROPB_HEIGHT		36.0f
#define PROPB_SIZE			0.6f

#define BLINK_DIVISOR		200
#define PULSE_DIVISOR		75

#define NUM_CROSSHAIRS		19

enum {
	FONT_CENTER = (1 << 0),		// align center
	FONT_RIGHT = (1 << 1),		// align right
	FONT_INVERSE = (1 << 2),	//
	FONT_SHADOW = (1 << 3),		// drop shadow
	FONT_NOCOLOR = (1 << 4),	// ignore color strings
	FONT_PULSE = (1 << 6),		// pulsating text
	FONT_BLINK = (1 << 7),		//
	FONT_SMALL = (1 << 8)		// smaller font for banner and prop string
};

void	SCR_DrawBannerString(float x, float y, const char *string, int style, const vec4_t color);
float	SCR_BannerStringWidth(const char *string);
void	SCR_DrawPropString(float x, float y, const char *string, int style, const vec4_t color);
float	SCR_PropStringWidth(const char *string);
void	SCR_DrawString(float x, float y, const char *string, float size, int style, const vec4_t color);
float	SCR_StringWidth(const char *str, float size);
int		SCR_Strlen(const char *str);
void	SCR_SetRGBA(byte *incolor, const vec4_t color);
int		SCR_ParseColor(vec4_t incolor, const char *string);
void	SCR_AdjustFrom640(float *x, float *y, float *width, float *height);
void	SCR_DrawRect(float x, float y, float width, float height, float size, const vec4_t color);
void	SCR_FillRect(float x, float y, float width, float height, const vec4_t color);

//
// scr_mfield.c
//

#define MAX_EDIT_LINE	256

typedef struct {
	int		cursor;
	int		scroll;
	int		widthInChars;
	char	buffer[MAX_EDIT_LINE];
	int		maxchars;
} mfield_t;

void		MField_Clear(mfield_t *edit);
void		MField_KeyDownEvent(mfield_t *edit, int key);
void		MField_CharEvent(mfield_t *edit, int ch);
void		MField_Draw(mfield_t *edit, int x, int y, float fontSize, int style, vec4_t color);


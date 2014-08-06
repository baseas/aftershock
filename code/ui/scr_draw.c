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
// scr_font.c

#ifdef CGAME
	#include "../cgame/cg_local.h"
	#define WHITE_SHADER cgs.media.whiteShader
#else
	#include "ui_local.h"
	#define WHITE_SHADER uis.whiteShader
#endif

static int	propMapB[26][3] = {
	{ 11, 12, 33 },
	{ 49, 12, 31 },
	{ 85, 12, 31 },
	{ 120, 12, 30 },
	{ 156, 12, 21 },
	{ 183, 12, 21 },
	{ 207, 12, 32 },

	{ 13, 55, 30 },
	{ 49, 55, 13 },
	{ 66, 55, 29 },
	{ 101, 55, 31 },
	{ 135, 55, 21 },
	{ 158, 55, 40 },
	{ 204, 55, 32 },

	{ 12, 97, 31 },
	{ 48, 97, 31 },
	{ 82, 97, 30 },
	{ 118, 97, 30 },
	{ 153, 97, 30 },
	{ 185, 97, 25 },
	{ 213, 97, 30 },

	{ 11, 139, 32 },
	{ 42, 139, 51 },
	{ 93, 139, 32 },
	{ 126, 139, 31 },
	{ 158, 139, 25 }
};

static int	propMap[128][3] = {
	{ 0, 0, -1 }, { 0, 0, -1 }, { 0, 0, -1 }, { 0, 0, -1 },
	{ 0, 0, -1 }, { 0, 0, -1 }, { 0, 0, -1 }, { 0, 0, -1 },
	{ 0, 0, -1 }, { 0, 0, -1 }, { 0, 0, -1 }, { 0, 0, -1 },
	{ 0, 0, -1 }, { 0, 0, -1 }, { 0, 0, -1 }, { 0, 0, -1 },
	{ 0, 0, -1 }, { 0, 0, -1 }, { 0, 0, -1 }, { 0, 0, -1 },
	{ 0, 0, -1 }, { 0, 0, -1 }, { 0, 0, -1 }, { 0, 0, -1 },
	{ 0, 0, -1 }, { 0, 0, -1 }, { 0, 0, -1 }, { 0, 0, -1 },
	{ 0, 0, -1 }, { 0, 0, -1 }, { 0, 0, -1 }, { 0, 0, -1 },

	{   0,   0, PROP_SPACE_WIDTH },		// SPACE
	{  11, 122,  7 },	// !
	{ 154, 181, 14 },	// "
	{  55, 122, 17 },	// #
	{  79, 122, 18 },	// $
	{ 101, 122, 23 },	// %
	{ 153, 122, 18 },	// &
	{   9,  93,  7 },	// '
	{ 207, 122,  8 },	// (
	{ 230, 122,  9 },	//)
	{ 177, 122, 18 },	// *
	{  30, 152, 18 },	// +
	{  85, 181,  7 },	// ,
	{  34,  93, 11 },	// -
	{ 110, 181,  6 },	// .
	{ 130, 152, 14 },	// /

	{  22,  64, 17 },	// 0
	{  41,  64, 12 },	// 1
	{  58,  64, 17 },	// 2
	{  78,  64, 18 },	// 3
	{  98,  64, 19 },	// 4
	{ 120,  64, 18 },	// 5
	{ 141,  64, 18 },	// 6
	{ 204,  64, 16 },	// 7
	{ 162,  64, 17 },	// 8
	{ 182,  64, 18 },	// 9
	{  59, 181,  7 },	// :
	{  35, 181,  7 },	// ;
	{ 203, 152, 14 },	// <
	{  56,  93, 14 },	// =
	{ 228, 152, 14 },	// >
	{ 177, 181, 18 },	// ?

	{  28, 122, 22 },	// @
	{   5,   4, 18 },	// A
	{  27,   4, 18 },	// B
	{  48,   4, 18 },	// C
	{  69,   4, 17 },	// D
	{  90,   4, 13 },	// E
	{ 106,   4, 13 },	// F
	{ 121,   4, 18 },	// G
	{ 143,   4, 17 },	// H
	{ 164,   4,  8 },	// I
	{ 175,   4, 16 },	// J
	{ 195,   4, 18 },	// K
	{ 216,   4, 12 },	// L
	{ 230,   4, 23 },	// M
	{   6,  34, 18 },	// N
	{  27,  34, 18 },	// O

	{  48,  34, 18 },	// P
	{  68,  34, 18 },	// Q
	{  90,  34, 17 },	// R
	{ 110,  34, 18 },	// S
	{ 130,  34, 14 },	// T
	{ 146,  34, 18 },	// U
	{ 166,  34, 19 },	// V
	{ 185,  34, 29 },	// W
	{ 215,  34, 18 },	// X
	{ 234,  34, 18 },	// Y
	{   5,  64, 14 },	// Z
	{  60, 152,  7 },	// [
	{ 106, 151, 13 },	// '\'
	{  83, 152,  7 },	// ]
	{ 128, 122, 17 },	// ^
	{   4, 152, 21 },	// _

	{ 134, 181,  5 },	// '
	{   5,   4, 18 },	// A
	{  27,   4, 18 },	// B
	{  48,   4, 18 },	// C
	{  69,   4, 17 },	// D
	{  90,   4, 13 },	// E
	{ 106,   4, 13 },	// F
	{ 121,   4, 18 },	// G
	{ 143,   4, 17 },	// H
	{ 164,   4,  8 },	// I
	{ 175,   4, 16 },	// J
	{ 195,   4, 18 },	// K
	{ 216,   4, 12 },	// L
	{ 230,   4, 23 },	// M
	{   6, 34,  18 },	// N
	{  27, 34,  18 },	// O

	{  48,  34, 18 },	// P
	{  68,  34, 18 },	// Q
	{  90,  34, 17 },	// R
	{ 110,  34, 18 },	// S
	{ 130,  34, 14 },	// T
	{ 146,  34, 18 },	// U
	{ 166,  34, 19 },	// V
	{ 185,  34, 29 },	// W
	{ 215,  34, 18 },	// X
	{ 234,  34, 18 },	// Y
	{   5,  64, 14 },	// Z
	{ 153, 152, 13 },	// { 
	{  11, 181,  5 },	// |
	{ 180, 152, 13 },	//  }
	{  79,  93, 17 },	// ~
	{   0,   0, -1 }	// DEL
};

static void SCR_DrawChar(float x, float y, float width, float height, int ch)
{
	int		row, col;
	float	frow, fcol;
	float	size;
	float	ax, ay, aw, ah;
	qhandle_t	shader;

	ch &= 255;

	if (ch == ' ') {
		return;
	}

	ax = x;
	ay = y;
	aw = width;
	ah = height;
	SCR_AdjustFrom640(&ax, &ay, &aw, &ah);

	row = ch >> 4;
	col = ch & 15;

	frow = row * 0.0625;
	fcol = col * 0.0625;
	size = 0.0625;

#if CGAME
	if (height * cgs.screenYScale <= 16) {
		shader = cgs.media.charsetShader;
	} else if (height * cgs.screenYScale <= 32) {
		shader = cgs.media.charsetShader32;
	} else if (height * cgs.screenYScale <= 64) {
		shader = cgs.media.charsetShader64;
	} else {
		shader = cgs.media.charsetShader128;
	}
#else
	if (height <= 16) {
		shader = uis.charsetShader;
	} else if (height <= 32) {
		shader = uis.charsetShader32;
	} else if (height <= 64) {
		shader = uis.charsetShader64;
	} else {
		shader = uis.charsetShader128;
	}
#endif

	trap_R_DrawStretchPic(ax, ay, aw, ah, fcol, frow, fcol + size, frow + size, shader);
}

static void SCR_DrawBannerString2(float x, float y, const char *string, float sizeScale, qhandle_t charset)
{
	const char		*s;
	unsigned char	ch;
	float	ax;
	float	ay;
	float	aw;
	float	ah;
	float	frow;
	float	fcol;
	float	fwidth;
	float	fheight;
	float	xscale, yscale;

#ifdef CGAME
	xscale = cgs.screenXScale;
	yscale = cgs.screenYScale;
	ax = x * xscale + cgs.screenXBias;
#else
	xscale = uis.xscale;
	yscale = uis.yscale;
	ax = x * xscale + uis.bias;
#endif

	ay = y * yscale;

	for (s = string; *s; s++) {
		ch = *s & 127;
		if (ch == ' ') {
			ax += PROPB_SIZE * (PROPB_SPACE_WIDTH + PROPB_GAP_WIDTH) * xscale * sizeScale;
		} else if (ch >= 'A' && ch <= 'Z') {
			ch -= 'A';
			fcol = propMapB[ch][0] / 256.0f;
			frow = propMapB[ch][1] / 256.0f;
			fwidth = propMapB[ch][2] / 256.0f;
			fheight = PROPB_HEIGHT/ 256.0f;
			aw = PROPB_SIZE * propMapB[ch][2] * xscale * sizeScale;
			ah = PROPB_SIZE * PROPB_HEIGHT * yscale * sizeScale;
#ifdef CGAME
			aw = CG_AdjustWidth(aw);
#endif
			trap_R_DrawStretchPic(ax, ay, aw, ah, fcol, frow, fcol + fwidth, frow + fheight, charset);
			ax += aw + PROP_GAP_WIDTH * xscale * sizeScale;
		}
	}
}

static void SCR_DrawPropString2(float x, float y, const char *string, float sizeScale, qhandle_t charset)
{
	const char		*s;
	unsigned char	ch;
	float	ax;
	float	ay;
	float	aw;
	float	ah;
	float	frow;
	float	fcol;
	float	fwidth;
	float	fheight;
	float	xscale, yscale;

#ifdef CGAME
	xscale = cgs.screenXScale;
	yscale = cgs.screenYScale;
	ax = x * xscale + cgs.screenXBias;
#else
	xscale = uis.xscale;
	yscale = uis.yscale;
	ax = x * xscale + uis.bias;
#endif

	ay = y * yscale;

	for (s = string; *s; s++) {
		ch = *s & 127;
		if (ch == ' ') {
			aw = PROP_SPACE_WIDTH * xscale * sizeScale;
		} else if (propMap[ch][2] != -1) {
			fcol = propMap[ch][0] / 256.0f;
			frow = propMap[ch][1] / 256.0f;
			fwidth = propMap[ch][2] / 256.0f;
			fheight = PROP_HEIGHT / 256.0f;
			aw = propMap[ch][2] * xscale * sizeScale;
			ah = PROP_HEIGHT * yscale * sizeScale;
#ifdef CGAME
			aw = CG_AdjustWidth(aw);
#endif
			trap_R_DrawStretchPic(ax, ay, aw, ah, fcol, frow, fcol + fwidth, frow + fheight, charset);
		} else {
			aw = 0;
		}

		ax += aw + PROP_GAP_WIDTH * xscale * sizeScale;
	}
}

float SCR_PropStringWidth(const char *string)
{
	const char	*s;
	int			ch;
	int			charWidth;
	float		width = 0;

	for (s = string; *s; s++) {
		ch = *s & 127;
		charWidth = propMap[ch][2];
		if (charWidth != -1) {
			width += charWidth;
			width += PROP_GAP_WIDTH;
		}
	}

	width -= PROP_GAP_WIDTH;
#ifdef CGAME
	return CG_AdjustWidth(width);
#else
	return width;
#endif
}

float SCR_BannerStringWidth(const char *string)
{
	const char	*s;
	float		width = 0;

	s = string;
	for (s = string; *s; s++) {
		if (*s == ' ') {
			width += PROPB_SIZE * (PROPB_SPACE_WIDTH + PROPB_GAP_WIDTH);
		} else if (*s >= 'A' && *s <= 'Z') {
			width += PROPB_SIZE * (propMapB[*s - 'A'][2] + PROPB_GAP_WIDTH);
		}
	}
	width -= PROPB_SIZE * PROPB_GAP_WIDTH;
#ifdef CGAME
	return CG_AdjustWidth(width);
#else
	return width;
#endif
}

void SCR_DrawPropString(float x, float y, const char *string, int style, const vec4_t color)
{
	vec4_t		drawcolor;
	int			time;
	qhandle_t	charset, charsetGlow;
	float		size;

	size = (style & FONT_SMALL ? PROP_SMALL_SIZE_SCALE : 1.0f);

#if CGAME
	time = cg.time;
	charset = cgs.media.charsetProp;
	charsetGlow = cgs.media.charsetPropGlow;
#else
	time = uis.realtime;
	charset = uis.charsetProp;
	charsetGlow = uis.charsetPropGlow;
#endif

	if (style & FONT_CENTER) {
		x -= SCR_PropStringWidth(string) * size / 2;
	} else if (style & FONT_RIGHT) {
		x -= SCR_PropStringWidth(string) * size;
	}

	if (style & FONT_SHADOW) {
		Vector4Copy(color, drawcolor);
		drawcolor[3] = 0.2f;
		trap_R_SetColor(drawcolor);
		SCR_DrawPropString2(x + 2.0f * size, y + 2.0f * size, string, size, charset);
	}

	if (style & FONT_INVERSE) {
		drawcolor[0] = color[0] * 0.7;
		drawcolor[1] = color[1] * 0.7;
		drawcolor[2] = color[2] * 0.7;
		drawcolor[3] = color[3];
		trap_R_SetColor(drawcolor);
		SCR_DrawPropString2(x, y, string, size, charset);
		return;
	}

	if (style & FONT_PULSE) {
		drawcolor[0] = color[0] * 0.7;
		drawcolor[1] = color[1] * 0.7;
		drawcolor[2] = color[2] * 0.7;
		drawcolor[3] = color[3];
		trap_R_SetColor(drawcolor);
		SCR_DrawPropString2(x, y, string, size, charset);

		drawcolor[0] = color[0];
		drawcolor[1] = color[1];
		drawcolor[2] = color[2];
		drawcolor[3] = 0.5 + 0.5 * sin(time / PULSE_DIVISOR);
		trap_R_SetColor(drawcolor);
		SCR_DrawPropString2(x, y, string, size, charsetGlow);
		return;
	}

	trap_R_SetColor(color);
	SCR_DrawPropString2(x, y, string, size, charset);
}

void SCR_DrawBannerString(float x, float y, const char *string, int style, const vec4_t color)
{
	vec4_t		drawcolor;
	qhandle_t	charset;
	float		size;

	size = (style & FONT_SMALL ? PROP_SMALL_SIZE_SCALE : 1.0f);

#if CGAME
	charset = cgs.media.charsetPropB;
#else
	charset = uis.charsetPropB;
#endif

	if (style & FONT_CENTER) {
		x -= SCR_BannerStringWidth(string) * size / 2;
	} else if (style & FONT_RIGHT) {
		x -= SCR_BannerStringWidth(string) * size;
	}

	if (style & FONT_SHADOW) {
		Vector4Copy(color, drawcolor);
		drawcolor[3] = 0.2f;
		trap_R_SetColor(drawcolor);
		SCR_DrawBannerString2(x + 2.0f * size, y + 2.0f * size, string, size, charset);
	}

	if (style & FONT_INVERSE) {
		drawcolor[0] = color[0] * 0.7;
		drawcolor[1] = color[1] * 0.7;
		drawcolor[2] = color[2] * 0.7;
		drawcolor[3] = color[3];
		trap_R_SetColor(drawcolor);
		SCR_DrawBannerString2(x, y, string, size, charset);
		return;
	}

	trap_R_SetColor(color);
	SCR_DrawBannerString2(x, y, string, size, charset);
}

void SCR_DrawString(float x, float y, const char *string, float size, int style, const vec4_t color)
{
	vec4_t		drawcolor;
	float		charWidth, charHeight;
	const char	*s;
	float		xx;
	int			cnt;
	int			colorLen;

	charWidth = size;
	charHeight = size;

#ifdef CGAME
	charWidth = CG_AdjustWidth(charWidth);
#endif

	if (style & FONT_CENTER) {
		x -= SCR_Strlen(string) * charWidth / 2 * 0.9f;
	} else if (style & FONT_RIGHT) {
		x -= SCR_Strlen(string) * charWidth * 0.9f;
	}

	if (style & FONT_SHADOW) {
		Vector4Copy(color, drawcolor);
		drawcolor[3] = 0.2f;
		SCR_DrawString(x + 0.1f * charWidth, y + 0.1f * charHeight, string, size,
			style & ~FONT_SHADOW & ~FONT_CENTER & ~FONT_RIGHT, drawcolor);
	}

	trap_R_SetColor(color);

	// draw the colored text
	s = string;
	xx = x;
	cnt = 0;
	while (*s) {
		if ((colorLen = Q_IsColorString(s))) {
			if (!(style & FONT_NOCOLOR)) {
				memcpy(drawcolor, g_color_table[ColorIndex(*(s + 1))], sizeof (vec4_t));
				trap_R_SetColor(drawcolor);
			}
			s += colorLen;
			continue;
		}
		SCR_DrawChar(xx, y, charWidth, charHeight, *s);
		xx += charWidth * 0.9f;
		cnt++;
		s++;
	}
}

float SCR_StringWidth(const char *string, float size)
{
#ifdef CGAME
	return SCR_Strlen(string) * CG_AdjustWidth(size) * 0.9f;
#else
	return SCR_Strlen(string) * size * 0.9f;
#endif
}

/**
Returns character count, skipping color escape codes
*/
int SCR_Strlen(const char *string)
{
	const char *s;
	int colorLen;
	int count = 0;

	s = string;
	while (*s) {
		if ((colorLen = Q_IsColorString(s))) {
			s += colorLen;
		} else {
			count++;
			s++;
		}
	}

	return count;
}

/**
Adjusted for resolution and screen aspect ratio
*/
void SCR_AdjustFrom640(float *x, float *y, float *w, float *h)
{
#ifdef CGAME
	*x *= cgs.screenXScale;
	*y *= cgs.screenYScale;
	*w *= cgs.screenXScale;
	*h *= cgs.screenYScale;
#else
	*x = *x * uis.xscale + uis.bias;
	*y *= uis.yscale;
	*w *= uis.xscale;
	*h *= uis.yscale;
#endif
}

void SCR_SetRGBA(byte incolor[4], const vec4_t color)
{
	incolor[0] = color[0] * 255.0f;
	incolor[1] = color[1] * 255.0f;
	incolor[2] = color[2] * 255.0f;
	incolor[3] = color[3] * 255.0f;
}

/**
Return value is non-zero if color string is invalid.
Fallback color is set to white.
*/
int SCR_ParseColor(vec4_t incolor, const char *str)
{
	if (strlen(str) == 1) {
		vec_t *color = g_color_table[ColorIndex(str[0])];
		incolor[0] = color[0];
		incolor[1] = color[1];
		incolor[2] = color[2];
		incolor[3] = color[3];
	} else if (!strcmp(str, "red")) {
		SCR_ParseColor(incolor, "1");
	} else if (!strcmp(str, "green")) {
		SCR_ParseColor(incolor, "2");
	} else if (!strcmp(str, "yellow")) {
		SCR_ParseColor(incolor, "3");
	} else if (!strcmp(str, "blue")) {
		SCR_ParseColor(incolor, "4");
	} else if (!strcmp(str, "cyan")) {
		SCR_ParseColor(incolor, "5");
	} else if (!strcmp(str, "purple")) {
		SCR_ParseColor(incolor, "6");
	} else if (!strcmp(str, "white")) {
		SCR_ParseColor(incolor, "7");
	} else if (!strcmp(str, "orange")) {
		SCR_ParseColor(incolor, "8");
	} else {
		int i, len, hex;
		if (str[0] != '0' || str[1] != 'x') {
			goto error;
		}

		str = &str[2];
		len = strlen(str);

		if (len != 6 && len != 8) {
			goto error;
		}

		for (i = 0; str[i]; ++i) {
			if ((str[i] < '0' || str[i] > '9') && (str[i] < 'a' || str[i] > 'f')
				&& (str[i] < 'A' || str[i] > 'F'))
			{
				goto error;
			}
		}

		hex = strtol(str, NULL, 16);
		if (len == 6) {
			incolor[0] = ((hex >> 16) & 0xFF) / 255.0f;
			incolor[1] = ((hex >> 8) & 0xFF) / 255.0f;
			incolor[2] = (hex & 0xFF) / 255.0f;
			incolor[3] = 1.0f;
		} else if (len == 8) {
			incolor[0] = ((hex >> 24) & 0xFF) / 255.0f;
			incolor[1] = ((hex >> 16) & 0xFF) / 255.0f;
			incolor[2] = ((hex >> 8) & 0xFF) / 255.0f;
			incolor[3] = (hex & 0xFF) / 255.0f;
		}
	}

	return 0;
error:
	incolor[0] = 1.0f;
	incolor[1] = 1.0f;
	incolor[2] = 1.0f;
	incolor[3] = 1.0f;
	return 1;
}

/**
Coordinates are 640*480 virtual values
*/
void SCR_FillRect(float x, float y, float width, float height, const vec4_t color)
{
	if (color[3] == 0.0f) {
		return;
	}

	trap_R_SetColor(color);
	SCR_AdjustFrom640(&x, &y, &width, &height);
	trap_R_DrawStretchPic(x, y, width, height, 0, 0, 0, 0, WHITE_SHADER);
}

/**
Coordinates are 640*480 virtual values
*/
void SCR_DrawRect(float x, float y, float width, float height, float size, const vec4_t color)
{
	if (color[3] == 0.0f) {
		return;
	}

	trap_R_SetColor(color);

	SCR_AdjustFrom640(&x, &y, &width, &height);

	trap_R_DrawStretchPic(x, y, width, size, 0, 0, 0, 0, WHITE_SHADER);
	trap_R_DrawStretchPic(x, y + height - size, width, size, 0, 0, 0, 0, WHITE_SHADER);
	trap_R_DrawStretchPic(x, y, size, height, 0, 0, 0, 0, WHITE_SHADER);
	trap_R_DrawStretchPic(x + width - size, y, size, height, 0, 0, 0, 0, WHITE_SHADER);
}


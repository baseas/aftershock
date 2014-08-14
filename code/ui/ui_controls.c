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
// ui_controls.c -- controls.menu

#include "ui_local.h"

typedef struct {
	char	*command;
	char	*label;
	int		id;
	int		anim;
	int		defaultbind;
	int		bind;
} bind_t;

typedef struct {
	char	*name;
	float	defaultvalue;
	float	value;
} configcvar_t;

enum {
	SAVE_NOOP,
	SAVE_YES,
	SAVE_NO,
	SAVE_CANCEL
};

// control sections
enum {
	C_MOVEMENT,
	C_LOOKING,
	C_BINDS,
	C_MISC,
	C_MAX
};

enum {
	ID_SHOWSCORES,
	ID_USEITEM,
	ID_SPEED,
	ID_FORWARD,
	ID_BACKPEDAL,
	ID_MOVELEFT,
	ID_MOVERIGHT,
	ID_MOVEUP,
	ID_MOVEDOWN,
	ID_ZOOMVIEW,
	ID_WEAPON1,
	ID_WEAPON2,
	ID_WEAPON3,
	ID_WEAPON4,
	ID_WEAPON5,
	ID_WEAPON6,
	ID_WEAPON7,
	ID_WEAPON8,
	ID_WEAPON9,
	ID_ATTACK,
	ID_WEAPPREV,
	ID_WEAPNEXT,
	ID_GESTURE,
	ID_CHAT,
	ID_CHAT2,
	ID_CHAT3,
	ID_CHAT4,
	ID_TOGGLEMENU,

	ID_AUTOSWITCH,
	ID_MOUSESPEED,
	ID_FOV,
	ID_ZOOMFOV,
	ID_ZOOMSCALING,

	ID_MOVEMENT,
	ID_LOOKING,
	ID_BINDS,
	ID_MISC,
	ID_DEFAULTS,
	ID_BACK,
	ID_SAVEANDEXIT,
	ID_EXIT
};

enum {
	ANIM_IDLE,
	ANIM_RUN,
	ANIM_WALK,
	ANIM_BACK,
	ANIM_JUMP,
	ANIM_CROUCH,
	ANIM_STEPLEFT,
	ANIM_STEPRIGHT,
	ANIM_TURNLEFT,
	ANIM_TURNRIGHT,
	ANIM_LOOKUP,
	ANIM_LOOKDOWN,
	ANIM_WEAPON1,
	ANIM_WEAPON2,
	ANIM_WEAPON3,
	ANIM_WEAPON4,
	ANIM_WEAPON5,
	ANIM_WEAPON6,
	ANIM_WEAPON7,
	ANIM_WEAPON8,
	ANIM_WEAPON9,
	ANIM_WEAPON10,
	ANIM_ATTACK,
	ANIM_GESTURE,
	ANIM_DIE,
	ANIM_CHAT
};

typedef struct {
	menuframework_s		menu;

	menutext_s			banner;
	menubitmap_s		player;

	menutext_s			movement;
	menutext_s			looking;
	menutext_s			weapons;
	menutext_s			misc;

	menuaction_s		walkforward;
	menuaction_s		backpedal;
	menuaction_s		stepleft;
	menuaction_s		stepright;
	menuaction_s		moveup;
	menuaction_s		movedown;
	menuaction_s		run;
	menuaction_s		machinegun;
	menuaction_s		chainsaw;
	menuaction_s		shotgun;
	menuaction_s		grenadelauncher;
	menuaction_s		rocketlauncher;
	menuaction_s		lightning;
	menuaction_s		railgun;
	menuaction_s		plasma;
	menuaction_s		bfg;
	menuaction_s		attack;
	menuaction_s		prevweapon;
	menuaction_s		nextweapon;
	menuaction_s		zoomview;
	menuaction_s		gesture;
	menufield_s			sensitivity;
	menufield_s			fov;
	menufield_s			zoomfov;
	menuradiobutton_s	zoomscaling;
	menuaction_s		showscores;
	menuradiobutton_s	autoswitch;
	menuaction_s		useitem;
	playerInfo_t		playerinfo;
	qboolean			changesmade;
	menuaction_s		chat;
	menuaction_s		chat2;
	menuaction_s		chat3;
	menuaction_s		chat4;
	menuaction_s		togglemenu;
	int					section;
	qboolean			waitingforkey;
	char				playerModel[64];
	vec3_t				playerViewangles;
	vec3_t				playerMoveangles;
	int					playerLegs;
	int					playerTorso;
	int					playerWeapon;
	qboolean			playerChat;

	menubutton_s		back;
} controls_t;

static controls_t s_controls;

static vec4_t colorControlsBinding  = { 1.00f, 0.43f, 0.00f, 1.00f };

static bind_t g_bindings[] = {
	{ "+scores",		"Show Scores",		ID_SHOWSCORES,	ANIM_IDLE,		-1,	-1 },
	{ "+button2",		"Use Item",			ID_USEITEM,		ANIM_IDLE,		-1,	-1 },
	{ "+speed", 		"Run / Walk",		ID_SPEED,		ANIM_RUN,		-1,	-1 },
	{ "+forward",		"Walk Forward",		ID_FORWARD,		ANIM_WALK,		-1,	-1 },
	{ "+back", 			"Backpedal",		ID_BACKPEDAL,	ANIM_BACK,		-1,	-1 },
	{ "+moveleft",		"Step Left",		ID_MOVELEFT,	ANIM_STEPLEFT,	-1,	-1 },
	{ "+moveright",		"Step Right",		ID_MOVERIGHT,	ANIM_STEPRIGHT,	-1,	-1 },
	{ "+moveup",		"Up / Jump",		ID_MOVEUP,		ANIM_JUMP,		-1,	-1 },
	{ "+movedown",		"Down / Crouch",	ID_MOVEDOWN,	ANIM_CROUCH,	-1,	-1 },
	{ "+zoom", 			"Zoom",				ID_ZOOMVIEW,	ANIM_IDLE,		-1,	-1 },
	{ "weapon 1",		"Gauntlet",			ID_WEAPON1,		ANIM_WEAPON1,	-1,	-1 },
	{ "weapon 2",		"Machinegun",		ID_WEAPON2,		ANIM_WEAPON2,	-1,	-1 },
	{ "weapon 3",		"Shotgun",			ID_WEAPON3,		ANIM_WEAPON3,	-1,	-1 },
	{ "weapon 4",		"Grenade Launcher",	ID_WEAPON4,		ANIM_WEAPON4,	-1,	-1 },
	{ "weapon 5",		"Rocket Launcher",	ID_WEAPON5,		ANIM_WEAPON5,	-1,	-1 },
	{ "weapon 6",		"Lightning Gun",	ID_WEAPON6,		ANIM_WEAPON6,	-1,	-1 },
	{ "weapon 7",		"Railgun",			ID_WEAPON7,		ANIM_WEAPON7,	-1,	-1 },
	{ "weapon 8",		"Plasma Gun",		ID_WEAPON8,		ANIM_WEAPON8,	-1,	-1 },
	{ "weapon 9",		"BFG",				ID_WEAPON9,		ANIM_WEAPON9,	-1,	-1 },
	{ "+attack", 		"Attack",			ID_ATTACK,		ANIM_ATTACK,	-1,	-1 },
	{ "weapprev",		"Prev weapon",		ID_WEAPPREV,	ANIM_IDLE,		-1,	-1 },
	{ "weapnext",		"Next weapon",		ID_WEAPNEXT,	ANIM_IDLE,		-1,	-1 },
	{ "+button3",		"Gesture",			ID_GESTURE,		ANIM_GESTURE,	-1,	-1 },
	{ "messagemode",	"Chat",				ID_CHAT,		ANIM_CHAT,		-1,	-1 },
	{ "messagemode2",	"Chat - Team",		ID_CHAT2,		ANIM_CHAT,		-1,	-1 },
	{ "messagemode3",	"Chat - Target",	ID_CHAT3,		ANIM_CHAT,		-1,	-1 },
	{ "messagemode4",	"Chat - Attacker",	ID_CHAT4,		ANIM_CHAT,		-1,	-1 },
	{ "togglemenu",		"Toggle Menu",		ID_TOGGLEMENU,	ANIM_IDLE,		-1,	-1 },
	{ NULL,				NULL,				0,				0,				-1,	-1 }
};

static configcvar_t g_configcvars[] = {
	{ "cg_autoswitch" },
	{ "sensitivity" },
	{ "cg_fov" },
	{ "cg_zoomFov" },
	{ "cg_zoomScaling" },
	{ NULL }
};

static menucommon_s *g_movement_controls[] = {
	(menucommon_s *)&s_controls.run,
	(menucommon_s *)&s_controls.walkforward,
	(menucommon_s *)&s_controls.backpedal,
	(menucommon_s *)&s_controls.stepleft,
	(menucommon_s *)&s_controls.stepright,
	(menucommon_s *)&s_controls.moveup,
	(menucommon_s *)&s_controls.movedown,
	NULL
};

static menucommon_s *g_binds_controls[] = {
	(menucommon_s *)&s_controls.zoomview,
	(menucommon_s *)&s_controls.attack,
	(menucommon_s *)&s_controls.nextweapon,
	(menucommon_s *)&s_controls.prevweapon,
	(menucommon_s *)&s_controls.chainsaw,
	(menucommon_s *)&s_controls.machinegun,
	(menucommon_s *)&s_controls.shotgun,
	(menucommon_s *)&s_controls.grenadelauncher,
	(menucommon_s *)&s_controls.rocketlauncher,
	(menucommon_s *)&s_controls.lightning,
	(menucommon_s *)&s_controls.railgun,
	(menucommon_s *)&s_controls.plasma,
	(menucommon_s *)&s_controls.bfg,
	NULL,
};

static menucommon_s *g_looking_controls[] = {
	(menucommon_s *)&s_controls.sensitivity,
	(menucommon_s *)&s_controls.fov,
	(menucommon_s *)&s_controls.zoomfov,
	(menucommon_s *)&s_controls.zoomscaling,
	(menucommon_s *)&s_controls.autoswitch,
	NULL,
};

static menucommon_s *g_misc_controls[] = {
	(menucommon_s *)&s_controls.showscores,
	(menucommon_s *)&s_controls.useitem,
	(menucommon_s *)&s_controls.gesture,
	(menucommon_s *)&s_controls.chat,
	(menucommon_s *)&s_controls.chat2,
	(menucommon_s *)&s_controls.chat3,
	(menucommon_s *)&s_controls.chat4,
	(menucommon_s *)&s_controls.togglemenu,
	NULL,
};

static menucommon_s **g_controls[] = {
	g_movement_controls,
	g_looking_controls,
	g_binds_controls,
	g_misc_controls,
};

static void Controls_InitCvars(void)
{
	int				i;
	configcvar_t	*cvarptr;

	cvarptr = g_configcvars;
	for (i = 0; ; i++,cvarptr++) {
		if (!cvarptr->name) {
			break;
		}

		// get current value
		cvarptr->value = trap_Cvar_VariableValue(cvarptr->name);

		// get default value
		trap_Cvar_Reset(cvarptr->name);
		cvarptr->defaultvalue = trap_Cvar_VariableValue(cvarptr->name);

		// restore current value
		trap_Cvar_SetValue(cvarptr->name, cvarptr->value);
	}
}

static float Controls_GetCvarDefault(char *name)
{
	configcvar_t	*cvarptr;
	int				i;

	cvarptr = g_configcvars;
	for (i = 0; ; i++, cvarptr++) {
		if (!cvarptr->name) {
			return 0;
		}

		if (!strcmp(cvarptr->name, name)) {
			break;
		}
	}

	return cvarptr->defaultvalue;
}

static float Controls_GetCvarValue(char *name)
{
	configcvar_t	*cvarptr;
	int				i;

	cvarptr = g_configcvars;
	for (i = 0; ; i++, cvarptr++) {
		if (!cvarptr->name) {
			return 0;
		}

		if (!strcmp(cvarptr->name, name)) {
			break;
		}
	}

	return cvarptr->value;
}

static void Controls_UpdateModel(int anim)
{
	VectorClear(s_controls.playerViewangles);
	VectorClear(s_controls.playerMoveangles);
	s_controls.playerViewangles[YAW] = 180 - 30;
	s_controls.playerMoveangles[YAW] = s_controls.playerViewangles[YAW];
	s_controls.playerLegs		     = LEGS_IDLE;
	s_controls.playerTorso			 = TORSO_STAND;
	s_controls.playerWeapon			 = -1;
	s_controls.playerChat			 = qfalse;

	switch (anim) {
	case ANIM_RUN:
		s_controls.playerLegs = LEGS_RUN;
		break;

	case ANIM_WALK:
		s_controls.playerLegs = LEGS_WALK;
		break;

	case ANIM_BACK:
		s_controls.playerLegs = LEGS_BACK;
		break;

	case ANIM_JUMP:
		s_controls.playerLegs = LEGS_JUMP;
		break;

	case ANIM_CROUCH:
		s_controls.playerLegs = LEGS_IDLECR;
		break;

	case ANIM_TURNLEFT:
		s_controls.playerViewangles[YAW] += 90;
		break;

	case ANIM_TURNRIGHT:
		s_controls.playerViewangles[YAW] -= 90;
		break;

	case ANIM_STEPLEFT:
		s_controls.playerLegs = LEGS_WALK;
		s_controls.playerMoveangles[YAW] = s_controls.playerViewangles[YAW] + 90;
		break;

	case ANIM_STEPRIGHT:
		s_controls.playerLegs = LEGS_WALK;
		s_controls.playerMoveangles[YAW] = s_controls.playerViewangles[YAW] - 90;
		break;

	case ANIM_LOOKUP:
		s_controls.playerViewangles[PITCH] = -45;
		break;

	case ANIM_LOOKDOWN:
		s_controls.playerViewangles[PITCH] = 45;
		break;

	case ANIM_WEAPON1:
		s_controls.playerWeapon = WP_GAUNTLET;
		break;

	case ANIM_WEAPON2:
		s_controls.playerWeapon = WP_MACHINEGUN;
		break;

	case ANIM_WEAPON3:
		s_controls.playerWeapon = WP_SHOTGUN;
		break;

	case ANIM_WEAPON4:
		s_controls.playerWeapon = WP_GRENADE_LAUNCHER;
		break;

	case ANIM_WEAPON5:
		s_controls.playerWeapon = WP_ROCKET_LAUNCHER;
		break;

	case ANIM_WEAPON6:
		s_controls.playerWeapon = WP_LIGHTNING;
		break;

	case ANIM_WEAPON7:
		s_controls.playerWeapon = WP_RAILGUN;
		break;

	case ANIM_WEAPON8:
		s_controls.playerWeapon = WP_PLASMAGUN;
		break;

	case ANIM_WEAPON9:
		s_controls.playerWeapon = WP_BFG;
		break;

	case ANIM_WEAPON10:
		s_controls.playerWeapon = WP_GRAPPLING_HOOK;
		break;

	case ANIM_ATTACK:
		s_controls.playerTorso = TORSO_ATTACK;
		break;

	case ANIM_GESTURE:
		s_controls.playerTorso = TORSO_GESTURE;
		break;

	case ANIM_DIE:
		s_controls.playerLegs = BOTH_DEATH1;
		s_controls.playerTorso = BOTH_DEATH1;
		s_controls.playerWeapon = WP_NONE;
		break;

	case ANIM_CHAT:
		s_controls.playerChat = qtrue;
		break;

	default:
		break;
	}

	UI_PlayerInfo_SetInfo(&s_controls.playerinfo, s_controls.playerLegs, s_controls.playerTorso, s_controls.playerViewangles, s_controls.playerMoveangles, s_controls.playerWeapon, s_controls.playerChat);
}

static void Controls_Update(void)
{
	int		i;
	int		j;
	int		y;
	menucommon_s	**controls;
	menucommon_s	*control;

	// disable all controls in all groups
	for (i = 0; i < C_MAX; i++) {
		controls = g_controls[i];
		for (j = 0; (control = controls[j]); j++) {
			control->flags |= (QMF_HIDDEN|QMF_INACTIVE);
		}
	}

	controls = g_controls[s_controls.section];

	// enable controls in active group (and count number of items for vertical centering)
	for (j = 0; (control = controls[j]); j++) {
		control->flags &= ~(QMF_GRAYED|QMF_HIDDEN|QMF_INACTIVE);
	}

	// position controls
	y = (SCREEN_HEIGHT - j * SMALLCHAR_SIZE) / 2;
	for (j = 0; (control = controls[j]); j++, y += SMALLCHAR_SIZE) {
		control->x      = 320;
		control->y      = y;
		control->left   = 320 - 19*SMALLCHAR_SIZE;
		control->right  = 320 + 21*SMALLCHAR_SIZE;
		control->top    = y;
		control->bottom = y + SMALLCHAR_SIZE;
	}

	if (s_controls.waitingforkey) {
		// disable everybody
		for (i = 0; i < s_controls.menu.nitems; i++) {
			((menucommon_s*)(s_controls.menu.items[i]))->flags |= QMF_GRAYED;
		}

		// enable action item
		((menucommon_s*)(s_controls.menu.items[s_controls.menu.cursor]))->flags &= ~QMF_GRAYED;
		return;
	}

	// enable everybody
	for (i = 0; i < s_controls.menu.nitems; i++) {
		((menucommon_s*)(s_controls.menu.items[i]))->flags &= ~QMF_GRAYED;
	}

	// makes sure flags are right on the group selection controls
	s_controls.looking.generic.flags  &= ~(QMF_GRAYED|QMF_HIGHLIGHT|QMF_HIGHLIGHT_IF_FOCUS);
	s_controls.movement.generic.flags &= ~(QMF_GRAYED|QMF_HIGHLIGHT|QMF_HIGHLIGHT_IF_FOCUS);
	s_controls.weapons.generic.flags  &= ~(QMF_GRAYED|QMF_HIGHLIGHT|QMF_HIGHLIGHT_IF_FOCUS);
	s_controls.misc.generic.flags     &= ~(QMF_GRAYED|QMF_HIGHLIGHT|QMF_HIGHLIGHT_IF_FOCUS);

	s_controls.looking.generic.flags  |= QMF_PULSEIFFOCUS;
	s_controls.movement.generic.flags |= QMF_PULSEIFFOCUS;
	s_controls.weapons.generic.flags  |= QMF_PULSEIFFOCUS;
	s_controls.misc.generic.flags     |= QMF_PULSEIFFOCUS;

	// set buttons
	switch (s_controls.section) {
	case C_MOVEMENT:
		s_controls.movement.generic.flags &= ~QMF_PULSEIFFOCUS;
		s_controls.movement.generic.flags |= (QMF_HIGHLIGHT|QMF_HIGHLIGHT_IF_FOCUS);
		break;

	case C_LOOKING:
		s_controls.looking.generic.flags &= ~QMF_PULSEIFFOCUS;
		s_controls.looking.generic.flags |= (QMF_HIGHLIGHT|QMF_HIGHLIGHT_IF_FOCUS);
		break;

	case C_BINDS:
		s_controls.weapons.generic.flags &= ~QMF_PULSEIFFOCUS;
		s_controls.weapons.generic.flags |= (QMF_HIGHLIGHT|QMF_HIGHLIGHT_IF_FOCUS);
		break;

	case C_MISC:
		s_controls.misc.generic.flags &= ~QMF_PULSEIFFOCUS;
		s_controls.misc.generic.flags |= (QMF_HIGHLIGHT|QMF_HIGHLIGHT_IF_FOCUS);
		break;
	}
}

static void Controls_DrawKeyBinding(void *self)
{
	menuaction_s	*a;
	int				x;
	int				y;
	qboolean		c;
	char			name[32];

	a = (menuaction_s*) self;

	x = a->generic.x;
	y = a->generic.y;

	c = (Menu_ItemAtCursor(a->generic.parent) == a);

	if (g_bindings[a->generic.id].bind == -1) {
		strcpy(name, "???");
	} else {
		trap_Key_KeynumToStringBuf(g_bindings[a->generic.id].bind, name, 32);
		Q_strupr(name);
	}

	if (c) {
		SCR_FillRect(a->generic.left, a->generic.top, a->generic.right-a->generic.left+1, a->generic.bottom-a->generic.top+1, colorListbar);

		SCR_DrawString(x - SMALLCHAR_SIZE, y, g_bindings[a->generic.id].label,
			SMALLCHAR_SIZE, FONT_RIGHT, colorControlsBinding);
		SCR_DrawString(x + SMALLCHAR_SIZE, y, name, SMALLCHAR_SIZE, FONT_PULSE, colorTextHighlight);

		if (s_controls.waitingforkey) {
			SCR_DrawString(x, y, "=", SMALLCHAR_SIZE, FONT_CENTER | FONT_BLINK, colorTextHighlight);
			SCR_DrawString(SCREEN_WIDTH * 0.50, SCREEN_HEIGHT * 0.80,
				"Waiting for new key ... ESCAPE to cancel", SMALLCHAR_SIZE, FONT_CENTER | FONT_PULSE, colorWhite);
		} else {
			SCR_DrawString(x, y, "\15", SMALLCHAR_SIZE, FONT_CENTER | FONT_BLINK, colorTextHighlight);
			SCR_DrawString(SCREEN_WIDTH * 0.50, SCREEN_HEIGHT * 0.78,
				"Press ENTER or CLICK to change", SMALLCHAR_SIZE, FONT_CENTER, colorWhite);
			SCR_DrawString(SCREEN_WIDTH * 0.50, SCREEN_HEIGHT * 0.82,
				"Press BACKSPACE to clear", SMALLCHAR_SIZE, FONT_CENTER, colorWhite);
		}
	} else {
		if (a->generic.flags & QMF_GRAYED) {
			SCR_DrawString(x - SMALLCHAR_SIZE, y, g_bindings[a->generic.id].label,
				SMALLCHAR_SIZE, FONT_RIGHT, colorTextDisabled);
		} else {
			SCR_DrawString(x - SMALLCHAR_SIZE, y, g_bindings[a->generic.id].label,
				SMALLCHAR_SIZE, FONT_RIGHT, colorControlsBinding);
			SCR_DrawString(x + SMALLCHAR_SIZE, y, name, SMALLCHAR_SIZE, 0, colorControlsBinding);
		}
	}
}

static void Controls_DrawPlayer(void *self)
{
	menubitmap_s	*b;
	char			buf[MAX_QPATH];

	trap_Cvar_VariableStringBuffer("model", buf, sizeof(buf));
	if (strcmp(buf, s_controls.playerModel) != 0) {
		UI_PlayerInfo_SetModel(&s_controls.playerinfo, buf);
		strcpy(s_controls.playerModel, buf);
		Controls_UpdateModel(ANIM_IDLE);
	}

	b = (menubitmap_s*) self;
	UI_DrawPlayer(b->generic.x, b->generic.y, b->width, b->height, &s_controls.playerinfo, uis.realtime/2);
}

static void Controls_GetKeyAssignment(char *command, int *key)
{
	int		j;
	char	b[256];

	for (j = 0; j < 256; j++) {
		trap_Key_GetBindingBuf(j, b, 256);
		if (*b == 0) {
			continue;
		}
		if (!Q_stricmp(b, command)) {
			*key = j;
		}
	}
}

static void Controls_GetConfig(void)
{
	int		i;
	bind_t	*bindptr;

	// put the bindings into a local store
	bindptr = g_bindings;

	// iterate each command, get its numeric binding
	for (i = 0; ; i++, bindptr++) {
		if (!bindptr->label) {
			break;
		}

		Controls_GetKeyAssignment(bindptr->command, &bindptr->bind);
	}

	s_controls.autoswitch.curvalue = Controls_GetCvarValue("cg_autoswitch");
	s_controls.zoomscaling.curvalue = Controls_GetCvarValue("cg_zoomScaling");
	Com_sprintf(s_controls.sensitivity.field.buffer, 6, "%g", Controls_GetCvarValue("sensitivity"));
	Com_sprintf(s_controls.fov.field.buffer, 6, "%g", Controls_GetCvarValue("cg_fov"));
	Com_sprintf(s_controls.zoomfov.field.buffer, 6, "%g", Controls_GetCvarValue("cg_zoomFov"));
}

static void Controls_SetConfig(void)
{
	int		i;
	bind_t	*bindptr;

	// set the bindings from the local store
	bindptr = g_bindings;

	// iterate each command, get its numeric binding
	for (i = 0; ; i++, bindptr++) {
		if (!bindptr->label) {
			break;
		}

		if (bindptr->bind != -1) {
			trap_Key_SetBinding(bindptr->bind, bindptr->command);
		}
	}

	trap_Cvar_SetValue("cg_autoswitch", s_controls.autoswitch.curvalue);
	trap_Cvar_SetValue("cg_zoomScaling", s_controls.zoomscaling.curvalue);
	trap_Cvar_SetValue("sensitivity", atof(s_controls.sensitivity.field.buffer));
	trap_Cvar_SetValue("cg_fov", atof(s_controls.fov.field.buffer));
	trap_Cvar_SetValue("cg_zoomFov", atof(s_controls.zoomfov.field.buffer));
	trap_Cmd_ExecuteText(EXEC_APPEND, "in_restart\n");
}

static void Controls_SetDefaults(void)
{
	int		i;
	bind_t	*bindptr;

	// set the bindings from the local store
	bindptr = g_bindings;

	// iterate each command, set its default binding
	for (i = 0; ; i++, bindptr++) {
		if (!bindptr->label) {
			break;
		}

		bindptr->bind = bindptr->defaultbind;
	}

	s_controls.autoswitch.curvalue = Controls_GetCvarDefault("cg_autoswitch");
	s_controls.zoomscaling.curvalue = Controls_GetCvarDefault("cg_zoomScaling");
	Com_sprintf(s_controls.sensitivity.field.buffer, 6, "%g", Controls_GetCvarDefault("sensitivity"));
	Com_sprintf(s_controls.fov.field.buffer, 6, "%g", Controls_GetCvarDefault("cg_fov"));
	Com_sprintf(s_controls.zoomfov.field.buffer, 6, "%g", Controls_GetCvarDefault("cg_zoomFov"));
}

static sfxHandle_t Controls_MenuKey(int key)
{
	int			id;
	int			i;
	qboolean	found;
	bind_t		*bindptr;
	found = qfalse;

	if (!s_controls.waitingforkey) {
		switch (key) {
		case K_BACKSPACE:
		case K_DEL:
		case K_KP_DEL:
			key = -1;
			break;

		case K_MOUSE2:
		case K_ESCAPE:
			if (s_controls.changesmade)
				Controls_SetConfig();
			goto ignorekey;

		default:
			goto ignorekey;
		}
	} else {
		if (key & K_CHAR_FLAG) {
			goto ignorekey;
		}

		switch (key) {
		case K_ESCAPE:
			s_controls.waitingforkey = qfalse;
			Controls_Update();
			return (menu_out_sound);
		case '`':
			goto ignorekey;
		}
	}

	s_controls.changesmade = qtrue;

	if (key != -1) {
		// remove from any other bind
		bindptr = g_bindings;
		for (i = 0; ; i++, bindptr++) {
			if (!bindptr->label) {
				break;
			}

			if (bindptr->bind == key) {
				bindptr->bind = -1;
			}
		}
	}

	// assign key to local store
	id = ((menucommon_s*)(s_controls.menu.items[s_controls.menu.cursor]))->id;
	bindptr = g_bindings;
	for (i = 0; ; i++, bindptr++) {
		if (!bindptr->label) {
			break;
		}

		if (bindptr->id == id) {
			found = qtrue;
			trap_Key_SetBinding(bindptr->bind, "");
			bindptr->bind = key;
			break;
		}
	}

	s_controls.waitingforkey = qfalse;

	if (found) {
		Controls_Update();
		return (menu_out_sound);
	}

ignorekey:
	return Menu_DefaultKey(&s_controls.menu, key);
}

static void Controls_ResetDefaults_Action(qboolean result)
{
	if (!result) {
		return;
	}

	s_controls.changesmade = qtrue;
	Controls_SetDefaults();
	Controls_Update();
}

static void Controls_ResetDefaults_Draw(void)
{
	SCR_DrawPropString(SCREEN_WIDTH / 2, 356 + PROP_HEIGHT * 0, "WARNING: This will reset all",
		FONT_CENTER | FONT_SMALL, colorYellow);
	SCR_DrawPropString(SCREEN_WIDTH / 2, 356 + PROP_HEIGHT * 1, "controls to their default values.",
		FONT_CENTER | FONT_SMALL, colorYellow);
}

static void Controls_MenuEvent(void* ptr, int event)
{
	switch (((menucommon_s*)ptr)->id) {
	case ID_MOVEMENT:
		if (event == QM_ACTIVATED) {
			s_controls.section = C_MOVEMENT;
			Controls_Update();
		}
		break;
	case ID_LOOKING:
		if (event == QM_ACTIVATED) {
			s_controls.section = C_LOOKING;
			Controls_Update();
		}
		break;
	case ID_BINDS:
		if (event == QM_ACTIVATED) {
			s_controls.section = C_BINDS;
			Controls_Update();
		}
		break;
	case ID_MISC:
		if (event == QM_ACTIVATED) {
			s_controls.section = C_MISC;
			Controls_Update();
		}
		break;
	case ID_DEFAULTS:
		if (event == QM_ACTIVATED) {
			UI_ConfirmMenu("SET TO DEFAULTS?", Controls_ResetDefaults_Draw, Controls_ResetDefaults_Action);
		}
		break;
	case ID_BACK:
		if (event == QM_ACTIVATED) {
			if (s_controls.changesmade)
				Controls_SetConfig();
			UI_PopMenu();
		}
		break;
	case ID_SAVEANDEXIT:
		if (event == QM_ACTIVATED) {
			Controls_SetConfig();
			UI_PopMenu();
		}
		break;
	case ID_EXIT:
		if (event == QM_ACTIVATED) {
			UI_PopMenu();
		}
		break;
	case ID_MOUSESPEED:
	case ID_FOV:
	case ID_ZOOMFOV:
	case ID_ZOOMSCALING:
	case ID_AUTOSWITCH:
		if (event == QM_ACTIVATED) {
			s_controls.changesmade = qtrue;
		}
		break;
	}
}

static void Controls_ActionEvent(void *ptr, int event)
{
	if (event == QM_LOSTFOCUS) {
		Controls_UpdateModel(ANIM_IDLE);
	} else if (event == QM_GOTFOCUS) {
		Controls_UpdateModel(g_bindings[((menucommon_s*)ptr)->id].anim);
	} else if ((event == QM_ACTIVATED) && !s_controls.waitingforkey) {
		s_controls.waitingforkey = 1;
		Controls_Update();
	}
}

static void Controls_InitModel(void)
{
	memset(&s_controls.playerinfo, 0, sizeof(playerInfo_t));

	UI_PlayerInfo_SetModel(&s_controls.playerinfo, UI_Cvar_VariableString("model"));

	Controls_UpdateModel(ANIM_IDLE);
}

static void Controls_InitWeapons(void)
{
	gitem_t	*item;

	for (item = bg_itemlist + 1; item->classname; item++) {
		if (item->giType != IT_WEAPON) {
			continue;
		}
		trap_R_RegisterModel(item->world_model[0]);
	}
}

static void Controls_MenuInit(void)
{
	// zero set all our globals
	memset(&s_controls, 0 ,sizeof(controls_t));

	Controls_Cache();

	s_controls.menu.key				= Controls_MenuKey;
	s_controls.menu.wrapAround		= qtrue;
	s_controls.menu.fullscreen		= qtrue;

	s_controls.banner.generic.type	= MTYPE_BTEXT;
	s_controls.banner.generic.flags	= QMF_CENTER_JUSTIFY;
	s_controls.banner.generic.x		= 320;
	s_controls.banner.generic.y		= 16;
	s_controls.banner.string		= "CONTROLS";
	s_controls.banner.color			= colorBanner;
	s_controls.banner.style			= FONT_CENTER | FONT_SHADOW;

	s_controls.looking.generic.type		= MTYPE_PTEXT;
	s_controls.looking.generic.flags	= QMF_RIGHT_JUSTIFY|QMF_PULSEIFFOCUS;
	s_controls.looking.generic.id		= ID_LOOKING;
	s_controls.looking.generic.callback	= Controls_MenuEvent;
	s_controls.looking.generic.x		= 152;
	s_controls.looking.generic.y		= 240 - 2 * PROP_HEIGHT;
	s_controls.looking.string			= "LOOK";
	s_controls.looking.style			= FONT_RIGHT;
	s_controls.looking.color			= colorRed;

	s_controls.movement.generic.type		= MTYPE_PTEXT;
	s_controls.movement.generic.flags		= QMF_RIGHT_JUSTIFY|QMF_PULSEIFFOCUS;
	s_controls.movement.generic.id			= ID_MOVEMENT;
	s_controls.movement.generic.callback	= Controls_MenuEvent;
	s_controls.movement.generic.x			= 152;
	s_controls.movement.generic.y			= 240 - PROP_HEIGHT;
	s_controls.movement.string				= "MOVE";
	s_controls.movement.style				= FONT_RIGHT;
	s_controls.movement.color				= colorRed;

	s_controls.weapons.generic.type		= MTYPE_PTEXT;
	s_controls.weapons.generic.flags	= QMF_RIGHT_JUSTIFY|QMF_PULSEIFFOCUS;
	s_controls.weapons.generic.id		= ID_BINDS;
	s_controls.weapons.generic.callback	= Controls_MenuEvent;
	s_controls.weapons.generic.x		= 152;
	s_controls.weapons.generic.y		= 240;
	s_controls.weapons.string			= "SHOOT";
	s_controls.weapons.style			= FONT_RIGHT;
	s_controls.weapons.color			= colorRed;

	s_controls.misc.generic.type		= MTYPE_PTEXT;
	s_controls.misc.generic.flags		= QMF_RIGHT_JUSTIFY|QMF_PULSEIFFOCUS;
	s_controls.misc.generic.id			= ID_MISC;
	s_controls.misc.generic.callback	= Controls_MenuEvent;
	s_controls.misc.generic.x			= 152;
	s_controls.misc.generic.y			= 240 + PROP_HEIGHT;
	s_controls.misc.string				= "MISC";
	s_controls.misc.style				= FONT_RIGHT;
	s_controls.misc.color				= colorRed;

	s_controls.back.generic.type		= MTYPE_BUTTON;
	s_controls.back.generic.flags		= QMF_LEFT_JUSTIFY;
	s_controls.back.generic.x			= 0;
	s_controls.back.generic.y			= 480-64;
	s_controls.back.generic.id			= ID_BACK;
	s_controls.back.generic.callback	= Controls_MenuEvent;
	s_controls.back.width				= 128;
	s_controls.back.height				= 64;
	s_controls.back.string				= "Back";

	s_controls.player.generic.type		= MTYPE_BITMAP;
	s_controls.player.generic.flags		= QMF_INACTIVE;
	s_controls.player.generic.ownerdraw	= Controls_DrawPlayer;
	s_controls.player.generic.x			= 400;
	s_controls.player.generic.y			= -40;
	s_controls.player.width				= 32*10;
	s_controls.player.height			= 56*10;

	s_controls.walkforward.generic.type			= MTYPE_ACTION;
	s_controls.walkforward.generic.flags		= QMF_LEFT_JUSTIFY|QMF_PULSEIFFOCUS|QMF_GRAYED|QMF_HIDDEN;
	s_controls.walkforward.generic.callback		= Controls_ActionEvent;
	s_controls.walkforward.generic.ownerdraw	= Controls_DrawKeyBinding;
	s_controls.walkforward.generic.id			= ID_FORWARD;

	s_controls.backpedal.generic.type		= MTYPE_ACTION;
	s_controls.backpedal.generic.flags		= QMF_LEFT_JUSTIFY|QMF_PULSEIFFOCUS|QMF_GRAYED|QMF_HIDDEN;
	s_controls.backpedal.generic.callback	= Controls_ActionEvent;
	s_controls.backpedal.generic.ownerdraw	= Controls_DrawKeyBinding;
	s_controls.backpedal.generic.id			= ID_BACKPEDAL;

	s_controls.stepleft.generic.type		= MTYPE_ACTION;
	s_controls.stepleft.generic.flags		= QMF_LEFT_JUSTIFY|QMF_PULSEIFFOCUS|QMF_GRAYED|QMF_HIDDEN;
	s_controls.stepleft.generic.callback	= Controls_ActionEvent;
	s_controls.stepleft.generic.ownerdraw	= Controls_DrawKeyBinding;
	s_controls.stepleft.generic.id			= ID_MOVELEFT;

	s_controls.stepright.generic.type		= MTYPE_ACTION;
	s_controls.stepright.generic.flags		= QMF_LEFT_JUSTIFY|QMF_PULSEIFFOCUS|QMF_GRAYED|QMF_HIDDEN;
	s_controls.stepright.generic.callback	= Controls_ActionEvent;
	s_controls.stepright.generic.ownerdraw	= Controls_DrawKeyBinding;
	s_controls.stepright.generic.id			= ID_MOVERIGHT;

	s_controls.moveup.generic.type		= MTYPE_ACTION;
	s_controls.moveup.generic.flags		= QMF_LEFT_JUSTIFY|QMF_PULSEIFFOCUS|QMF_GRAYED|QMF_HIDDEN;
	s_controls.moveup.generic.callback	= Controls_ActionEvent;
	s_controls.moveup.generic.ownerdraw	= Controls_DrawKeyBinding;
	s_controls.moveup.generic.id		= ID_MOVEUP;

	s_controls.movedown.generic.type		= MTYPE_ACTION;
	s_controls.movedown.generic.flags		= QMF_LEFT_JUSTIFY|QMF_PULSEIFFOCUS|QMF_GRAYED|QMF_HIDDEN;
	s_controls.movedown.generic.callback	= Controls_ActionEvent;
	s_controls.movedown.generic.ownerdraw	= Controls_DrawKeyBinding;
	s_controls.movedown.generic.id			= ID_MOVEDOWN;

	s_controls.run.generic.type			= MTYPE_ACTION;
	s_controls.run.generic.flags		= QMF_LEFT_JUSTIFY|QMF_PULSEIFFOCUS|QMF_GRAYED|QMF_HIDDEN;
	s_controls.run.generic.callback		= Controls_ActionEvent;
	s_controls.run.generic.ownerdraw	= Controls_DrawKeyBinding;
	s_controls.run.generic.id			= ID_SPEED;

	s_controls.chainsaw.generic.type		= MTYPE_ACTION;
	s_controls.chainsaw.generic.flags		= QMF_LEFT_JUSTIFY|QMF_PULSEIFFOCUS|QMF_GRAYED|QMF_HIDDEN;
	s_controls.chainsaw.generic.callback	= Controls_ActionEvent;
	s_controls.chainsaw.generic.ownerdraw	= Controls_DrawKeyBinding;
	s_controls.chainsaw.generic.id			= ID_WEAPON1;

	s_controls.machinegun.generic.type		= MTYPE_ACTION;
	s_controls.machinegun.generic.flags		= QMF_LEFT_JUSTIFY|QMF_PULSEIFFOCUS|QMF_GRAYED|QMF_HIDDEN;
	s_controls.machinegun.generic.callback	= Controls_ActionEvent;
	s_controls.machinegun.generic.ownerdraw	= Controls_DrawKeyBinding;
	s_controls.machinegun.generic.id		= ID_WEAPON2;

	s_controls.shotgun.generic.type			= MTYPE_ACTION;
	s_controls.shotgun.generic.flags		= QMF_LEFT_JUSTIFY|QMF_PULSEIFFOCUS|QMF_GRAYED|QMF_HIDDEN;
	s_controls.shotgun.generic.callback		= Controls_ActionEvent;
	s_controls.shotgun.generic.ownerdraw	= Controls_DrawKeyBinding;
	s_controls.shotgun.generic.id			= ID_WEAPON3;

	s_controls.grenadelauncher.generic.type			= MTYPE_ACTION;
	s_controls.grenadelauncher.generic.flags		= QMF_LEFT_JUSTIFY|QMF_PULSEIFFOCUS|QMF_GRAYED|QMF_HIDDEN;
	s_controls.grenadelauncher.generic.callback		= Controls_ActionEvent;
	s_controls.grenadelauncher.generic.ownerdraw	= Controls_DrawKeyBinding;
	s_controls.grenadelauncher.generic.id			= ID_WEAPON4;

	s_controls.rocketlauncher.generic.type		= MTYPE_ACTION;
	s_controls.rocketlauncher.generic.flags		= QMF_LEFT_JUSTIFY|QMF_PULSEIFFOCUS|QMF_GRAYED|QMF_HIDDEN;
	s_controls.rocketlauncher.generic.callback	= Controls_ActionEvent;
	s_controls.rocketlauncher.generic.ownerdraw	= Controls_DrawKeyBinding;
	s_controls.rocketlauncher.generic.id		= ID_WEAPON5;

	s_controls.lightning.generic.type		= MTYPE_ACTION;
	s_controls.lightning.generic.flags		= QMF_LEFT_JUSTIFY|QMF_PULSEIFFOCUS|QMF_GRAYED|QMF_HIDDEN;
	s_controls.lightning.generic.callback	= Controls_ActionEvent;
	s_controls.lightning.generic.ownerdraw	= Controls_DrawKeyBinding;
	s_controls.lightning.generic.id			= ID_WEAPON6;

	s_controls.railgun.generic.type			= MTYPE_ACTION;
	s_controls.railgun.generic.flags		= QMF_LEFT_JUSTIFY|QMF_PULSEIFFOCUS|QMF_GRAYED|QMF_HIDDEN;
	s_controls.railgun.generic.callback		= Controls_ActionEvent;
	s_controls.railgun.generic.ownerdraw	= Controls_DrawKeyBinding;
	s_controls.railgun.generic.id			= ID_WEAPON7;

	s_controls.plasma.generic.type		= MTYPE_ACTION;
	s_controls.plasma.generic.flags		= QMF_LEFT_JUSTIFY|QMF_PULSEIFFOCUS|QMF_GRAYED|QMF_HIDDEN;
	s_controls.plasma.generic.callback	= Controls_ActionEvent;
	s_controls.plasma.generic.ownerdraw	= Controls_DrawKeyBinding;
	s_controls.plasma.generic.id		= ID_WEAPON8;

	s_controls.bfg.generic.type			= MTYPE_ACTION;
	s_controls.bfg.generic.flags		= QMF_LEFT_JUSTIFY|QMF_PULSEIFFOCUS|QMF_GRAYED|QMF_HIDDEN;
	s_controls.bfg.generic.callback		= Controls_ActionEvent;
	s_controls.bfg.generic.ownerdraw	= Controls_DrawKeyBinding;
	s_controls.bfg.generic.id			= ID_WEAPON9;

	s_controls.attack.generic.type		= MTYPE_ACTION;
	s_controls.attack.generic.flags		= QMF_LEFT_JUSTIFY|QMF_PULSEIFFOCUS|QMF_GRAYED|QMF_HIDDEN;
	s_controls.attack.generic.callback	= Controls_ActionEvent;
	s_controls.attack.generic.ownerdraw	= Controls_DrawKeyBinding;
	s_controls.attack.generic.id		= ID_ATTACK;

	s_controls.prevweapon.generic.type		= MTYPE_ACTION;
	s_controls.prevweapon.generic.flags		= QMF_LEFT_JUSTIFY|QMF_PULSEIFFOCUS|QMF_GRAYED|QMF_HIDDEN;
	s_controls.prevweapon.generic.callback	= Controls_ActionEvent;
	s_controls.prevweapon.generic.ownerdraw	= Controls_DrawKeyBinding;
	s_controls.prevweapon.generic.id		= ID_WEAPPREV;

	s_controls.nextweapon.generic.type		= MTYPE_ACTION;
	s_controls.nextweapon.generic.flags		= QMF_LEFT_JUSTIFY|QMF_PULSEIFFOCUS|QMF_GRAYED|QMF_HIDDEN;
	s_controls.nextweapon.generic.callback	= Controls_ActionEvent;
	s_controls.nextweapon.generic.ownerdraw	= Controls_DrawKeyBinding;
	s_controls.nextweapon.generic.id		= ID_WEAPNEXT;

	s_controls.zoomview.generic.type		= MTYPE_ACTION;
	s_controls.zoomview.generic.flags		= QMF_LEFT_JUSTIFY|QMF_PULSEIFFOCUS|QMF_GRAYED|QMF_HIDDEN;
	s_controls.zoomview.generic.callback	= Controls_ActionEvent;
	s_controls.zoomview.generic.ownerdraw	= Controls_DrawKeyBinding;
	s_controls.zoomview.generic.id			= ID_ZOOMVIEW;

	s_controls.useitem.generic.type			= MTYPE_ACTION;
	s_controls.useitem.generic.flags		= QMF_LEFT_JUSTIFY|QMF_PULSEIFFOCUS|QMF_GRAYED|QMF_HIDDEN;
	s_controls.useitem.generic.callback		= Controls_ActionEvent;
	s_controls.useitem.generic.ownerdraw	= Controls_DrawKeyBinding;
	s_controls.useitem.generic.id			= ID_USEITEM;

	s_controls.showscores.generic.type		= MTYPE_ACTION;
	s_controls.showscores.generic.flags		= QMF_LEFT_JUSTIFY|QMF_PULSEIFFOCUS|QMF_GRAYED|QMF_HIDDEN;
	s_controls.showscores.generic.callback	= Controls_ActionEvent;
	s_controls.showscores.generic.ownerdraw	= Controls_DrawKeyBinding;
	s_controls.showscores.generic.id		= ID_SHOWSCORES;

	s_controls.autoswitch.generic.type		= MTYPE_RADIOBUTTON;
	s_controls.autoswitch.generic.flags		= QMF_SMALLFONT;
	s_controls.autoswitch.generic.x			= SCREEN_WIDTH/2;
	s_controls.autoswitch.generic.name		= "Autoswitch Weapons";
	s_controls.autoswitch.generic.id		= ID_AUTOSWITCH;
	s_controls.autoswitch.generic.callback	= Controls_MenuEvent;

	s_controls.sensitivity.generic.type			= MTYPE_FIELD;
	s_controls.sensitivity.generic.x			= SCREEN_WIDTH / 2;
	s_controls.sensitivity.generic.flags		= QMF_NUMBERSONLY | QMF_SMALLFONT;
	s_controls.sensitivity.generic.name			= "Sensitivity";
	s_controls.sensitivity.generic.id			= ID_MOUSESPEED;
	s_controls.sensitivity.generic.callback		= Controls_MenuEvent;
	s_controls.sensitivity.field.widthInChars	= 6;
	s_controls.sensitivity.field.maxchars		= 6;

	s_controls.fov.generic.type			= MTYPE_FIELD;
	s_controls.fov.generic.x			= SCREEN_WIDTH / 2;
	s_controls.fov.generic.flags		= QMF_NUMBERSONLY | QMF_SMALLFONT;
	s_controls.fov.generic.name			= "Field of View";
	s_controls.fov.generic.id			= ID_FOV;
	s_controls.fov.generic.callback		= Controls_MenuEvent;
	s_controls.fov.field.widthInChars	= 6;
	s_controls.fov.field.maxchars		= 6;

	s_controls.zoomfov.generic.type			= MTYPE_FIELD;
	s_controls.zoomfov.generic.x			= SCREEN_WIDTH / 2;
	s_controls.zoomfov.generic.flags		= QMF_NUMBERSONLY | QMF_SMALLFONT;
	s_controls.zoomfov.generic.name			= "Zoom FOV";
	s_controls.zoomfov.generic.id			= ID_ZOOMFOV;
	s_controls.zoomfov.generic.callback		= Controls_MenuEvent;
	s_controls.zoomfov.field.widthInChars	= 6;
	s_controls.zoomfov.field.maxchars		= 6;

	s_controls.zoomscaling.generic.type			= MTYPE_RADIOBUTTON;
	s_controls.zoomscaling.generic.flags		= QMF_SMALLFONT;
	s_controls.zoomscaling.generic.x			= SCREEN_WIDTH / 2;
	s_controls.zoomscaling.generic.name			= "Zoom Scaling";
	s_controls.zoomscaling.generic.id			= ID_ZOOMSCALING;
	s_controls.zoomscaling.generic.callback		= Controls_MenuEvent;

	s_controls.gesture.generic.type			= MTYPE_ACTION;
	s_controls.gesture.generic.flags		= QMF_LEFT_JUSTIFY|QMF_PULSEIFFOCUS|QMF_GRAYED|QMF_HIDDEN;
	s_controls.gesture.generic.callback		= Controls_ActionEvent;
	s_controls.gesture.generic.ownerdraw	= Controls_DrawKeyBinding;
	s_controls.gesture.generic.id			= ID_GESTURE;

	s_controls.chat.generic.type		= MTYPE_ACTION;
	s_controls.chat.generic.flags		= QMF_LEFT_JUSTIFY|QMF_PULSEIFFOCUS|QMF_GRAYED|QMF_HIDDEN;
	s_controls.chat.generic.callback	= Controls_ActionEvent;
	s_controls.chat.generic.ownerdraw	= Controls_DrawKeyBinding;
	s_controls.chat.generic.id			= ID_CHAT;

	s_controls.chat2.generic.type		= MTYPE_ACTION;
	s_controls.chat2.generic.flags		= QMF_LEFT_JUSTIFY|QMF_PULSEIFFOCUS|QMF_GRAYED|QMF_HIDDEN;
	s_controls.chat2.generic.callback	= Controls_ActionEvent;
	s_controls.chat2.generic.ownerdraw	= Controls_DrawKeyBinding;
	s_controls.chat2.generic.id			= ID_CHAT2;

	s_controls.chat3.generic.type		= MTYPE_ACTION;
	s_controls.chat3.generic.flags		= QMF_LEFT_JUSTIFY|QMF_PULSEIFFOCUS|QMF_GRAYED|QMF_HIDDEN;
	s_controls.chat3.generic.callback	= Controls_ActionEvent;
	s_controls.chat3.generic.ownerdraw	= Controls_DrawKeyBinding;
	s_controls.chat3.generic.id			= ID_CHAT3;

	s_controls.chat4.generic.type		= MTYPE_ACTION;
	s_controls.chat4.generic.flags		= QMF_LEFT_JUSTIFY|QMF_PULSEIFFOCUS|QMF_GRAYED|QMF_HIDDEN;
	s_controls.chat4.generic.callback	= Controls_ActionEvent;
	s_controls.chat4.generic.ownerdraw	= Controls_DrawKeyBinding;
	s_controls.chat4.generic.id			= ID_CHAT4;

	s_controls.togglemenu.generic.type		= MTYPE_ACTION;
	s_controls.togglemenu.generic.flags		= QMF_LEFT_JUSTIFY|QMF_PULSEIFFOCUS|QMF_GRAYED|QMF_HIDDEN;
	s_controls.togglemenu.generic.callback	= Controls_ActionEvent;
	s_controls.togglemenu.generic.ownerdraw	= Controls_DrawKeyBinding;
	s_controls.togglemenu.generic.id		= ID_TOGGLEMENU;

	Menu_AddItem(&s_controls.menu, &s_controls.banner);
	Menu_AddItem(&s_controls.menu, &s_controls.player);

	Menu_AddItem(&s_controls.menu, &s_controls.looking);
	Menu_AddItem(&s_controls.menu, &s_controls.movement);
	Menu_AddItem(&s_controls.menu, &s_controls.weapons);
	Menu_AddItem(&s_controls.menu, &s_controls.misc);

	Menu_AddItem(&s_controls.menu, &s_controls.sensitivity);
	Menu_AddItem(&s_controls.menu, &s_controls.fov);
	Menu_AddItem(&s_controls.menu, &s_controls.zoomfov);
	Menu_AddItem(&s_controls.menu, &s_controls.zoomscaling);
	Menu_AddItem(&s_controls.menu, &s_controls.zoomview);

	Menu_AddItem(&s_controls.menu, &s_controls.run);
	Menu_AddItem(&s_controls.menu, &s_controls.walkforward);
	Menu_AddItem(&s_controls.menu, &s_controls.backpedal);
	Menu_AddItem(&s_controls.menu, &s_controls.stepleft);
	Menu_AddItem(&s_controls.menu, &s_controls.stepright);
	Menu_AddItem(&s_controls.menu, &s_controls.moveup);
	Menu_AddItem(&s_controls.menu, &s_controls.movedown);

	Menu_AddItem(&s_controls.menu, &s_controls.attack);
	Menu_AddItem(&s_controls.menu, &s_controls.nextweapon);
	Menu_AddItem(&s_controls.menu, &s_controls.prevweapon);
	Menu_AddItem(&s_controls.menu, &s_controls.autoswitch);
	Menu_AddItem(&s_controls.menu, &s_controls.chainsaw);
	Menu_AddItem(&s_controls.menu, &s_controls.machinegun);
	Menu_AddItem(&s_controls.menu, &s_controls.shotgun);
	Menu_AddItem(&s_controls.menu, &s_controls.grenadelauncher);
	Menu_AddItem(&s_controls.menu, &s_controls.rocketlauncher);
	Menu_AddItem(&s_controls.menu, &s_controls.lightning);
	Menu_AddItem(&s_controls.menu, &s_controls.railgun);
	Menu_AddItem(&s_controls.menu, &s_controls.plasma);
	Menu_AddItem(&s_controls.menu, &s_controls.bfg);

	Menu_AddItem(&s_controls.menu, &s_controls.showscores);
	Menu_AddItem(&s_controls.menu, &s_controls.useitem);
	Menu_AddItem(&s_controls.menu, &s_controls.gesture);
	Menu_AddItem(&s_controls.menu, &s_controls.chat);
	Menu_AddItem(&s_controls.menu, &s_controls.chat2);
	Menu_AddItem(&s_controls.menu, &s_controls.chat3);
	Menu_AddItem(&s_controls.menu, &s_controls.chat4);
	Menu_AddItem(&s_controls.menu, &s_controls.togglemenu);

	Menu_AddItem(&s_controls.menu, &s_controls.back);

	// initialize the configurable cvars
	Controls_InitCvars();

	// initialize the current config
	Controls_GetConfig();

	// intialize the model
	Controls_InitModel();

	// intialize the weapons
	Controls_InitWeapons ();

	// initial default section
	s_controls.section = C_LOOKING;

	// update the ui
	Controls_Update();
}

void Controls_Cache(void) { }

void UI_ControlsMenu(void)
{
	Controls_MenuInit();
	UI_PushMenu(&s_controls.menu);
}


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
#include "ui_local.h"

#define MODEL_BACK0			"menu/art/back_0"
#define MODEL_BACK1			"menu/art/back_1"
#define MODEL_SELECT		"menu/art/opponents_select"
#define MODEL_SELECTED		"menu/art/opponents_selected"
#define MODEL_PORTS			"menu/art/player_models_ports"
#define MODEL_ARROWS		"menu/art/gs_arrows_0"
#define MODEL_ARROWSL		"menu/art/gs_arrows_l"
#define MODEL_ARROWSR		"menu/art/gs_arrows_r"

#define LOW_MEMORY			(5 * 1024 * 1024)

static char* playermodel_artlist[] = {
	MODEL_BACK0,
	MODEL_BACK1,
	MODEL_SELECT,
	MODEL_SELECTED,
	MODEL_PORTS,
	MODEL_ARROWS,
	MODEL_ARROWSL,
	MODEL_ARROWSR,
	NULL
};

#define PLAYERGRID_COLS		4
#define PLAYERGRID_ROWS		4
#define MAX_MODELSPERPAGE	(PLAYERGRID_ROWS*PLAYERGRID_COLS)

#define MAX_PLAYERMODELS	256

enum {
	ID_PLAYERPIC0,
	ID_PLAYERPIC1,
	ID_PLAYERPIC2,
	ID_PLAYERPIC3,
	ID_PLAYERPIC4,
	ID_PLAYERPIC5,
	ID_PLAYERPIC6,
	ID_PLAYERPIC7,
	ID_PLAYERPIC8,
	ID_PLAYERPIC9,
	ID_PLAYERPIC10,
	ID_PLAYERPIC11,
	ID_PLAYERPIC12,
	ID_PLAYERPIC13,
	ID_PLAYERPIC14,
	ID_PLAYERPIC15
};

enum {
	ID_PREVPAGE = 100,
	ID_NEXTPAGE,
	ID_BACK
};

typedef struct {
	menuframework_s	menu;
	menubitmap_s	pics[MAX_MODELSPERPAGE];
	menubitmap_s	picbuttons[MAX_MODELSPERPAGE];
	menubitmap_s	ports;
	menutext_s		banner;
	menubitmap_s	back;
	menubitmap_s	player;
	menubitmap_s	arrows;
	menubitmap_s	left;
	menubitmap_s	right;
	menutext_s		modelname;
	menutext_s		skinname;
	menutext_s		playername;
	playerInfo_t	playerinfo;
	int				nummodels;
	char			modelnames[MAX_PLAYERMODELS][128];
	int				modelpage;
	int				numpages;
	char			modelskin[64];
	int				selectedmodel;
} playermodel_t;

static playermodel_t s_playermodel;

static void PlayerModel_UpdateGrid(void)
{
	int	i;
    int	j;

	j = s_playermodel.modelpage * MAX_MODELSPERPAGE;
	for (i = 0; i<PLAYERGRID_ROWS*PLAYERGRID_COLS; i++,j++)
	{
		if (j < s_playermodel.nummodels)
		{ 
			// model/skin portrait
 			s_playermodel.pics[i].generic.name         = s_playermodel.modelnames[j];
			s_playermodel.picbuttons[i].generic.flags &= ~QMF_INACTIVE;
		}
		else
		{
			// dead slot
 			s_playermodel.pics[i].generic.name         = NULL;
			s_playermodel.picbuttons[i].generic.flags |= QMF_INACTIVE;
		}

 		s_playermodel.pics[i].generic.flags       &= ~QMF_HIGHLIGHT;
 		s_playermodel.pics[i].shader               = 0;
 		s_playermodel.picbuttons[i].generic.flags |= QMF_PULSEIFFOCUS;
	}

	if (s_playermodel.selectedmodel/MAX_MODELSPERPAGE == s_playermodel.modelpage)
	{
		// set selected model
		i = s_playermodel.selectedmodel % MAX_MODELSPERPAGE;

		s_playermodel.pics[i].generic.flags       |= QMF_HIGHLIGHT;
		s_playermodel.picbuttons[i].generic.flags &= ~QMF_PULSEIFFOCUS;
	}

	if (s_playermodel.numpages > 1)
	{
		if (s_playermodel.modelpage > 0)
			s_playermodel.left.generic.flags &= ~QMF_INACTIVE;
		else
			s_playermodel.left.generic.flags |= QMF_INACTIVE;

		if (s_playermodel.modelpage < s_playermodel.numpages-1)
			s_playermodel.right.generic.flags &= ~QMF_INACTIVE;
		else
			s_playermodel.right.generic.flags |= QMF_INACTIVE;
	}
	else
	{
		// hide left/right markers
		s_playermodel.left.generic.flags |= QMF_INACTIVE;
		s_playermodel.right.generic.flags |= QMF_INACTIVE;
	}
}

static void PlayerModel_UpdateModel(void)
{
	vec3_t	viewangles;
	vec3_t	moveangles;

	memset(&s_playermodel.playerinfo, 0, sizeof(playerInfo_t));

	viewangles[YAW]   = 180 - 30;
	viewangles[PITCH] = 0;
	viewangles[ROLL]  = 0;
	VectorClear(moveangles);

	UI_PlayerInfo_SetModel(&s_playermodel.playerinfo, s_playermodel.modelskin);
	UI_PlayerInfo_SetInfo(&s_playermodel.playerinfo, LEGS_IDLE, TORSO_STAND, viewangles, moveangles, WP_MACHINEGUN, qfalse);
}

static void PlayerModel_SaveChanges(void)
{
	trap_Cvar_Set("model", s_playermodel.modelskin);
	trap_Cvar_Set("headmodel", s_playermodel.modelskin);
	trap_Cvar_Set("team_model", s_playermodel.modelskin);
	trap_Cvar_Set("team_headmodel", s_playermodel.modelskin);
}

static void PlayerModel_MenuEvent(void* ptr, int event)
{
	if (event != QM_ACTIVATED)
		return;

	switch (((menucommon_s *)ptr)->id) {
		case ID_PREVPAGE:
			if (s_playermodel.modelpage > 0) {
				s_playermodel.modelpage--;
				PlayerModel_UpdateGrid();
			}
			break;

		case ID_NEXTPAGE:
			if (s_playermodel.modelpage < s_playermodel.numpages - 1) {
				s_playermodel.modelpage++;
				PlayerModel_UpdateGrid();
			}
			break;

		case ID_BACK:
			PlayerModel_SaveChanges();
			UI_PopMenu();
			break;
	}
}

static sfxHandle_t PlayerModel_MenuKey(int key)
{
	menucommon_s	*m;
	int				picnum;

	switch (key) {
		case K_KP_LEFTARROW:
		case K_LEFTARROW:
			m = Menu_ItemAtCursor(&s_playermodel.menu);
			picnum = m->id - ID_PLAYERPIC0;
			if (picnum >= 0 && picnum <= 15) {
				if (picnum > 0) {
					Menu_SetCursor(&s_playermodel.menu,s_playermodel.menu.cursor-1);
					return (menu_move_sound);
				} else if (s_playermodel.modelpage > 0) {
					s_playermodel.modelpage--;
					Menu_SetCursor(&s_playermodel.menu,s_playermodel.menu.cursor+15);
					PlayerModel_UpdateGrid();
					return (menu_move_sound);
				} else {
					return (menu_buzz_sound);
				}
			}
			break;

		case K_KP_RIGHTARROW:
		case K_RIGHTARROW:
			m = Menu_ItemAtCursor(&s_playermodel.menu);
			picnum = m->id - ID_PLAYERPIC0;
			if (picnum >= 0 && picnum <= 15) {
				if ((picnum < 15) && (s_playermodel.modelpage*MAX_MODELSPERPAGE + picnum+1 < s_playermodel.nummodels)) {
					Menu_SetCursor(&s_playermodel.menu,s_playermodel.menu.cursor+1);
					return (menu_move_sound);
				} else if ((picnum == 15) && (s_playermodel.modelpage < s_playermodel.numpages-1)) {
					s_playermodel.modelpage++;
					Menu_SetCursor(&s_playermodel.menu,s_playermodel.menu.cursor-15);
					PlayerModel_UpdateGrid();
					return menu_move_sound;
				} else {
					return menu_buzz_sound;
				}
			}
			break;

		case K_MOUSE2:
		case K_ESCAPE:
			PlayerModel_SaveChanges();
			break;
	}

	return Menu_DefaultKey(&s_playermodel.menu, key);
}

static void PlayerModel_PicEvent(void* ptr, int event)
{
	int				modelnum;
	int				maxlen;
	char*			buffptr;
	char*			pdest;
	int				i;

	if (event != QM_ACTIVATED)
		return;

	for (i = 0; i<PLAYERGRID_ROWS*PLAYERGRID_COLS; i++) {
		// reset
		s_playermodel.pics[i].generic.flags &= ~QMF_HIGHLIGHT;
		s_playermodel.picbuttons[i].generic.flags |= QMF_PULSEIFFOCUS;
	}

	// set selected
	i = ((menucommon_s*)ptr)->id - ID_PLAYERPIC0;
	s_playermodel.pics[i].generic.flags       |= QMF_HIGHLIGHT;
	s_playermodel.picbuttons[i].generic.flags &= ~QMF_PULSEIFFOCUS;

	// get model and strip icon_
	modelnum = s_playermodel.modelpage*MAX_MODELSPERPAGE + i;
	buffptr = s_playermodel.modelnames[modelnum] + strlen("models/players/");
	pdest = strstr(buffptr,"icon_");
	if (pdest) {
		// track the whole model/skin name
		Q_strncpyz(s_playermodel.modelskin,buffptr,pdest-buffptr+1);
		strcat(s_playermodel.modelskin,pdest + 5);

		// separate the model name
		maxlen = pdest-buffptr;
		if (maxlen > 16) {
			maxlen = 16;
		}
		Q_strncpyz(s_playermodel.modelname.string, buffptr, maxlen);
		Q_strupr(s_playermodel.modelname.string);

		// separate the skin name
		maxlen = strlen(pdest+5)+1;
		if (maxlen > 16) {
			maxlen = 16;
		}
		Q_strncpyz(s_playermodel.skinname.string, pdest+5, maxlen);
		Q_strupr(s_playermodel.skinname.string);

		s_playermodel.selectedmodel = modelnum;

		if (trap_MemoryRemaining() > LOW_MEMORY) {
			PlayerModel_UpdateModel();
		}
	}
}

static void PlayerModel_DrawPlayer(void *self)
{
	menubitmap_s*	b;

	b = (menubitmap_s*) self;

	if (trap_MemoryRemaining() <= LOW_MEMORY) {
		SCR_DrawPropString(b->generic.x, b->generic.y + b->height / 2, "LOW MEMORY", 0, color_red);
		return;
	}

	UI_DrawPlayer(b->generic.x, b->generic.y, b->width, b->height, &s_playermodel.playerinfo, uis.realtime/2);
}

static void PlayerModel_BuildList(void)
{
	int		numdirs;
	int		numfiles;
	char	dirlist[2048];
	char	filelist[2048];
	char	skinname[MAX_QPATH];
	char	*dirptr;
	char	*fileptr;
	int		i;
	int		j;
	int		dirlen;
	int		filelen;
	qboolean precache;

	precache = trap_Cvar_VariableValue("com_buildscript");

	s_playermodel.modelpage = 0;
	s_playermodel.nummodels = 0;

	// iterate directory of all player models
	numdirs = trap_FS_GetFileList("models/players", "/", dirlist, 2048);
	dirptr  = dirlist;
	for (i = 0; i < numdirs && s_playermodel.nummodels < MAX_PLAYERMODELS; i++, dirptr += dirlen + 1) {
		dirlen = strlen(dirptr);

		if (dirlen && dirptr[dirlen-1]=='/') {
			dirptr[dirlen - 1] = '\0';
		}

		if (!strcmp(dirptr,".") || !strcmp(dirptr,"..")) {
			continue;
		}

		// iterate all skin files in directory
		numfiles = trap_FS_GetFileList(va("models/players/%s",dirptr), "tga", filelist, 2048);
		fileptr = filelist;
		for (j = 0; j<numfiles && s_playermodel.nummodels < MAX_PLAYERMODELS; j++, fileptr += filelen + 1) {
			filelen = strlen(fileptr);

			COM_StripExtension(fileptr, skinname, sizeof skinname);

			// look for icon_????
			if (!Q_stricmpn(skinname,"icon_", 5)) {
				Com_sprintf(s_playermodel.modelnames[s_playermodel.nummodels++],
					sizeof(s_playermodel.modelnames[s_playermodel.nummodels]),
					"models/players/%s/%s", dirptr, skinname);
				//if (s_playermodel.nummodels >= MAX_PLAYERMODELS)
				//	return;
			}

			if (precache) {
				trap_S_RegisterSound(va("sound/player/announce/%s_wins.wav", skinname), qfalse);
			}
		}
	}

	//APSFIXME - Degenerate no models case

	s_playermodel.numpages = s_playermodel.nummodels/MAX_MODELSPERPAGE;
	if (s_playermodel.nummodels % MAX_MODELSPERPAGE) {
		s_playermodel.numpages++;
	}
}

static void PlayerModel_SetMenuItems(void)
{
	int		i;
	int		maxlen;
	char	modelskin[64];
	char	*buffptr;
	char	*pdest;

	// name
	trap_Cvar_VariableStringBuffer("name", s_playermodel.playername.string, 16);
	Q_CleanStr(s_playermodel.playername.string);

	// model
	trap_Cvar_VariableStringBuffer("model", s_playermodel.modelskin, 64);

	// use default skin if none is set
	if (!strchr(s_playermodel.modelskin, '/')) {
		Q_strcat(s_playermodel.modelskin, 64, "/default");
	}

	// find model in our list
	for (i = 0; i < s_playermodel.nummodels; i++) {
		// strip icon_
		buffptr = s_playermodel.modelnames[i] + strlen("models/players/");
		pdest = strstr(buffptr,"icon_");
		if (pdest) {
			Q_strncpyz(modelskin,buffptr,pdest - buffptr + 1);
			strcat(modelskin,pdest + 5);
		} else {
			continue;
		}

		if (!Q_stricmp(s_playermodel.modelskin, modelskin)) {
			// found pic, set selection here
			s_playermodel.selectedmodel = i;
			s_playermodel.modelpage     = i / MAX_MODELSPERPAGE;

			// separate the model name
			maxlen = pdest-buffptr;
			if (maxlen > 16) {
				maxlen = 16;
			}
			Q_strncpyz(s_playermodel.modelname.string, buffptr, maxlen);
			Q_strupr(s_playermodel.modelname.string);

			// separate the skin name
			maxlen = strlen(pdest + 5) + 1;
			if (maxlen > 16) {
				maxlen = 16;
			}
			Q_strncpyz(s_playermodel.skinname.string, pdest + 5, maxlen);
			Q_strupr(s_playermodel.skinname.string);
			break;
		}
	}
}

static void PlayerModel_MenuInit(void)
{
	int			i;
	int			j;
	int			k;
	int			x;
	int			y;
	static char	playername[32];
	static char	modelname[32];
	static char	skinname[32];

	// zero set all our globals
	memset(&s_playermodel, 0, sizeof (playermodel_t));

	PlayerModel_Cache();

	s_playermodel.menu.key			= PlayerModel_MenuKey;
	s_playermodel.menu.wrapAround	= qtrue;
	s_playermodel.menu.fullscreen	= qtrue;

	s_playermodel.banner.generic.type	= MTYPE_BTEXT;
	s_playermodel.banner.generic.x		= 320;
	s_playermodel.banner.generic.y		= 16;
	s_playermodel.banner.string			= "PLAYER MODEL";
	s_playermodel.banner.color			= color_white;
	s_playermodel.banner.style			= FONT_CENTER;

	s_playermodel.ports.generic.type	= MTYPE_BITMAP;
	s_playermodel.ports.generic.name	= MODEL_PORTS;
	s_playermodel.ports.generic.flags	= QMF_LEFT_JUSTIFY|QMF_INACTIVE;
	s_playermodel.ports.generic.x		= 50;
	s_playermodel.ports.generic.y		= 59;
	s_playermodel.ports.width			= 274;
	s_playermodel.ports.height			= 274;

	y = 59;
	for (i = 0, k = 0; i < PLAYERGRID_ROWS; i++) {
		x = 50;
		for (j = 0; j < PLAYERGRID_COLS; j++,k++) {
			s_playermodel.pics[k].generic.type	= MTYPE_BITMAP;
			s_playermodel.pics[k].generic.flags	= QMF_LEFT_JUSTIFY|QMF_INACTIVE;
			s_playermodel.pics[k].generic.x		= x;
			s_playermodel.pics[k].generic.y		= y;
			s_playermodel.pics[k].width			= 64;
			s_playermodel.pics[k].height		= 64;
			s_playermodel.pics[k].focuspic		= MODEL_SELECTED;
			s_playermodel.pics[k].focuscolor	= colorRed;

			s_playermodel.picbuttons[k].generic.type	= MTYPE_BITMAP;
			s_playermodel.picbuttons[k].generic.flags	= QMF_LEFT_JUSTIFY|QMF_NODEFAULTINIT|QMF_PULSEIFFOCUS;
			s_playermodel.picbuttons[k].generic.id		= ID_PLAYERPIC0+k;
			s_playermodel.picbuttons[k].generic.callback = PlayerModel_PicEvent;
			s_playermodel.picbuttons[k].generic.x		= x - 16;
			s_playermodel.picbuttons[k].generic.y		= y - 16;
			s_playermodel.picbuttons[k].generic.left	= x;
			s_playermodel.picbuttons[k].generic.top		= y;
			s_playermodel.picbuttons[k].generic.right	= x + 64;
			s_playermodel.picbuttons[k].generic.bottom	= y + 64;
			s_playermodel.picbuttons[k].width			= 128;
			s_playermodel.picbuttons[k].height			= 128;
			s_playermodel.picbuttons[k].focuspic		= MODEL_SELECT;
			s_playermodel.picbuttons[k].focuscolor		= colorRed;

			x += 64 + 6;
		}
		y += 64 + 6;
	}

	s_playermodel.playername.generic.type	= MTYPE_PTEXT;
	s_playermodel.playername.generic.flags	= QMF_CENTER_JUSTIFY|QMF_INACTIVE;
	s_playermodel.playername.generic.x		= 320;
	s_playermodel.playername.generic.y		= 440;
	s_playermodel.playername.string			= playername;
	s_playermodel.playername.style			= FONT_CENTER;
	s_playermodel.playername.color			= text_color_normal;

	s_playermodel.modelname.generic.type	= MTYPE_PTEXT;
	s_playermodel.modelname.generic.flags	= QMF_CENTER_JUSTIFY|QMF_INACTIVE;
	s_playermodel.modelname.generic.x		= 497;
	s_playermodel.modelname.generic.y		= 54;
	s_playermodel.modelname.string			= modelname;
	s_playermodel.modelname.style			= FONT_CENTER;
	s_playermodel.modelname.color			= text_color_normal;

	s_playermodel.skinname.generic.type		= MTYPE_PTEXT;
	s_playermodel.skinname.generic.flags	= QMF_CENTER_JUSTIFY|QMF_INACTIVE;
	s_playermodel.skinname.generic.x		= 497;
	s_playermodel.skinname.generic.y		= 394;
	s_playermodel.skinname.string			= skinname;
	s_playermodel.skinname.style			= FONT_CENTER;
	s_playermodel.skinname.color			= text_color_normal;

	s_playermodel.player.generic.type		= MTYPE_BITMAP;
	s_playermodel.player.generic.flags		= QMF_INACTIVE;
	s_playermodel.player.generic.ownerdraw	= PlayerModel_DrawPlayer;
	s_playermodel.player.generic.x			= 400;
	s_playermodel.player.generic.y			= -40;
	s_playermodel.player.width				= 32 * 10;
	s_playermodel.player.height				= 56 * 10;

	s_playermodel.arrows.generic.type		= MTYPE_BITMAP;
	s_playermodel.arrows.generic.name		= MODEL_ARROWS;
	s_playermodel.arrows.generic.flags		= QMF_INACTIVE;
	s_playermodel.arrows.generic.x			= 125;
	s_playermodel.arrows.generic.y			= 340;
	s_playermodel.arrows.width				= 128;
	s_playermodel.arrows.height				= 32;

	s_playermodel.left.generic.type			= MTYPE_BITMAP;
	s_playermodel.left.generic.flags		= QMF_LEFT_JUSTIFY|QMF_PULSEIFFOCUS;
	s_playermodel.left.generic.callback		= PlayerModel_MenuEvent;
	s_playermodel.left.generic.id			= ID_PREVPAGE;
	s_playermodel.left.generic.x			= 125;
	s_playermodel.left.generic.y			= 340;
	s_playermodel.left.width				= 64;
	s_playermodel.left.height				= 32;
	s_playermodel.left.focuspic				= MODEL_ARROWSL;

	s_playermodel.right.generic.type		= MTYPE_BITMAP;
	s_playermodel.right.generic.flags		= QMF_LEFT_JUSTIFY|QMF_PULSEIFFOCUS;
	s_playermodel.right.generic.callback	= PlayerModel_MenuEvent;
	s_playermodel.right.generic.id			= ID_NEXTPAGE;
	s_playermodel.right.generic.x			= 125+61;
	s_playermodel.right.generic.y			= 340;
	s_playermodel.right.width				= 64;
	s_playermodel.right.height				= 32;
	s_playermodel.right.focuspic			= MODEL_ARROWSR;

	s_playermodel.back.generic.type		= MTYPE_BITMAP;
	s_playermodel.back.generic.name		= MODEL_BACK0;
	s_playermodel.back.generic.flags	= QMF_LEFT_JUSTIFY|QMF_PULSEIFFOCUS;
	s_playermodel.back.generic.callback	= PlayerModel_MenuEvent;
	s_playermodel.back.generic.id		= ID_BACK;
	s_playermodel.back.generic.x		= 0;
	s_playermodel.back.generic.y		= 480-64;
	s_playermodel.back.width			= 128;
	s_playermodel.back.height			= 64;
	s_playermodel.back.focuspic			= MODEL_BACK1;

	Menu_AddItem(&s_playermodel.menu, &s_playermodel.banner);
	Menu_AddItem(&s_playermodel.menu, &s_playermodel.ports);
	Menu_AddItem(&s_playermodel.menu, &s_playermodel.playername);
	Menu_AddItem(&s_playermodel.menu, &s_playermodel.modelname);
	Menu_AddItem(&s_playermodel.menu, &s_playermodel.skinname);

	for (i = 0; i<MAX_MODELSPERPAGE; i++) {
		Menu_AddItem(&s_playermodel.menu, &s_playermodel.pics[i]);
		Menu_AddItem(&s_playermodel.menu, &s_playermodel.picbuttons[i]);
	}

	Menu_AddItem(&s_playermodel.menu, &s_playermodel.player);
	Menu_AddItem(&s_playermodel.menu, &s_playermodel.arrows);
	Menu_AddItem(&s_playermodel.menu, &s_playermodel.left);
	Menu_AddItem(&s_playermodel.menu, &s_playermodel.right);
	Menu_AddItem(&s_playermodel.menu, &s_playermodel.back);

	// find all available models
//	PlayerModel_BuildList();

	// set initial states
	PlayerModel_SetMenuItems();

	// update user interface
	PlayerModel_UpdateGrid();
	PlayerModel_UpdateModel();
}

void PlayerModel_Cache(void)
{
	int	i;

	for (i = 0; playermodel_artlist[i]; i++) {
		trap_R_RegisterShaderNoMip(playermodel_artlist[i]);
	}

	PlayerModel_BuildList();
	for (i = 0; i < s_playermodel.nummodels; i++) {
		trap_R_RegisterShaderNoMip(s_playermodel.modelnames[i]);
	}
}

void UI_PlayerModelMenu(void)
{
	PlayerModel_MenuInit();

	UI_PushMenu(&s_playermodel.menu);

	Menu_SetCursorToItem(&s_playermodel.menu, &s_playermodel.pics[s_playermodel.selectedmodel % MAX_MODELSPERPAGE]);
}


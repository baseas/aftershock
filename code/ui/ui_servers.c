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
// ui_servers.s -- multiplayer menu (server browser)

#include "ui_local.h"

#define MAX_GLOBALSERVERS		128
#define MAX_PINGREQUESTS		32
#define MAX_ADDRESSLENGTH		64
#define MAX_HOSTNAMELENGTH		22
#define MAX_MAPNAMELENGTH		16
#define MAX_LISTBOXITEMS		128
#define MAX_LOCALSERVERS		128
#define MAX_STATUSLENGTH		64
#define MAX_LEAGUELENGTH		28
#define MAX_LISTBOXWIDTH		68

#define ART_ARROWS0				"menu/art/arrows_vert_0"
#define ART_ARROWS_UP			"menu/art/arrows_vert_top"
#define ART_ARROWS_DOWN			"menu/art/arrows_vert_bot"
#define ART_UNKNOWNMAP			"menu/art/unknownmap"
#define ART_REMOVE0				"menu/art/delete_0"
#define ART_REMOVE1				"menu/art/delete_1"

#define UI_MAX_MASTER_SERVERS	5

enum {
	ID_MASTER,
	ID_GAMETYPE,
	ID_FILTER,
	ID_SHOW_FULL,
	ID_SHOW_EMPTY,
	ID_LABEL_HOST,
	ID_LABEL_PING,
	ID_LABEL_MAP,
	ID_LABEL_TYPE,
	ID_LABEL_PLAYERS,
	ID_LIST,
	ID_SCROLL_UP,
	ID_SCROLL_DOWN,
	ID_BACK,
	ID_REFRESH,
	ID_SPECIFY,
	ID_CREATE,
	ID_CONNECT,
	ID_REMOVE
};

enum {
	UIAS_LOCAL,
	UIAS_GLOBAL1,
	UIAS_GLOBAL2,
	UIAS_GLOBAL3,
	UIAS_GLOBAL4,
	UIAS_GLOBAL5,
	UIAS_FAVORITES
};

enum {
	GAMES_ALL,
	GAMES_FFA,
	GAMES_TEAMPLAY,
	GAMES_TOURNEY,
	GAMES_CTF
};

static char *gamenames[] = {
	"DM ",	// deathmatch
	"1v1",	// tournament
	"SP ",	// single player
	"Team DM",	// team deathmatch
	"CTF",	// capture the flag
	"One Flag CTF",		// one flag ctf
	"OverLoad",				// Overload
	"Harvester",			// Harvester
	"Rocket Arena 3",	// Rocket Arena 3
	"Q3F",						// Q3F
	"Urban Terror",		// Urban Terror
	"OSP",						// Orange Smoothie Productions
	"???",			// unknown
	NULL
};

static char* netnames[] = {
	"??? ",
	"UDP ",
	"UDP6",
	NULL
};

typedef struct {
	char	adrstr[MAX_ADDRESSLENGTH];
	int		start;
} pinglist_t;

typedef struct servernode_s {
	char	adrstr[MAX_ADDRESSLENGTH];
	char	hostname[MAX_HOSTNAMELENGTH+3];
	char	mapname[MAX_MAPNAMELENGTH];
	int		numclients;
	int		maxclients;
	int		pingtime;
	int		gametype;
	char	gamename[12];
	int		nettype;
	int		minPing;
	int		maxPing;
	qboolean bPB;

} servernode_t;

typedef struct {
	char			buff[MAX_LISTBOXWIDTH];
	servernode_t*	servernode;
} table_t;

typedef struct {
	menuframework_s		menu;

	menutext_s			banner;

	menulist_s			master;
	menulist_s			gametype;
	menuradiobutton_s	showfull;
	menuradiobutton_s	showempty;

	menufield_s			filter;
	menutext_s			labelHost;
	menutext_s			labelPing;
	menutext_s			labelType;
	menutext_s			labelMap;
	menutext_s			labelPlayers;
	menulist_s			list;
	menubitmap_s		mappic;
	menubitmap_s		arrows;
	menubitmap_s		up;
	menubitmap_s		down;
	menutext_s			status;
	menutext_s			statusbar;

	menubutton_s		remove;
	menubutton_s		back;
	menubutton_s		refresh;
	menubutton_s		specify;
	menubutton_s		go;

	pinglist_t			pinglist[MAX_PINGREQUESTS];
	table_t				table[MAX_LISTBOXITEMS];
	char*				items[MAX_LISTBOXITEMS];
	int					numqueriedservers;
	int					*numservers;
	servernode_t		*serverlist;
	int					currentping;
	qboolean			refreshservers;
	int					nextpingtime;
	int					maxservers;
	int					refreshtime;
	char				favoriteaddresses[MAX_FAVORITESERVERS][MAX_ADDRESSLENGTH];
	int					numfavoriteaddresses;
} arenaservers_t;

static arenaservers_t	g_arenaservers;

static servernode_t		g_globalserverlist[UI_MAX_MASTER_SERVERS][MAX_GLOBALSERVERS];
static int				g_numglobalservers[UI_MAX_MASTER_SERVERS];
static servernode_t		g_localserverlist[MAX_LOCALSERVERS];
static int				g_numlocalservers;
static servernode_t		g_favoriteserverlist[MAX_FAVORITESERVERS];
static int				g_numfavoriteservers;
static int				g_servertype;
static int				g_gametype;
static int				g_sortkey;
static int				g_emptyservers;
static int				g_fullservers;

static int ArenaServers_MaxPing(void)
{
	int		maxPing;

	maxPing = (int)trap_Cvar_VariableValue("cl_maxPing");
	if (maxPing < 100) {
		maxPing = 100;
	}
	return maxPing;
}

static int QDECL ArenaServers_Compare(const void *arg1, const void *arg2)
{
	float			f1;
	float			f2;
	servernode_t*	t1;
	servernode_t*	t2;

	t1 = (servernode_t *) arg1;
	t2 = (servernode_t *) arg2;

	switch (g_sortkey) {
	case SORT_HOST:
		return Q_stricmp(t1->hostname, t2->hostname);

	case SORT_MAP:
		return Q_stricmp(t1->mapname, t2->mapname);

	case SORT_CLIENTS:
		f1 = t1->maxclients - t1->numclients;
		if (f1 < 0) {
			f1 = 0;
		}

		f2 = t2->maxclients - t2->numclients;
		if (f2 < 0) {
			f2 = 0;
		}

		if (f1 < f2) {
			return 1;
		}
		if (f1 == f2) {
			return 0;
		}
		return -1;

	case SORT_GAME:
		if (t1->gametype < t2->gametype) {
			return -1;
		}
		if (t1->gametype == t2->gametype) {
			return 0;
		}
		return 1;

	case SORT_PING:
		if (t1->pingtime < t2->pingtime) {
			return -1;
		}
		if (t1->pingtime > t2->pingtime) {
			return 1;
		}
		return Q_stricmp(t1->hostname, t2->hostname);
	}

	return 0;
}

/**
Convert ui's g_servertype to AS_* used by trap calls.
*/
int ArenaServers_SourceForLAN(void)
{
	switch (g_servertype) {
	default:
	case UIAS_LOCAL:
		return AS_LOCAL;
	case UIAS_GLOBAL1:
	case UIAS_GLOBAL2:
	case UIAS_GLOBAL3:
	case UIAS_GLOBAL4:
	case UIAS_GLOBAL5:
		return AS_GLOBAL;
	case UIAS_FAVORITES:
		return AS_FAVORITES;
	}
}

static void ArenaServers_Go(void)
{
	servernode_t*	servernode;

	servernode = g_arenaservers.table[g_arenaservers.list.curvalue].servernode;
	if (servernode) {
		trap_Cmd_ExecuteText(EXEC_APPEND, va("connect %s\n", servernode->adrstr));
	}
}

static void ArenaServers_UpdatePicture(void)
{
	static char		picname[64];
	servernode_t*	servernodeptr;

	if (!g_arenaservers.list.numitems) {
		g_arenaservers.mappic.generic.name = NULL;
	}
	else {
		servernodeptr = g_arenaservers.table[g_arenaservers.list.curvalue].servernode;
		Com_sprintf(picname, sizeof(picname), "levelshots/%s.tga", servernodeptr->mapname);
		g_arenaservers.mappic.generic.name = picname;

	}

	// force shader update during draw
	g_arenaservers.mappic.shader = 0;
}

static void ArenaServers_UpdateMenu(void)
{
	int				i;
	int				j;
	int				count;
	char*			buff;
	servernode_t*	servernodeptr;
	table_t*		tableptr;
	char			*pingColor;

	if (g_arenaservers.numqueriedservers > 0) {
		// servers found
		if (g_arenaservers.refreshservers && (g_arenaservers.currentping <= g_arenaservers.numqueriedservers)) {
			// show progress
			Com_sprintf(g_arenaservers.status.string, MAX_STATUSLENGTH, "%d of %d Arena Servers.", g_arenaservers.currentping, g_arenaservers.numqueriedservers);
			g_arenaservers.statusbar.string  = "Press SPACE to stop";
			qsort(g_arenaservers.serverlist, *g_arenaservers.numservers, sizeof(servernode_t), ArenaServers_Compare);
		}
	} else {
		// no servers found
		if (g_arenaservers.refreshservers) {
			strcpy(g_arenaservers.status.string,"Scanning For Servers.");
			g_arenaservers.statusbar.string = "Press SPACE to stop";
		} else {
			if (g_arenaservers.numqueriedservers < 0) {
				strcpy(g_arenaservers.status.string,"No Response From Master Server.");
			} else {
				strcpy(g_arenaservers.status.string,"No Servers Found.");
			}
		}

		// zero out list box
		g_arenaservers.list.numitems = 0;
		g_arenaservers.list.curvalue = 0;
		g_arenaservers.list.top      = 0;

		// update picture
		ArenaServers_UpdatePicture();
		return;
	}

	// build list box strings - apply culling filters
	servernodeptr = g_arenaservers.serverlist;
	count = *g_arenaservers.numservers;
	for (i = 0, j = 0; i < count; i++, servernodeptr++) {
		tableptr = &g_arenaservers.table[j];
		tableptr->servernode = servernodeptr;
		buff = tableptr->buff;

		// can only cull valid results
		if (!g_emptyservers && !servernodeptr->numclients) {
			continue;
		}

		if (!g_fullservers && (servernodeptr->numclients == servernodeptr->maxclients)) {
			continue;
		}

		switch (g_gametype) {
		case GAMES_ALL:
			break;

		case GAMES_FFA:
			if (servernodeptr->gametype != GT_FFA) {
				continue;
			}
			break;

		case GAMES_TEAMPLAY:
			if (servernodeptr->gametype != GT_TEAM) {
				continue;
			}
			break;

		case GAMES_TOURNEY:
			if (servernodeptr->gametype != GT_TOURNAMENT) {
				continue;
			}
			break;

		case GAMES_CTF:
			if (servernodeptr->gametype != GT_CTF) {
				continue;
			}
			break;
		}

		if (servernodeptr->pingtime < servernodeptr->minPing) {
			pingColor = S_COLOR_BLUE;
		} else if (servernodeptr->maxPing && servernodeptr->pingtime > servernodeptr->maxPing) {
			pingColor = S_COLOR_BLUE;
		} else if (servernodeptr->pingtime < 200) {
			pingColor = S_COLOR_GREEN;
		} else if (servernodeptr->pingtime < 400) {
			pingColor = S_COLOR_YELLOW;
		} else {
			pingColor = S_COLOR_RED;
		}

		Com_sprintf(buff, MAX_LISTBOXWIDTH, "%-20.20s %-12.12s %2d/%2d %-8.8s %4s%s%3d " S_COLOR_YELLOW "%s",
			servernodeptr->hostname, servernodeptr->mapname, servernodeptr->numclients,
			servernodeptr->maxclients, servernodeptr->gamename,
			netnames[servernodeptr->nettype], pingColor, servernodeptr->pingtime, servernodeptr->bPB ? "Yes" : "No");
		j++;
	}

	g_arenaservers.list.numitems = j;
	g_arenaservers.list.curvalue = 0;
	g_arenaservers.list.top = 0;

	// update picture
	ArenaServers_UpdatePicture();
}

static void ArenaServers_Remove(void)
{
	int				i;
	servernode_t*	servernodeptr;
	table_t*		tableptr;

	if (!g_arenaservers.list.numitems)
		return;

	// remove selected item from display list
	// items are in scattered order due to sort and cull
	// perform delete on list box contents, resync all lists

	tableptr      = &g_arenaservers.table[g_arenaservers.list.curvalue];
	servernodeptr = tableptr->servernode;

	// find address in master list
	for (i = 0; i < g_arenaservers.numfavoriteaddresses; i++) {
		if (!Q_stricmp(g_arenaservers.favoriteaddresses[i],servernodeptr->adrstr)) {
			// delete address from master list
			if (i < g_arenaservers.numfavoriteaddresses-1) {
				// shift items up
				memcpy(&g_arenaservers.favoriteaddresses[i], &g_arenaservers.favoriteaddresses[i+1], (g_arenaservers.numfavoriteaddresses - i - 1)* MAX_ADDRESSLENGTH);
			}
			g_arenaservers.numfavoriteaddresses--;
			memset(&g_arenaservers.favoriteaddresses[g_arenaservers.numfavoriteaddresses], 0, MAX_ADDRESSLENGTH);
			break;
		}
	}

	// find address in server list
	for (i = 0; i < g_numfavoriteservers; i++) {
		if (&g_favoriteserverlist[i] == servernodeptr) {
			// delete address from server list
			if (i < g_numfavoriteservers-1) {
				// shift items up
				memcpy(&g_favoriteserverlist[i], &g_favoriteserverlist[i+1], (g_numfavoriteservers - i - 1)*sizeof(servernode_t));
			}
			g_numfavoriteservers--;
			memset(&g_favoriteserverlist[ g_numfavoriteservers ], 0, sizeof(servernode_t));
			break;
		}
	}

	g_arenaservers.numqueriedservers = g_arenaservers.numfavoriteaddresses;
	g_arenaservers.currentping = g_arenaservers.numfavoriteaddresses;
}

static void ArenaServers_Insert(char* adrstr, char* info, int pingtime)
{
	servernode_t*	servernodeptr;
	char*			s;
	int				i;


	if ((pingtime >= ArenaServers_MaxPing()) && (g_servertype != UIAS_FAVORITES)) {
		// slow global or local servers do not get entered
		return;
	}

	if (*g_arenaservers.numservers >= g_arenaservers.maxservers) {
		// list full;
		servernodeptr = g_arenaservers.serverlist+(*g_arenaservers.numservers)-1;
	} else {
		// next slot
		servernodeptr = g_arenaservers.serverlist+(*g_arenaservers.numservers);
		(*g_arenaservers.numservers)++;
	}

	Q_strncpyz(servernodeptr->adrstr, adrstr, MAX_ADDRESSLENGTH);

	Q_strncpyz(servernodeptr->hostname, Info_ValueForKey(info, "hostname"), MAX_HOSTNAMELENGTH);
	Q_CleanStr(servernodeptr->hostname);
	Q_strupr(servernodeptr->hostname);

	Q_strncpyz(servernodeptr->mapname, Info_ValueForKey(info, "mapname"), MAX_MAPNAMELENGTH);
	Q_CleanStr(servernodeptr->mapname);
	Q_strupr(servernodeptr->mapname);

	servernodeptr->numclients = atoi(Info_ValueForKey(info, "clients"));
	servernodeptr->maxclients = atoi(Info_ValueForKey(info, "sv_maxclients"));
	servernodeptr->pingtime   = pingtime;
	servernodeptr->minPing    = atoi(Info_ValueForKey(info, "minPing"));
	servernodeptr->maxPing    = atoi(Info_ValueForKey(info, "maxPing"));

	/*
	s = Info_ValueForKey(info, "nettype");
	for (i = 0; ;i++)
	{
		if (!netnames[i])
		{
			servernodeptr->nettype = 0;
			break;
		}
		else if (!Q_stricmp(netnames[i], s))
		{
			servernodeptr->nettype = i;
			break;
		}
	}
	*/
	servernodeptr->nettype = atoi(Info_ValueForKey(info, "nettype"));
	if (servernodeptr->nettype < 0 || servernodeptr->nettype >= ARRAY_LEN(netnames) - 1) {
		servernodeptr->nettype = 0;
	}

	s = Info_ValueForKey(info, "game");
	i = atoi(Info_ValueForKey(info, "gametype"));
	if (i < 0) {
		i = 0;
	} else if (i > 11) {
		i = 12;
	}

	if (*s) {
		servernodeptr->gametype = i;//-1;
		Q_strncpyz(servernodeptr->gamename, s, sizeof(servernodeptr->gamename));
	} else {
		servernodeptr->gametype = i;
		Q_strncpyz(servernodeptr->gamename, gamenames[i], sizeof(servernodeptr->gamename));
	}
}

/**
ArenaServers_InsertFavorites
Insert nonresponsive address book entries into display lists.
*/
void ArenaServers_InsertFavorites(void)
{
	int		i;
	int		j;
	char	info[MAX_INFO_STRING];

	// resync existing results with new or deleted cvars
	info[0] = '\0';
	Info_SetValueForKey(info, "hostname", "No Response");
	for (i = 0; i < g_arenaservers.numfavoriteaddresses; i++) {
		// find favorite address in refresh list
		for (j = 0; j < g_numfavoriteservers; j++) {
			if (!Q_stricmp(g_arenaservers.favoriteaddresses[i],g_favoriteserverlist[j].adrstr)) {
				break;
			}
		}

		if (j >= g_numfavoriteservers) {
			// not in list, add it
			ArenaServers_Insert(g_arenaservers.favoriteaddresses[i], info, ArenaServers_MaxPing());
		}
	}
}

/**
Load cvar address book entries into local lists.
*/
void ArenaServers_LoadFavorites(void)
{
	int				i;
	int				j;
	int				numtempitems;
	char			adrstr[MAX_ADDRESSLENGTH];
	servernode_t	templist[MAX_FAVORITESERVERS];
	qboolean		found;

	found        = qfalse;

	// copy the old
	memcpy(templist, g_favoriteserverlist, sizeof(servernode_t)*MAX_FAVORITESERVERS);
	numtempitems = g_numfavoriteservers;

	// clear the current for sync
	memset(g_favoriteserverlist, 0, sizeof(servernode_t)*MAX_FAVORITESERVERS);
	g_numfavoriteservers = 0;

	// resync existing results with new or deleted cvars
	for (i = 0; i < MAX_FAVORITESERVERS; i++) {
		trap_Cvar_VariableStringBuffer(va("server%d",i+1), adrstr, MAX_ADDRESSLENGTH);
		if (!adrstr[0]) {
			continue;
		}

		// favorite server addresses must be maintained outside refresh list
		// this mimics local and global netadr's stored in client
		// these can be fetched to fill ping list
		strcpy(g_arenaservers.favoriteaddresses[g_numfavoriteservers], adrstr);

		// find this server in the old list
		for (j = 0; j < numtempitems; j++) {
			if (!Q_stricmp(templist[j].adrstr, adrstr)) {
				break;
			}
		}

		if (j < numtempitems) {
			// found server - add exisiting results
			memcpy(&g_favoriteserverlist[g_numfavoriteservers], &templist[j], sizeof(servernode_t));
			found = qtrue;
		} else {
			// add new server
			Q_strncpyz(g_favoriteserverlist[g_numfavoriteservers].adrstr, adrstr, MAX_ADDRESSLENGTH);
			g_favoriteserverlist[g_numfavoriteservers].pingtime = ArenaServers_MaxPing();
		}

		g_numfavoriteservers++;
	}

	g_arenaservers.numfavoriteaddresses = g_numfavoriteservers;

	if (!found) {
		// no results were found, reset server list
		// list will be automatically refreshed when selected
		g_numfavoriteservers = 0;
	}
}

static void ArenaServers_StopRefresh(void)
{
	if (!g_arenaservers.refreshservers) {
		// not currently refreshing
		return;
	}

	g_arenaservers.refreshservers = qfalse;

	if (g_servertype == UIAS_FAVORITES) {
		// nonresponsive favorites must be shown
		ArenaServers_InsertFavorites();
	}

	// final tally
	if (g_arenaservers.numqueriedservers >= 0) {
		g_arenaservers.currentping = *g_arenaservers.numservers;
		g_arenaservers.numqueriedservers = *g_arenaservers.numservers;
	}

	// sort
	qsort(g_arenaservers.serverlist, *g_arenaservers.numservers, sizeof(servernode_t), ArenaServers_Compare);

	ArenaServers_UpdateMenu();
}

static void ArenaServers_DoRefresh(void)
{
	int		i;
	int		j;
	int		time;
	int		maxPing;
	char	adrstr[MAX_ADDRESSLENGTH];
	char	info[MAX_INFO_STRING];

	if (uis.realtime < g_arenaservers.refreshtime) {
		if (g_servertype != UIAS_FAVORITES) {
			if (g_servertype == UIAS_LOCAL) {
				if (!trap_LAN_GetServerCount(AS_LOCAL)) {
					return;
				}
			}
			if (trap_LAN_GetServerCount(ArenaServers_SourceForLAN()) < 0) {
				// still waiting for response
				return;
			}
		}
	}

	if (uis.realtime < g_arenaservers.nextpingtime) {
		// wait for time trigger
		return;
	}

	// trigger at 10Hz intervals
	g_arenaservers.nextpingtime = uis.realtime + 10;

	// process ping results
	maxPing = ArenaServers_MaxPing();
	for (i = 0; i < MAX_PINGREQUESTS; i++) {
		trap_LAN_GetPing(i, adrstr, MAX_ADDRESSLENGTH, &time);
		if (!adrstr[0]) {
			// ignore empty or pending pings
			continue;
		}

		// find ping result in our local list
		for (j = 0; j < MAX_PINGREQUESTS; j++) {
			if (!Q_stricmp(adrstr, g_arenaservers.pinglist[j].adrstr)) {
				break;
			}
		}

		if (j < MAX_PINGREQUESTS) {
			// found it
			if (!time) {
				time = uis.realtime - g_arenaservers.pinglist[j].start;
				if (time < maxPing) {
					// still waiting
					continue;
				}
			}

			if (time > maxPing) {
				// stale it out
				info[0] = '\0';
				time    = maxPing;
			} else {
				trap_LAN_GetPingInfo(i, info, MAX_INFO_STRING);
			}

			// insert ping results
			ArenaServers_Insert(adrstr, info, time);

			// clear this query from internal list
			g_arenaservers.pinglist[j].adrstr[0] = '\0';
		}

		// clear this query from external list
		trap_LAN_ClearPing(i);
	}

	// get results of servers query
	// counts can increase as servers respond
	if (g_servertype == UIAS_FAVORITES) {
		g_arenaservers.numqueriedservers = g_arenaservers.numfavoriteaddresses;
	} else {
		g_arenaservers.numqueriedservers = trap_LAN_GetServerCount(ArenaServers_SourceForLAN());
	}

//	if (g_arenaservers.numqueriedservers > g_arenaservers.maxservers)
//		g_arenaservers.numqueriedservers = g_arenaservers.maxservers;

	// send ping requests in reasonable bursts
	// iterate ping through all found servers
	for (i = 0; i < MAX_PINGREQUESTS && g_arenaservers.currentping < g_arenaservers.numqueriedservers; i++) {
		if (trap_LAN_GetPingQueueCount() >= MAX_PINGREQUESTS) {
			// ping queue is full
			break;
		}

		// find empty slot
		for (j = 0; j < MAX_PINGREQUESTS; j++) {
			if (!g_arenaservers.pinglist[j].adrstr[0]) {
				break;
			}
		}

		if (j >= MAX_PINGREQUESTS) {
			// no empty slots available yet - wait for timeout
			break;
		}

		// get an address to ping

		if (g_servertype == UIAS_FAVORITES) {
			strcpy(adrstr, g_arenaservers.favoriteaddresses[g_arenaservers.currentping]); 
		} else {
			trap_LAN_GetServerAddressString(ArenaServers_SourceForLAN(), g_arenaservers.currentping, adrstr, MAX_ADDRESSLENGTH);
		}

		strcpy(g_arenaservers.pinglist[j].adrstr, adrstr);
		g_arenaservers.pinglist[j].start = uis.realtime;

		trap_Cmd_ExecuteText(EXEC_NOW, va("ping %s\n", adrstr));

		// advance to next server
		g_arenaservers.currentping++;
	}

	if (!trap_LAN_GetPingQueueCount()) {
		// all pings completed
		ArenaServers_StopRefresh();
		return;
	}

	// update the user interface with ping status
	ArenaServers_UpdateMenu();
}

static void ArenaServers_StartRefresh(void)
{
	int		i;
	char	myargs[32], protocol[32];

	memset(g_arenaservers.serverlist, 0, g_arenaservers.maxservers*sizeof(table_t));

	for (i = 0; i < MAX_PINGREQUESTS; i++) {
		g_arenaservers.pinglist[i].adrstr[0] = '\0';
		trap_LAN_ClearPing(i);
	}

	g_arenaservers.refreshservers = qtrue;
	g_arenaservers.currentping = 0;
	g_arenaservers.nextpingtime = 0;
	*g_arenaservers.numservers = 0;
	g_arenaservers.numqueriedservers = 0;

	// allow max 5 seconds for responses
	g_arenaservers.refreshtime = uis.realtime + 5000;

	// place menu in zeroed state
	ArenaServers_UpdateMenu();

	if (g_servertype == UIAS_LOCAL) {
		trap_Cmd_ExecuteText(EXEC_APPEND, "localservers\n");
		return;
	}

	if (g_servertype >= UIAS_GLOBAL1 && g_servertype <= UIAS_GLOBAL5) {
		switch (g_arenaservers.gametype.curvalue) {
		default:
		case GAMES_ALL:
			myargs[0] = 0;
			break;

		case GAMES_FFA:
			strcpy(myargs, " ffa");
			break;

		case GAMES_TEAMPLAY:
			strcpy(myargs, " team");
			break;

		case GAMES_TOURNEY:
			strcpy(myargs, " tourney");
			break;

		case GAMES_CTF:
			strcpy(myargs, " ctf");
			break;
		}

		if (g_emptyservers) {
			strcat(myargs, " empty");
		}

		if (g_fullservers) {
			strcat(myargs, " full");
		}

		protocol[0] = '\0';
		trap_Cvar_VariableStringBuffer("debug_protocol", protocol, sizeof(protocol));
		if (strlen(protocol)) {
			trap_Cmd_ExecuteText(EXEC_APPEND, va("globalservers %d %s%s\n", g_servertype - 1, protocol, myargs));
		} else {
			trap_Cmd_ExecuteText(EXEC_APPEND, va("globalservers %d %d%s\n", g_servertype - 1, (int)trap_Cvar_VariableValue("protocol"), myargs));
		}
	}
}

void ArenaServers_SaveChanges(void)
{
	int	i;

	for (i = 0; i < g_arenaservers.numfavoriteaddresses; i++) {
		trap_Cvar_Set(va("server%d",i+1), g_arenaservers.favoriteaddresses[i]);
	}

	for (; i < MAX_FAVORITESERVERS; i++) {
		trap_Cvar_Set(va("server%d",i+1), "");
	}
}

void ArenaServers_Sort(int type)
{
	if (g_sortkey == type) {
		return;
	}

	g_sortkey = type;
	qsort(g_arenaservers.serverlist, *g_arenaservers.numservers, sizeof(servernode_t), ArenaServers_Compare);
}

int ArenaServers_SetType(int type)
{
	if (type >= UIAS_GLOBAL1 && type <= UIAS_GLOBAL5) {
		char masterstr[2], cvarname[sizeof("sv_master1")];

		while (type <= UIAS_GLOBAL5) {
			Com_sprintf(cvarname, sizeof(cvarname), "sv_master%d", type);
			trap_Cvar_VariableStringBuffer(cvarname, masterstr, sizeof(masterstr));
			if (*masterstr) {
				break;
			}

			type++;
		}
	}

	g_servertype = type;

	switch (type) {
	default:
	case UIAS_LOCAL:
		g_arenaservers.remove.generic.flags |= (QMF_INACTIVE|QMF_HIDDEN);
		g_arenaservers.serverlist = g_localserverlist;
		g_arenaservers.numservers = &g_numlocalservers;
		g_arenaservers.maxservers = MAX_LOCALSERVERS;
		break;

	case UIAS_GLOBAL1:
	case UIAS_GLOBAL2:
	case UIAS_GLOBAL3:
	case UIAS_GLOBAL4:
	case UIAS_GLOBAL5:
		g_arenaservers.remove.generic.flags |= (QMF_INACTIVE|QMF_HIDDEN);
		g_arenaservers.serverlist = g_globalserverlist[type-UIAS_GLOBAL1];
		g_arenaservers.numservers = &g_numglobalservers[type-UIAS_GLOBAL1];
		g_arenaservers.maxservers = MAX_GLOBALSERVERS;
		break;

	case UIAS_FAVORITES:
		g_arenaservers.remove.generic.flags &= ~(QMF_INACTIVE|QMF_HIDDEN);
		g_arenaservers.serverlist = g_favoriteserverlist;
		g_arenaservers.numservers = &g_numfavoriteservers;
		g_arenaservers.maxservers = MAX_FAVORITESERVERS;
		break;

	}

	if (!*g_arenaservers.numservers) {
		ArenaServers_StartRefresh();
	}
	else {
		// avoid slow operation, use existing results
		g_arenaservers.currentping       = *g_arenaservers.numservers;
		g_arenaservers.numqueriedservers = *g_arenaservers.numservers;
		ArenaServers_UpdateMenu();
		strcpy(g_arenaservers.status.string,"hit refresh to update");
	}

	return type;
}

static void ArenaServers_Event(void* ptr, int event)
{
	int		id;

	id = ((menucommon_s*)ptr)->id;

	if (event != QM_ACTIVATED && id != ID_LIST) {
		return;
	}

	switch (id) {
	case ID_MASTER:
		g_arenaservers.master.curvalue = ArenaServers_SetType(g_arenaservers.master.curvalue);
		trap_Cvar_SetValue("ui_browserMaster", g_arenaservers.master.curvalue);
		break;

	case ID_GAMETYPE:
		trap_Cvar_SetValue("ui_browserGameType", g_arenaservers.gametype.curvalue);
		g_gametype = g_arenaservers.gametype.curvalue;
		ArenaServers_UpdateMenu();
		break;

	case ID_SHOW_FULL:
		trap_Cvar_SetValue("ui_browserShowFull", g_arenaservers.showfull.curvalue);
		g_fullservers = g_arenaservers.showfull.curvalue;
		ArenaServers_UpdateMenu();
		break;

	case ID_SHOW_EMPTY:
		trap_Cvar_SetValue("ui_browserShowEmpty", g_arenaservers.showempty.curvalue);
		g_emptyservers = g_arenaservers.showempty.curvalue;
		ArenaServers_UpdateMenu();
		break;

	case ID_LIST:
		if (event == QM_GOTFOCUS) {
			ArenaServers_UpdatePicture();
		}
		break;

	case ID_SCROLL_UP:
		ScrollList_Key(&g_arenaservers.list, K_UPARROW);
		break;

	case ID_SCROLL_DOWN:
		ScrollList_Key(&g_arenaservers.list, K_DOWNARROW);
		break;

	case ID_BACK:
		ArenaServers_StopRefresh();
		ArenaServers_SaveChanges();
		UI_PopMenu();
		break;

	case ID_REFRESH:
		ArenaServers_StartRefresh();
		break;

	case ID_SPECIFY:
		UI_SpecifyServerMenu();
		break;

	case ID_CONNECT:
		ArenaServers_Go();
		break;

	case ID_REMOVE:
		ArenaServers_Remove();
		ArenaServers_UpdateMenu();
		break;
	}
}

static void ArenaServers_MenuDraw(void)
{
	if (g_arenaservers.refreshservers) {
		ArenaServers_DoRefresh();
	}

	Menu_Draw(&g_arenaservers.menu);
}

static sfxHandle_t ArenaServers_MenuKey(int key)
{
	if (key == K_SPACE  && g_arenaservers.refreshservers) {
		ArenaServers_StopRefresh();
		return menu_move_sound;
	}

	if ((key == K_DEL || key == K_KP_DEL) && (g_servertype == UIAS_FAVORITES) &&
		(Menu_ItemAtCursor(&g_arenaservers.menu) == &g_arenaservers.list)) {
		ArenaServers_Remove();
		ArenaServers_UpdateMenu();
		return menu_move_sound;
	}

	if (key == K_MOUSE2 || key == K_ESCAPE) {
		ArenaServers_StopRefresh();
		ArenaServers_SaveChanges();
	}


	return Menu_DefaultKey(&g_arenaservers.menu, key);
}

static void ArenaServers_MenuInit(void)
{
	int			i;
	int			x, y;
	int			style;
	static char	statusbuffer[MAX_STATUSLENGTH];

	// zero set all our globals
	memset(&g_arenaservers, 0 ,sizeof(arenaservers_t));

	ArenaServers_Cache();

	g_arenaservers.menu.fullscreen	= qtrue;
	g_arenaservers.menu.wrapAround	= qtrue;
	g_arenaservers.menu.draw		= ArenaServers_MenuDraw;
	g_arenaservers.menu.key			= ArenaServers_MenuKey;

	g_arenaservers.banner.generic.type	= MTYPE_BTEXT;
	g_arenaservers.banner.generic.flags	= QMF_CENTER_JUSTIFY;
	g_arenaservers.banner.generic.x		= 320;
	g_arenaservers.banner.generic.y		= 16;
	g_arenaservers.banner.string		= "ARENA SERVERS";
	g_arenaservers.banner.color			= colorBanner;
	g_arenaservers.banner.style			= FONT_CENTER | FONT_SHADOW;

	y = 80;
	x = 90;

	g_arenaservers.filter.generic.type			= MTYPE_FIELD;
	g_arenaservers.filter.generic.name			= "Filter:";
	g_arenaservers.filter.generic.flags			= QMF_PULSEIFFOCUS | QMF_SMALLFONT;
	g_arenaservers.filter.generic.callback		= ArenaServers_Event;
	g_arenaservers.filter.generic.id			= ID_FILTER;
	g_arenaservers.filter.generic.x				= x;
	g_arenaservers.filter.generic.y				= y;
	g_arenaservers.filter.field.widthInChars	= 20;
	g_arenaservers.filter.field.maxchars		= 20;

	g_arenaservers.showfull.generic.type		= MTYPE_RADIOBUTTON;
	g_arenaservers.showfull.generic.name		= "Full";
	g_arenaservers.showfull.generic.flags		= QMF_PULSEIFFOCUS|QMF_SMALLFONT;
	g_arenaservers.showfull.generic.callback	= ArenaServers_Event;
	g_arenaservers.showfull.generic.id			= ID_SHOW_FULL;
	g_arenaservers.showfull.generic.x			= 340;
	g_arenaservers.showfull.generic.y			= y;

	g_arenaservers.showempty.generic.type		= MTYPE_RADIOBUTTON;
	g_arenaservers.showempty.generic.name		= "Empty";
	g_arenaservers.showempty.generic.flags		= QMF_PULSEIFFOCUS|QMF_SMALLFONT;
	g_arenaservers.showempty.generic.callback	= ArenaServers_Event;
	g_arenaservers.showempty.generic.id			= ID_SHOW_EMPTY;
	g_arenaservers.showempty.generic.x			= 400;
	g_arenaservers.showempty.generic.y			= y;

	y += SMALLCHAR_SIZE;

	style = FONT_SHADOW | FONT_SMALL | FONT_RIGHT;

	g_arenaservers.labelHost.generic.type		= MTYPE_PTEXT;
	g_arenaservers.labelHost.generic.flags		= QMF_PULSEIFFOCUS;
	g_arenaservers.labelHost.generic.callback	= ArenaServers_Event;
	g_arenaservers.labelHost.generic.id			= ID_LABEL_HOST;
	g_arenaservers.labelHost.generic.x			= x;
	g_arenaservers.labelHost.generic.y			= y;
	g_arenaservers.labelHost.string				= "Host";
	g_arenaservers.labelHost.color				= colorRed;
	g_arenaservers.labelHost.style				= style;

	g_arenaservers.labelMap.generic.type		= MTYPE_PTEXT;
	g_arenaservers.labelMap.generic.flags		= QMF_PULSEIFFOCUS;
	g_arenaservers.labelMap.generic.callback	= ArenaServers_Event;
	g_arenaservers.labelMap.generic.id			= ID_LABEL_MAP;
	g_arenaservers.labelMap.generic.x			= x + 20 * SMALLCHAR_SIZE;
	g_arenaservers.labelMap.generic.y			= y;
	g_arenaservers.labelMap.string				= "Map";
	g_arenaservers.labelMap.color				= colorRed;
	g_arenaservers.labelMap.style				= style;

	g_arenaservers.labelPlayers.generic.type		= MTYPE_PTEXT;
	g_arenaservers.labelPlayers.generic.flags		= QMF_PULSEIFFOCUS;
	g_arenaservers.labelPlayers.generic.callback	= ArenaServers_Event;
	g_arenaservers.labelPlayers.generic.id			= ID_LABEL_PLAYERS;
	g_arenaservers.labelPlayers.generic.x			= x + 40 * SMALLCHAR_SIZE;
	g_arenaservers.labelPlayers.generic.y			= y;
	g_arenaservers.labelPlayers.string				= "Players";
	g_arenaservers.labelPlayers.color				= colorRed;
	g_arenaservers.labelPlayers.style				= style;

	g_arenaservers.labelType.generic.type		= MTYPE_PTEXT;
	g_arenaservers.labelType.generic.flags		= QMF_PULSEIFFOCUS;
	g_arenaservers.labelType.generic.callback	= ArenaServers_Event;
	g_arenaservers.labelType.generic.id			= ID_LABEL_TYPE;
	g_arenaservers.labelType.generic.x			= x + 50 * SMALLCHAR_SIZE;
	g_arenaservers.labelType.generic.y			= y;
	g_arenaservers.labelType.string				= "Type";
	g_arenaservers.labelType.color				= colorRed;
	g_arenaservers.labelType.style				= style;

	g_arenaservers.labelPing.generic.type		= MTYPE_PTEXT;
	g_arenaservers.labelPing.generic.flags		= QMF_PULSEIFFOCUS;
	g_arenaservers.labelPing.generic.callback	= ArenaServers_Event;
	g_arenaservers.labelPing.generic.id			= ID_LABEL_PING;
	g_arenaservers.labelPing.generic.x			= x + 60 * SMALLCHAR_SIZE;
	g_arenaservers.labelPing.generic.y			= y;
	g_arenaservers.labelPing.string				= "Ping";
	g_arenaservers.labelPing.color				= colorRed;
	g_arenaservers.labelPing.style				= style;

	y += 3 * SMALLCHAR_SIZE;
	g_arenaservers.list.generic.type			= MTYPE_SCROLLLIST;
	g_arenaservers.list.generic.flags			= QMF_HIGHLIGHT_IF_FOCUS;
	g_arenaservers.list.generic.id				= ID_LIST;
	g_arenaservers.list.generic.callback		= ArenaServers_Event;
	g_arenaservers.list.generic.x				= 72;
	g_arenaservers.list.generic.y				= y;
	g_arenaservers.list.width					= MAX_LISTBOXWIDTH;
	g_arenaservers.list.height					= 11;
	g_arenaservers.list.itemnames				= (const char **)g_arenaservers.items;
	for (i = 0; i < MAX_LISTBOXITEMS; i++) {
		g_arenaservers.items[i] = g_arenaservers.table[i].buff;
	}

	g_arenaservers.mappic.generic.type			= MTYPE_BITMAP;
	g_arenaservers.mappic.generic.flags			= QMF_LEFT_JUSTIFY|QMF_INACTIVE;
	g_arenaservers.mappic.generic.x				= 72;
	g_arenaservers.mappic.generic.y				= 80;
	g_arenaservers.mappic.width					= 128;
	g_arenaservers.mappic.height				= 96;
	g_arenaservers.mappic.errorpic				= ART_UNKNOWNMAP;

	g_arenaservers.arrows.generic.type			= MTYPE_BITMAP;
	g_arenaservers.arrows.generic.name			= ART_ARROWS0;
	g_arenaservers.arrows.generic.flags			= QMF_LEFT_JUSTIFY|QMF_INACTIVE;
	g_arenaservers.arrows.generic.callback		= ArenaServers_Event;
	g_arenaservers.arrows.generic.x				= 512+48;
	g_arenaservers.arrows.generic.y				= 240-64+16;
	g_arenaservers.arrows.width					= 64;
	g_arenaservers.arrows.height				= 128;

	g_arenaservers.up.generic.type				= MTYPE_BITMAP;
	g_arenaservers.up.generic.flags				= QMF_LEFT_JUSTIFY|QMF_PULSEIFFOCUS|QMF_MOUSEONLY;
	g_arenaservers.up.generic.callback			= ArenaServers_Event;
	g_arenaservers.up.generic.id				= ID_SCROLL_UP;
	g_arenaservers.up.generic.x					= 512+48;
	g_arenaservers.up.generic.y					= 240-64+16;
	g_arenaservers.up.width						= 64;
	g_arenaservers.up.height					= 64;
	g_arenaservers.up.focuspic					= ART_ARROWS_UP;

	g_arenaservers.down.generic.type			= MTYPE_BITMAP;
	g_arenaservers.down.generic.flags			= QMF_LEFT_JUSTIFY|QMF_PULSEIFFOCUS|QMF_MOUSEONLY;
	g_arenaservers.down.generic.callback		= ArenaServers_Event;
	g_arenaservers.down.generic.id				= ID_SCROLL_DOWN;
	g_arenaservers.down.generic.x				= 512+48;
	g_arenaservers.down.generic.y				= 240+16;
	g_arenaservers.down.width					= 64;
	g_arenaservers.down.height					= 64;
	g_arenaservers.down.focuspic				= ART_ARROWS_DOWN;

	y = 376;
	g_arenaservers.status.generic.type		= MTYPE_TEXT;
	g_arenaservers.status.generic.x			= 320;
	g_arenaservers.status.generic.y			= y;
	g_arenaservers.status.string			= statusbuffer;
	g_arenaservers.status.style				= FONT_CENTER;
	g_arenaservers.status.color				= colorMenuText;

	y += SMALLCHAR_SIZE;
	g_arenaservers.statusbar.generic.type   = MTYPE_TEXT;
	g_arenaservers.statusbar.generic.x	    = 320;
	g_arenaservers.statusbar.generic.y	    = y;
	g_arenaservers.statusbar.string	        = "";
	g_arenaservers.statusbar.style	        = FONT_CENTER;
	g_arenaservers.statusbar.color	        = colorTextNormal;

	g_arenaservers.back.generic.type		= MTYPE_BUTTON;
	g_arenaservers.back.generic.flags		= QMF_LEFT_JUSTIFY;
	g_arenaservers.back.generic.callback	= ArenaServers_Event;
	g_arenaservers.back.generic.id			= ID_BACK;
	g_arenaservers.back.generic.x			= 0;
	g_arenaservers.back.generic.y			= 480-64;
	g_arenaservers.back.string				= "Back";

	g_arenaservers.specify.generic.type		= MTYPE_BUTTON;
	g_arenaservers.specify.generic.flags	= QMF_LEFT_JUSTIFY;
	g_arenaservers.specify.generic.callback	= ArenaServers_Event;
	g_arenaservers.specify.generic.id		= ID_SPECIFY;
	g_arenaservers.specify.generic.x		= 128;
	g_arenaservers.specify.generic.y		= 480-64;
	g_arenaservers.specify.string			= "Specify";

	g_arenaservers.refresh.generic.type		= MTYPE_BUTTON;
	g_arenaservers.refresh.generic.flags	= QMF_LEFT_JUSTIFY;
	g_arenaservers.refresh.generic.callback	= ArenaServers_Event;
	g_arenaservers.refresh.generic.id		= ID_REFRESH;
	g_arenaservers.refresh.generic.x		= 256;
	g_arenaservers.refresh.generic.y		= 480-64;
	g_arenaservers.refresh.string			= "Refresh";

	g_arenaservers.go.generic.type			= MTYPE_BUTTON;
	g_arenaservers.go.generic.flags			= QMF_RIGHT_JUSTIFY;
	g_arenaservers.go.generic.callback		= ArenaServers_Event;
	g_arenaservers.go.generic.id			= ID_CONNECT;
	g_arenaservers.go.generic.x				= 640;
	g_arenaservers.go.generic.y				= 480-64;
	g_arenaservers.go.string				= "Connect";

	Menu_AddItem(&g_arenaservers.menu, (void*) &g_arenaservers.banner);

	Menu_AddItem(&g_arenaservers.menu, (void*) &g_arenaservers.filter);
	Menu_AddItem(&g_arenaservers.menu, (void*) &g_arenaservers.showfull);
	Menu_AddItem(&g_arenaservers.menu, (void*) &g_arenaservers.showempty);

	Menu_AddItem(&g_arenaservers.menu, (void*) &g_arenaservers.labelHost);
	Menu_AddItem(&g_arenaservers.menu, (void*) &g_arenaservers.labelMap);
	Menu_AddItem(&g_arenaservers.menu, (void*) &g_arenaservers.labelPing);
	Menu_AddItem(&g_arenaservers.menu, (void*) &g_arenaservers.labelType);
	Menu_AddItem(&g_arenaservers.menu, (void*) &g_arenaservers.labelPlayers);

	Menu_AddItem(&g_arenaservers.menu, (void*) &g_arenaservers.mappic);
	Menu_AddItem(&g_arenaservers.menu, (void*) &g_arenaservers.status);
	Menu_AddItem(&g_arenaservers.menu, (void*) &g_arenaservers.statusbar);
	Menu_AddItem(&g_arenaservers.menu, (void*) &g_arenaservers.arrows);
	Menu_AddItem(&g_arenaservers.menu, (void*) &g_arenaservers.up);
	Menu_AddItem(&g_arenaservers.menu, (void*) &g_arenaservers.down);
	Menu_AddItem(&g_arenaservers.menu, (void*) &g_arenaservers.list);

	Menu_AddItem(&g_arenaservers.menu, (void*) &g_arenaservers.back);
	Menu_AddItem(&g_arenaservers.menu, (void*) &g_arenaservers.specify);
	Menu_AddItem(&g_arenaservers.menu, (void*) &g_arenaservers.refresh);
	Menu_AddItem(&g_arenaservers.menu, (void*) &g_arenaservers.go);

	ArenaServers_LoadFavorites();

	g_arenaservers.master.curvalue = g_servertype = Com_Clamp(0, 6, ui_browserMaster.integer);

	g_gametype = Com_Clamp(0, 4, ui_browserGameType.integer);
	g_arenaservers.gametype.curvalue = g_gametype;

	g_fullservers = Com_Clamp(0, 1, ui_browserShowFull.integer);
	g_arenaservers.showfull.curvalue = g_fullservers;

	g_emptyservers = Com_Clamp(0, 1, ui_browserShowEmpty.integer);
	g_arenaservers.showempty.curvalue = g_emptyservers;

	// force to initial state and refresh
	g_arenaservers.master.curvalue = g_servertype = ArenaServers_SetType(g_servertype);

	trap_Cvar_Register(NULL, "debug_protocol", "", 0);
}

void ArenaServers_Cache(void)
{
	trap_R_RegisterShaderNoMip(ART_ARROWS0);
	trap_R_RegisterShaderNoMip(ART_ARROWS_UP);
	trap_R_RegisterShaderNoMip(ART_ARROWS_DOWN);
	trap_R_RegisterShaderNoMip(ART_UNKNOWNMAP);
}

void UI_ArenaServersMenu(void)
{
	ArenaServers_MenuInit();
	UI_PushMenu(&g_arenaservers.menu);
}


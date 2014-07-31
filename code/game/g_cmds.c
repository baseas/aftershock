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
// g_cmds.c

#include "g_local.h"

static gitem_t *FindArmorForQuantity(int quantity)
{
	gitem_t	*it;

	for (it = bg_itemlist + 1; it->classname; it++) {
		if (it->giType == IT_ARMOR && it->quantity == quantity) {
			return it;
		}
	}

	Com_Error(ERR_DROP, "Couldn't find armor item for quantity %i", quantity);
	return NULL;
}

static gitem_t *FindHealthForQuantity(int quantity)
{
	gitem_t	*it;

	for (it = bg_itemlist + 1; it->classname; it++) {
		if (it->giType == IT_HEALTH && it->quantity == quantity) {
			return it;
		}
	}

	Com_Error(ERR_DROP, "Couldn't find health item for quantity %i", quantity);
	return NULL;
}

static gitem_t *FindAmmoForWeapon(weapon_t weapon)
{
	gitem_t	*it;

	for (it = bg_itemlist + 1; it->classname; it++) {
		if (it->giType == IT_AMMO && it->giTag == weapon) {
			return it;
		}
	}

	Com_Error(ERR_DROP, "Couldn't find item for weapon %i", weapon);
	return NULL;
}

qboolean CheatsOk(gentity_t *ent)
{
	if (!g_cheats.integer) {
		ClientPrint(ent, "Cheats are not enabled on this server.");
		return qfalse;
	}
	if (ent->health <= 0) {
		ClientPrint(ent, "You must be alive to use this command.");
		return qfalse;
	}
	return qtrue;
}

char *ConcatArgs(int start)
{
	int			i, c, tlen;
	static char	line[MAX_STRING_CHARS];
	int			len;
	const char	*arg;

	len = 0;
	c = trap_Argc();
	for (i = start; i < c; i++) {
		arg = BG_Argv(i);
		tlen = strlen(arg);
		if (len + tlen >= MAX_STRING_CHARS - 1) {
			break;
		}
		memcpy(line + len, arg, tlen);
		len += tlen;
		if (i != c - 1) {
			line[len] = ' ';
			len++;
		}
	}

	line[len] = 0;

	return line;
}

/**
Give items to a client
*/
void Cmd_Give_f(gentity_t *ent)
{
	char		*name;
	gitem_t		*it;
	int			i;
	qboolean	give_all;
	gentity_t		*it_ent;
	trace_t		trace;

	if (!CheatsOk(ent)) {
		return;
	}

	name = ConcatArgs(1);

	if (Q_stricmp(name, "all") == 0) {
		give_all = qtrue;
	} else {
		give_all = qfalse;
	}

	if (give_all || Q_stricmp(name, "health") == 0) {
		ent->health = ent->client->ps.stats[STAT_MAX_HEALTH];
		if (!give_all) {
			return;
		}
	}

	if (give_all || Q_stricmp(name, "weapons") == 0) {
		ent->client->ps.stats[STAT_WEAPONS] = (1 << WP_NUM_WEAPONS) - 1 -
			(1 << WP_GRAPPLING_HOOK) - (1 << WP_NONE);
		if (!give_all) {
			return;
		}
	}

	if (give_all || Q_stricmp(name, "ammo") == 0) {
		for (i = 0; i < MAX_WEAPONS; i++) {
			ent->client->ps.ammo[i] = -1;
		}
		if (!give_all) {
			return;
		}
	}

	if (give_all || Q_stricmp(name, "armor") == 0) {
		ent->client->ps.stats[STAT_ARMOR] = 200;

		if (!give_all) {
			return;
		}
	}

	// spawn a specific item right on the player
	if (!give_all) {
		it = BG_FindItem (name);
		if (!it) {
			return;
		}

		it_ent = G_Spawn();
		VectorCopy(ent->r.currentOrigin, it_ent->s.origin);
		it_ent->classname = it->classname;
		G_SpawnItem (it_ent, it);
		FinishSpawningItem(it_ent);
		memset(&trace, 0, sizeof(trace));
		Touch_Item (it_ent, ent, &trace);
		if (it_ent->inuse) {
			G_FreeEntity(it_ent);
		}
	}
}

/**
Sets client to godmode

argv(0) god
*/
void Cmd_God_f(gentity_t *ent)
{
	if (!CheatsOk(ent)) {
		return;
	}

	ent->flags ^= FL_GODMODE;
	if (!(ent->flags & FL_GODMODE)) {
		ClientPrint(ent, "godmode OFF");
	} else {
		ClientPrint(ent, "godmode ON");
	}
}

/**
Sets client to notarget

argv(0) notarget
*/
void Cmd_Notarget_f(gentity_t *ent)
{
	if (!CheatsOk(ent)) {
		return;
	}

	ent->flags ^= FL_NOTARGET;
	if (!(ent->flags & FL_NOTARGET)) {
		ClientPrint(ent, "notarget OFF");
	} else {
		ClientPrint(ent, "notarget ON");
	}
}

/**
argv(0) noclip
*/
void Cmd_Noclip_f(gentity_t *ent)
{
	if (!CheatsOk(ent)) {
		return;
	}

	if (ent->client->noclip) {
		ClientPrint(ent, "noclip OFF");
	} else {
		ClientPrint(ent, "noclip ON");
	}
	ent->client->noclip = !ent->client->noclip;
}

/**
This is just to help generate the level pictures
for the menus.  It goes to the intermission immediately
and sends over a command to the client to resize the view,
hide the scoreboard, and take a special screenshot
*/
void Cmd_LevelShot_f(gentity_t *ent)
{
	if (!ent->client->pers.localClient) {
		ClientPrint(ent, "The levelshot command must be executed by a local client.");
		return;
	}

	if (!CheatsOk(ent)) {
		return;
	}

	BeginIntermission();
	trap_SendServerCommand(ent - g_entities, "clientLevelShot");
}

void Cmd_TeamTask_f(gentity_t *ent)
{
	char userinfo[MAX_INFO_STRING];
	int task;
	int client = ent->client - level.clients;

	if (trap_Argc() != 2) {
		return;
	}
	task = atoi(BG_Argv(1));

	trap_GetUserinfo(client, userinfo, sizeof(userinfo));
	Info_SetValueForKey(userinfo, "teamtask", va("%d", task));
	trap_SetUserinfo(client, userinfo);
	ClientUserinfoChanged(client);
}

void Cmd_Kill_f(gentity_t *ent)
{
	if (ent->client->sess.sessionTeam == TEAM_SPECTATOR) {
		return;
	}

	// avoid kill by accident just after round start
	if (g_gametype.integer == GT_ELIMINATION
		&& level.roundStarted && level.time < level.roundStartTime + 1000)
	{
		return;
	}

	if (ent->health <= 0) {
		return;
	}
	ent->flags &= ~FL_GODMODE;
	ent->client->ps.stats[STAT_HEALTH] = ent->health = -999;
	player_die(ent, ent, ent, 100000, MOD_SUICIDE);
}

void LogTeamChange(gclient_t *client, int oldTeam)
{
	if (client->sess.sessionTeam == TEAM_RED) {
		G_LogPrintf("%s ^7joined the red team.\n", client->pers.netname);
	} else if (client->sess.sessionTeam == TEAM_BLUE) {
		G_LogPrintf("%s ^7joined the blue team.\n", client->pers.netname);
	} else if (client->sess.sessionTeam == TEAM_SPECTATOR && oldTeam != TEAM_SPECTATOR) {
		G_LogPrintf("%s ^7joined the spectators.\n", client->pers.netname);
	} else if (client->sess.sessionTeam == TEAM_FREE) {
		G_LogPrintf("%s ^7joined the battle.\n", client->pers.netname);
	}
}

void G_Pause(int start, int end)
{
	if (!start) {
		level.pauseTime = level.totalTime - level.time;
		trap_SetConfigstring(CS_PAUSE_TIME, va("%d", level.pauseTime));
	}

	if (!end && g_timeinDelay.integer) {
		end = level.totalTime + 3500;
	}

	level.pauseStart = start;
	level.pauseEnd = end;
	trap_SetConfigstring(CS_PAUSE_START, va("%d", start));
	trap_SetConfigstring(CS_PAUSE_END, va("%d", end));
}

void SetTeam(gentity_t *ent, const char *s)
{
	int					team, oldTeam;
	gclient_t			*client;
	int					clientNum;
	spectatorState_t	specState;
	int					specClient;
	int					teamLeader;

	//
	// see what change is requested
	//
	client = ent->client;

	clientNum = client - level.clients;
	specClient = 0;
	specState = SPECTATOR_NOT;
	if (!Q_stricmp(s, "speconly") || !Q_stricmp(s, "so")) {
		team = TEAM_SPECTATOR;
		specState = SPECTATOR_FREE;
		client->sess.specOnly = (g_gametype.integer == GT_TOURNAMENT);
	} else if (!Q_stricmp(s, "scoreboard") || !Q_stricmp(s, "score")) {
		team = TEAM_SPECTATOR;
		specState = SPECTATOR_SCOREBOARD;
		client->sess.specOnly = qtrue;
	} else if (!Q_stricmp(s, "follow1")) {
		team = TEAM_SPECTATOR;
		specState = SPECTATOR_FOLLOW;
		specClient = -1;
	} else if (!Q_stricmp(s, "follow2")) {
		team = TEAM_SPECTATOR;
		specState = SPECTATOR_FOLLOW;
		specClient = -2;
	} else if (!Q_stricmp(s, "spectator") || !Q_stricmp(s, "s")) {
		team = TEAM_SPECTATOR;
		specState = SPECTATOR_FREE;
		client->sess.specOnly = qfalse;
	} else if (g_gametype.integer >= GT_TEAM) {
		// if running a team game, assign player to one of the teams
		client->sess.specOnly = qfalse;
		if (!Q_stricmp(s, "red") || !Q_stricmp(s, "r")) {
			team = TEAM_RED;
		} else if (!Q_stricmp(s, "blue") || !Q_stricmp(s, "b")) {
			team = TEAM_BLUE;
		} else if (!Q_stricmp(s, "free") || !Q_stricmp(s, "f")) {
			// pick the team with the least number of players
			team = PickTeam(clientNum);
			if (team == TEAM_SPECTATOR) {
				return;
			}
		} else {
			return;
		}

		if (g_teamForceBalance.integer) {
			int		counts[TEAM_NUM_TEAMS];

			counts[TEAM_BLUE] = TeamCount(clientNum, TEAM_BLUE);
			counts[TEAM_RED] = TeamCount(clientNum, TEAM_RED);

			// We allow a spread of two
			if (team == TEAM_RED && counts[TEAM_RED] - counts[TEAM_BLUE] > 1) {
				trap_SendServerCommand(clientNum,
					"cp \"Red team has too many players.\n\"");
				return; // ignore the request
			}
			if (team == TEAM_BLUE && counts[TEAM_BLUE] - counts[TEAM_RED] > 1) {
				trap_SendServerCommand(clientNum,
					"cp \"Blue team has too many players.\n\"");
				return; // ignore the request
			}

			// It's ok, the team we are switching to has less or same number of players
		}

	} else if (!Q_stricmp(s, "free") || !Q_stricmp(s, "f")) {
		team = TEAM_FREE;
		client->sess.specOnly = qfalse;
	} else {
		return;
	}

	// override decision if limiting the players
	if ((g_gametype.integer == GT_TOURNAMENT)
		&& level.numNonSpectatorClients >= 2) {
		team = TEAM_SPECTATOR;
	} else if (g_maxGameClients.integer > 0 &&
		level.numNonSpectatorClients >= g_maxGameClients.integer) {
		team = TEAM_SPECTATOR;
	}

	//
	// decide if we will allow the change
	//
	oldTeam = client->sess.sessionTeam;
	if (team == oldTeam && team != TEAM_SPECTATOR) {
		return;
	}

	//
	// execute the team change
	//

	// if the player was dead leave the body
	if (client->ps.stats[STAT_HEALTH] <= 0) {
		CopyToBodyQue(ent);
	}

	// he starts at 'base'
	if (oldTeam != TEAM_SPECTATOR) {
		// Kill him (makes sure he loses flags, etc)
		ent->flags &= ~FL_GODMODE;
		ent->client->ps.stats[STAT_HEALTH] = ent->health = 0;
		player_die(ent, ent, ent, 100000, MOD_SUICIDE);

	}

	// they go to the end of the line for tournements
	if (team == TEAM_SPECTATOR && oldTeam != team)
		AddTournamentQueue(client);

	client->sess.sessionTeam = team;
	client->sess.spectatorState = specState;
	client->sess.spectatorClient = specClient;
	client->sess.fullStatsSent = qfalse;

	client->sess.teamLeader = qfalse;
	if (team == TEAM_RED || team == TEAM_BLUE) {
		teamLeader = TeamLeader(team);
		// if there is no team leader or the team leader is a bot and this client is not a bot
		if (teamLeader == -1 || (!(g_entities[clientNum].r.svFlags & SVF_BOT) && (g_entities[teamLeader].r.svFlags & SVF_BOT))) {
			SetLeader(team, clientNum);
		}
	}
	// make sure there is a team leader on the team the player came from
	if (oldTeam == TEAM_RED || oldTeam == TEAM_BLUE) {
		CheckTeamLeader(oldTeam);
	}

	LogTeamChange(client, oldTeam);

	// get and distribute relevent paramters
	ClientUserinfoChanged(clientNum);

	ClientBegin(clientNum);
}

/**
If the client being followed leaves the game, or you just want to drop
to free floating spectator mode
*/
void StopFollowing(gentity_t *ent)
{
	ent->client->sess.spectatorState = SPECTATOR_FREE;
	ent->client->ps.pm_flags &= ~PMF_FOLLOW;
	ent->r.svFlags &= ~SVF_BOT;
	ent->client->ps.clientNum = ent - g_entities;
}

void Cmd_Team_f(gentity_t *ent)
{
	int			oldTeam;

	oldTeam = ent->client->sess.sessionTeam;
	if (trap_Argc() != 2) {
		switch (oldTeam) {
		case TEAM_BLUE:
			ClientPrint(ent, "Blue team");
			break;
		case TEAM_RED:
			ClientPrint(ent, "Red team");
			break;
		case TEAM_FREE:
			ClientPrint(ent, "Free team");
			break;
		case TEAM_SPECTATOR:
			if (ent->client->sess.specOnly) {
				ClientPrint(ent, "SpecOnly team");
			} else {
				ClientPrint(ent, "Spectator team");
			}
			break;
		}
		return;
	}

	if (ent->client->switchTeamTime > level.time) {
		ClientPrint(ent, "May not switch teams more than once per 5 seconds.");
		return;
	}

	// if they are playing a tournement game, count as a loss
	if ((g_gametype.integer == GT_TOURNAMENT)
		&& ent->client->sess.sessionTeam == TEAM_FREE) {
		ent->client->sess.losses++;
	}

	SetTeam(ent, BG_Argv(1));

	if (ent->client->sess.sessionTeam != oldTeam) {
		ent->client->switchTeamTime = level.time + 5000;
	}
}

void Cmd_Follow_f(gentity_t *ent)
{
	gclient_t	*cl;

	if (trap_Argc() != 2) {
		if (ent->client->sess.spectatorState == SPECTATOR_FOLLOW) {
			StopFollowing(ent);
		}
		return;
	}

	cl = ClientFromString(BG_Argv(1));
	if (!cl) {
		ClientPrint(ent, "Player not found.");
		return;
	}

	// can't follow self
	if (cl == ent->client) {
		return;
	}

	// can't follow another spectator
	if (cl->sess.sessionTeam == TEAM_SPECTATOR) {
		return;
	}

	// can't follow eliminated player
	if (level.roundStarted && cl->eliminated) {
		return;
	}

	// if they are playing a tournement game, count as a loss
	if ((g_gametype.integer == GT_TOURNAMENT)
		&& ent->client->sess.sessionTeam == TEAM_FREE) {
		ent->client->sess.losses++;
	}

	// first set them to spectator
	if (ent->client->sess.spectatorState == SPECTATOR_NOT) {
		SetTeam(ent, "spectator");
	}

	ent->client->sess.spectatorState = SPECTATOR_FOLLOW;
	ent->client->sess.spectatorClient = cl - level.clients;
	ent->client->sess.fullStatsSent = qfalse;
}

void Cmd_FollowCycle_f(gentity_t *ent)
{
	int		dir;
	int		clientnum;
	int		original;

	dir = (strcmp(BG_Argv(0), "followprev") ? 1 : -1);

	// if they are playing a tournement game, count as a loss
	if ((g_gametype.integer == GT_TOURNAMENT)
		&& ent->client->sess.sessionTeam == TEAM_FREE) {
		ent->client->sess.losses++;
	}
	// first set them to spectator
	if (ent->client->sess.spectatorState == SPECTATOR_NOT) {
		SetTeam(ent, "spectator");
	}

	if (dir != 1 && dir != -1) {
		G_Error("Cmd_FollowCycle_f: bad dir %i", dir);
	}

	// if dedicated follow client, just switch between the two auto clients
	if (ent->client->sess.spectatorClient < 0) {
		if (ent->client->sess.spectatorClient == -1) {
			ent->client->sess.spectatorClient = -2;
		} else if (ent->client->sess.spectatorClient == -2) {
			ent->client->sess.spectatorClient = -1;
		}
		ent->client->sess.fullStatsSent = qfalse;
		return;
	}

	clientnum = ent->client->sess.spectatorClient;
	original = clientnum;
	do {
		clientnum += dir;
		if (clientnum >= level.maxclients) {
			clientnum = 0;
		}
		if (clientnum < 0) {
			clientnum = level.maxclients - 1;
		}

		// can only follow connected clients
		if (level.clients[ clientnum ].pers.connected != CON_CONNECTED) {
			continue;
		}

		// can't follow another spectator
		if (level.clients[ clientnum ].sess.spectatorState != SPECTATOR_NOT) {
			continue;
		}

		// stop players from spectating players on the enemy team in elimination
		if (g_gametype.integer == GT_ELIMINATION && ent->client->sess.sessionTeam != TEAM_SPECTATOR
			&& ent->client->sess.sessionTeam != level.clients[clientnum].sess.sessionTeam)
		{
			continue;
		}

		// this is good, we can use it
		ent->client->sess.spectatorClient = clientnum;
		ent->client->sess.spectatorState = SPECTATOR_FOLLOW;
		ent->client->sess.fullStatsSent = qfalse;
		return;
	} while (clientnum != original);

	// leave it where it was
}

static void G_SayTo(gentity_t *ent, gentity_t *other, int mode, int color, const char *name, const char *message)
{
	if (!other) {
		return;
	}
	if (!other->inuse) {
		return;
	}
	if (!other->client) {
		return;
	}
	if (other->client->pers.connected != CON_CONNECTED) {
		return;
	}
	if (other->client->pers.blocklist[ent - g_entities]) {
		return;
	}
	if (mode == SAY_TEAM  && !OnSameTeam(ent, other)) {
		return;
	}
	// no chatting to players in tournements
	if ((g_gametype.integer == GT_TOURNAMENT)
		&& other->client->sess.sessionTeam == TEAM_FREE
		&& ent->client->sess.sessionTeam != TEAM_FREE) {
		return;
	}

	trap_SendServerCommand(other-g_entities, va("%s \"%s%c%c%s\"",
		mode == SAY_TEAM ? "tchat" : "chat",
		name, Q_COLOR_ESCAPE, color, message));
}

static int G_ChatToken(gentity_t *ent, char *text, int length, char letter)
{
	int		intval;
	char	loc[MAX_SAY_TEXT];
	int			i, written;
	qboolean	first;

	text[0] = '\0';

	switch (letter) {
	case 'A': // colored armor
		intval = ent->client->ps.stats[STAT_ARMOR];
		if (intval < 50) {
			Com_sprintf(text, length, "^1%i", intval);
		} else if (intval < 100) {
			Com_sprintf(text, length, "^2%i", intval);
		} else {
			Com_sprintf(text, length, "^3%i", intval);
		}
		break;
	case 'H': // colored health
		intval = ent->client->ps.stats[STAT_HEALTH];
		if (intval < 50) {
			Com_sprintf(text, length, "^1%i", intval);
		} else if (intval < 100) {
			Com_sprintf(text, length, "^2%i", intval);
		} else {
			Com_sprintf(text, length, "^3%i", intval);
		}
		break;
	case 'a': // white armor
		Com_sprintf(text, length, "%i", ent->client->ps.stats[STAT_ARMOR]);
		break;
	case 'h': // white health
		Com_sprintf(text, length, "%i", ent->client->ps.stats[STAT_HEALTH]);
		break;
	case 'K': // killer name
		if (ent->client->pers.lastKiller) {
			Com_sprintf(text, length, "%s^7", ent->client->pers.lastKiller->pers.netname);
		} else {
			Q_strncpyz(text, "<nobody>", length);
		}
		break;
	case 'T': // target name
		if (ent->client->pers.lastTarget) {
			Com_sprintf(text, length, "%s^7", ent->client->pers.lastTarget->pers.netname);
		} else {
			Q_strncpyz(text, "<nobody>", length);
		}
		break;
	case 'D': // attacker name
		intval = ent->client->ps.persistant[PERS_ATTACKER];
		if (g_entities[intval].client) {
			Com_sprintf(text, length, "%s^7", g_entities[intval].client->pers.netname);
		} else {
			Q_strncpyz(text, "<nobody>", length);
		}
		break;
	case 'd': // last drop
		if (ent->client->pers.lastDrop) {
			Com_sprintf(text, length, "%s", ent->client->pers.lastDrop->pickup_name);
		} else {
			Q_strncpyz(text, "<nothing>", length);
		}
		break;
	case 'P': // last pickup
		if (ent->client->pers.lastPickup) {
			Com_sprintf(text, length, "%s", ent->client->pers.lastPickup->pickup_name);
		} else {
			Q_strncpyz(text, "<nothing>", length);
		}
		break;
	case 'U': // list of player's powerups
		first = qtrue;
		written = 0;
		for (i = 0; i < bg_numItems; ++i) {
			if (bg_itemlist[i].giType != IT_POWERUP) {
				continue;
			}
			written += Com_sprintf(text, length - written, text, bg_itemlist[i].pickup_name);
			if (!first) {
				written += Com_sprintf(text + written, length - written, ", ");
			}
			first = qfalse;
		}
		break;
	case 'L': // current location
		Team_GetLocationMsg(ent->r.currentOrigin, loc, sizeof loc);
		Q_strncpyz(text, loc, length);
		break;
	case 'C': // death location
		Team_GetLocationMsg(ent->client->pers.lastDeathOrigin, loc, sizeof loc);
		Q_strncpyz(text, loc, length);
		break;
	default:
		return 1;
	}

	return 0;
}

#define EC		"\x19"

void G_Say(gentity_t *ent, gentity_t *target, int mode, const char *chatText)
{
	int			j, k;
	gentity_t	*other;
	int			color;
	char		name[64];
	// don't let text be too long for malicious reasons
	char		text[MAX_SAY_TEXT];
	char		location[64];

	if (g_gametype.integer < GT_TEAM && mode == SAY_TEAM) {
		mode = SAY_ALL;
	}

	switch (mode) {
	default:
	case SAY_ALL:
		G_LogPrintf("say: %s: %s\n", ent->client->pers.netname, chatText);
		Com_sprintf(name, sizeof(name), "%s%c%c"EC": ", ent->client->pers.netname, Q_COLOR_ESCAPE, COLOR_WHITE);
		color = COLOR_GREEN;
		break;
	case SAY_TEAM:
		G_LogPrintf("sayteam: %s: %s\n", ent->client->pers.netname, chatText);
		if (Team_GetLocationMsg(ent->r.currentOrigin, location, sizeof location))
		{
			Com_sprintf(name, sizeof(name), EC"(%s%c%c"EC") (%s)"EC": ",
				ent->client->pers.netname, Q_COLOR_ESCAPE, COLOR_WHITE, location);
		} else {
			Com_sprintf(name, sizeof(name), EC"(%s%c%c"EC")"EC": ",
				ent->client->pers.netname, Q_COLOR_ESCAPE, COLOR_WHITE);
		}
		color = COLOR_CYAN;
		break;
	case SAY_TELL:
		if (target && target->inuse && target->client && g_gametype.integer >= GT_TEAM &&
			target->client->sess.sessionTeam == ent->client->sess.sessionTeam &&
			Team_GetLocationMsg(ent->r.currentOrigin, location, sizeof location))
		{
			Com_sprintf(name, sizeof(name), EC"[%s%c%c"EC"] (%s)"EC": ", ent->client->pers.netname, Q_COLOR_ESCAPE, COLOR_WHITE, location);
		} else {
			Com_sprintf(name, sizeof(name), EC"[%s%c%c"EC"]"EC": ", ent->client->pers.netname, Q_COLOR_ESCAPE, COLOR_WHITE);
		}
		color = COLOR_MAGENTA;
		break;
	}

	if (mode != SAY_TEAM || ent->client->ps.persistant[PERS_TEAM] == TEAM_SPECTATOR) {
		Q_strncpyz(text, chatText, sizeof text);
	} else {
		for (j = 0, k = 0; k < sizeof text && chatText[j]; ++j) {
			char	token[MAX_SAY_TEXT];

			if (chatText[j] != '#' || !chatText[j + 1]) {
				text[k++] = chatText[j];
				continue;
			}

			if (!G_ChatToken(ent, token, sizeof token, chatText[j + 1])) {
				k += Com_sprintf(&text[k], sizeof text - k, "%s", token);
				++j;
			} else {
				text[k++] = '#';
			}
		}

		text[k] = '\0';
	}

	if (target) {
		G_SayTo(ent, target, mode, color, name, text);
		return;
	}

	// echo the text to the console
	if (g_dedicated.integer) {
		G_Printf("%s%s\n", name, text);
	}

	// send it to all the apropriate clients
	for (j = 0; j < level.maxclients; j++) {
		other = &g_entities[j];
		G_SayTo(ent, other, mode, color, name, text);
	}
}

static void Cmd_Say_f(gentity_t *ent, int mode, qboolean arg0)
{
	char		*p;

	if (ent->client->pers.muted) {
		ClientPrint(ent, "You are muted.");
		return;
	}

	if (trap_Argc() < 2 && !arg0) {
		return;
	}

	if (arg0) {
		p = ConcatArgs(0);
	}
	else {
		p = ConcatArgs(1);
	}

	G_Say(ent, NULL, mode, p);
}

static void Cmd_Tell_f(gentity_t *ent)
{
	gclient_t	*cl;
	gentity_t	*target;
	char		*p;

	if (trap_Argc() < 3) {
		ClientPrint(ent, "Usage: tell <player> <message>");
		return;
	}

	cl = ClientFromString(BG_Argv(1));
	if (!cl) {
		ClientPrint(ent, "Player not found.");
		return;
	}

	target = &g_entities[cl - level.clients];
	if (!target->inuse || !target->client) {
		return;
	}

	p = ConcatArgs(2);

	G_LogPrintf("tell: %s to %s: %s\n", ent->client->pers.netname, target->client->pers.netname, p);
	G_Say(ent, target, SAY_TELL, p);
	// don't tell to the player self if it was already directed to this player
	// also don't send the chat back to a bot
	if (ent != target && !(ent->r.svFlags & SVF_BOT)) {
		G_Say(ent, ent, SAY_TELL, p);
	}
}

static void Cmd_Where_f(gentity_t *ent)
{
	ClientPrint(ent, "%s", vtos(ent->r.currentOrigin));
}

static void Cmd_CallVote_f(gentity_t *ent)
{
	int			i;
	const char	*arg;
	char		callString[MAX_STRING_CHARS];

	if (!g_allowVote.integer) {
		ClientPrint(ent, "Voting not allowed here.");
		return;
	}
	if (ent->client->pers.muted) {
		ClientPrint(ent, "You are muted.");
		return;
	}
	if (level.voteTime) {
		ClientPrint(ent, "A vote is already in progress.");
		return;
	}
	if (ent->client->pers.voteCount >= MAX_VOTE_COUNT) {
		ClientPrint(ent, "You have called the maximum number of votes.");
		return;
	}
	if (ent->client->sess.sessionTeam == TEAM_SPECTATOR) {
		ClientPrint(ent, "Not allowed to call a vote as spectator.");
		return;
	}

	if (G_VoteCall(ent)) {
		return;
	}

	// reconstruct the vote string and send it to clients
	Com_sprintf(callString, sizeof callString, "%s called a vote:", G_UserName(ent));
	arg = BG_Argv(1);
	i = 2;
	do {
		Q_strcat(callString, sizeof callString, " ");
		Q_strcat(callString, sizeof callString, arg);
	} while (*(arg = BG_Argv(i++)));
	Q_strcat(callString, sizeof callString, ".");
	ClientPrint(NULL, "%s", callString);

	// start the voting, the caller automatically votes yes
	level.voteTime = level.time;
	for (i = 0; i < level.maxclients; i++) {
		level.clients[i].pers.vote = 0;
	}
	ent->client->pers.vote = 1;

	G_VoteUpdateCount();

	// vote passed if there is only one client
	G_VoteCheck();

	// else give others time to vote
	if (level.voteTime) {
		trap_SetConfigstring(CS_VOTE_STRING, level.voteDisplay);
		trap_SetConfigstring(CS_VOTE_TIME, va("%i", level.voteTime));
	} else {
		trap_SetConfigstring(CS_VOTE_TIME, "");
	}
}

static void Cmd_Vote_f(gentity_t *ent)
{
	char		msg[1];

	if (!level.voteTime) {
		ClientPrint(ent, "No vote in progress.");
		return;
	}
	if (ent->client->pers.vote && level.time - ent->client->pers.voteTime < 5000) {
		ClientPrint(ent, "May not vote more than once per 5 seconds.");
		return;
	}
	if (ent->client->sess.sessionTeam == TEAM_SPECTATOR) {
		ClientPrint(ent, "Not allowed to vote as spectator.");
		return;
	}

	trap_Argv(1, msg, sizeof msg);

	if (tolower(msg[0]) == 'y' || msg[0] == '1') {
		ent->client->pers.vote = 1;
	} else if (msg[0] == 'n' || msg[0] == '0') {
		ent->client->pers.vote = -1;
	} else {
		return;
	}

	ent->client->pers.voteTime = level.time;

	// a majority will be determined in CheckVote, which will also account
	// for players entering or leaving
	G_VoteUpdateCount();
}

static void Cmd_SetViewpos_f(gentity_t *ent)
{
	vec3_t		origin, angles;
	int			i;

	if (!g_cheats.integer) {
		ClientPrint(ent, "Cheats are not enabled on this server.");
		return;
	}
	if (trap_Argc() != 5) {
		ClientPrint(ent, "Usage: setviewpos x y z yaw");
		return;
	}

	VectorClear(angles);
	for (i = 0; i < 3; i++) {
		origin[i] = atof(BG_Argv(i + 1));
	}

	angles[YAW] = atof(BG_Argv(4));

	TeleportPlayer(ent, origin, angles);
}

static void Cmd_DropArmor_f(gentity_t *ent)
{
	gitem_t	*item;
	int		amount;

	if (g_gametype.integer == GT_DEFRAG) {
		ClientPrint(ent, "Item drop is not available in DeFRaG.");
		return;
	}

	if (!(g_allowDrop.integer & 16) || g_instantgib.integer) {
		return;
	}

	if (ent->client->ps.pm_type == PM_DEAD) {
		return;
	}

	if (trap_Argc() > 1) {
		amount = atoi(BG_Argv(1));
    } else {
		amount = 50;
	}

	if (amount >= 100) {
		amount = 100;
	} else if (amount >= 50) {
		amount = 50;
	} else if (amount >= 25) {
		amount = 25;
	} else {
		amount = 5;
	}

	item = FindArmorForQuantity(amount);
	Drop_Item_Armor(ent, item);
}

static void Cmd_DropHealth_f(gentity_t *ent)
{
	gitem_t	*item;
	int		amount;

	if (g_gametype.integer == GT_DEFRAG) {
		ClientPrint(ent, "Item drop is not available in DeFRaG.");
		return;
	}

	if (!(g_allowDrop.integer & 8) || g_instantgib.integer) {
		return;
	}

	if (ent->client->ps.pm_type == PM_DEAD) {
		return;
	}

	if (trap_Argc() > 1) {
		amount = atoi(BG_Argv(1));
	} else {
		amount = 25;
	}

	if (amount >= 100) {
		amount = 100;
	} else if (amount >= 50) {
		amount = 50;
	} else if (amount >= 25) {
		amount = 25;
	} else {
		amount = 5;
	}

	item = FindHealthForQuantity(amount);
	Drop_Item_Health(ent, item);
}

static void Cmd_DropAmmo_f(gentity_t *ent)
{
	gitem_t	*item;
	int		weapon;

	if (g_gametype.integer == GT_DEFRAG) {
		ClientPrint(ent, "Item drop is not available in DeFRaG.");
		return;
	}

	if (!(g_allowDrop.integer & 4) || g_instantgib.integer || g_rockets.integer) {
		return;
	}

	if (ent->client->ps.pm_type == PM_DEAD) {
		return;
	}

	if (trap_Argc() > 1) {
		weapon = atoi(BG_Argv(1));
	} else {
		weapon = ent->s.weapon;
	}

	if (weapon <= WP_GAUNTLET || weapon >= WP_NUM_WEAPONS) {
		return;
	}

	item = FindAmmoForWeapon(weapon);
	if ((ent->client->ps.stats[STAT_WEAPONS] & (1 << item->giTag))) {
		Drop_Item_Ammo(ent, item);
	}
}

static void Cmd_DropWeapon_f(gentity_t *ent)
{
	gitem_t	*item;
	int		weapon;

	if (g_gametype.integer == GT_DEFRAG) {
		ClientPrint(ent, "Item drop is not available in DeFRaG.");
		return;
	}

	if (!(g_allowDrop.integer & 2) || g_instantgib.integer || g_rockets.integer) {
		return;
	}

	if (ent->client->ps.pm_type == PM_DEAD) {
		return;
	}

	if (trap_Argc() > 1) {
		weapon = atoi(BG_Argv(1));
	} else {
		weapon = ent->s.weapon;
	}

	if (weapon <= WP_GAUNTLET || weapon >= WP_NUM_WEAPONS) {
		return;
	}

	item = BG_FindItemForWeapon(weapon);
	if ((ent->client->ps.stats[STAT_WEAPONS] & (1 << item->giTag))) {
		Drop_Item_Weapon(ent, item);
	}
}

static void Cmd_DropFlag_f(gentity_t *ent)
{
	if (g_gametype.integer == GT_DEFRAG) {
		ClientPrint(ent, "Item drop is not available in DeFRaG.");
		return;
	}

	if (!(g_allowDrop.integer & 1)) {
		return;
	}

	if (ent->client->ps.pm_type == PM_DEAD) {
		return;
	}

	if (ent->client->ps.powerups[PW_REDFLAG]) {
		Drop_Item_Flag(ent, BG_FindItemForPowerup(PW_REDFLAG));
		ent->client->ps.powerups[PW_REDFLAG] = 0;
	} else if (ent->client->ps.powerups[PW_BLUEFLAG]) {
		Drop_Item_Flag(ent, BG_FindItemForPowerup(PW_BLUEFLAG));
		ent->client->ps.powerups[PW_BLUEFLAG] = 0;
	}
}

static void Cmd_Drop_f(gentity_t *ent)
{
	if (g_gametype.integer == GT_DEFRAG) {
		ClientPrint(ent, "Item drop is not available in DeFRaG.");
		return;
	}

	if ((ent->client->ps.powerups[PW_REDFLAG] || ent->client->ps.powerups[PW_BLUEFLAG])
		&& g_allowDrop.integer & 1)
	{
		Cmd_DropFlag_f(ent);
	} else if (g_allowDrop.integer & 2) {
		Cmd_DropWeapon_f(ent);
	}
}

static void Cmd_Ready_f(gentity_t *ent)
{
	if (!g_startWhenReady.integer || level.warmupTime >= 0) {
		return;
	}
	ent->client->pers.ready = !ent->client->pers.ready;
	ClientUserinfoChanged(ent->client - level.clients);
}

static void Cmd_Lock_f(gentity_t *ent)
{
	qboolean	lock;
	const char	*name;

	if (!ent->client->userid && !g_allowLock.integer) {
		ClientPrint(ent, "Teamlock not allowed on this server.");
		return;
	}

	// if admin rights are not needed, keep the login secret
	if (g_allowLock.integer) {
		name = va("%s^7", ent->client->pers.netname);
	} else {
		name = G_UserName(ent);
	}

	if (g_gametype.integer < GT_TEAM) {
		ClientPrint(ent, "Teamlock not available in this gametype.");
		return;
	}

	if (ent->client->ps.persistant[PERS_TEAM] == TEAM_SPECTATOR) {
		ClientPrint(ent, "Teamlock is not available for spectators.");
		return;
	}

	lock = !strcmp(BG_Argv(0), "lock");

	if (ent->client->ps.persistant[PERS_TEAM] == TEAM_RED) {
		if (g_redLocked.integer && lock) {
			ClientPrint(ent, "Red team is already locked.");
		} else if (!g_redLocked.integer && !lock) {
			ClientPrint(ent, "Red team is already unlocked.");
		} else if (lock) {
			ClientPrint(NULL, "Red team locked by %s.", name);
			trap_Cvar_Set("g_redLocked", "1");
		} else if (!lock) {
			ClientPrint(NULL, "Red team unlocked by %s.", name);
			trap_Cvar_Set("g_redLocked", "0");
		}
	} else if (ent->client->ps.persistant[PERS_TEAM] == TEAM_BLUE) {
		if (g_blueLocked.integer && lock) {
			ClientPrint(ent, "Blue team is already locked.");
		} else if (!g_blueLocked.integer && !lock) {
			ClientPrint(ent, "Blue team is already unlocked.");
		} else if (lock) {
			ClientPrint(NULL, "Blue team locked by %s.", name);
			trap_Cvar_Set("g_redLocked", "1");
		} else if (!lock) {
			ClientPrint(NULL, "Blue team unlocked by %s.", name);
			trap_Cvar_Set("g_blueLocked", "0");
		}
	}
}

static void Cmd_Forfeit_f(gentity_t *ent)
{
	if (g_gametype.integer != GT_TOURNAMENT) {
		ClientPrint(ent, "Forfeit is only available in tournament.");
		return;
	}

	if (ent->client->sess.sessionTeam != TEAM_FREE) {
		ClientPrint(ent, "You are not playing.");
		return;
	}

	if (level.warmupTime) {
		ClientPrint(ent, "Forfeit is not available during warmup.");
		return;
	}

	if (ent->client->ps.clientNum != level.sortedClients[1]) {
		ClientPrint(ent, "Forfeit is only available for the losing player.");
		return;
	}

	level.intermissionQueued = level.time;
	ClientPrint(NULL, "The match has been forfeited.");
}

static void Cmd_Block_f(gentity_t *ent)
{
	gclient_t	*cl;
	qboolean	block;

	cl = ClientFromString(BG_Argv(1));
	if (!cl) {
		ClientPrint(ent, "Player not found.");
		return;
	}

	block = !strcmp(BG_Argv(0), "block");

	if (ent->client->pers.blocklist[cl - level.clients] == block) {
		if (block) {
			ClientPrint(ent, "Player '%s^7' is already blocked.", cl->pers.netname);
		} else {
			ClientPrint(ent, "Player '%s^7' is already unblocked.", cl->pers.netname);
		}
		return;
	}

	if (block) {
		ent->client->pers.blocklist[cl - level.clients] = qtrue;
		ClientPrint(ent, "Blocked '%s^7'.", cl->pers.netname);
	} else {
		ent->client->pers.blocklist[cl - level.clients] = qfalse;
		ClientPrint(ent, "Unblocked '%s^7'.", cl->pers.netname);
	}
}

/**
Send full statistics after a vid_restart.
*/
static void Cmd_GameData_f(gentity_t *ent)
{
	ent->client->sess.fullStatsSent = qfalse;
}

static void Cmd_Timeout_f(gentity_t *ent)
{
	if (!g_timeoutAllowed.integer) {
		ClientPrint(ent, "Timeout is not allowed on this server.");
		return;
	}
	if (g_gametype.integer == GT_DEFRAG) {
		ClientPrint(ent, "Pause is not available in this gametype.");
		return;
	}
	if (level.pauseStart) {
		ClientPrint(ent, "The game is already paused.");
		return;
	}
	if (level.warmupTime) {
		ClientPrint(ent, "Timeout is not allowed in warmup.");
		return;
	}
	if (ent->client->sess.sessionTeam == TEAM_SPECTATOR) {
		ClientPrint(ent, "Timeout is not allowed for spectators.");
		return;
	}

	// when the player who paused the game disconnects, then unpause
	level.pauseCaller = ent;
	G_Pause(level.time, level.totalTime + 1000 * g_timeoutTime.integer);

	ClientPrint(NULL, "%s called a timeout", G_UserName(ent));
}

static void Cmd_Timein_f(gentity_t *ent)
{
	if (!level.pauseStart) {
		ClientPrint(ent, "There is no active timeout.");
		return;
	}

	G_Pause(0, 0);

	ClientPrint(NULL, "%s called a timein.", G_UserName(ent));
}

static void Cmd_Pause_f(gentity_t *ent)
{
	if (level.pauseStart) {
		ClientPrint(ent, "The game is already paused.");
		return;
	}
	if (g_gametype.integer == GT_DEFRAG) {
		ClientPrint(ent, "Pause is not available in this gametype.");
		return;
	}
	if (level.warmupTime) {
		ClientPrint(ent, "Pause is not available in warmup.");
		return;
	}

	// when the player who paused the game disconnects, then unpause
	level.pauseCaller = ent;

	G_Pause(level.time, 0);

	ClientPrint(NULL, "%s paused the game.", G_UserName(ent));
}

static void Cmd_Unpause_f(gentity_t *ent)
{
	if (!level.pauseStart) {
		ClientPrint(ent, "The game is already unpaused.");
		return;
	}

	G_Pause(0, 0);

	ClientPrint(NULL, "%s unpaused the game.", G_UserName(ent));
}

static void Cmd_AllReady_f(gentity_t *ent)
{
	int	i;

	for (i = 0; i < level.maxclients; ++i) {
		if (level.intermissiontime) {
		}

		ent->client->pers.ready = qtrue;
	}
}

static void Cmd_Kick_f(gentity_t *ent)
{
	gclient_t	*cl;

	if (trap_Argc() != 2) {
		ClientPrint(ent, "Usage: kick <player>");
		return;
	}

	cl = ClientFromString(BG_Argv(1));
	if (!cl) {
		ClientPrint(ent, "Player not found.");
		return;
		}

	if (cl->pers.localClient) {
		ClientPrint(ent, "Cannot kick host player.");
		return;
	}

	trap_DropClient(cl - level.clients, va("was kicked by %s.", G_UserName(ent)));
}

static void Cmd_KickAll_f(gentity_t *ent)
{
	int	i;
	for (i = 0; i < level.maxclients; ++i) {
		if (level.clients[i].pers.connected != CON_CONNECTED) {
			continue;
		}
		if (level.clients[i].pers.localClient) {
			continue;
		}
		trap_DropClient(i, va("was kicked by %s.", G_UserName(ent)));
	}
}

static void Cmd_KickBots_f(gentity_t *ent)
{
	int	i;
	for (i = 0; i < level.maxclients; ++i) {
		if (level.clients[i].pers.connected != CON_CONNECTED) {
			continue;
		}
		if (g_entities[i].r.svFlags & SVF_BOT) {
			trap_DropClient(i, va("was kicked by %s.", G_UserName(ent)));
		}
	}
}

static void Cmd_Put_f(gentity_t *ent)
{
	gclient_t	*cl;

	if (trap_Argc() != 3) {
		ClientPrint(ent, "Usage: put <player> <team>");
		return;
	}

	// find the player
	cl = ClientFromString(BG_Argv(1));
	if (!cl) {
		ClientPrint(ent, "Player not found.");
		return;
	}

	// set the team
	SetTeam(&g_entities[cl - level.clients], BG_Argv(2));

	trap_SendConsoleCommand(cl - level.clients, "cp \"You were put in another team.\"\n");
	ClientPrint(NULL, "%s ^7has been put to another team by %s.",
		cl->pers.netname, G_UserName(ent));
}

static void Cmd_Mute_f(gentity_t *ent)
{
	gclient_t	*cl;

	if (trap_Argc() != 2) {
		ClientPrint(ent, "Usage: mute <player>");
		return;
	}

	cl = ClientFromString(BG_Argv(1));
	if (!cl) {
		ClientPrint(ent, "Player not found.");
		return;
	}

	if (cl->pers.muted) {
		ClientPrint(ent, "%s ^7is already muted.", cl->pers.netname);
		return;
	}

	cl->pers.muted = qtrue;
	ClientPrint(NULL, "%s ^7has been muted by %s.", ent->client->pers.netname, G_UserName(ent));
}

static void Cmd_Unmute_f(gentity_t *ent)
{
	gclient_t	*cl;

	if (trap_Argc() != 2) {
		ClientPrint(ent, "Usage: mute <player name>");
		return;
	}

	cl = ClientFromString(BG_Argv(1));
	if (!cl) {
		ClientPrint(ent, "Player not found.");
		return;
	}

	if (!cl->pers.muted) {
		ClientPrint(ent, "%s ^7is already unmuted.", cl->pers.netname);
		return;
	}

	cl->pers.muted = qfalse;
	ClientPrint(NULL, "%s ^7has been unmuted by %s.", cl->pers.netname, G_UserName(ent));
}

static void Cmd_AddBot_f(gentity_t *ent)
{
	float		skill;
	char		name[MAX_TOKEN_CHARS];
	char		team[MAX_TOKEN_CHARS];
	const char	*skillString;

	// are bots enabled?
	if (!trap_Cvar_VariableIntegerValue("bot_enable")) {
		ClientPrint(ent, "Bots are not enabled.");
		return;
	}

	if (g_gametype.integer == GT_DEFRAG) {
		ClientPrint(ent, "Bots cannot play DeFRaG.");
		return;
	}

	// name
	trap_Argv(1, name, sizeof name);
	if (!name[0]) {
		ClientPrint(ent, "Usage: addbot <botname> [skill 1-5] [team] [altname]");
		return;
	}

	// skill
	skillString = BG_Argv(2);
	if (!skillString[0]) {
		skill = 4;
	} else {
		skill = atof(skillString);
	}

	trap_Argv(3, team, sizeof team);
	G_AddBot(name, skill, team, BG_Argv(4));

	ClientPrint(NULL, "The bot '%s' has been added by %s.", name, G_UserName(ent));
}

static void Cmd_BotList_f(gentity_t *ent)
{
#if 0
	int i;
	char name[MAX_TOKEN_CHARS];
	char funname[MAX_TOKEN_CHARS];
	char aifile[MAX_TOKEN_CHARS];

	ClientPrint(ent, "^1name            aifile              funname");
	for (i = 0; i < g_numBots; i++) {
		strcpy(name, Info_ValueForKey(g_botInfos[i], "name"));
		if (!*name) {
			strcpy(name, "UnnamedPlayer");
		}
		strcpy(funname, Info_ValueForKey(g_botInfos[i], "funname"));
		if (!*funname) {
			strcpy(funname, "");
		}
		strcpy(aifile, Info_ValueForKey(g_botInfos[i], "aifile"));
		if (!*aifile) {
			strcpy(aifile, "bots/default_c.c");
		}
		ClientPrint(ent, "%-16s %-20s %-20s", name, aifile, funname);
	}
#endif
}

static void Cmd_PassVote_f(gentity_t *ent)
{
	if (!level.voteTime) {
		ClientPrint(ent, "No vote is in progress.");
		return;
	}
	level.voteExecuteTime = 0;
	level.voteTime = 0;
	trap_SendConsoleCommand(EXEC_APPEND, va("%s\n", level.voteString));
	trap_SetConfigstring(CS_VOTE_TIME, "passed");
	ClientPrint(NULL, "The vote has been passed by %s.", G_UserName(ent));
}

static void Cmd_CancelVote_f(gentity_t *ent)
{
	trap_SetConfigstring(CS_VOTE_TIME, "failed");
	ClientPrint(NULL, "The vote has been canceled by %s.", G_UserName(ent));
}

static void Cmd_Rename_f(gentity_t *ent)
{
	gclient_t	*client;
	char		userinfo[MAX_INFO_STRING];

	if (trap_Argc() < 3) {
		ClientPrint(ent, "Usage: rename <player> <new name>");
		return;
	}

	client = ClientFromString(BG_Argv(1));
	if (!client) {
		ClientPrint(NULL, "Player not found.");
		return;
	}

	ClientPrint(NULL, "%s ^7has been renamed by %s.", client->pers.netname, G_UserName(ent));
	trap_GetUserinfo(client - level.clients, userinfo, sizeof userinfo);
	Info_SetValueForKey(userinfo, "name", BG_Argv(2));
	trap_SetUserinfo(client - level.clients, userinfo);
	ClientUserinfoChanged(client - level.clients);
}

static void Cmd_LoadMap_f(gentity_t *ent)
{
	const char	*map;
	char		path[MAX_QPATH];

	if (trap_Argc() < 2) {
		trap_SendConsoleCommand(EXEC_APPEND, "vstr nextmap");
		return;
	}

	map = BG_Argv(1);

	Com_sprintf(path, sizeof path, "maps/%s.bsp", map);
	if (!trap_FS_FOpenFile(path, NULL, FS_READ)) {
		ClientPrint(ent, "Cannot find map %s.", path);
		return;
	}

	trap_SendConsoleCommand(EXEC_APPEND, va("map %s", map));
}

void ClientCommand(int clientNum)
{
	gentity_t	*ent;
	const char	*cmd;

	ent = g_entities + clientNum;
	if (!ent->client || ent->client->pers.connected != CON_CONNECTED) {
		return;		// not fully in game yet
	}

	cmd = BG_Argv(0);

	if (!Q_stricmp(cmd, "say")) {
		Cmd_Say_f(ent, SAY_ALL, qfalse);
		return;
	}
	if (!Q_stricmp(cmd, "say_team")) {
		Cmd_Say_f(ent, SAY_TEAM, qfalse);
		return;
	}
	if (!Q_stricmp(cmd, "tell")) {
		Cmd_Tell_f(ent);
		return;
	}

	// ignore all other commands when at intermission
	if (level.intermissiontime) {
		Cmd_Say_f(ent, qfalse, qtrue);
		return;
	}

	if (!Q_stricmp(cmd, "give")) {
		Cmd_Give_f(ent);
	} else if (!Q_stricmp(cmd, "god")) {
		Cmd_God_f(ent);
	} else if (!Q_stricmp(cmd, "notarget")) {
		Cmd_Notarget_f(ent);
	} else if (!Q_stricmp(cmd, "noclip")) {
		Cmd_Noclip_f(ent);
	} else if (!Q_stricmp(cmd, "kill") && !level.pauseStart) {
		Cmd_Kill_f(ent);
	} else if (!Q_stricmp(cmd, "teamtask")) {
		Cmd_TeamTask_f(ent);
	} else if (!Q_stricmp(cmd, "levelshot")) {
		Cmd_LevelShot_f(ent);
	} else if (!Q_stricmp(cmd, "follow")) {
		Cmd_Follow_f(ent);
	} else if (!Q_stricmp(cmd, "follownext")) {
		Cmd_FollowCycle_f(ent);
	} else if (!Q_stricmp(cmd, "followprev")) {
		Cmd_FollowCycle_f(ent);
	} else if (!Q_stricmp(cmd, "team")) {
		Cmd_Team_f(ent);
	} else if (!Q_stricmp(cmd, "where")) {
		Cmd_Where_f(ent);
	} else if (!Q_stricmp(cmd, "callvote")) {
		Cmd_CallVote_f(ent);
	} else if (!Q_stricmp(cmd, "vote")) {
		Cmd_Vote_f(ent);
	} else if (!Q_stricmp(cmd, "setviewpos")) {
		Cmd_SetViewpos_f(ent);
	} else if (!Q_stricmp(cmd, "dropammo") && !level.pauseStart) {
		Cmd_DropAmmo_f(ent);
	} else if (!Q_stricmp(cmd, "droparmor") && !level.pauseStart) {
		Cmd_DropArmor_f(ent);
	} else if (!Q_stricmp(cmd, "drophealth") && !level.pauseStart) {
		Cmd_DropHealth_f(ent);
	} else if (!Q_stricmp(cmd, "dropweapon") && !level.pauseStart) {
		Cmd_DropWeapon_f(ent);
	} else if (!Q_stricmp(cmd, "dropflag") && !level.pauseStart) {
		Cmd_DropFlag_f(ent);
	} else if (!Q_stricmp(cmd, "drop") && !level.pauseStart) {
		Cmd_Drop_f(ent);
	} else if (!Q_stricmp(cmd, "ready")) {
		Cmd_Ready_f(ent);
	} else if (!Q_stricmp(cmd, "lock")) {
		Cmd_Lock_f(ent);
	} else if (!Q_stricmp(cmd, "unlock")) {
		Cmd_Lock_f(ent);
	} else if (!Q_stricmp(cmd, "forfeit")) {
		Cmd_Forfeit_f(ent);
	} else if (!Q_stricmp(cmd, "block")) {
		Cmd_Block_f(ent);
	} else if (!Q_stricmp(cmd, "unblock")) {
		Cmd_Block_f(ent);
	} else if (!Q_stricmp(cmd, "login")) {
		Cmd_Login_f(ent);
	} else if (!Q_stricmp(cmd, "gamedata")) {
		Cmd_GameData_f(ent);
	}

	// check for operator commands

	if (!G_UserAllowed(ent, cmd)) {
		return;
	}

	if (!Q_stricmp(cmd, "pause")) {
		Cmd_Pause_f(ent);
	} else if (!Q_stricmp(cmd, "unpause")) {
		Cmd_Unpause_f(ent);
	} else if (!Q_stricmp(cmd, "user")) {
		Cmd_User_f(ent);
	} else if (!Q_stricmp(cmd, "mapcycle")) {
		Cmd_MapCycle_f(ent);
	} else if (!Q_stricmp(cmd, "ban")) {
		Cmd_Ban_f(ent);
	} else if (!Q_stricmp(cmd, "timeout")) {
		Cmd_Timeout_f(ent);
	} else if (!Q_stricmp(cmd, "timein")) {
		Cmd_Timein_f(ent);
	} else if (!Q_stricmp(cmd, "mute")) {
		Cmd_Mute_f(ent);
	} else if (!Q_stricmp(cmd, "unmute")) {
		Cmd_Unmute_f(ent);
	} else if (!Q_stricmp(cmd, "kick")) {
		Cmd_Kick_f(ent);
	} else if (!Q_stricmp(cmd, "kickbots")) {
		Cmd_KickBots_f(ent);
	} else if (!Q_stricmp(cmd, "kickall")) {
		Cmd_KickAll_f(ent);
	} else if (!Q_stricmp(cmd, "put")) {
		Cmd_Put_f(ent);
	} else if (!Q_stricmp(cmd, "addbot")) {
		Cmd_AddBot_f(ent);
	} else if (!Q_stricmp(cmd, "botlist")) {
		Cmd_BotList_f(ent);
	} else if (!Q_stricmp(cmd, "passvote")) {
		Cmd_PassVote_f(ent);
	} else if (!Q_stricmp(cmd, "cancelvote")) {
		Cmd_CancelVote_f(ent);
	} else if (!Q_stricmp(cmd, "rename")) {
		Cmd_Rename_f(ent);
	} else if (!Q_stricmp(cmd, "usertest")) {
		Cmd_UserTest_f(ent);
	} else if (!Q_stricmp(cmd, "allready")) {
		Cmd_AllReady_f(ent);
	} else if (!Q_stricmp(cmd, "loadmap")) {
		Cmd_LoadMap_f(ent);
	}
}


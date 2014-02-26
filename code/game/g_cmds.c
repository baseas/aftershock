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
		trap_SendServerCommand(ent-g_entities, "print \"Cheats are not enabled on this server.\n\"");
		return qfalse;
	}
	if (ent->health <= 0) {
		trap_SendServerCommand(ent-g_entities, "print \"You must be alive to use this command.\n\"");
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
void Cmd_Give_f (gentity_t *ent)
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

	if (Q_stricmp(name, "all") == 0)
		give_all = qtrue;
	else
		give_all = qfalse;

	if (give_all || Q_stricmp(name, "health") == 0)
	{
		ent->health = ent->client->ps.stats[STAT_MAX_HEALTH];
		if (!give_all)
			return;
	}

	if (give_all || Q_stricmp(name, "weapons") == 0)
	{
		ent->client->ps.stats[STAT_WEAPONS] = (1 << WP_NUM_WEAPONS) - 1 -
			(1 << WP_GRAPPLING_HOOK) - (1 << WP_NONE);
		if (!give_all)
			return;
	}

	if (give_all || Q_stricmp(name, "ammo") == 0)
	{
		for (i = 0; i < MAX_WEAPONS; i++) {
			ent->client->ps.ammo[i] = -1;
		}
		if (!give_all)
			return;
	}

	if (give_all || Q_stricmp(name, "armor") == 0)
	{
		ent->client->ps.stats[STAT_ARMOR] = 200;

		if (!give_all)
			return;
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
void Cmd_God_f (gentity_t *ent)
{
	char	*msg;

	if (!CheatsOk(ent)) {
		return;
	}

	ent->flags ^= FL_GODMODE;
	if (!(ent->flags & FL_GODMODE))
		msg = "godmode OFF\n";
	else
		msg = "godmode ON\n";

	trap_SendServerCommand(ent-g_entities, va("print \"%s\"", msg));
}

/**
Sets client to notarget

argv(0) notarget
*/
void Cmd_Notarget_f(gentity_t *ent)
{
	char	*msg;

	if (!CheatsOk(ent)) {
		return;
	}

	ent->flags ^= FL_NOTARGET;
	if (!(ent->flags & FL_NOTARGET))
		msg = "notarget OFF\n";
	else
		msg = "notarget ON\n";

	trap_SendServerCommand(ent-g_entities, va("print \"%s\"", msg));
}

/**
argv(0) noclip
*/
void Cmd_Noclip_f(gentity_t *ent)
{
	char	*msg;

	if (!CheatsOk(ent)) {
		return;
	}

	if (ent->client->noclip) {
		msg = "noclip OFF\n";
	} else {
		msg = "noclip ON\n";
	}
	ent->client->noclip = !ent->client->noclip;

	trap_SendServerCommand(ent-g_entities, va("print \"%s\"", msg));
}

/**
This is just to help generate the level pictures
for the menus.  It goes to the intermission immediately
and sends over a command to the client to resize the view,
hide the scoreboard, and take a special screenshot
*/
void Cmd_LevelShot_f(gentity_t *ent)
{
	if (!ent->client->pers.localClient)
	{
		trap_SendServerCommand(ent-g_entities,
			"print \"The levelshot command must be executed by a local client\n\"");
		return;
	}

	if (!CheatsOk(ent))
		return;

	BeginIntermission();
	trap_SendServerCommand(ent-g_entities, "clientLevelShot");
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
		G_LogPrintf("%s" S_COLOR_WHITE " joined the red team.\n", client->pers.netname);
	} else if (client->sess.sessionTeam == TEAM_BLUE) {
		G_LogPrintf("%s" S_COLOR_WHITE " joined the blue team.\n", client->pers.netname);
	} else if (client->sess.sessionTeam == TEAM_SPECTATOR && oldTeam != TEAM_SPECTATOR) {
		G_LogPrintf("%s" S_COLOR_WHITE " joined the spectators.\n", client->pers.netname);
	} else if (client->sess.sessionTeam == TEAM_FREE) {
		G_LogPrintf("%s" S_COLOR_WHITE " joined the battle.\n", client->pers.netname);
	}
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
	if (!Q_stricmp(s, "scoreboard") || !Q_stricmp(s, "score")) {
		team = TEAM_SPECTATOR;
		specState = SPECTATOR_SCOREBOARD;
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
	} else if (g_gametype.integer >= GT_TEAM) {
		// if running a team game, assign player to one of the teams
		specState = SPECTATOR_NOT;
		if (!Q_stricmp(s, "red") || !Q_stricmp(s, "r")) {
			team = TEAM_RED;
		} else if (!Q_stricmp(s, "blue") || !Q_stricmp(s, "b")) {
			team = TEAM_BLUE;
		} else {
			// pick the team with the least number of players
			team = PickTeam(clientNum);
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

	} else {
		// force them to spectators if there aren't any spots free
		team = TEAM_FREE;
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
	client->pers.teamState.state = TEAM_BEGIN;
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
	ent->client->ps.persistant[PERS_TEAM] = TEAM_SPECTATOR;
	ent->client->sess.sessionTeam = TEAM_SPECTATOR;
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
			trap_SendServerCommand(ent-g_entities, "print \"Blue team\n\"");
			break;
		case TEAM_RED:
			trap_SendServerCommand(ent-g_entities, "print \"Red team\n\"");
			break;
		case TEAM_FREE:
			trap_SendServerCommand(ent-g_entities, "print \"Free team\n\"");
			break;
		case TEAM_SPECTATOR:
			trap_SendServerCommand(ent-g_entities, "print \"Spectator team\n\"");
			break;
		}
		return;
	}

	if (ent->client->switchTeamTime > level.time) {
		trap_SendServerCommand(ent-g_entities, "print \"May not switch teams more than once per 5 seconds.\n\"");
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

	// if they are playing a tournement game, count as a loss
	if ((g_gametype.integer == GT_TOURNAMENT)
		&& ent->client->sess.sessionTeam == TEAM_FREE) {
		ent->client->sess.losses++;
	}

	// first set them to spectator
	if (ent->client->sess.sessionTeam != TEAM_SPECTATOR) {
		SetTeam(ent, "spectator");
	}

	ent->client->sess.spectatorState = SPECTATOR_FOLLOW;
	ent->client->sess.spectatorClient = cl - level.clients;
	ent->client->sess.fullStatsSent = qfalse;
}

void Cmd_FollowCycle_f(gentity_t *ent, int dir)
{
	int		clientnum;
	int		original;

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
		if (level.clients[ clientnum ].sess.sessionTeam == TEAM_SPECTATOR) {
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
	case 'A':
		intval = ent->client->ps.stats[STAT_ARMOR];
		if (intval < 50) {
			Com_sprintf(text, length, "^1%i", intval);
		} else if (intval < 100) {
			Com_sprintf(text, length, "^2%i", intval);
		} else {
			Com_sprintf(text, length, "^3%i", intval);
		}
		break;
	case 'H':
		intval = ent->client->ps.stats[STAT_HEALTH];
		if (intval < 50) {
			Com_sprintf(text, length, "^1%i", intval);
		} else if (intval < 100) {
			Com_sprintf(text, length, "^2%i", intval);
		} else {
			Com_sprintf(text, length, "^3%i", intval);
		}
		break;
	case 'a':
		Com_sprintf(text, length, "%i", ent->client->ps.stats[STAT_ARMOR]);
		break;
	case 'h':
		Com_sprintf(text, length, "%i", ent->client->ps.stats[STAT_HEALTH]);
		break;
	case 'K':
		if (ent->client->pers.lastKiller != -1) {
			Q_strncpyz(text, g_entities[ent->client->pers.lastKiller].client->pers.netname, length);
		}
		break;
	case 'T':
		if (ent->client->pers.lastTarget != -1) {
			Q_strncpyz(text, g_entities[ent->client->pers.lastTarget].client->pers.netname, length);
		}
		break;
	case 'D':
		intval = ent->client->ps.persistant[PERS_ATTACKER];
		Q_strncpyz(text, g_entities[intval].client->pers.netname, length);
		break;
	case 'd':
		if (ent->client->pers.lastDrop) {
			Q_strncpyz(text, ent->client->pers.lastDrop->pickup_name, length);
		}
		break;
	case 'P':
		if (ent->client->pers.lastPickup) {
			Q_strncpyz(text, ent->client->pers.lastPickup->pickup_name, length);
		}
		break;
	case 'U':
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
	case 'L':
		Team_GetLocationMsg(ent->r.currentOrigin, loc, sizeof loc);
		Q_strncpyz(text, loc, length);
		break;
	case 'C':
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
		Com_sprintf (name, sizeof(name), "%s%c%c"EC": ", ent->client->pers.netname, Q_COLOR_ESCAPE, COLOR_WHITE);
		color = COLOR_GREEN;
		break;
	case SAY_TEAM:
		G_LogPrintf("sayteam: %s: %s\n", ent->client->pers.netname, chatText);
		if (Team_GetLocationMsg(ent->r.currentOrigin, location, sizeof(location)))
			Com_sprintf(name, sizeof(name), EC"(%s%c%c"EC") (%s)"EC": ",
				ent->client->pers.netname, Q_COLOR_ESCAPE, COLOR_WHITE, location);
		else
			Com_sprintf(name, sizeof(name), EC"(%s%c%c"EC")"EC": ",
				ent->client->pers.netname, Q_COLOR_ESCAPE, COLOR_WHITE);
		color = COLOR_CYAN;
		break;
	case SAY_TELL:
		if (target && target->inuse && target->client && g_gametype.integer >= GT_TEAM &&
			target->client->sess.sessionTeam == ent->client->sess.sessionTeam &&
			Team_GetLocationMsg(ent->r.currentOrigin, location, sizeof(location)))
			Com_sprintf(name, sizeof(name), EC"[%s%c%c"EC"] (%s)"EC": ", ent->client->pers.netname, Q_COLOR_ESCAPE, COLOR_WHITE, location);
		else
			Com_sprintf(name, sizeof(name), EC"[%s%c%c"EC"]"EC": ", ent->client->pers.netname, Q_COLOR_ESCAPE, COLOR_WHITE);
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

	if (trap_Argc () < 2 && !arg0) {
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

	if (trap_Argc () < 3) {
		trap_SendServerCommand(ent-g_entities, "print \"Usage: tell <player id> <message>\n\"");
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

static char	*gc_orders[] = {
	"hold your position",
	"hold this position",
	"come here",
	"cover me",
	"guard location",
	"search and destroy",
	"report"
};

static const int numgc_orders = ARRAY_LEN(gc_orders);

void Cmd_GameCommand_f(gentity_t *ent)
{
	gclient_t	*cl;
	gentity_t	*target;
	int			order;

	if (trap_Argc() != 3) {
		trap_SendServerCommand(ent-g_entities, va("print \"Usage: gc <player id> <order 0-%d>\n\"", numgc_orders - 1));
		return;
	}

	order = atoi(BG_Argv(2));

	if (order < 0 || order >= numgc_orders) {
		trap_SendServerCommand(ent-g_entities, va("print \"Bad order: %i\n\"", order));
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

	G_LogPrintf("tell: %s to %s: %s\n", ent->client->pers.netname, target->client->pers.netname, gc_orders[order]);
	G_Say(ent, target, SAY_TELL, gc_orders[order]);
	// don't tell to the player self if it was already directed to this player
	// also don't send the chat back to a bot
	if (ent != target && !(ent->r.svFlags & SVF_BOT)) {
		G_Say(ent, ent, SAY_TELL, gc_orders[order]);
	}
}

void Cmd_Where_f(gentity_t *ent)
{
	trap_SendServerCommand(ent-g_entities, va("print \"%s\n\"", vtos(ent->r.currentOrigin)));
}

static const char *gameNames[] = {
	"Free For All",
	"Tournament",
	"Single Player",
	"Team Deathmatch",
	"Capture the Flag",
	"One Flag CTF",
	"Overload",
	"Harvester"
};

static void Cmd_CallVote_f(gentity_t *ent)
{
	char*	c;
	int		i;
	char	arg1[MAX_STRING_TOKENS];
	char	arg2[MAX_STRING_TOKENS];

	if (!g_allowVote.integer) {
		trap_SendServerCommand(ent-g_entities, "print \"Voting not allowed here.\n\"");
		return;
	}

	if (level.voteTime) {
		trap_SendServerCommand(ent-g_entities, "print \"A vote is already in progress.\n\"");
		return;
	}
	if (ent->client->pers.voteCount >= MAX_VOTE_COUNT) {
		trap_SendServerCommand(ent-g_entities, "print \"You have called the maximum number of votes.\n\"");
		return;
	}
	if (ent->client->sess.sessionTeam == TEAM_SPECTATOR) {
		trap_SendServerCommand(ent-g_entities, "print \"Not allowed to call a vote as spectator.\n\"");
		return;
	}

	// make sure it is a valid command to vote on
	trap_Argv(1, arg1, sizeof(arg1));
	trap_Argv(2, arg2, sizeof(arg2));

	// check for command separators in arg2
	for (c = arg2; *c; ++c) {
		switch(*c) {
			case '\n':
			case '\r':
			case ';':
				trap_SendServerCommand(ent-g_entities, "print \"Invalid vote string.\n\"");
				return;
			break;
		}
	}

	if (!Q_stricmp(arg1, "map_restart")) {
	} else if (!Q_stricmp(arg1, "nextmap")) {
	} else if (!Q_stricmp(arg1, "map")) {
	} else if (!Q_stricmp(arg1, "g_gametype")) {
	} else if (!Q_stricmp(arg1, "kick")) {
	} else if (!Q_stricmp(arg1, "clientkick")) {
	} else if (!Q_stricmp(arg1, "timelimit")) {
	} else if (!Q_stricmp(arg1, "fraglimit")) {
	} else {
		trap_SendServerCommand(ent-g_entities, "print \"Invalid vote string.\n\"");
		trap_SendServerCommand(ent-g_entities, "print \"Vote commands are: map_restart, nextmap, map <mapname>, g_gametype <n>, kick <player>, clientkick <clientnum>, timelimit <time>, fraglimit <frags>.\n\"");
		return;
	}

	// if there is still a vote to be executed
	if (level.voteExecuteTime) {
		level.voteExecuteTime = 0;
		trap_SendConsoleCommand(EXEC_APPEND, va("%s\n", level.voteString));
	}

	// special case for g_gametype, check for bad values
	if (!Q_stricmp(arg1, "g_gametype")) {
		i = atoi(arg2);
		if (i < GT_FFA || i >= GT_MAX_GAME_TYPE) {
			trap_SendServerCommand(ent-g_entities, "print \"Invalid gametype.\n\"");
			return;
		}

		Com_sprintf(level.voteString, sizeof(level.voteString), "%s %d", arg1, i);
		Com_sprintf(level.voteDisplayString, sizeof(level.voteDisplayString), "%s %s", arg1, gameNames[i]);
	} else if (!Q_stricmp(arg1, "map")) {
		// special case for map changes, we want to reset the nextmap setting
		// this allows a player to change maps, but not upset the map rotation
		char	s[MAX_STRING_CHARS];

		trap_Cvar_VariableStringBuffer("nextmap", s, sizeof(s));
		if (*s) {
			Com_sprintf(level.voteString, sizeof(level.voteString), "%s %s; set nextmap \"%s\"", arg1, arg2, s);
		} else {
			Com_sprintf(level.voteString, sizeof(level.voteString), "%s %s", arg1, arg2);
		}
		Com_sprintf(level.voteDisplayString, sizeof(level.voteDisplayString), "%s", level.voteString);
	} else if (!Q_stricmp(arg1, "nextmap")) {
		char	s[MAX_STRING_CHARS];

		trap_Cvar_VariableStringBuffer("nextmap", s, sizeof(s));
		if (!*s) {
			trap_SendServerCommand(ent-g_entities, "print \"nextmap not set.\n\"");
			return;
		}
		Com_sprintf(level.voteString, sizeof(level.voteString), "vstr nextmap");
		Com_sprintf(level.voteDisplayString, sizeof(level.voteDisplayString), "%s", level.voteString);
	} else {
		Com_sprintf(level.voteString, sizeof(level.voteString), "%s \"%s\"", arg1, arg2);
		Com_sprintf(level.voteDisplayString, sizeof(level.voteDisplayString), "%s", level.voteString);
	}

	trap_SendServerCommand(-1, va("print \"%s called a vote.\n\"", ent->client->pers.netname));

	// start the voting, the caller automatically votes yes
	level.voteTime = level.time;
	level.voteYes = 1;
	level.voteNo = 0;

	for (i = 0; i < level.maxclients; i++) {
		level.clients[i].ps.eFlags &= ~EF_VOTED;
	}
	ent->client->ps.eFlags |= EF_VOTED;

	trap_SetConfigstring(CS_VOTE_TIME, va("%i", level.voteTime));
	trap_SetConfigstring(CS_VOTE_STRING, level.voteDisplayString);
	trap_SetConfigstring(CS_VOTE_YES, va("%i", level.voteYes));
	trap_SetConfigstring(CS_VOTE_NO, va("%i", level.voteNo));
}

static void Cmd_Vote_f(gentity_t *ent)
{
	char		msg[1];

	if (!level.voteTime) {
		trap_SendServerCommand(ent-g_entities, "print \"No vote in progress.\n\"");
		return;
	}
	if (ent->client->ps.eFlags & EF_VOTED) {
		trap_SendServerCommand(ent-g_entities, "print \"Vote already cast.\n\"");
		return;
	}
	if (ent->client->sess.sessionTeam == TEAM_SPECTATOR) {
		trap_SendServerCommand(ent-g_entities, "print \"Not allowed to vote as spectator.\n\"");
		return;
	}

	ent->client->ps.eFlags |= EF_VOTED;

	trap_Argv(1, msg, sizeof msg);

	if (tolower(msg[0]) == 'y' || msg[0] == '1') {
		level.voteYes++;
		trap_SetConfigstring(CS_VOTE_YES, va("%i", level.voteYes));
	} else {
		level.voteNo++;
		trap_SetConfigstring(CS_VOTE_NO, va("%i", level.voteNo));
	}

	// a majority will be determined in CheckVote, which will also account
	// for players entering or leaving
}

static void Cmd_CallTeamVote_f(gentity_t *ent)
{
	int		i, team, cs_offset;
	char	arg1[MAX_STRING_TOKENS];
	char	arg2[MAX_STRING_TOKENS];

	team = ent->client->sess.sessionTeam;
	if (team == TEAM_RED)
		cs_offset = 0;
	else if (team == TEAM_BLUE)
		cs_offset = 1;
	else
		return;

	if (!g_allowVote.integer) {
		trap_SendServerCommand(ent-g_entities, "print \"Voting not allowed here.\n\"");
		return;
	}

	if (level.teamVoteTime[cs_offset]) {
		trap_SendServerCommand(ent-g_entities, "print \"A team vote is already in progress.\n\"");
		return;
	}
	if (ent->client->pers.teamVoteCount >= MAX_VOTE_COUNT) {
		trap_SendServerCommand(ent-g_entities, "print \"You have called the maximum number of team votes.\n\"");
		return;
	}
	if (ent->client->sess.sessionTeam == TEAM_SPECTATOR) {
		trap_SendServerCommand(ent-g_entities, "print \"Not allowed to call a vote as spectator.\n\"");
		return;
	}

	// make sure it is a valid command to vote on
	trap_Argv(1, arg1, sizeof(arg1));
	arg2[0] = '\0';
	for (i = 2; i < trap_Argc(); i++) {
		if (i > 2)
			strcat(arg2, " ");
		trap_Argv(i, &arg2[strlen(arg2)], sizeof(arg2) - strlen(arg2));
	}

	if (strchr(arg1, ';') || strchr(arg2, ';')) {
		trap_SendServerCommand(ent-g_entities, "print \"Invalid vote string.\n\"");
		return;
	}

	if (!Q_stricmp(arg1, "leader")) {
		char netname[MAX_NETNAME], leader[MAX_NETNAME];

		if (!arg2[0]) {
			i = ent->client->ps.clientNum;
		}
		else {
			// numeric values are just slot numbers
			for (i = 0; i < 3; i++) {
				if (!arg2[i] || arg2[i] < '0' || arg2[i] > '9')
					break;
			}
			if (i >= 3 || !arg2[i]) {
				i = atoi(arg2);
				if (i < 0 || i >= level.maxclients) {
					trap_SendServerCommand(ent-g_entities, va("print \"Bad client slot: %i\n\"", i));
					return;
				}

				if (!g_entities[i].inuse) {
					trap_SendServerCommand(ent-g_entities, va("print \"Client %i is not active\n\"", i));
					return;
				}
			}
			else {
				Q_strncpyz(leader, arg2, sizeof(leader));
				Q_CleanStr(leader);
				for (i = 0; i < level.maxclients; i++) {
					if (level.clients[i].pers.connected == CON_DISCONNECTED)
						continue;
					if (level.clients[i].sess.sessionTeam != team)
						continue;
					Q_strncpyz(netname, level.clients[i].pers.netname, sizeof(netname));
					Q_CleanStr(netname);
					if (!Q_stricmp(netname, leader)) {
						break;
					}
				}
				if (i >= level.maxclients) {
					trap_SendServerCommand(ent-g_entities, va("print \"%s is not a valid player on your team.\n\"", arg2));
					return;
				}
			}
		}
		Com_sprintf(arg2, sizeof(arg2), "%d", i);
	} else {
		trap_SendServerCommand(ent-g_entities, "print \"Invalid vote string.\n\"");
		trap_SendServerCommand(ent-g_entities, "print \"Team vote commands are: leader <player>.\n\"");
		return;
	}

	Com_sprintf(level.teamVoteString[cs_offset], sizeof(level.teamVoteString[cs_offset]), "%s %s", arg1, arg2);

	for (i = 0; i < level.maxclients; i++) {
		if (level.clients[i].pers.connected == CON_DISCONNECTED)
			continue;
		if (level.clients[i].sess.sessionTeam == team)
			trap_SendServerCommand(i, va("print \"%s called a team vote.\n\"", ent->client->pers.netname));
	}

	// start the voting, the caller automatically votes yes
	level.teamVoteTime[cs_offset] = level.time;
	level.teamVoteYes[cs_offset] = 1;
	level.teamVoteNo[cs_offset] = 0;

	for (i = 0; i < level.maxclients; i++) {
		if (level.clients[i].sess.sessionTeam == team)
			level.clients[i].ps.eFlags &= ~EF_TEAMVOTED;
	}
	ent->client->ps.eFlags |= EF_TEAMVOTED;

	trap_SetConfigstring(CS_TEAMVOTE_TIME + cs_offset, va("%i", level.teamVoteTime[cs_offset]));
	trap_SetConfigstring(CS_TEAMVOTE_STRING + cs_offset, level.teamVoteString[cs_offset]);
	trap_SetConfigstring(CS_TEAMVOTE_YES + cs_offset, va("%i", level.teamVoteYes[cs_offset]));
	trap_SetConfigstring(CS_TEAMVOTE_NO + cs_offset, va("%i", level.teamVoteNo[cs_offset]));
}

static void Cmd_TeamVote_f(gentity_t *ent)
{
	int			team, cs_offset;
	char		msg[1];

	team = ent->client->sess.sessionTeam;
	if (team == TEAM_RED)
		cs_offset = 0;
	else if (team == TEAM_BLUE)
		cs_offset = 1;
	else
		return;

	if (!level.teamVoteTime[cs_offset]) {
		trap_SendServerCommand(ent-g_entities, "print \"No team vote in progress.\n\"");
		return;
	}
	if (ent->client->ps.eFlags & EF_TEAMVOTED) {
		trap_SendServerCommand(ent-g_entities, "print \"Team vote already cast.\n\"");
		return;
	}
	if (ent->client->sess.sessionTeam == TEAM_SPECTATOR) {
		trap_SendServerCommand(ent-g_entities, "print \"Not allowed to vote as spectator.\n\"");
		return;
	}

	ent->client->ps.eFlags |= EF_TEAMVOTED;

	trap_Argv(1, msg, sizeof msg);

	if (tolower(msg[0]) == 'y' || msg[0] == '1') {
		level.teamVoteYes[cs_offset]++;
		trap_SetConfigstring(CS_TEAMVOTE_YES + cs_offset, va("%i", level.teamVoteYes[cs_offset]));
	} else {
		level.teamVoteNo[cs_offset]++;
		trap_SetConfigstring(CS_TEAMVOTE_NO + cs_offset, va("%i", level.teamVoteNo[cs_offset]));
	}

	// a majority will be determined in TeamCheckVote, which will also account
	// for players entering or leaving
}

static void Cmd_SetViewpos_f(gentity_t *ent)
{
	vec3_t		origin, angles;
	int			i;

	if (!g_cheats.integer) {
		trap_SendServerCommand(ent-g_entities, "print \"Cheats are not enabled on this server.\n\"");
		return;
	}
	if (trap_Argc() != 5) {
		trap_SendServerCommand(ent-g_entities, "print \"usage: setviewpos x y z yaw\n\"");
		return;
	}

	VectorClear(angles);
	for (i = 0; i < 3; i++) {
		origin[i] = atof(BG_Argv(i + 1));
	}

	angles[YAW] = atof(BG_Argv(4));

	TeleportPlayer(ent, origin, angles);
}

static void Cmd_Stats_f(gentity_t *ent) { }

static void Cmd_DropArmor_f(gentity_t *ent)
{
	gitem_t	*item;
	int		amount;

	if (!(g_itemDrop.integer & 16) || g_instantgib.integer) {
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

	if (!(g_itemDrop.integer & 8) || g_instantgib.integer) {
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

	if (!(g_itemDrop.integer & 4) || g_instantgib.integer || g_rockets.integer) {
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

	if (!(g_itemDrop.integer & 2) || g_instantgib.integer || g_rockets.integer) {
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

static void Cmd_DropFlag_f(gentity_t *other)
{
	if (!(g_itemDrop.integer & 1)) {
		return;
	}

	if (other->client->ps.pm_type == PM_DEAD) {
		return;
	}

	if (other->client->ps.powerups[PW_REDFLAG]) {
		Drop_Item_Flag(other, BG_FindItemForPowerup(PW_REDFLAG));
		other->client->ps.powerups[PW_REDFLAG] = 0;
	} else if (other->client->ps.powerups[PW_BLUEFLAG]) {
		Drop_Item_Flag(other, BG_FindItemForPowerup(PW_BLUEFLAG));
		other->client->ps.powerups[PW_BLUEFLAG] = 0;
	}
}

static void Cmd_Drop_f(gentity_t *ent)
{
	if ((ent->client->ps.powerups[PW_REDFLAG] || ent->client->ps.powerups[PW_BLUEFLAG])
		&& g_itemDrop.integer & 1)
	{
		Cmd_DropFlag_f(ent);
	} else if (g_itemDrop.integer & 2) {
		Cmd_DropWeapon_f(ent);
	}
}

static void Cmd_Ready_f(gentity_t *ent)
{
	if (!g_startWhenReady.integer || level.warmupTime != -1) {
		return;
	}
	ent->client->pers.ready = !ent->client->pers.ready;
	ClientUserinfoChanged(ent->client - level.clients);
}

static void Cmd_Lock_f(gentity_t *ent)
{
	if (!g_teamLock.integer) {
		ClientPrint(ent, "Teamlock not allowed on this server.");
		return;
	}

	if (g_gametype.integer < GT_TEAM) {
		ClientPrint(ent, "Teamlock not available in this gametype.");
		return;
	}

	if (ent->client->ps.persistant[PERS_TEAM] == TEAM_SPECTATOR) {
		ClientPrint(ent, "Teamlock is not available for spectators.");
		return;
	}

	if (ent->client->ps.persistant[PERS_TEAM] == TEAM_RED) {
		if (g_redLocked.integer) {
			ClientPrint(ent, "Red team is already locked.");
		} else {
			ClientPrint(NULL, va("Red team locked by %s.", ent->client->pers.netname));
			trap_Cvar_Set("g_redLocked", "1");
		}
	} else if (ent->client->ps.persistant[PERS_TEAM] == TEAM_BLUE) {
		if (g_blueLocked.integer) {
			ClientPrint(ent, "Blue team is already locked.");
		} else {
			ClientPrint(NULL, va("Blue team locked by %s.", ent->client->pers.netname));
			trap_Cvar_Set("g_blueLocked", "1");
		}
	}
}

static void Cmd_Unlock_f(gentity_t *ent)
{
	if (!g_teamLock.integer) {
		ClientPrint(ent, "Teamlock not allowed on this server.");
		return;
	}

	if (g_gametype.integer < GT_TEAM) {
		ClientPrint(ent, "Teamlock not available in this gametype.");
		return;
	}

	if (ent->client->ps.persistant[PERS_TEAM] == TEAM_SPECTATOR) {
		ClientPrint(ent, "Teamlock is not available for spectators.");
		return;
	}

	if (ent->client->ps.persistant[PERS_TEAM] == TEAM_RED) {
		if (!g_redLocked.integer) {
			ClientPrint(ent, "Red team is already unlocked.");
		} else {
			ClientPrint(NULL, va("Red team unlocked by %s.", ent->client->pers.netname));
			trap_Cvar_Set("g_redLocked", "0");
		}
	} else if (ent->client->ps.persistant[PERS_TEAM] == TEAM_BLUE) {
		if (!g_blueLocked.integer) {
			ClientPrint(ent, "Blue team is already unlocked.");
		} else {
			ClientPrint(NULL, va("Blue team unlocked by %s.", ent->client->pers.netname));
			trap_Cvar_Set("g_blueLocked", "0");
		}
	}
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

	if (Q_stricmp(cmd, "say") == 0) {
		Cmd_Say_f(ent, SAY_ALL, qfalse);
		return;
	}
	if (Q_stricmp(cmd, "say_team") == 0) {
		Cmd_Say_f(ent, SAY_TEAM, qfalse);
		return;
	}
	if (Q_stricmp(cmd, "tell") == 0) {
		Cmd_Tell_f(ent);
		return;
	}

	// ignore all other commands when at intermission
	if (level.intermissiontime) {
		Cmd_Say_f(ent, qfalse, qtrue);
		return;
	}

	if (Q_stricmp(cmd, "give") == 0)
		Cmd_Give_f(ent);
	else if (Q_stricmp(cmd, "god") == 0)
		Cmd_God_f(ent);
	else if (Q_stricmp(cmd, "notarget") == 0)
		Cmd_Notarget_f(ent);
	else if (Q_stricmp(cmd, "noclip") == 0)
		Cmd_Noclip_f(ent);
	else if (Q_stricmp(cmd, "kill") == 0)
		Cmd_Kill_f(ent);
	else if (Q_stricmp(cmd, "teamtask") == 0)
		Cmd_TeamTask_f(ent);
	else if (Q_stricmp(cmd, "levelshot") == 0)
		Cmd_LevelShot_f(ent);
	else if (Q_stricmp(cmd, "follow") == 0)
		Cmd_Follow_f(ent);
	else if (Q_stricmp(cmd, "follownext") == 0)
		Cmd_FollowCycle_f(ent, 1);
	else if (Q_stricmp(cmd, "followprev") == 0)
		Cmd_FollowCycle_f(ent, -1);
	else if (Q_stricmp(cmd, "team") == 0)
		Cmd_Team_f(ent);
	else if (Q_stricmp(cmd, "where") == 0)
		Cmd_Where_f(ent);
	else if (Q_stricmp(cmd, "callvote") == 0)
		Cmd_CallVote_f(ent);
	else if (Q_stricmp(cmd, "vote") == 0)
		Cmd_Vote_f(ent);
	else if (Q_stricmp(cmd, "callteamvote") == 0)
		Cmd_CallTeamVote_f(ent);
	else if (Q_stricmp(cmd, "teamvote") == 0)
		Cmd_TeamVote_f(ent);
	else if (Q_stricmp(cmd, "gc") == 0)
		Cmd_GameCommand_f(ent);
	else if (Q_stricmp(cmd, "setviewpos") == 0)
		Cmd_SetViewpos_f(ent);
	else if (Q_stricmp(cmd, "stats") == 0)
		Cmd_Stats_f(ent);
	else if (Q_stricmp(cmd, "dropammo") == 0)
		Cmd_DropAmmo_f(ent);
	else if (Q_stricmp(cmd, "droparmor") == 0)
		Cmd_DropArmor_f(ent);
	else if (Q_stricmp(cmd, "drophealth") == 0)
		Cmd_DropHealth_f(ent);
	else if (Q_stricmp(cmd, "dropweapon") == 0)
		Cmd_DropWeapon_f(ent);
	else if (Q_stricmp(cmd, "dropflag") == 0)
		Cmd_DropFlag_f(ent);
	else if (Q_stricmp(cmd, "drop") == 0)
		Cmd_Drop_f(ent);
	else if (Q_stricmp(cmd, "ready") == 0)
		Cmd_Ready_f(ent);
	else if (Q_stricmp(cmd, "lock") == 0)
		Cmd_Lock_f(ent);
	else if (Q_stricmp(cmd, "unlock") == 0)
		Cmd_Unlock_f(ent);
	else
		trap_SendServerCommand(clientNum, va("print \"unknown cmd %s\n\"", cmd));
}


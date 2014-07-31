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
// g_client.c -- client functions that don't happen every frame

#include "g_local.h"

#define MAX_SPAWN_POINTS	128

static vec3_t	playerMins = { -15, -15, -24 };
static vec3_t	playerMaxs = {  15,  15,  32 };

/**
QUAKED info_player_deathmatch (1 0 1) (-16 -16 -24) (16 16 32) initial
potential spawning position for deathmatch games.
The first time a player enters the game, they will be at an 'initial' spot.
Targets will be fired when someone spawns in on them.
"nobots" will prevent bots from using this spot.
"nohumans" will prevent non-bots from using this spot.
*/
void SP_info_player_deathmatch(gentity_t *ent)
{
	int		i;

	G_SpawnInt("nobots", "0", &i);
	if (i) {
		ent->flags |= FL_NO_BOTS;
	}
	G_SpawnInt("nohumans", "0", &i);
	if (i) {
		ent->flags |= FL_NO_HUMANS;
	}
}

/**
QUAKED info_player_start (1 0 0) (-16 -16 -24) (16 16 32)
equivelant to info_player_deathmatch
*/
void SP_info_player_start(gentity_t *ent)
{
	ent->classname = "info_player_deathmatch";
	SP_info_player_deathmatch(ent);
}

/**
QUAKED info_player_intermission (1 0 1) (-16 -16 -24) (16 16 32)
The intermission will be viewed from this point.  Target an info_notnull for the view direction.
*/
void SP_info_player_intermission(gentity_t *ent) { }

qboolean SpotWouldTelefrag(gentity_t *spot)
{
	int			i, num;
	int			touch[MAX_GENTITIES];
	gentity_t	*hit;
	vec3_t		mins, maxs;

	VectorAdd(spot->s.origin, playerMins, mins);
	VectorAdd(spot->s.origin, playerMaxs, maxs);
	num = trap_EntitiesInBox(mins, maxs, touch, MAX_GENTITIES);

	for (i = 0; i < num; i++) {
		hit = &g_entities[touch[i]];
		//if (hit->client && hit->client->ps.stats[STAT_HEALTH] > 0) {
		if (hit->client) {
			return qtrue;
		}
	}

	return qfalse;
}

/**
Find the spot that we DON'T want to use
*/
gentity_t *SelectNearestDeathmatchSpawnPoint(vec3_t from)
{
	gentity_t	*spot;
	vec3_t		delta;
	float		dist, nearestDist;
	gentity_t	*nearestSpot;

	nearestDist = 999999;
	nearestSpot = NULL;
	spot = NULL;

	while ((spot = G_Find(spot, FOFS(classname), "info_player_deathmatch")) != NULL) {

		VectorSubtract(spot->s.origin, from, delta);
		dist = VectorLength(delta);
		if (dist < nearestDist) {
			nearestDist = dist;
			nearestSpot = spot;
		}
	}

	return nearestSpot;
}

/**
Go to a random point that doesn't telefrag
*/
gentity_t *SelectRandomDeathmatchSpawnPoint(qboolean isbot)
{
	gentity_t	*spot;
	int			count;
	int			selection;
	gentity_t	*spots[MAX_SPAWN_POINTS];

	count = 0;
	spot = NULL;

	while ((spot = G_Find(spot, FOFS(classname), "info_player_deathmatch")) != NULL && count < MAX_SPAWN_POINTS) {
		if (SpotWouldTelefrag(spot))
			continue;

		if (((spot->flags & FL_NO_BOTS) && isbot) ||
		   ((spot->flags & FL_NO_HUMANS) && !isbot))
		{
			// spot is not for this human/bot player
			continue;
		}

		spots[count] = spot;
		count++;
	}

	if (!count) {	// no spots that won't telefrag
		return G_Find(NULL, FOFS(classname), "info_player_deathmatch");
	}

	selection = rand() % count;
	return spots[ selection ];
}

/**
Chooses a player start, deathmatch start, etc
*/
gentity_t *SelectRandomFurthestSpawnPoint (vec3_t avoidPoint, vec3_t origin, vec3_t angles, qboolean isbot)
{
	gentity_t	*spot;
	vec3_t		delta;
	float		dist;
	float		list_dist[MAX_SPAWN_POINTS];
	gentity_t	*list_spot[MAX_SPAWN_POINTS];
	int			numSpots, rnd, i, j;

	numSpots = 0;
	spot = NULL;

	while ((spot = G_Find(spot, FOFS(classname), "info_player_deathmatch")) != NULL) {
		if (SpotWouldTelefrag(spot))
			continue;

		if (((spot->flags & FL_NO_BOTS) && isbot) ||
		   ((spot->flags & FL_NO_HUMANS) && !isbot))
		{
			// spot is not for this human/bot player
			continue;
		}

		VectorSubtract(spot->s.origin, avoidPoint, delta);
		dist = VectorLength(delta);

		for (i = 0; i < numSpots; i++) {
			if (dist > list_dist[i]) {
				if (numSpots >= MAX_SPAWN_POINTS)
					numSpots = MAX_SPAWN_POINTS - 1;

				for (j = numSpots; j > i; j--) {
					list_dist[j] = list_dist[j-1];
					list_spot[j] = list_spot[j-1];
				}

				list_dist[i] = dist;
				list_spot[i] = spot;

				numSpots++;
				break;
			}
		}

		if (i >= numSpots && numSpots < MAX_SPAWN_POINTS) {
			list_dist[numSpots] = dist;
			list_spot[numSpots] = spot;
			numSpots++;
		}
	}

	if (!numSpots) {
		spot = G_Find(NULL, FOFS(classname), "info_player_deathmatch");

		if (!spot)
			G_Error("Couldn't find a spawn point");

		VectorCopy(spot->s.origin, origin);
		origin[2] += 9;
		VectorCopy(spot->s.angles, angles);
		return spot;
	}

	// select a random spot from the spawn points furthest away
	rnd = random() * (numSpots / 2);

	VectorCopy(list_spot[rnd]->s.origin, origin);
	origin[2] += 9;
	VectorCopy(list_spot[rnd]->s.angles, angles);

	return list_spot[rnd];
}

/**
Chooses a player start, deathmatch start, etc
*/
gentity_t *SelectSpawnPoint(vec3_t avoidPoint, vec3_t origin, vec3_t angles, qboolean isbot)
{
	return SelectRandomFurthestSpawnPoint(avoidPoint, origin, angles, isbot);
}

/**
Try to find a spawn point marked 'initial', otherwise
use normal spawn selection.
*/
gentity_t *SelectInitialSpawnPoint(vec3_t origin, vec3_t angles, qboolean isbot)
{
	gentity_t	*spot;

	spot = NULL;

	while ((spot = G_Find(spot, FOFS(classname), "info_player_deathmatch")) != NULL) {
		if (((spot->flags & FL_NO_BOTS) && isbot) ||
		   ((spot->flags & FL_NO_HUMANS) && !isbot))
		{
			continue;
		}

		if ((spot->spawnflags & 0x01))
			break;
	}

	if (!spot || SpotWouldTelefrag(spot))
		return SelectSpawnPoint(vec3_origin, origin, angles, isbot);

	VectorCopy(spot->s.origin, origin);
	origin[2] += 9;
	VectorCopy(spot->s.angles, angles);

	return spot;
}

gentity_t *SelectSpectatorSpawnPoint(vec3_t origin, vec3_t angles)
{
	FindIntermissionPoint();

	VectorCopy(level.intermission_origin, origin);
	VectorCopy(level.intermission_angle, angles);

	return NULL;
}

void InitBodyQue(void)
{
	int			i;
	gentity_t	*ent;

	level.bodyQueIndex = 0;
	for (i = 0; i < BODY_QUEUE_SIZE; i++) {
		ent = G_Spawn();
		ent->classname = "bodyque";
		ent->neverFree = qtrue;
		level.bodyQue[i] = ent;
	}
}

/**
After sitting around for five seconds, fall into the ground and dissapear
*/
void BodySink(gentity_t *ent)
{
	if (level.time - ent->timestamp > 6500) {
		// the body ques are never actually freed, they are just unlinked
		trap_UnlinkEntity(ent);
		ent->physicsObject = qfalse;
		return;
	}
	ent->nextthink = level.time + 100;
	ent->s.pos.trBase[2] -= 1;
}

/**
A player is respawning, so make an entity that looks
just like the existing corpse to leave behind.
*/
void CopyToBodyQue(gentity_t *ent)
{
	gentity_t		*body;
	int			contents;

	trap_UnlinkEntity (ent);

	// if client is in a nodrop area, don't leave the body
	contents = trap_PointContents(ent->s.origin, -1);
	if (contents & CONTENTS_NODROP) {
		return;
	}

	// grab a body que and cycle to the next one
	body = level.bodyQue[ level.bodyQueIndex ];
	level.bodyQueIndex = (level.bodyQueIndex + 1) % BODY_QUEUE_SIZE;

	body->s = ent->s;
	body->s.eFlags = EF_DEAD;		// clear EF_TALK, etc
	body->s.powerups = 0;	// clear powerups
	body->s.loopSound = 0;	// clear lava burning
	body->s.number = body - g_entities;
	body->timestamp = level.time;
	body->physicsObject = qtrue;
	body->physicsBounce = 0;		// don't bounce
	if (body->s.groundEntityNum == ENTITYNUM_NONE) {
		body->s.pos.trType = TR_GRAVITY;
		body->s.pos.trTime = level.time;
		VectorCopy(ent->client->ps.velocity, body->s.pos.trDelta);
	} else {
		body->s.pos.trType = TR_STATIONARY;
	}
	body->s.event = 0;

	// change the animation to the last-frame only, so the sequence
	// doesn't repeat anew for the body
	switch (body->s.legsAnim & ~ANIM_TOGGLEBIT) {
	case BOTH_DEATH1:
	case BOTH_DEAD1:
		body->s.torsoAnim = body->s.legsAnim = BOTH_DEAD1;
		break;
	case BOTH_DEATH2:
	case BOTH_DEAD2:
		body->s.torsoAnim = body->s.legsAnim = BOTH_DEAD2;
		break;
	case BOTH_DEATH3:
	case BOTH_DEAD3:
	default:
		body->s.torsoAnim = body->s.legsAnim = BOTH_DEAD3;
		break;
	}

	body->r.svFlags = ent->r.svFlags;
	VectorCopy(ent->r.mins, body->r.mins);
	VectorCopy(ent->r.maxs, body->r.maxs);
	VectorCopy(ent->r.absmin, body->r.absmin);
	VectorCopy(ent->r.absmax, body->r.absmax);

	body->clipmask = CONTENTS_SOLID | CONTENTS_PLAYERCLIP;
	body->r.contents = CONTENTS_CORPSE;
	body->r.ownerNum = ent->s.number;

	body->nextthink = level.time + 5000;
	body->think = BodySink;

	body->die = body_die;

	// don't take more damage if already gibbed
	if (ent->health <= GIB_HEALTH) {
		body->takedamage = qfalse;
	} else {
		body->takedamage = qtrue;
	}


	VectorCopy(body->s.pos.trBase, body->r.currentOrigin);
	trap_LinkEntity (body);
}

void SetClientViewAngle(gentity_t *ent, vec3_t angle)
{
	int	i;

	// set the delta angle
	for (i = 0; i < 3; i++) {
		int	cmdAngle;

		cmdAngle = ANGLE2SHORT(angle[i]);
		ent->client->ps.delta_angles[i] = cmdAngle - ent->client->pers.cmd.angles[i];
	}
	VectorCopy(angle, ent->s.angles);
	VectorCopy(ent->s.angles, ent->client->ps.viewangles);
}

void ClientRespawn(gentity_t *ent)
{
	CopyToBodyQue(ent);
	ClientSpawn(ent);
}

int TeamLivingCount(team_t team)
{
	int			i;
	int			count;
	gclient_t	*cl;

	for (i = 0, count = 0, cl = level.clients; i < level.maxclients; ++i, ++cl) {
		if (cl->pers.connected != CON_CONNECTED) {
			continue;
		}
		if (cl->sess.sessionTeam == team && !cl->eliminated) {
			count++;
		}
	}

	return count;
}

/**
Returns number of players on a team
*/
int TeamCount(int ignoreClientNum, team_t team)
{
	int		i;
	int		count = 0;

	for (i = 0; i < level.maxclients; i++) {
		if (i == ignoreClientNum) {
			continue;
		}
		if (level.clients[i].pers.connected == CON_DISCONNECTED) {
			continue;
		}
		if (level.clients[i].sess.sessionTeam == team) {
			count++;
		}
	}

	return count;
}

/**
Returns the client number of the team leader
*/
int TeamLeader(int team)
{
	int		i;

	for (i = 0; i < level.maxclients; i++) {
		if (level.clients[i].pers.connected == CON_DISCONNECTED) {
			continue;
		}
		if (level.clients[i].sess.sessionTeam == team) {
			if (level.clients[i].sess.teamLeader)
				return i;
		}
	}

	return -1;
}

team_t PickTeam(int ignoreClientNum)
{
	int		counts[TEAM_NUM_TEAMS];

	counts[TEAM_BLUE] = TeamCount(ignoreClientNum, TEAM_BLUE);
	counts[TEAM_RED] = TeamCount(ignoreClientNum, TEAM_RED);

	if (!g_redLocked.integer && counts[TEAM_BLUE] > counts[TEAM_RED]) {
		return TEAM_RED;
	}

	if (!g_blueLocked.integer && counts[TEAM_RED] > counts[TEAM_BLUE]) {
		return TEAM_BLUE;
	}

	// equal team count, so join the team with the lowest score
	if (!g_redLocked.integer && level.teamScores[TEAM_RED] < level.teamScores[TEAM_BLUE]) {
		return TEAM_RED;
	}

	if (!g_blueLocked.integer && level.teamScores[TEAM_RED] > level.teamScores[TEAM_BLUE]) {
		return TEAM_BLUE;
	}

	// equal score, join random team
	if (!g_redLocked.integer && !g_blueLocked.integer) {
		return (rand() % 2 ? TEAM_RED : TEAM_BLUE);
	}

	return TEAM_SPECTATOR;
}

static void ClientCleanName(const char *in, char *out, int outSize)
{
	int outpos = 0, colorlessLen = 0, spaces = 0;

	// discard leading spaces
	for (; *in == ' '; in++);

	for (; *in && outpos < outSize - 1; in++) {
		out[outpos] = *in;

		if (*in == ' ') {
			// don't allow too many consecutive spaces
			if (spaces > 2)
				continue;

			spaces++;
		}
		else if (outpos > 0 && out[outpos - 1] == Q_COLOR_ESCAPE) {
			if (Q_IsColorString(&out[outpos - 1])) {
				colorlessLen--;

				if (ColorIndex(*in) == 0) {
					// Disallow color black in names to prevent players
					// from getting advantage playing in front of black backgrounds
					outpos--;
					continue;
				}
			}
			else {
				spaces = 0;
				colorlessLen++;
			}
		}
		else {
			spaces = 0;
			colorlessLen++;
		}

		outpos++;
	}

	out[outpos] = '\0';

	// don't allow empty names
	if (*out == '\0' || colorlessLen == 0)
		Q_strncpyz(out, "UnnamedPlayer", outSize);
}

void ClientPrint(gentity_t *ent, char *str)
{
	int	clientNum;
	clientNum = (ent ? ent - g_entities : -1);
	trap_SendServerCommand(clientNum, va("print \"%s\n\"", str));
}

/**
Called from ClientConnect when the player first connects and
directly by the server system when the player updates a userinfo variable.

The game can override any of the settings and call trap_SetUserinfo
if desired.
*/
void ClientUserinfoChanged(int clientNum)
{
	gentity_t *ent;
	int		teamTask, teamLeader, team, health;
	char	*s;
	char	oldname[MAX_STRING_CHARS];
	gclient_t	*client;
	char	userinfo[MAX_INFO_STRING];

	ent = g_entities + clientNum;
	client = ent->client;

	trap_GetUserinfo(clientNum, userinfo, sizeof(userinfo));

	// check for malformed or illegal info strings
	if (!Info_Validate(userinfo)) {
		strcpy (userinfo, "\\name\\badinfo");
		// don't keep those clients and userinfo
		trap_DropClient(clientNum, "Invalid userinfo");
	}

	// check for local client
	s = Info_ValueForKey(userinfo, "ip");
	if (!strcmp(s, "localhost")) {
		client->pers.localClient = qtrue;
	}

	// check the item prediction
	s = Info_ValueForKey(userinfo, "cg_predictItems");
	if (!atoi(s)) {
		client->pers.predictItemPickup = qfalse;
	} else {
		client->pers.predictItemPickup = qtrue;
	}

	// set name
	Q_strncpyz(oldname, client->pers.netname, sizeof(oldname));
	s = Info_ValueForKey (userinfo, "name");
	ClientCleanName(s, client->pers.netname, sizeof(client->pers.netname));

	if (client->sess.sessionTeam == TEAM_SPECTATOR) {
		if (client->sess.spectatorState == SPECTATOR_SCOREBOARD) {
			Q_strncpyz(client->pers.netname, "scoreboard", sizeof(client->pers.netname));
		}
	}

	if (client->pers.connected == CON_CONNECTED && strcmp(oldname, client->pers.netname)) {
		G_LogPrintf("%s" S_COLOR_WHITE " renamed to %s\n", oldname, client->pers.netname);
	}

	// set max health
	health = atoi(Info_ValueForKey(userinfo, "handicap"));
	client->pers.maxHealth = health;
	if (client->pers.maxHealth < 1 || client->pers.maxHealth > 100) {
		client->pers.maxHealth = 100;
	}
	client->ps.stats[STAT_MAX_HEALTH] = client->pers.maxHealth;

	// bots set their team a few frames later
	if (g_gametype.integer >= GT_TEAM && g_entities[clientNum].r.svFlags & SVF_BOT) {
		s = Info_ValueForKey(userinfo, "team");
		if (!Q_stricmp(s, "red") || !Q_stricmp(s, "r")) {
			team = TEAM_RED;
		} else if (!Q_stricmp(s, "blue") || !Q_stricmp(s, "b")) {
			team = TEAM_BLUE;
		} else {
			// pick the team with the least number of players
			team = PickTeam(clientNum);
		}
	}
	else {
		team = client->sess.sessionTeam;
	}

	// team task (0 = none, 1 = offence, 2 = defence)
	teamTask = atoi(Info_ValueForKey(userinfo, "teamtask"));
	// team Leader (1 = leader, 0 is normal player)
	teamLeader = client->sess.teamLeader;

	// send over a subset of the userinfo keys so other clients can
	// print scoreboards, display models, and play custom sounds
	if (ent->r.svFlags & SVF_BOT) {
		s = va("n\\%s\\t\\%i\\hc\\%i\\w\\%i\\l\\%i\\skill\\%s\\tt\\%d\\tl\\%d",
			client->pers.netname, team, client->pers.maxHealth,
			client->sess.wins, client->sess.losses,
			Info_ValueForKey(userinfo, "skill"), teamTask, teamLeader);
	}
	else {
		s = va("n\\%s\\t\\%i\\s\\%d\\r\\%i\\hc\\%i\\w\\%i\\l\\%i\\tt\\%d\\tl\\%d",
			client->pers.netname, team, client->sess.specOnly,
			client->pers.ready, client->pers.maxHealth,
			client->sess.wins, client->sess.losses, teamTask, teamLeader);
	}

	trap_SetConfigstring(CS_PLAYERS+clientNum, s);

	// this is not the userinfo, more like the configstring actually
	G_LogPrintf("ClientUserinfoChanged: %i %s\n", clientNum, s);
}

/**
Called when a player begins connecting to the server.
Called again for every map change or tournement restart.

The session information will be valid after exit.

Return NULL if the client should be allowed, otherwise return
a string with the reason for denial.

Otherwise, the client will be sent the current gamestate
and will eventually get to ClientBegin.

firstTime will be qtrue the very first time a client connects
to the server machine, but qfalse on map changes and tournement
restarts.
*/
char *ClientConnect(int clientNum, qboolean firstTime, qboolean isBot)
{
	char		*value;
	gclient_t	*client;
	char		userinfo[MAX_INFO_STRING];
	gentity_t	*ent;

	ent = &g_entities[ clientNum ];

	trap_GetUserinfo(clientNum, userinfo, sizeof(userinfo));

	// IP filtering
	// https://zerowing.idsoftware.com/bugzilla/show_bug.cgi?id=500
	// recommanding PB based IP / GUID banning, the builtin system is pretty limited
	// check to see if they are on the banned IP list
	value = Info_ValueForKey (userinfo, "ip");
	if (G_FilterPacket(value)) {
		return "You are banned from this server.";
	}

	// we don't check password for bots and local client
	// NOTE: local client <-> "ip" "localhost"
	// this means this client is not running in our current process
	if (!isBot && (strcmp(value, "localhost") != 0)) {
		// check for a password
		value = Info_ValueForKey (userinfo, "password");
		if (g_password.string[0] && Q_stricmp(g_password.string, "none") &&
			strcmp(g_password.string, value) != 0)
		{
			return "Invalid password";
		}
	}
	// if a player reconnects quickly after a disconnect, the client disconnect may never be called, thus flag can get lost in the ether
	if (ent->inuse) {
		G_LogPrintf("Forcing disconnect on active client: %i\n", clientNum);
		// so lets just fix up anything that should happen on a disconnect
		ClientDisconnect(clientNum);
	}
	// they can connect
	ent->client = level.clients + clientNum;
	client = ent->client;

	memset(client, 0, sizeof(*client));

	client->pers.connected = CON_CONNECTING;

	// read or initialize the session data
	if (firstTime || level.newSession) {
		G_InitSessionData(client, userinfo);
	}
	G_ReadSessionData(client);

	if (isBot) {
		ent->r.svFlags |= SVF_BOT;
		ent->inuse = qtrue;
		if (!G_BotConnect(clientNum, !firstTime)) {
			return "BotConnectfailed";
		}
	}

	// get and distribute relevent paramters
	G_LogPrintf("ClientConnect: %i\n", clientNum);
	ClientUserinfoChanged(clientNum);

	// don't do the "xxx connected" messages if they were caried over from previous level
	if (firstTime) {
		trap_SendServerCommand(-1, va("print \"%s" S_COLOR_WHITE " connected\n\"", client->pers.netname));
	}

	if (g_gametype.integer >= GT_TEAM && client->sess.sessionTeam != TEAM_SPECTATOR) {
		LogTeamChange(client, -1);
	}

	// count current clients and rank for scoreboard
	CalculateRanks();

	return NULL;
}

/**
Called when a client has finished connecting, and is ready
to be placed into the level.  This will happen every level load,
and on transition between teams, but doesn't happen on respawns
*/
void ClientBegin(int clientNum)
{
	gentity_t	*ent;
	gclient_t	*client;
	int			flags;

	ent = g_entities + clientNum;

	client = level.clients + clientNum;

	if (ent->r.linked) {
		trap_UnlinkEntity(ent);
	}
	G_InitGentity(ent);
	ent->touch = 0;
	ent->pain = 0;
	ent->client = client;

	client->pers.connected = CON_CONNECTED;
	client->pers.enterTime = level.time;
	client->pers.teamState.state = TEAM_BEGIN;

	// save eflags around this, because changing teams will
	// cause this to happen with a valid entity, and we
	// want to make sure the teleport bit is set right
	// so the viewpoint doesn't interpolate through the
	// world to the new position
	flags = client->ps.eFlags;
	memset(&client->ps, 0, sizeof(client->ps));
	client->ps.eFlags = flags;

	// locate ent at a spawn point
	ClientSpawn(ent);

	if (client->sess.sessionTeam != TEAM_SPECTATOR) {
		if (g_gametype.integer != GT_TOURNAMENT ) {
			trap_SendServerCommand(-1, va("print \"%s" S_COLOR_WHITE " entered the game\n\"", client->pers.netname));
		}
	}
	G_LogPrintf("ClientBegin: %i\n", clientNum);

	// count current clients and rank for scoreboard
	CalculateRanks();

	ent->client->pers.lastTarget = -1;
	ent->client->pers.lastKiller = -1;

	CheckPings(qtrue);
	G_SendScoreboard(client);
}

static void ClientGiveWeapons(gclient_t *client)
{
	int	i;

	if (client->ps.persistant[PERS_TEAM] == TEAM_SPECTATOR) {
		return;
	}

	client->ps.ammo[WP_GRAPPLING_HOOK] = -1;
	client->ps.ammo[WP_GAUNTLET] = -1;

	if (g_instantgib.integer) {
		client->ps.stats[STAT_WEAPONS] = (1 << WP_RAILGUN);
		client->ps.ammo[WP_RAILGUN] = -1;
	}

	if (g_instantgibGauntlet.integer) {
		client->ps.stats[STAT_WEAPONS] |= (1 << WP_GAUNTLET);
	}

	if (g_rockets.integer) {
		client->ps.stats[STAT_WEAPONS] |= (1 << WP_ROCKET_LAUNCHER);
		client->ps.ammo[WP_ROCKET_LAUNCHER] = -1;
	}

	if (g_gametype.integer == GT_ELIMINATION && level.warmupTime == -1
		&& !g_rockets.integer && !g_instantgib.integer)
	{
		for (i = WP_MACHINEGUN; i < WP_BFG; ++i) {
			client->ps.stats[STAT_WEAPONS] |= (1 << i);
			client->ps.ammo[i] = -1;
		}
	} else if (!g_rockets.integer && !g_instantgib.integer) {
		client->ps.stats[STAT_WEAPONS] = (1 << WP_GAUNTLET);
		client->ps.stats[STAT_WEAPONS] |= (1 << WP_MACHINEGUN);

		if (g_gametype.integer == GT_TEAM) {
			client->ps.ammo[WP_MACHINEGUN] = 50;
		} else {
			client->ps.ammo[WP_MACHINEGUN] = 100;
		}

		if (g_gametype.integer == GT_ELIMINATION) {
			client->ps.stats[STAT_WEAPONS] |= (1 << WP_SHOTGUN);
			client->ps.stats[STAT_WEAPONS] |= (1 << WP_GRENADE_LAUNCHER);
			client->ps.stats[STAT_WEAPONS] |= (1 << WP_ROCKET_LAUNCHER);
			client->ps.stats[STAT_WEAPONS] |= (1 << WP_LIGHTNING);
			client->ps.stats[STAT_WEAPONS] |= (1 << WP_RAILGUN);
			client->ps.stats[STAT_WEAPONS] |= (1 << WP_PLASMAGUN);
			client->ps.ammo[WP_SHOTGUN] = 30;
			client->ps.ammo[WP_GRENADE_LAUNCHER] = 20;
			client->ps.ammo[WP_ROCKET_LAUNCHER] = 50;
			client->ps.ammo[WP_LIGHTNING] = 200;
			client->ps.ammo[WP_RAILGUN] = 20;
			client->ps.ammo[WP_PLASMAGUN] = 150;
		}
	}

	if (g_gametype.integer == GT_ELIMINATION) {
		client->ps.weapon = WP_ROCKET_LAUNCHER;
		return;
	}

	// force the base weapon up
	client->ps.weapon = WP_MACHINEGUN;
	client->ps.weaponstate = WEAPON_READY;

	// select the highest weapon number available, after any spawn given items have fired
	for (i = WP_NUM_WEAPONS - 1; i > 0; i--) {
		if (client->ps.stats[STAT_WEAPONS] & (1 << i)) {
			client->ps.weapon = i;
			break;
		}
	}
}

/**
Called every time a client is placed fresh in the world:
after the first ClientBegin, and after each respawn
Initializes all non-persistant parts of playerState
*/
void ClientSpawn(gentity_t *ent)
{
	int		index;
	vec3_t	spawn_origin, spawn_angles;
	gclient_t	*client;
	clientPersistant_t	saved;
	clientSession_t		savedSess;
	int				persistant[MAX_PERSISTANT];
	gentity_t		*spawnPoint;
	gentity_t		*tent;
	int		flags;
	int		eventSequence;
	char	userinfo[MAX_INFO_STRING];

	index = ent - g_entities;
	client = ent->client;

	// follow other players when eliminated
	if (!level.warmupTime && g_gametype.integer == GT_ELIMINATION
		&& client->sess.sessionTeam != TEAM_SPECTATOR)
	{
		if (level.roundStarted) {
			client->sess.spectatorState = SPECTATOR_FREE;
			client->eliminated = qtrue;
			return;
		} else {
			client->sess.spectatorState = SPECTATOR_NOT;
			client->eliminated = qfalse;
		}
	}

	VectorClear(spawn_origin);

	// find a spawn point
	// do it before setting health back up, so farthest
	// ranging doesn't count this client
	if (client->sess.sessionTeam == TEAM_SPECTATOR) {
		spawnPoint = SelectSpectatorSpawnPoint (
						spawn_origin, spawn_angles);
	} else if (g_gametype.integer >= GT_CTF) {
		// all base oriented team games use the CTF spawn points
		spawnPoint = SelectCTFSpawnPoint (
						client->sess.sessionTeam,
						client->pers.teamState.state,
						spawn_origin, spawn_angles,
						!!(ent->r.svFlags & SVF_BOT));
	}
	else {
		// the first spawn should be at a good looking spot
		if (!client->pers.initialSpawn && client->pers.localClient) {
			client->pers.initialSpawn = qtrue;
			spawnPoint = SelectInitialSpawnPoint(spawn_origin, spawn_angles,
							     !!(ent->r.svFlags & SVF_BOT));
		}
		else {
			// don't spawn near existing origin if possible
			spawnPoint = SelectSpawnPoint (
				client->ps.origin,
				spawn_origin, spawn_angles, !!(ent->r.svFlags & SVF_BOT));
		}
	}
	client->pers.teamState.state = TEAM_ACTIVE;

	// toggle the teleport bit so the client knows to not lerp
	// and never clear the voted flag
	flags = ent->client->ps.eFlags & (EF_TELEPORT_BIT | EF_TEAMVOTED);
	flags ^= EF_TELEPORT_BIT;

	// clear everything but the persistant data

	saved = client->pers;
	savedSess = client->sess;
	eventSequence = client->ps.eventSequence;
	Com_Memcpy(persistant, client->ps.persistant, sizeof persistant);

	Com_Memset(client, 0, sizeof(*client));

	client->pers = saved;
	client->sess = savedSess;
	client->ps.eventSequence = eventSequence;
	Com_Memcpy(client->ps.persistant, persistant, sizeof client->ps.persistant);

	// increment the spawncount so the client will detect the respawn
	client->ps.persistant[PERS_SPAWN_COUNT]++;
	client->ps.persistant[PERS_TEAM] = client->sess.sessionTeam;

	client->airOutTime = level.time + 12000;

	trap_GetUserinfo(index, userinfo, sizeof(userinfo));
	// set max health
	client->pers.maxHealth = atoi(Info_ValueForKey(userinfo, "handicap"));
	if (client->pers.maxHealth < 1 || client->pers.maxHealth > 100) {
		client->pers.maxHealth = 100;
	}
	// clear entity values
	client->ps.stats[STAT_MAX_HEALTH] = client->pers.maxHealth;
	client->ps.eFlags = flags;

	ent->s.groundEntityNum = ENTITYNUM_NONE;
	ent->client = &level.clients[index];
	ent->takedamage = qtrue;
	ent->inuse = qtrue;
	ent->classname = "player";
	ent->r.contents = CONTENTS_BODY;
	ent->clipmask = MASK_PLAYERSOLID;
	ent->die = player_die;
	ent->waterlevel = 0;
	ent->watertype = 0;
	ent->flags = 0;

	VectorCopy(playerMins, ent->r.mins);
	VectorCopy(playerMaxs, ent->r.maxs);

	client->ps.clientNum = index;

	ClientGiveWeapons(client);

	if (g_gametype.integer == GT_ELIMINATION) {
		client->ps.stats[STAT_HEALTH] = client->ps.stats[STAT_MAX_HEALTH] * 2;
		client->ps.stats[STAT_ARMOR] = client->ps.stats[STAT_MAX_HEALTH] * 1.5;
	} else {
		client->ps.stats[STAT_HEALTH] = client->ps.stats[STAT_MAX_HEALTH] + 25;
	}
	ent->health = client->ps.stats[STAT_HEALTH];

	G_SetOrigin(ent, spawn_origin);
	VectorCopy(spawn_origin, client->ps.origin);

	// the respawned flag will be cleared after the attack and jump keys come up
	client->ps.pm_flags |= PMF_RESPAWNED;

	trap_GetUsercmd(client - level.clients, &ent->client->pers.cmd);
	SetClientViewAngle(ent, spawn_angles);
	// don't allow full run speed for a bit
	client->ps.pm_flags |= PMF_TIME_KNOCKBACK;
	client->ps.pm_time = 100;

	client->respawnTime = level.time;
	client->inactivityTime = level.time + g_inactivity.integer * 1000;
	client->latched_buttons = 0;

	// set default animations
	client->ps.torsoAnim = TORSO_STAND;
	client->ps.legsAnim = LEGS_IDLE;

	if (level.intermissiontime) {
		// move players to intermission
		MoveClientToIntermission(ent);
	} else if (ent->client->sess.sessionTeam != TEAM_SPECTATOR) {
		G_KillBox(ent);

		// fire the targets of the spawn point
		G_UseTargets(spawnPoint, ent);

		// positively link the client, even if the command times are weird
		VectorCopy(ent->client->ps.origin, ent->r.currentOrigin);

		tent = G_TempEntity(ent->client->ps.origin, EV_PLAYER_TELEPORT_IN);
		tent->s.clientNum = ent->s.clientNum;

		trap_LinkEntity (ent);
	}

	// run a client frame to drop exactly to the floor,
	// initialize animations and other things
	client->ps.commandTime = level.time - 100;
	client->pers.cmd.serverTime = level.time;
	ClientThink(ent-g_entities);
	// run the presend to set anything else, follow spectators wait
	// until all clients have been reconnected after map_restart
	if (ent->client->sess.spectatorState != SPECTATOR_FOLLOW) {
		ClientEndFrame(ent);
	}

	// clear entity state values
	BG_PlayerStateToEntityState(&client->ps, &ent->s, qtrue);
}

/**
Called when a player drops from the server.
Will not be called between levels.

This should NOT be called directly by any game logic,
call trap_DropClient(), which will call this and do
server system housekeeping.
*/
void ClientDisconnect(int clientNum)
{
	gentity_t	*ent;
	gentity_t	*tent;
	int			i;

	// cleanup if we are kicking a bot that
	// hasn't spawned yet
	G_RemoveQueuedBotBegin(clientNum);

	ent = g_entities + clientNum;
	if (!ent->client || ent->client->pers.connected == CON_DISCONNECTED) {
		return;
	}

	if (ent->client->pers.connected == CON_CONNECTED && !level.warmupTime) {
		gclient_t	*disco;
		disco = &level.disconnectedClients[level.numDisconnectedClients];
		*disco = *ent->client;
		disco->pers.enterTime = level.time - disco->pers.enterTime;
		level.numDisconnectedClients++;
	}

	// stop any following clients
	for (i = 0; i < level.maxclients; i++) {
		if (level.clients[i].sess.sessionTeam == TEAM_SPECTATOR
			&& level.clients[i].sess.spectatorState == SPECTATOR_FOLLOW
			&& level.clients[i].sess.spectatorClient == clientNum) {
			StopFollowing(&g_entities[i]);
		}
	}

	// send effect if they were completely connected
	if (ent->client->pers.connected == CON_CONNECTED
		&& ent->client->sess.sessionTeam != TEAM_SPECTATOR) {
		tent = G_TempEntity(ent->client->ps.origin, EV_PLAYER_TELEPORT_OUT);
		tent->s.clientNum = ent->s.clientNum;

		// They don't get to take powerups with them!
		// Especially important for stuff like CTF flags
		TossClientItems(ent);
	}

	G_LogPrintf("ClientDisconnect: %i\n", clientNum);

	// if we are playing in tourney mode and losing, give a win to the other player
	if ((g_gametype.integer == GT_TOURNAMENT)
		&& !level.intermissiontime
		&& !level.warmupTime && level.sortedClients[1] == clientNum) {
		level.clients[ level.sortedClients[0] ].sess.wins++;
		ClientUserinfoChanged(level.sortedClients[0]);
	}

	if (g_gametype.integer == GT_TOURNAMENT &&
		ent->client->sess.sessionTeam == TEAM_FREE &&
		level.intermissiontime) {

		trap_SendConsoleCommand(EXEC_APPEND, "map_restart\n");
		level.restarted = qtrue;
		level.changemap = NULL;
		level.intermissiontime = 0;
	}

	trap_UnlinkEntity (ent);
	ent->s.modelindex = 0;
	ent->inuse = qfalse;
	ent->classname = "disconnected";
	ent->client->pers.connected = CON_DISCONNECTED;
	ent->client->ps.persistant[PERS_TEAM] = TEAM_FREE;
	ent->client->sess.spectatorState = SPECTATOR_NOT;
	ent->client->sess.sessionTeam = TEAM_FREE;

	trap_SetConfigstring(CS_PLAYERS + clientNum, "");

	CalculateRanks();

	G_Vote_UpdateCount();

	if (ent->r.svFlags & SVF_BOT) {
		BotAIShutdownClient(clientNum, qfalse);
	}
}

gclient_t *ClientFromString(const char *str)
{
	gclient_t	*cl;
	int			idnum;
	char		cleanName[MAX_STRING_CHARS];

	// numeric values could be slot numbers
	if (Q_isanumber(str) && Q_isintegral(atof(str))) {
		idnum = atoi(str);
		if (idnum >= 0 && idnum < level.maxclients) {
			cl = &level.clients[idnum];
			if (cl->pers.connected == CON_CONNECTED) {
				return cl;
			}
		}
	}

	// check for a name match
	for (idnum = 0, cl = level.clients; idnum < level.maxclients; idnum++, cl++) {
		if (cl->pers.connected != CON_CONNECTED) {
			continue;
		}
		Q_strncpyz(cleanName, cl->pers.netname, sizeof cleanName);
		Q_CleanStr(cleanName);
		if (!Q_stricmp(cleanName, str)) {
			return cl;
		}
	}

	return NULL;
}


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
// g_arenas.c

#include "g_local.h"

#define TIMER_GESTURE	(34 * 66 + 50)

gentity_t	*podium1;
gentity_t	*podium2;
gentity_t	*podium3;

static vec3_t	offsetFirst  = {   0,   0, 74 };
static vec3_t	offsetSecond = { -10,  60, 54 };
static vec3_t	offsetThird  = { -19, -60, 45 };

void UpdateTournamentInfo(void)
{
}

static gentity_t *SpawnModelOnVictoryPad(gentity_t *pad, vec3_t offset, gentity_t *ent, int place)
{
	gentity_t	*body;
	vec3_t		vec;
	vec3_t		f, r, u;

	body = G_Spawn();
	if (!body) {
		G_Printf(S_COLOR_RED "ERROR: out of gentities\n");
		return NULL;
	}

	body->classname = ent->client->pers.netname;
	body->client = ent->client;
	body->s = ent->s;
	body->s.eType = ET_PLAYER;		// could be ET_INVISIBLE
	body->s.eFlags = 0;				// clear EF_TALK, etc
	body->s.powerups = 0;			// clear powerups
	body->s.loopSound = 0;			// clear lava burning
	body->s.number = body - g_entities;
	body->timestamp = level.time;
	body->physicsObject = qtrue;
	body->physicsBounce = 0;		// don't bounce
	body->s.event = 0;
	body->s.pos.trType = TR_STATIONARY;
	body->s.groundEntityNum = ENTITYNUM_WORLD;
	body->s.legsAnim = LEGS_IDLE;
	body->s.torsoAnim = TORSO_STAND;
	if (body->s.weapon == WP_NONE) {
		body->s.weapon = WP_MACHINEGUN;
	}
	if (body->s.weapon == WP_GAUNTLET) {
		body->s.torsoAnim = TORSO_STAND2;
	}
	body->s.event = 0;
	body->r.svFlags = ent->r.svFlags;
	VectorCopy (ent->r.mins, body->r.mins);
	VectorCopy (ent->r.maxs, body->r.maxs);
	VectorCopy (ent->r.absmin, body->r.absmin);
	VectorCopy (ent->r.absmax, body->r.absmax);
	body->clipmask = CONTENTS_SOLID | CONTENTS_PLAYERCLIP;
	body->r.contents = CONTENTS_BODY;
	body->r.ownerNum = ent->r.ownerNum;
	body->takedamage = qfalse;

	VectorSubtract(level.intermission_origin, pad->r.currentOrigin, vec);
	vectoangles(vec, body->s.apos.trBase);
	body->s.apos.trBase[PITCH] = 0;
	body->s.apos.trBase[ROLL] = 0;

	AngleVectors(body->s.apos.trBase, f, r, u);
	VectorMA(pad->r.currentOrigin, offset[0], f, vec);
	VectorMA(vec, offset[1], r, vec);
	VectorMA(vec, offset[2], u, vec);

	G_SetOrigin(body, vec);

	trap_LinkEntity (body);

	body->count = place;

	return body;
}

static void CelebrateStop(gentity_t *player)
{
	int		anim;

	if (player->s.weapon == WP_GAUNTLET) {
		anim = TORSO_STAND2;
	}
	else {
		anim = TORSO_STAND;
	}
	player->s.torsoAnim = ((player->s.torsoAnim & ANIM_TOGGLEBIT) ^ ANIM_TOGGLEBIT) | anim;
}

static void CelebrateStart(gentity_t *player)
{
	player->s.torsoAnim = ((player->s.torsoAnim & ANIM_TOGGLEBIT) ^ ANIM_TOGGLEBIT) | TORSO_GESTURE;
	player->nextthink = level.time + TIMER_GESTURE;
	player->think = CelebrateStop;
	G_AddEvent(player, EV_TAUNT, 0);
}

static void PodiumPlacementThink(gentity_t *podium)
{
	vec3_t		vec;
	vec3_t		origin;
	vec3_t		f, r, u;

	podium->nextthink = level.time + 100;

	AngleVectors(level.intermission_angle, vec, NULL, NULL);
	VectorMA(level.intermission_origin, trap_Cvar_VariableIntegerValue("g_podiumDist"), vec, origin);
	origin[2] -= trap_Cvar_VariableIntegerValue("g_podiumDrop");
	G_SetOrigin(podium, origin);

	if (podium1) {
		VectorSubtract(level.intermission_origin, podium->r.currentOrigin, vec);
		vectoangles(vec, podium1->s.apos.trBase);
		podium1->s.apos.trBase[PITCH] = 0;
		podium1->s.apos.trBase[ROLL] = 0;

		AngleVectors(podium1->s.apos.trBase, f, r, u);
		VectorMA(podium->r.currentOrigin, offsetFirst[0], f, vec);
		VectorMA(vec, offsetFirst[1], r, vec);
		VectorMA(vec, offsetFirst[2], u, vec);

		G_SetOrigin(podium1, vec);
	}

	if (podium2) {
		VectorSubtract(level.intermission_origin, podium->r.currentOrigin, vec);
		vectoangles(vec, podium2->s.apos.trBase);
		podium2->s.apos.trBase[PITCH] = 0;
		podium2->s.apos.trBase[ROLL] = 0;

		AngleVectors(podium2->s.apos.trBase, f, r, u);
		VectorMA(podium->r.currentOrigin, offsetSecond[0], f, vec);
		VectorMA(vec, offsetSecond[1], r, vec);
		VectorMA(vec, offsetSecond[2], u, vec);

		G_SetOrigin(podium2, vec);
	}

	if (podium3) {
		VectorSubtract(level.intermission_origin, podium->r.currentOrigin, vec);
		vectoangles(vec, podium3->s.apos.trBase);
		podium3->s.apos.trBase[PITCH] = 0;
		podium3->s.apos.trBase[ROLL] = 0;

		AngleVectors(podium3->s.apos.trBase, f, r, u);
		VectorMA(podium->r.currentOrigin, offsetThird[0], f, vec);
		VectorMA(vec, offsetThird[1], r, vec);
		VectorMA(vec, offsetThird[2], u, vec);

		G_SetOrigin(podium3, vec);
	}
}

static gentity_t *SpawnPodium(void)
{
	gentity_t	*podium;
	vec3_t		vec;
	vec3_t		origin;

	podium = G_Spawn();
	if (!podium) {
		return NULL;
	}

	podium->classname = "podium";
	podium->s.eType = ET_GENERAL;
	podium->s.number = podium - g_entities;
	podium->clipmask = CONTENTS_SOLID;
	podium->r.contents = CONTENTS_SOLID;
	podium->s.modelindex = G_ModelIndex(SP_PODIUM_MODEL);

	AngleVectors(level.intermission_angle, vec, NULL, NULL);
	VectorMA(level.intermission_origin, trap_Cvar_VariableIntegerValue("g_podiumDist"), vec, origin);
	origin[2] -= trap_Cvar_VariableIntegerValue("g_podiumDrop");
	G_SetOrigin(podium, origin);

	VectorSubtract(level.intermission_origin, podium->r.currentOrigin, vec);
	podium->s.apos.trBase[YAW] = vectoyaw(vec);
	trap_LinkEntity (podium);

	podium->think = PodiumPlacementThink;
	podium->nextthink = level.time + 100;
	return podium;
}

void SpawnModelsOnVictoryPads(void)
{
	gentity_t	*player;
	gentity_t	*podium;

	podium1 = NULL;
	podium2 = NULL;
	podium3 = NULL;

	podium = SpawnPodium();

	player = SpawnModelOnVictoryPad(podium, offsetFirst, &g_entities[level.sortedClients[0]],
				level.clients[ level.sortedClients[0] ].ps.persistant[PERS_RANK] &~ RANK_TIED_FLAG);
	if (player) {
		player->nextthink = level.time + 2000;
		player->think = CelebrateStart;
		podium1 = player;
	}

	player = SpawnModelOnVictoryPad(podium, offsetSecond, &g_entities[level.sortedClients[1]],
				level.clients[ level.sortedClients[1] ].ps.persistant[PERS_RANK] &~ RANK_TIED_FLAG);
	if (player) {
		podium2 = player;
	}

	if (level.numNonSpectatorClients > 2) {
		player = SpawnModelOnVictoryPad(podium, offsetThird, &g_entities[level.sortedClients[2]],
				level.clients[ level.sortedClients[2] ].ps.persistant[PERS_RANK] &~ RANK_TIED_FLAG);
		if (player) {
			podium3 = player;
		}
	}
}

void Svcmd_AbortPodium_f(void)
{
	if (g_gametype.integer != GT_SINGLE_PLAYER) {
		return;
	}

	if (podium1) {
		podium1->nextthink = level.time;
		podium1->think = CelebrateStop;
	}
}


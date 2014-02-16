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
// g_weapon.c
// perform the server side effects of a weapon firing

#include "g_local.h"

// spreads are in bg_public.h, because client predicts them
#define	DEFAULT_SHOTGUN_DAMAGE	10
#define	MACHINEGUN_DAMAGE		7
#define	MACHINEGUN_TEAM_DAMAGE	5		// wimpier MG in teamplay
#define MAX_RAIL_HITS			4

static float	s_quadFactor;
static qboolean	teamHit, enemyHit;
static vec3_t	forward, right, up;
static vec3_t	muzzle;

void G_BounceProjectile(vec3_t start, vec3_t impact, vec3_t dir, vec3_t endout)
{
	vec3_t v, newv;
	float dot;

	VectorSubtract(impact, start, v);
	dot = DotProduct(v, dir);
	VectorMA(v, -2*dot, dir, newv);

	VectorNormalize(newv);
	VectorMA(impact, MAX_WEAPON_RANGE, newv, endout);
}

qboolean CheckGauntletAttack(gentity_t *ent)
{
	trace_t		tr;
	vec3_t		end;
	gentity_t	*tent;
	gentity_t	*traceEnt;
	int			damage;

	// set aiming directions
	AngleVectors (ent->client->ps.viewangles, forward, right, up);

	CalcMuzzlePoint (ent, forward, right, up, muzzle);

	VectorMA (muzzle, 32, forward, end);

	trap_Trace (&tr, muzzle, NULL, NULL, end, ent->s.number, MASK_SHOT);
	if (tr.surfaceFlags & SURF_NOIMPACT) {
		return qfalse;
	}

	if (ent->client->noclip) {
		return qfalse;
	}

	traceEnt = &g_entities[tr.entityNum];

	// send blood impact
	if (traceEnt->takedamage && traceEnt->client) {
		tent = G_TempEntity(tr.endpos, EV_MISSILE_HIT);
		tent->s.otherEntityNum = traceEnt->s.number;
		tent->s.eventParm = DirToByte(tr.plane.normal);
		tent->s.weapon = ent->s.weapon;
	}

	if (!traceEnt->takedamage) {
		return qfalse;
	}

	if (ent->client->ps.powerups[PW_QUAD]) {
		G_AddEvent(ent, EV_POWERUP_QUAD, 0);
		s_quadFactor = g_quadfactor.value;
	} else {
		s_quadFactor = 1;
	}

	damage = (g_instantgib.integer ? 500: 50 * s_quadFactor);
	G_Damage(traceEnt, ent, ent, forward, tr.endpos, damage, 0, MOD_GAUNTLET);

	return qtrue;
}

static void Weapon_Machinegun_Fire(gentity_t *ent)
{
	trace_t		tr;
	vec3_t		end;
	float		r;
	float		u;
	gentity_t	*tent;
	gentity_t	*traceEnt;
	int			i, passent;
	int			damage;

	if (g_gametype.integer >= GT_TEAM) {
		damage = MACHINEGUN_TEAM_DAMAGE * s_quadFactor;
	} else {
		damage = MACHINEGUN_DAMAGE * s_quadFactor;
	}

	r = random() * M_PI * 2.0f;
	u = sin(r) * crandom() * MACHINEGUN_SPREAD * 16;
	r = cos(r) * crandom() * MACHINEGUN_SPREAD * 16;
	VectorMA (muzzle, MAX_WEAPON_RANGE, forward, end);
	VectorMA (end, r, right, end);
	VectorMA (end, u, up, end);

	passent = ent->s.number;
	for (i = 0; i < 10; i++) {
		trap_Trace (&tr, muzzle, NULL, NULL, end, passent, MASK_SHOT);
		if (tr.surfaceFlags & SURF_NOIMPACT) {
			return;
		}

		traceEnt = &g_entities[ tr.entityNum ];

		// snap the endpos to integers, but nudged towards the line
		BG_SnapVectorTowards(tr.endpos, muzzle);

		LogAccuracyHit(traceEnt, ent, &teamHit, &enemyHit);

		// send bullet impact
		if (traceEnt->takedamage && traceEnt->client) {
			tent = G_TempEntity(tr.endpos, EV_BULLET_HIT_FLESH);
			tent->s.eventParm = traceEnt->s.number;
		} else {
			tent = G_TempEntity(tr.endpos, EV_BULLET_HIT_WALL);
			tent->s.eventParm = DirToByte(tr.plane.normal);
		}
		tent->s.otherEntityNum = ent->s.number;

		if (traceEnt->takedamage) {
			G_Damage(traceEnt, ent, ent, forward, tr.endpos, damage, 0, MOD_MACHINEGUN);
		}
		break;
	}
}

static void BFG_Fire (gentity_t *ent)
{
	gentity_t	*m;

	m = fire_bfg (ent, muzzle, forward);
	m->damage *= s_quadFactor;
	m->splashDamage *= s_quadFactor;
}

static qboolean ShotgunPellet(vec3_t start, vec3_t end, gentity_t *ent)
{
	trace_t		tr;
	int			damage, i, passent;
	gentity_t	*traceEnt;
	vec3_t		tr_start, tr_end;

	passent = ent->s.number;
	VectorCopy(start, tr_start);
	VectorCopy(end, tr_end);
	for (i = 0; i < 10; i++) {
		trap_Trace (&tr, tr_start, NULL, NULL, tr_end, passent, MASK_SHOT);
		traceEnt = &g_entities[tr.entityNum];

		// send bullet impact
		if (tr.surfaceFlags & SURF_NOIMPACT) {
			return qfalse;
		}

		if (traceEnt->takedamage) {
			damage = DEFAULT_SHOTGUN_DAMAGE * s_quadFactor;
			G_Damage(traceEnt, ent, ent, forward, tr.endpos, damage, 0, MOD_SHOTGUN);
			if (LogAccuracyHit(traceEnt, ent, &teamHit, &enemyHit)) {
				return qtrue;
			}
		}
		return qfalse;
	}
	return qfalse;
}

/**
This should match CG_ShotgunPattern
*/
static void ShotgunPattern(vec3_t origin, vec3_t origin2, int seed, gentity_t *ent)
{
	int			i;
	float		r, u;
	vec3_t		end;
	vec3_t		forward, right, up;
	int			hits;

	// derive the right and up vectors from the forward vector, because
	// the client won't have any other information
	VectorNormalize2(origin2, forward);
	PerpendicularVector(right, forward);
	CrossProduct(forward, right, up);

	// generate the "random" spread pattern
	for (i = 0; i < DEFAULT_SHOTGUN_COUNT; i++) {
		r = Q_crandom(&seed) * DEFAULT_SHOTGUN_SPREAD * 16;
		u = Q_crandom(&seed) * DEFAULT_SHOTGUN_SPREAD * 16;
		VectorMA(origin, MAX_WEAPON_RANGE, forward, end);
		VectorMA(end, r, right, end);
		VectorMA(end, u, up, end);
		if (ShotgunPellet(origin, end, ent)) {
			++hits;
		}
	}

	if (hits == DEFAULT_SHOTGUN_COUNT) {
		ent->client->pers.stats.rewards[REWARD_FULLSG]++;
	}
}

static void Weapon_Shotgun_Fire(gentity_t *ent)
{
	gentity_t		*tent;

	// send shotgun blast
	tent = G_TempEntity(muzzle, EV_SHOTGUN);
	VectorScale(forward, 4096, tent->s.origin2);
	SnapVector(tent->s.origin2);
	tent->s.eventParm = rand() & 255;		// seed for spread pattern
	tent->s.otherEntityNum = ent->s.number;

	ShotgunPattern(tent->s.pos.trBase, tent->s.origin2, tent->s.eventParm, ent);
}

static void Weapon_Grenadelauncher_Fire(gentity_t *ent)
{
	gentity_t	*m;

	// extra vertical velocity
	forward[2] += 0.2f;
	VectorNormalize(forward);

	m = fire_grenade (ent, muzzle, forward);
	m->damage *= s_quadFactor;
	m->splashDamage *= s_quadFactor;
}

static void Weapon_RocketLauncher_Fire(gentity_t *ent)
{
	gentity_t	*m;

	m = fire_rocket (ent, muzzle, forward);
	m->damage *= s_quadFactor;
	m->splashDamage *= s_quadFactor;
}

static void Weapon_Plasmagun_Fire(gentity_t *ent)
{
	gentity_t	*m;

	m = fire_plasma (ent, muzzle, forward);
	m->damage *= s_quadFactor;
	m->splashDamage *= s_quadFactor;
}

static void Weapon_Railgun_Fire(gentity_t *ent)
{
	vec3_t		end;
	trace_t		trace;
	gentity_t	*tent;
	gentity_t	*traceEnt;
	int			damage;
	int			i;
	int			hits;
	int			unlinked;
	int			passent;
	gentity_t	*unlinkedEntities[MAX_RAIL_HITS];

	damage = (g_instantgib.integer ? 800 : 80 * s_quadFactor);

	VectorMA (muzzle, MAX_WEAPON_RANGE, forward, end);

	// trace only against the solids, so the railgun will go through people
	unlinked = 0;
	hits = 0;
	passent = ent->s.number;
	do {
		trap_Trace (&trace, muzzle, NULL, NULL, end, passent, MASK_SHOT);
		if (trace.entityNum >= ENTITYNUM_MAX_NORMAL) {
			break;
		}
		traceEnt = &g_entities[ trace.entityNum ];
		if (traceEnt->takedamage) {
			if (LogAccuracyHit(traceEnt, ent, &teamHit, &enemyHit)) {
				hits++;
			}
			G_Damage(traceEnt, ent, ent, forward, trace.endpos, damage, 0, MOD_RAILGUN);
		}
		if (trace.contents & CONTENTS_SOLID) {
			break;		// we hit something solid enough to stop the beam
		}
		// unlink this entity, so the next trace will go past it
		trap_UnlinkEntity(traceEnt);
		unlinkedEntities[unlinked] = traceEnt;
		unlinked++;
	} while (unlinked < MAX_RAIL_HITS);

	// link back in any entities we unlinked
	for (i = 0; i < unlinked; i++) {
		trap_LinkEntity(unlinkedEntities[i]);
	}

	// the final trace endpos will be the terminal point of the rail trail

	// snap the endpos to integers to save net bandwidth, but nudged towards the line
	BG_SnapVectorTowards(trace.endpos, muzzle);

	// send railgun beam effect
	tent = G_TempEntity(trace.endpos, EV_RAILTRAIL);

	// set player number for custom colors on the railtrail
	tent->s.clientNum = ent->s.clientNum;

	VectorCopy(muzzle, tent->s.origin2);
	// move origin a bit to come closer to the drawn gun muzzle
	VectorMA(tent->s.origin2, 4, right, tent->s.origin2);
	VectorMA(tent->s.origin2, -1, up, tent->s.origin2);

	// no explosion at end if SURF_NOIMPACT, but still make the trail
	if (trace.surfaceFlags & SURF_NOIMPACT) {
		tent->s.eventParm = 255;	// don't make the explosion at the end
	} else {
		tent->s.eventParm = DirToByte(trace.plane.normal);
	}
	tent->s.clientNum = ent->s.clientNum;

	// give the shooter a reward sound if they have made two railgun hits in a row
	if (hits == 0) {
		// complete miss
		ent->client->accurateCount = 0;
	} else {
		// check for "impressive" reward sound
		ent->client->accurateCount += hits;
		if (ent->client->accurateCount >= 2) {
			ent->client->accurateCount -= 2;
			ent->client->pers.stats.rewards[REWARD_IMPRESSIVE]++;
			// add the sprite over the player's head
			ent->client->ps.eFlags &= ~(EF_AWARD_IMPRESSIVE | EF_AWARD_EXCELLENT | EF_AWARD_GAUNTLET | EF_AWARD_ASSIST | EF_AWARD_DEFEND | EF_AWARD_CAP);
			ent->client->ps.eFlags |= EF_AWARD_IMPRESSIVE;
			ent->client->rewardTime = level.time + REWARD_SPRITE_TIME;
		}
	}

	if (g_instantgib.integer && g_instantgibRailjump.integer) {
		G_RadiusKnockback(trace.endpos, ent, 100, 120.0f);
	}
}

static void Weapon_GrapplingHook_Fire(gentity_t *ent)
{
	if (!ent->client->fireHeld && !ent->client->hook)
		fire_grapple (ent, muzzle, forward);

	ent->client->fireHeld = qtrue;
}

static void Weapon_Lightning_Fire(gentity_t *ent)
{
	trace_t		tr;
	vec3_t		end;
	gentity_t	*traceEnt, *tent;
	int			damage, i, passent;
	int			lastTarget;

	damage = 7 * s_quadFactor;

	passent = ent->s.number;
	for (i = 0; i < 10; i++) {
		VectorMA(muzzle, LIGHTNING_RANGE, forward, end);

		trap_Trace(&tr, muzzle, NULL, NULL, end, passent, MASK_SHOT);

		if (tr.entityNum == ENTITYNUM_NONE) {
			return;
		}

		traceEnt = &g_entities[ tr.entityNum ];

		if (traceEnt->takedamage) {
			lastTarget = ent->client->pers.lastTarget;
			G_Damage(traceEnt, ent, ent, forward, tr.endpos, damage, 0, MOD_LIGHTNING);
		}

		if (traceEnt->takedamage && traceEnt->client) {
			tent = G_TempEntity(tr.endpos, EV_MISSILE_HIT);
			tent->s.otherEntityNum = traceEnt->s.number;
			tent->s.eventParm = DirToByte(tr.plane.normal);
			tent->s.weapon = ent->s.weapon;
			if (LogAccuracyHit(traceEnt, ent, &teamHit, &enemyHit)) {
				if (ent->client->pers.lastTarget == lastTarget) {
					ent->client->lightningHits++;
				} else {
					ent->client->lightningHits = 0;
				}
			}

			if (ent->client->lightningHits >= 20) {
				ent->client->pers.stats.rewards[REWARD_LGACCURACY]++;
				ent->client->lightningHits = 0;
			}
		} else if (!(tr.surfaceFlags & SURF_NOIMPACT)) {
			tent = G_TempEntity(tr.endpos, EV_MISSILE_MISS);
			tent->s.eventParm = DirToByte(tr.plane.normal);
		}

		break;
	}
}

void Weapon_HookFree(gentity_t *ent)
{
	ent->parent->client->hook = NULL;
	ent->parent->client->ps.pm_flags &= ~PMF_GRAPPLE_PULL;
	G_FreeEntity(ent);
}

void Weapon_HookThink(gentity_t *ent)
{
	if (ent->enemy) {
		vec3_t v, oldorigin;

		VectorCopy(ent->r.currentOrigin, oldorigin);
		v[0] = ent->enemy->r.currentOrigin[0] + (ent->enemy->r.mins[0] + ent->enemy->r.maxs[0]) * 0.5;
		v[1] = ent->enemy->r.currentOrigin[1] + (ent->enemy->r.mins[1] + ent->enemy->r.maxs[1]) * 0.5;
		v[2] = ent->enemy->r.currentOrigin[2] + (ent->enemy->r.mins[2] + ent->enemy->r.maxs[2]) * 0.5;
		BG_SnapVectorTowards(v, oldorigin);	// save net bandwidth

		G_SetOrigin(ent, v);
	}

	VectorCopy(ent->r.currentOrigin, ent->parent->client->ps.grapplePoint);
}

qboolean LogAccuracyHit(gentity_t *target, gentity_t *attacker,
	qboolean *teamHit, qboolean *enemyHit)
{
	if (!target->takedamage) {
		return qfalse;
	}

	if (target == attacker) {
		return qfalse;
	}

	if (!target->client) {
		return qfalse;
	}

	if (!attacker->client) {
		return qfalse;
	}

	if (target->client->ps.stats[STAT_HEALTH] <= 0) {
		return qfalse;
	}

	if (OnSameTeam(target, attacker)) {
		*teamHit = qtrue;
	} else {
		*enemyHit = qtrue;
	}

	return qtrue;
}

/**
Set muzzle location relative to pivoting eye
*/
void CalcMuzzlePoint (gentity_t *ent, vec3_t forward, vec3_t right, vec3_t up, vec3_t muzzlePoint)
{
	VectorCopy(ent->s.pos.trBase, muzzlePoint);
	muzzlePoint[2] += ent->client->ps.viewheight;
	VectorMA(muzzlePoint, 14, forward, muzzlePoint);
	// snap to integer coordinates for more efficient network bandwidth usage
	SnapVector(muzzlePoint);
}

/**
Set muzzle location relative to pivoting eye
*/
void CalcMuzzlePointOrigin (gentity_t *ent, vec3_t origin, vec3_t forward, vec3_t right, vec3_t up, vec3_t muzzlePoint)
{
	VectorCopy(ent->s.pos.trBase, muzzlePoint);
	muzzlePoint[2] += ent->client->ps.viewheight;
	VectorMA(muzzlePoint, 14, forward, muzzlePoint);
	// snap to integer coordinates for more efficient network bandwidth usage
	SnapVector(muzzlePoint);
}

void FireWeapon(gentity_t *ent)
{
	teamHit = qfalse;
	enemyHit = qfalse;

	if (ent->client->ps.powerups[PW_QUAD]) {
		s_quadFactor = g_quadfactor.value;
	} else {
		s_quadFactor = 1;
	}

	ent->client->pers.stats.shots[ent->s.weapon]++;

	// set aiming directions
	AngleVectors(ent->client->ps.viewangles, forward, right, up);

	CalcMuzzlePointOrigin(ent, ent->client->oldOrigin, forward, right, up, muzzle);

	// fire the specific weapon
	switch(ent->s.weapon) {
	case WP_GAUNTLET:
		break;
	case WP_LIGHTNING:
		Weapon_Lightning_Fire(ent);
		break;
	case WP_SHOTGUN:
		Weapon_Shotgun_Fire(ent);
		break;
	case WP_MACHINEGUN:
		Weapon_Machinegun_Fire(ent);
		break;
	case WP_GRENADE_LAUNCHER:
		Weapon_Grenadelauncher_Fire(ent);
		break;
	case WP_ROCKET_LAUNCHER:
		Weapon_RocketLauncher_Fire(ent);
		break;
	case WP_PLASMAGUN:
		Weapon_Plasmagun_Fire(ent);
		break;
	case WP_RAILGUN:
		Weapon_Railgun_Fire(ent);
		break;
	case WP_BFG:
		BFG_Fire(ent);
		break;
	case WP_GRAPPLING_HOOK:
		Weapon_GrapplingHook_Fire(ent);
		break;
	default:
		return;
	}

	if (teamHit) {
		ent->client->pers.stats.teamHits[ent->s.weapon]++;
	}

	if (enemyHit) {
		ent->client->pers.stats.enemyHits[ent->s.weapon]++;
		ent->client->ps.persistant[PERS_HITS]++;
	}

	if (teamHit && !enemyHit) {
		ent->client->ps.persistant[PERS_HITS]--;
	}
}


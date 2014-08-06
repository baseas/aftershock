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
// cg_weapons.c -- events and effects dealing with weapons

#include "cg_local.h"

#define MG_SPIN_SPEED	0.9
#define MG_COAST_TIME	1000

void CG_GetWeaponColor(vec4_t color, clientInfo_t *ci, weaponColor_t wpcolor)
{
	if (cg_forceWeaponColor.integer & wpcolor) {
		Vector4Copy(ci->model->weaponColor, color);
	} else {
		color[0] = 1.0f;
		color[1] = 1.0f;
		color[2] = 1.0f;
		color[3] = 1.0f;
	}
}

void CG_RailTrail(clientInfo_t *ci, vec3_t start, vec3_t end)
{
	localEntity_t	*le;
	refEntity_t		*re;

	if (!cg_railTrail.integer) {
		return;
	}

	start[2] -= 4;

	le = CG_AllocLocalEntity();
	re = &le->refEntity;

	if (cg_railTrail.integer == 1) {
		le->leType = LE_FADE_RGB;
	} else if (cg_railTrail.integer == 2) {
		le->leType = LE_EXPLOSION;
	}

	le->startTime = cg.time;
	le->endTime = cg.time + cg_railTrailTime.value;
	le->lifeRate = 1.0 / (le->endTime - le->startTime);

	re->radius = cg_railTrailRadius.value;
	re->shaderTime = cg.time / 1000.0f;
	re->reType = RT_RAIL_CORE;
	re->customShader = cgs.media.railCoreShader;

	VectorCopy(start, re->origin);
	VectorCopy(end, re->oldorigin);

	CG_GetWeaponColor(le->color, ci, WPCOLOR_RAILTRAIL);
	SCR_SetRGBA(re->shaderRGBA, le->color);

	AxisClear(re->axis);
}

static void CG_ProjectileTrail(centity_t *ent, const weaponInfo_t *wi)
{
	int		step;
	vec3_t	origin, lastPos, lastPosSaved;
	int		t;
	int		startTime, contents;
	int		trailTime, trailType;
	int		lastContents;
	entityState_t	*es;
	vec4_t			up;
	vec4_t			color;
	localEntity_t	*le;
	refEntity_t		*re;
	clientInfo_t	*ci;
	float			radius;
	
	if ((wi->item->giTag == WP_GRENADE_LAUNCHER && !cg_grenadeTrail.integer)
		|| (wi->item->giTag == WP_ROCKET_LAUNCHER && !cg_rocketTrail.integer))
	{
		return;
	}

	ci = &cgs.clientinfo[cg_entities[ent->currentState.otherEntityNum].currentState.number];

	if (wi->item->giTag == WP_GRENADE_LAUNCHER) {
		CG_GetWeaponColor(color, ci, WPCOLOR_GRENADETRAIL);
		radius = cg_grenadeTrailRadius.integer;
		trailTime = cg_grenadeTrailTime.integer;
		trailType = cg_grenadeTrail.integer;
	} else {
		CG_GetWeaponColor(color, ci, WPCOLOR_ROCKETTRAIL);
		radius = cg_rocketTrailRadius.integer;
		trailTime = cg_rocketTrailTime.integer;
		trailType = cg_rocketTrail.integer;
	}

	up[0] = 0;
	up[1] = 0;
	up[2] = 0;

	step = 20;

	es = &ent->currentState;
	startTime = ent->trailTime;
	t = step * ((startTime + step) / step);

	BG_EvaluateTrajectory(&es->pos, cg.time, origin);
	contents = CG_PointContents(origin, -1);

	// if object (e.g. grenade) is stationary, don't toss up smoke
	if (es->pos.trType == TR_STATIONARY) {
		ent->trailTime = cg.time;
		return;
	}

	BG_EvaluateTrajectory(&es->pos, ent->trailTime, lastPos);
	lastContents = CG_PointContents(lastPos, -1);

	ent->trailTime = cg.time;

	if (contents & (CONTENTS_WATER | CONTENTS_SLIME | CONTENTS_LAVA)) {
		if (contents & lastContents & CONTENTS_WATER) {
			CG_BubbleTrail(lastPos, origin, 8);
		}
		return;
	}

	for (; t <= ent->trailTime; t += step) {
		BG_EvaluateTrajectory(&es->pos, t + step, lastPosSaved);
		BG_EvaluateTrajectory(&es->pos, t, lastPos);

		if (trailType == 1) {
			CG_SmokePuff(lastPos, up, radius, color, trailTime, t, 0, 0, cgs.media.smokePuffShader);
		} else if (trailType == 2) {
			le = CG_AllocLocalEntity();
			re = &le->refEntity;

			le->radius = radius;
			re->radius = radius;

			le->leType = LE_FADE_RGB;
			le->startTime = cg.time;
			le->endTime = cg.time + 300;
			le->lifeRate = 1.0 / (le->endTime - le->startTime);

			re->shaderTime = cg.time / 1000.0f;
			re->reType = RT_RAIL_CORE;
			re->customShader = cgs.media.railCoreShader;

			Vector4Copy(color, le->color);
			SCR_SetRGBA(re->shaderRGBA, color);

			VectorCopy(lastPosSaved, re->origin);
			VectorCopy(lastPos, re->oldorigin);
		}
	}
}

void CG_GrappleTrail(centity_t *ent, const weaponInfo_t *wi)
{
	vec3_t	origin;
	entityState_t	*es;
	vec3_t			forward, up;
	refEntity_t		beam;

	es = &ent->currentState;

	BG_EvaluateTrajectory(&es->pos, cg.time, origin);
	ent->trailTime = cg.time;

	memset(&beam, 0, sizeof(beam));
	//FIXME adjust for muzzle position
	VectorCopy(cg_entities[ent->currentState.otherEntityNum].lerpOrigin, beam.origin);
	beam.origin[2] += 26;
	AngleVectors(cg_entities[ent->currentState.otherEntityNum].lerpAngles, forward, NULL, up);
	VectorMA(beam.origin, -6, up, beam.origin);
	VectorCopy(origin, beam.oldorigin);

	if (Distance(beam.origin, beam.oldorigin) < 64)
		return; // Don't draw if close

	beam.reType = RT_LIGHTNING;
	beam.customShader = cgs.media.lightningShader[0];

	AxisClear(beam.axis);
	beam.shaderRGBA[0] = 0xFF;
	beam.shaderRGBA[1] = 0xFF;
	beam.shaderRGBA[2] = 0xFF;
	beam.shaderRGBA[3] = 0xFF;
	trap_R_AddRefEntityToScene(&beam);
}

/**
The server says this item is used on this level
*/
static void CG_RegisterWeapon(int weaponNum)
{
	weaponInfo_t	*weaponInfo;
	gitem_t			*item, *ammo;
	char			path[MAX_QPATH];
	vec3_t			mins, maxs;
	int				i;

	weaponInfo = &cg_weapons[weaponNum];

	if (weaponNum == 0) {
		return;
	}

	if (weaponInfo->registered) {
		return;
	}

	memset(weaponInfo, 0, sizeof(*weaponInfo));
	weaponInfo->registered = qtrue;

	for (item = bg_itemlist + 1; item->classname; item++) {
		if (item->giType == IT_WEAPON && item->giTag == weaponNum) {
			weaponInfo->item = item;
			break;
		}
	}
	if (!item->classname) {
		CG_Error("Couldn't find weapon %i", weaponNum);
	}
	CG_RegisterItem(item - bg_itemlist);

	// load cmodel before model so filecache works
	weaponInfo->weaponModel = trap_R_RegisterModel(item->world_model[0]);

	// calc midpoint for rotation
	trap_R_ModelBounds(weaponInfo->weaponModel, mins, maxs);
	for (i = 0; i < 3; i++) {
		weaponInfo->weaponMidpoint[i] = mins[i] + 0.5 * (maxs[i] - mins[i]);
	}

	weaponInfo->weaponIcon = trap_R_RegisterShader(item->icon);
	weaponInfo->ammoIcon = trap_R_RegisterShader(item->icon);

	for (ammo = bg_itemlist + 1; ammo->classname; ammo++) {
		if (ammo->giType == IT_AMMO && ammo->giTag == weaponNum) {
			break;
		}
	}
	if (ammo->classname && ammo->world_model[0]) {
		weaponInfo->ammoModel = trap_R_RegisterModel(ammo->world_model[0]);
	}

	strcpy(path, item->world_model[0]);
	COM_StripExtension(path, path, sizeof(path));
	strcat(path, "_flash.md3");
	weaponInfo->flashModel = trap_R_RegisterModel(path);

	strcpy(path, item->world_model[0]);
	COM_StripExtension(path, path, sizeof(path));
	strcat(path, "_barrel.md3");
	weaponInfo->barrelModel = trap_R_RegisterModel(path);

	strcpy(path, item->world_model[0]);
	COM_StripExtension(path, path, sizeof(path));
	strcat(path, "_hand.md3");
	weaponInfo->handsModel = trap_R_RegisterModel(path);

	if (!weaponInfo->handsModel) {
		weaponInfo->handsModel = trap_R_RegisterModel("models/weapons2/shotgun/shotgun_hand.md3");
	}

	switch (weaponNum) {
	case WP_GAUNTLET:
		MAKERGB(weaponInfo->flashDlightColor, 0.6f, 0.6f, 1.0f);
		weaponInfo->firingSound = trap_S_RegisterSound("sound/weapons/melee/fstrun.wav", qfalse);
		weaponInfo->flashSound[0] = trap_S_RegisterSound("sound/weapons/melee/fstatck.wav", qfalse);
		break;

	case WP_LIGHTNING:
		MAKERGB(weaponInfo->flashDlightColor, 0.6f, 0.6f, 1.0f);
		weaponInfo->readySound = trap_S_RegisterSound("sound/weapons/melee/fsthum.wav", qfalse);
		weaponInfo->firingSound = trap_S_RegisterSound("sound/weapons/lightning/lg_hum.wav", qfalse);

		weaponInfo->flashSound[0] = trap_S_RegisterSound("sound/weapons/lightning/lg_fire.wav", qfalse);
		cgs.media.lightningExplosionModel = trap_R_RegisterModel("models/weaphits/crackle.md3");
		cgs.media.sfx_lghit1 = trap_S_RegisterSound("sound/weapons/lightning/lg_hit.wav", qfalse);
		cgs.media.sfx_lghit2 = trap_S_RegisterSound("sound/weapons/lightning/lg_hit2.wav", qfalse);
		cgs.media.sfx_lghit3 = trap_S_RegisterSound("sound/weapons/lightning/lg_hit3.wav", qfalse);

		cgs.media.lightningShader[0] = trap_R_RegisterShaderNoMip("lightningBoltScrollColor");
		cgs.media.lightningShader[1] = trap_R_RegisterShaderNoMip("lightningBoltScrollThinColor");
		cgs.media.lightningShader[2] = trap_R_RegisterShaderNoMip("lightningBoltnewColor");
		cgs.media.lightningShader[3] = trap_R_RegisterShaderNoMip("lightningBoltSimpleColor");

		break;

	case WP_GRAPPLING_HOOK:
		MAKERGB(weaponInfo->flashDlightColor, 0.6f, 0.6f, 1.0f);
		weaponInfo->missileModel = trap_R_RegisterModel("models/ammo/rocket/rocket.md3");
		weaponInfo->missileTrailFunc = CG_GrappleTrail;
		weaponInfo->missileDlight = 200;
		MAKERGB(weaponInfo->missileDlightColor, 1, 0.75f, 0);
		weaponInfo->readySound = trap_S_RegisterSound("sound/weapons/melee/fsthum.wav", qfalse);
		weaponInfo->firingSound = trap_S_RegisterSound("sound/weapons/melee/fstrun.wav", qfalse);

		cgs.media.lightningShader[0] = trap_R_RegisterShaderNoMip("lightningBoltScrollColor");
		cgs.media.lightningShader[1] = trap_R_RegisterShaderNoMip("lightningBoltScrollThinColor");
		cgs.media.lightningShader[2] = trap_R_RegisterShaderNoMip("lightningBoltnewColor");
		cgs.media.lightningShader[3] = trap_R_RegisterShaderNoMip("lightningBoltSimpleColor");
		break;

	case WP_MACHINEGUN:
		MAKERGB(weaponInfo->flashDlightColor, 0.6f, 0.6f, 1.0f);
		weaponInfo->flashSound[0] = trap_S_RegisterSound("sound/weapons/machinegun/machgf1b.wav", qfalse);
		weaponInfo->flashSound[1] = trap_S_RegisterSound("sound/weapons/machinegun/machgf2b.wav", qfalse);
		weaponInfo->flashSound[2] = trap_S_RegisterSound("sound/weapons/machinegun/machgf3b.wav", qfalse);
		weaponInfo->flashSound[3] = trap_S_RegisterSound("sound/weapons/machinegun/machgf4b.wav", qfalse);
		cgs.media.bulletExplosionShader = trap_R_RegisterShaderNoMip("bulletExplosion");
		break;

	case WP_SHOTGUN:
		MAKERGB(weaponInfo->flashDlightColor, 1, 1, 0);
		weaponInfo->flashSound[0] = trap_S_RegisterSound("sound/weapons/machinegun/machgf1b.wav", qfalse);
		weaponInfo->flashSound[0] = trap_S_RegisterSound("sound/weapons/shotgun/sshotf1b.wav", qfalse);
		break;

	case WP_ROCKET_LAUNCHER:
		weaponInfo->missileModel = trap_R_RegisterModel("models/ammo/rocket/rocket.md3");
		weaponInfo->missileSound = trap_S_RegisterSound("sound/weapons/rocket/rockfly.wav", qfalse);
		weaponInfo->missileTrailFunc = CG_ProjectileTrail;
		weaponInfo->missileDlight = 200;
		
		MAKERGB(weaponInfo->missileDlightColor, 1, 0.75f, 0);
		MAKERGB(weaponInfo->flashDlightColor, 1, 0.75f, 0);

		weaponInfo->flashSound[0] = trap_S_RegisterSound("sound/weapons/rocket/rocklf1a.wav", qfalse);
		cgs.media.rocketExplosionShader = trap_R_RegisterShaderNoMip("rocketExplosion");
		break;

	case WP_GRENADE_LAUNCHER:
		weaponInfo->missileModel = trap_R_RegisterModel("models/ammo/grenade.md3");
		weaponInfo->missileTrailFunc = CG_ProjectileTrail;
		MAKERGB(weaponInfo->flashDlightColor, 1, 0.70f, 0);
		weaponInfo->flashSound[0] = trap_S_RegisterSound("sound/weapons/grenade/grenlf1a.wav", qfalse);
		cgs.media.grenadeExplosionShader = trap_R_RegisterShaderNoMip("grenadeExplosion2");
		break;

	case WP_PLASMAGUN:
		weaponInfo->missileSound = trap_S_RegisterSound("sound/weapons/plasma/lasfly.wav", qfalse);
		MAKERGB(weaponInfo->flashDlightColor, 0.6f, 0.6f, 1.0f);
		weaponInfo->flashSound[0] = trap_S_RegisterSound("sound/weapons/plasma/hyprbf1a.wav", qfalse);
		cgs.media.plasmaExplosionShader = trap_R_RegisterShaderNoMip("plasmaExplosion");
		break;

	case WP_RAILGUN:
		weaponInfo->readySound = trap_S_RegisterSound("sound/weapons/railgun/rg_hum.wav", qfalse);
		MAKERGB(weaponInfo->flashDlightColor, 1, 0.5f, 0);
		weaponInfo->flashSound[0] = trap_S_RegisterSound("sound/weapons/railgun/railgf1a.wav", qfalse);
		cgs.media.railExplosionShader = trap_R_RegisterShaderNoMip("railExplosion");
		cgs.media.railCoreShader = trap_R_RegisterShaderNoMip("railCore");
		break;

	case WP_BFG:
		weaponInfo->readySound = trap_S_RegisterSound("sound/weapons/bfg/bfg_hum.wav", qfalse);
		MAKERGB(weaponInfo->flashDlightColor, 1, 0.5f, 0);
		weaponInfo->flashSound[0] = trap_S_RegisterSound("sound/weapons/bfg/bfg_fire.wav", qfalse);
		cgs.media.bfgExplosionShader = trap_R_RegisterShaderNoMip("bfgExplosion");
		weaponInfo->missileModel = trap_R_RegisterModel("models/weaphits/bfg.md3");
		weaponInfo->missileSound = trap_S_RegisterSound("sound/weapons/rocket/rockfly.wav", qfalse);
		break;

	 default:
		MAKERGB(weaponInfo->flashDlightColor, 1, 1, 1);
		weaponInfo->flashSound[0] = trap_S_RegisterSound("sound/weapons/rocket/rocklf1a.wav", qfalse);
		break;
	}
}

/**
The server says this item is used on this level
*/
void CG_RegisterItem(int itemNum)
{
	itemInfo_t		*itemInfo;
	gitem_t			*item;

	if (itemNum < 0 || itemNum >= bg_numItems) {
		CG_Error("CG_RegisterItemVisuals: itemNum %d out of range [0-%d]", itemNum, bg_numItems-1);
	}

	itemInfo = &cg_items[itemNum];
	if (itemInfo->registered) {
		return;
	}

	item = &bg_itemlist[itemNum];

	memset(itemInfo, 0, sizeof(*itemInfo));
	itemInfo->registered = qtrue;

	itemInfo->models[0] = trap_R_RegisterModel(item->world_model[0]);
	itemInfo->icon = trap_R_RegisterShader(item->icon);
	if (item->pickup_sound) {
		itemInfo->pickupSound = trap_S_RegisterSound(item->pickup_sound, qfalse);
	}

	if (item->giType == IT_WEAPON) {
		CG_RegisterWeapon(item->giTag);
	}

	//
	// powerups have an accompanying ring or sphere
	//
	if (item->giType == IT_POWERUP || item->giType == IT_HEALTH || 
		item->giType == IT_ARMOR || item->giType == IT_HOLDABLE) {
		if (item->world_model[1]) {
			itemInfo->models[1] = trap_R_RegisterModel(item->world_model[1]);
		}
	}
}

// VIEW WEAPON

static int CG_MapTorsoToWeaponFrame(clientInfo_t *ci, int frame)
{

	// change weapon
	if (frame >= ci->model->animations[TORSO_DROP].firstFrame
		&& frame < ci->model->animations[TORSO_DROP].firstFrame + 9) {
		return frame - ci->model->animations[TORSO_DROP].firstFrame + 6;
	}

	// stand attack
	if (frame >= ci->model->animations[TORSO_ATTACK].firstFrame
		&& frame < ci->model->animations[TORSO_ATTACK].firstFrame + 6) {
		return 1 + frame - ci->model->animations[TORSO_ATTACK].firstFrame;
	}

	// stand attack 2
	if (frame >= ci->model->animations[TORSO_ATTACK2].firstFrame
		&& frame < ci->model->animations[TORSO_ATTACK2].firstFrame + 6) {
		return 1 + frame - ci->model->animations[TORSO_ATTACK2].firstFrame;
	}
	
	return 0;
}

static void CG_CalculateWeaponPosition(vec3_t origin, vec3_t angles)
{
	float	scale;
	int		delta;
	float	fracsin;

	VectorCopy(cg.refdef.vieworg, origin);
	VectorCopy(cg.refdefViewAngles, angles);

	if (cg_weaponBobbing.integer != 2) {
		return;
	}

	// on odd legs, invert some angles
	if (cg.bobcycle & 1) {
		scale = -cg.xyspeed;
	} else {
		scale = cg.xyspeed;
	}

	// gun angles from bobbing
	angles[ROLL] += scale * cg.bobfracsin * 0.005;
	angles[YAW] += scale * cg.bobfracsin * 0.01;
	angles[PITCH] += cg.xyspeed * cg.bobfracsin * 0.005;

	// drop the weapon when landing
	delta = cg.time - cg.landTime;
	if (delta < LAND_DEFLECT_TIME) {
		origin[2] += cg.landChange * 0.25 * delta / LAND_DEFLECT_TIME;
	} else if (delta < LAND_DEFLECT_TIME + LAND_RETURN_TIME) {
		origin[2] += cg.landChange * 0.25 *
			(LAND_DEFLECT_TIME + LAND_RETURN_TIME - delta) / LAND_RETURN_TIME;
	}

	// idle drift
	scale = cg.xyspeed + 40;
	fracsin = sin(cg.time * 0.001);
	angles[ROLL] += scale * fracsin * 0.01;
	angles[YAW] += scale * fracsin * 0.01;
	angles[PITCH] += scale * fracsin * 0.01;
}

/**
Origin will be the exact tag point, which is slightly
different than the muzzle point used for determining hits.
The cent should be the non-predicted cent if it is from the player,
so the endpoint will reflect the simulated strike (lagging the predicted
angle)
*/
static void CG_LightningBolt(centity_t *cent, vec3_t origin)
{
	trace_t			trace;
	refEntity_t		beam;
	vec3_t			forward;
	vec3_t			muzzlePoint, endPoint;
	int				anim;
	int				style;
	clientInfo_t	*ci;
	vec4_t			color;
	int				mask;

	if (cent->currentState.weapon != WP_LIGHTNING) {
		return;
	}

	ci = &cgs.clientinfo[cg_entities[cent->currentState.otherEntityNum].currentState.number];

	memset(&beam, 0, sizeof(beam));

	// unlagged - attack prediction #1
	// if the entity is us, unlagged is on server-side, and we've got it on for the lightning gun
	if (cent->currentState.number == cg.predictedPlayerState.clientNum) {
		// always shoot straight forward from our current position
		AngleVectors(cg.predictedPlayerState.viewangles, forward, NULL, NULL);
		VectorCopy(cg.predictedPlayerState.origin, muzzlePoint);
	} else {
		AngleVectors(cent->lerpAngles, forward, NULL, NULL);
		VectorCopy(cent->lerpOrigin, muzzlePoint);
	}

	anim = cent->currentState.legsAnim & ~ANIM_TOGGLEBIT;
	if (anim == LEGS_WALKCR || anim == LEGS_IDLECR) {
		muzzlePoint[2] += CROUCH_VIEWHEIGHT;
	} else {
		muzzlePoint[2] += DEFAULT_VIEWHEIGHT;
	}

	VectorMA(muzzlePoint, 14, forward, muzzlePoint);

	// project forward by the lightning range
	VectorMA(muzzlePoint, LIGHTNING_RANGE, forward, endPoint);

	if (cgs.gametype == GT_DEFRAG) {
		mask = MASK_SOLID;
	} else {
		mask = MASK_SHOT;
	}

	// see if it hit a wall
	CG_Trace(&trace, muzzlePoint, vec3_origin, vec3_origin, endPoint,
		cent->currentState.number, mask);

	// this is the endpoint
	VectorCopy(trace.endpos, beam.oldorigin);

	// use the provided origin, even though it may be slightly
	// different than the muzzle origin
	VectorCopy(origin, beam.origin);

	beam.reType = RT_LIGHTNING;

	CG_GetWeaponColor(color, ci, WPCOLOR_LIGHTNING);
	SCR_SetRGBA(beam.shaderRGBA, color);

	style = cg_lightningStyle.integer;
	if (style < 0) {
		style = 0;
	} else if (style >= MAX_LGSTYLES) {
		style = MAX_LGSTYLES - 1;
	}

	beam.customShader = cgs.media.lightningShader[style];
	trap_R_AddRefEntityToScene(&beam);

	// add the impact flare if it hit something
	if (cg_lightningExplosion.integer && trace.fraction < 1.0) {
		vec3_t	dir, angles;

		VectorSubtract(beam.oldorigin, beam.origin, dir);
		VectorNormalize(dir);

		memset(&beam, 0, sizeof(beam));
		beam.hModel = cgs.media.lightningExplosionModel;
		VectorMA(trace.endpos, -16, dir, beam.origin);

		// make a random orientation
		angles[0] = rand() % 360;
		angles[1] = rand() % 360;
		angles[2] = rand() % 360;

		AnglesToAxis(angles, beam.axis);
		trap_R_AddRefEntityToScene(&beam);
	}
}

static float CG_MachinegunSpinAngle(centity_t *cent)
{
	int		delta;
	float	angle;
	float	speed;

	delta = cg.time - cent->pe.barrelTime;
	if (cent->pe.barrelSpinning) {
		angle = cent->pe.barrelAngle + delta * MG_SPIN_SPEED;
	} else {
		if (delta > MG_COAST_TIME) {
			delta = MG_COAST_TIME;
		}

		speed = 0.5 * (MG_SPIN_SPEED + (float)(MG_COAST_TIME - delta) / MG_COAST_TIME);
		angle = cent->pe.barrelAngle + delta * speed;
	}

	if (cent->pe.barrelSpinning == !(cent->currentState.eFlags & EF_FIRING)) {
		cent->pe.barrelTime = cg.time;
		cent->pe.barrelAngle = AngleMod(angle);
		cent->pe.barrelSpinning = !!(cent->currentState.eFlags & EF_FIRING);
	}

	return angle;
}

static void CG_AddWeaponWithPowerups(refEntity_t *gun, int powerups)
{
	if (powerups & (1 << PW_INVIS)) {
		return;
	}

	trap_R_AddRefEntityToScene(gun);

	if (powerups & (1 << PW_BATTLESUIT)) {
		gun->customShader = cgs.media.battleWeaponShader;
		trap_R_AddRefEntityToScene(gun);
	}

	if (powerups & (1 << PW_QUAD)) {
		gun->customShader = cgs.media.quadWeaponShader;
		trap_R_AddRefEntityToScene(gun);
	}
}

/**
Used for both the view weapon (ps is valid) and the world modelother character models (ps is NULL)
The main player will have this called for BOTH cases, so effects like light and
sound should only be done on the world model case.
*/
void CG_AddPlayerWeapon(refEntity_t *parent, playerState_t *ps, centity_t *cent, int team)
{
	refEntity_t	gun;
	refEntity_t	barrel;
	refEntity_t	flash;
	vec3_t		angles;
	weapon_t	weaponNum;
	weaponInfo_t	*weapon;
	orientation_t	lerped;
	clientInfo_t	*ci;
	vec4_t			color;


	ci = &cgs.clientinfo[cent->currentState.clientNum];
	weaponNum = cent->currentState.weapon;

	CG_RegisterWeapon(weaponNum);
	weapon = &cg_weapons[weaponNum];

	// add the weapon
	memset(&gun, 0, sizeof(gun));
	VectorCopy(parent->lightingOrigin, gun.lightingOrigin);
	gun.shadowPlane = parent->shadowPlane;
	gun.renderfx = parent->renderfx;

	// set custom shading for railgun refire rate
	if (weaponNum == WP_RAILGUN) {
		vec4_t			color;

		CG_GetWeaponColor(color, ci, WPCOLOR_RAILTRAIL);

		if (cent->pe.railFireTime + 1500 > cg.time) {
			Vector4Scale(color, (cg.time - cent->pe.railFireTime) / 1500.0f, color);
		}

		SCR_SetRGBA(gun.shaderRGBA, color);
	}

	gun.hModel = weapon->weaponModel;
	if (!gun.hModel) {
		return;
	}

	if (!ps) {
		// add weapon ready sound
		cent->pe.lightningFiring = qfalse;
		if ((cent->currentState.eFlags & EF_FIRING) && weapon->firingSound) {
			// lightning gun and guantlet make a different sound when fire is held down
			trap_S_AddLoopingSound(cent->currentState.number, cent->lerpOrigin, vec3_origin, weapon->firingSound);
			cent->pe.lightningFiring = qtrue;
		} else if (weapon->readySound) {
			trap_S_AddLoopingSound(cent->currentState.number, cent->lerpOrigin, vec3_origin, weapon->readySound);
		}
	}

	trap_R_LerpTag(&lerped, parent->hModel, parent->oldframe, parent->frame,
		1.0 - parent->backlerp, "tag_weapon");
	VectorCopy(parent->origin, gun.origin);

	VectorMA(gun.origin, lerped.origin[0], parent->axis[0], gun.origin);

	// Make weapon appear left-handed for 2 and centered for 3
	if (ps && cg_drawGun.integer == 2) {
		VectorMA(gun.origin, -lerped.origin[1], parent->axis[1], gun.origin);
	} else if (!ps || cg_drawGun.integer != 3) {
		VectorMA(gun.origin, lerped.origin[1], parent->axis[1], gun.origin);
	}

	VectorMA(gun.origin, lerped.origin[2], parent->axis[2], gun.origin);

	MatrixMultiply(lerped.axis, ((refEntity_t *)parent)->axis, gun.axis);
	gun.backlerp = parent->backlerp;

	CG_AddWeaponWithPowerups(&gun, cent->currentState.powerups);

	// add the spinning barrel
	if (weapon->barrelModel) {
		memset(&barrel, 0, sizeof(barrel));
		VectorCopy(parent->lightingOrigin, barrel.lightingOrigin);
		barrel.shadowPlane = parent->shadowPlane;
		barrel.renderfx = parent->renderfx;

		barrel.hModel = weapon->barrelModel;
		angles[YAW] = 0;
		angles[PITCH] = 0;
		angles[ROLL] = CG_MachinegunSpinAngle(cent);
		AnglesToAxis(angles, barrel.axis);

		CG_PositionRotatedEntityOnTag(&barrel, &gun, weapon->weaponModel, "tag_barrel");
		CG_AddWeaponWithPowerups(&barrel, cent->currentState.powerups);
	}

	// add the flash
	if ((weaponNum == WP_LIGHTNING || weaponNum == WP_GAUNTLET || weaponNum == WP_GRAPPLING_HOOK)
		&& (cent->currentState.eFlags & EF_FIRING)) 
	{
		// continuous flash
	} else {
		// impulse flash
		if (cg.time - cent->muzzleFlashTime > MUZZLE_FLASH_TIME) {
			return;
		}
	}

	if (cg_muzzleFlash.integer) {
		memset(&flash, 0, sizeof(flash));
		VectorCopy(parent->lightingOrigin, flash.lightingOrigin);
		flash.shadowPlane = parent->shadowPlane;
		flash.renderfx = parent->renderfx;

		flash.hModel = weapon->flashModel;
		if (!flash.hModel) {
			return;
		}
		angles[YAW] = 0;
		angles[PITCH] = 0;
		angles[ROLL] = crandom() * 10;
		AnglesToAxis(angles, flash.axis);

		if (weaponNum == WP_RAILGUN) {
			CG_GetWeaponColor(color, ci, WPCOLOR_RAILTRAIL);
		} else {
			color[0] = 1.0f;
			color[1] = 1.0f;
			color[2] = 1.0f;
			color[3] = 1.0f;
		}

		SCR_SetRGBA(flash.shaderRGBA, color);

		CG_PositionRotatedEntityOnTag(&flash, &gun, weapon->weaponModel, "tag_flash");
		trap_R_AddRefEntityToScene(&flash);
	} else {
		CG_PositionRotatedEntityOnTag(&flash, &gun, weapon->weaponModel, "tag_flash");
	}

	if (ps || cg.renderingThirdPerson
		|| cent->currentState.number != cg.predictedPlayerState.clientNum)
	{
		CG_LightningBolt(cent, flash.origin);

		if (weapon->flashDlightColor[0] || weapon->flashDlightColor[1] || weapon->flashDlightColor[2]) {
			trap_R_AddLightToScene(flash.origin, 300 + (rand()&31), color[0], color[1], color[2]);
		}
	}
}

/**
Add the weapon to  the player's view
*/
void CG_AddViewWeapon(playerState_t *ps)
{
	refEntity_t	hand;
	centity_t	*cent;
	clientInfo_t	*ci;
	float		fovOffset;
	vec3_t		angles;
	weaponInfo_t	*weapon;

	if (ps->persistant[PERS_TEAM] == TEAM_SPECTATOR) {
		return;
	}

	if (ps->pm_type == PM_INTERMISSION) {
		return;
	}

	// no gun if in third person view or a camera is active
	//if (cg.renderingThirdPerson || cg.cameraMode) {
	if (cg.renderingThirdPerson) {
		return;
	}


	// allow the gun to be completely removed
	if (!cg_drawGun.integer) {
		vec3_t		origin;

		if (cg.predictedPlayerState.eFlags & EF_FIRING) {
			// special hack for lightning gun...
			VectorCopy(cg.refdef.vieworg, origin);
			VectorMA(origin, -8, cg.refdef.viewaxis[2], origin);
			CG_LightningBolt(&cg_entities[ps->clientNum], origin);
		}
		return;
	}

	// don't draw if testing a gun model
	if (cg.testGun) {
		return;
	}

	// drop gun lower at higher fov
	if (cg_fov.integer > 90) {
		fovOffset = -0.2 * (cg_fov.integer - 90);
	} else {
		fovOffset = 0;
	}

	cent = &cg.predictedPlayerEntity;	// &cg_entities[cg.snap->ps.clientNum];
	CG_RegisterWeapon(ps->weapon);
	weapon = &cg_weapons[ps->weapon];

	memset(&hand, 0, sizeof(hand));

	// set up gun position
	CG_CalculateWeaponPosition(hand.origin, angles);

	VectorMA(hand.origin, cg_gun_x.value, cg.refdef.viewaxis[0], hand.origin);
	VectorMA(hand.origin, cg_gun_y.value, cg.refdef.viewaxis[1], hand.origin);
	VectorMA(hand.origin, (cg_gun_z.value+fovOffset), cg.refdef.viewaxis[2], hand.origin);

	AnglesToAxis(angles, hand.axis);

	// map torso animations to weapon animations
	if (cg_weaponBobbing.integer) {
		// get clientinfo for animation map
		ci = &cgs.clientinfo[cent->currentState.clientNum];
		hand.frame = CG_MapTorsoToWeaponFrame(ci, cent->pe.torso.frame);
		hand.oldframe = CG_MapTorsoToWeaponFrame(ci, cent->pe.torso.oldFrame);
		hand.backlerp = cent->pe.torso.backlerp;
	} else {
		hand.frame = 0;
		hand.oldframe = 0;
		hand.backlerp = 0;
	}

	hand.hModel = weapon->handsModel;
	hand.renderfx = RF_DEPTHHACK | RF_FIRST_PERSON | RF_MINLIGHT;

	// add everything onto the hand
	CG_AddPlayerWeapon(&hand, ps, &cg.predictedPlayerEntity, ps->persistant[PERS_TEAM]);
}

// WEAPON SELECTION

static qboolean CG_WeaponSelectable(int i)
{
	if (!cg.snap->ps.ammo[i]) {
		return qfalse;
	}
	if (!(cg.snap->ps.stats[STAT_WEAPONS] & (1 << i))) {
		return qfalse;
	}

	return qtrue;
}

void CG_RunWeaponScript(void)
{
	vmCvar_t	*configCvar;

	switch (cg.weaponSelect) {
	case WP_GAUNTLET:
		configCvar = &cg_weaponConfig_g;
		break;
	case WP_MACHINEGUN:
		configCvar = &cg_weaponConfig_mg;
		break;
	case WP_SHOTGUN:
		configCvar = &cg_weaponConfig_sg;
		break;
	case WP_GRENADE_LAUNCHER:
		configCvar = &cg_weaponConfig_gl;
		break;
	case WP_ROCKET_LAUNCHER:
		configCvar = &cg_weaponConfig_rl;
		break;
	case WP_LIGHTNING:
		configCvar = &cg_weaponConfig_lg;
		break;
	case WP_RAILGUN:
		configCvar = &cg_weaponConfig_rg;
		break;
	case WP_PLASMAGUN:
		configCvar = &cg_weaponConfig_pg;
		break;
	case WP_BFG:
		configCvar = &cg_weaponConfig_bfg;
		break;
	case WP_GRAPPLING_HOOK:
		configCvar = &cg_weaponConfig_gh;
		break;
	default:
		configCvar = &cg_weaponConfig;
	}

	if (configCvar->string[0] == '\0') {
		configCvar = &cg_weaponConfig;
	}

	trap_SendConsoleCommand(configCvar->string);
	trap_SendConsoleCommand("\n");
}

void CG_NextWeapon_f(void)
{
	int		i;
	int		original;

	if (!cg.snap) {
		return;
	}
	if (cg.snap->ps.pm_flags & PMF_FOLLOW) {
		return;
	}

	cg.weaponSelectTime = cg.time;
	original = cg.weaponSelect;

	for (i = 0; i < MAX_WEAPONS; i++) {
		cg.weaponSelect++;
		if (cg.weaponSelect == MAX_WEAPONS) {
			cg.weaponSelect = 0;
		}
		if (cg.weaponSelect == WP_GAUNTLET) {
			continue;		// never cycle to gauntlet
		}
		if (CG_WeaponSelectable(cg.weaponSelect)) {
			break;
		}
	}

	if (i == MAX_WEAPONS) {
		cg.weaponSelect = original;
	} else if (original != cg.weaponSelect) {
		CG_RunWeaponScript();
	}
}

void CG_PrevWeapon_f(void)
{
	int		i;
	int		original;

	if (!cg.snap) {
		return;
	}
	if (cg.snap->ps.pm_flags & PMF_FOLLOW) {
		return;
	}

	cg.weaponSelectTime = cg.time;
	original = cg.weaponSelect;

	for (i = 0; i < MAX_WEAPONS; i++) {
		cg.weaponSelect--;
		if (cg.weaponSelect == -1) {
			cg.weaponSelect = MAX_WEAPONS - 1;
		}
		if (cg.weaponSelect == WP_GAUNTLET) {
			continue;		// never cycle to gauntlet
		}
		if (CG_WeaponSelectable(cg.weaponSelect)) {
			break;
		}
	}

	if (i == MAX_WEAPONS) {
		cg.weaponSelect = original;
	} else if (original != cg.weaponSelect) {
		CG_RunWeaponScript();
	}
}

void CG_Weapon_f(void)
{
	int		num;

	if (!cg.snap) {
		return;
	}
	if (cg.snap->ps.pm_flags & PMF_FOLLOW) {
		return;
	}

	num = atoi(BG_Argv(1));

	if (num < 1 || num > MAX_WEAPONS-1) {
		return;
	}

	cg.weaponSelectTime = cg.time;

	if (!(cg.snap->ps.stats[STAT_WEAPONS] & (1 << num))) {
		return;		// don't have the weapon
	}

	if (cg.weaponSelect != num && (cg_switchToEmpty.integer || cg.snap->ps.ammo[num])) {
		cg.weaponSelect = num;
		CG_RunWeaponScript();
	}
}

void CG_OutOfAmmoChange(void)
{
	int		i;

	cg.weaponSelectTime = cg.time;

	for (i = MAX_WEAPONS - 1; i > 0; i--) {
		if (!CG_WeaponSelectable(i)) {
			continue;
		}

		cg.weaponSelect = i;
		CG_RunWeaponScript();
		break;
	}
}

// WEAPON EVENTS

/**
Caused by an EV_FIRE_WEAPON event
*/
void CG_FireWeapon(centity_t *cent)
{
	entityState_t	*ent;
	int				c;
	weaponInfo_t	*weap;

	ent = &cent->currentState;
	if (ent->weapon == WP_NONE) {
		return;
	}
	if (ent->weapon >= WP_NUM_WEAPONS) {
		CG_Error("CG_FireWeapon: ent->weapon >= WP_NUM_WEAPONS");
		return;
	}
	weap = &cg_weapons[ent->weapon];

	// mark the entity as muzzle flashing, so when it is added it will
	// append the flash to the weapon model
	cent->muzzleFlashTime = cg.time;

	// lightning gun only does this this on initial press
	if (ent->weapon == WP_LIGHTNING) {
		if (cent->pe.lightningFiring) {
			return;
		}
	}

	if (ent->weapon == WP_RAILGUN) {
		cent->pe.railFireTime = cg.time;
	}

	// play quad sound if needed
	if (cent->currentState.powerups & (1 << PW_QUAD)) {
		trap_S_StartSound(NULL, cent->currentState.number, CHAN_ITEM, cgs.media.quadSound);
	}

	// play a sound
	for (c = 0; c < 4; c++) {
		if (!weap->flashSound[c]) {
			break;
		}
	}
	if (c > 0) {
		c = rand() % c;
		if (weap->flashSound[c])
		{
			trap_S_StartSound(NULL, ent->number, CHAN_WEAPON, weap->flashSound[c]);
		}
	}

	// unlagged - attack prediction #1
	CG_PredictWeaponEffects(cent);
}

/**
Caused by an EV_MISSILE_MISS event, or directly by local bullet tracing
*/
void CG_MissileHitWall(int weapon, int clientNum, vec3_t origin, vec3_t dir, impactSound_t soundType)
{
	qhandle_t		mod;
	qhandle_t		mark;
	qhandle_t		shader;
	sfxHandle_t		sfx;
	float			radius;
	float			light;
	vec3_t			lightColor;
	localEntity_t	*le;
	int				r;
	qboolean		alphaFade;
	qboolean		isSprite;
	int				duration;
	vec4_t			color;


	mark = 0;
	radius = 32;
	sfx = 0;
	mod = 0;
	shader = 0;
	light = 0;
	lightColor[0] = 1;
	lightColor[1] = 1;
	lightColor[2] = 0;

	// set defaults
	isSprite = qfalse;
	duration = 600;

	switch (weapon) {
	default:
	case WP_LIGHTNING:
		// no explosion at LG impact, it is added with the beam
		r = rand() & 3;
		if (r < 2) {
			sfx = cgs.media.sfx_lghit2;
		} else if (r == 2) {
			sfx = cgs.media.sfx_lghit1;
		} else {
			sfx = cgs.media.sfx_lghit3;
		}
		mark = cgs.media.holeMarkShader;
		radius = 12;
		break;
	case WP_GRENADE_LAUNCHER:
		mod = cgs.media.dishFlashModel;
		shader = cgs.media.grenadeExplosionShader;
		sfx = cgs.media.sfx_rockexp;
		mark = cgs.media.burnMarkShader;
		radius = 64;
		light = 300;
		isSprite = qtrue;
		break;
	case WP_ROCKET_LAUNCHER:
		mod = cgs.media.dishFlashModel;
		shader = cgs.media.rocketExplosionShader;
		sfx = cgs.media.sfx_rockexp;
		mark = cgs.media.burnMarkShader;
		radius = 64;
		light = 300;
		isSprite = qtrue;
		duration = 1000;
		lightColor[0] = 1;
		lightColor[1] = 0.75;
		lightColor[2] = 0.0;
		break;
	case WP_RAILGUN:
		mod = cgs.media.ringFlashModel;
		shader = cgs.media.railExplosionShader;
		//sfx = cgs.media.sfx_railg;
		sfx = cgs.media.sfx_plasmaexp;
		mark = cgs.media.energyMarkShader;
		radius = 24;
		break;
	case WP_PLASMAGUN:
		mod = cgs.media.ringFlashModel;
		shader = cgs.media.plasmaExplosionShader;
		sfx = cgs.media.sfx_plasmaexp;
		mark = cgs.media.energyMarkShader;
		radius = 16;
		break;
	case WP_BFG:
		mod = cgs.media.dishFlashModel;
		shader = cgs.media.bfgExplosionShader;
		sfx = cgs.media.sfx_rockexp;
		mark = cgs.media.burnMarkShader;
		radius = 32;
		isSprite = qtrue;
		break;
	case WP_SHOTGUN:
		mod = cgs.media.bulletFlashModel;
		shader = cgs.media.bulletExplosionShader;
		mark = cgs.media.bulletMarkShader;
		sfx = 0;
		radius = 4;
		break;
	case WP_MACHINEGUN:
		mod = cgs.media.bulletFlashModel;
		shader = cgs.media.bulletExplosionShader;
		mark = cgs.media.bulletMarkShader;

		r = rand() & 3;
		if (r == 0) {
			sfx = cgs.media.sfx_ric1;
		} else if (r == 1) {
			sfx = cgs.media.sfx_ric2;
		} else {
			sfx = cgs.media.sfx_ric3;
		}

		radius = 8;
		break;
	}

	if (sfx) {
		trap_S_StartSound(origin, ENTITYNUM_WORLD, CHAN_AUTO, sfx);
	}

	if (weapon == WP_RAILGUN) {
		CG_GetWeaponColor(color, &cgs.clientinfo[clientNum], WPCOLOR_RAILTRAIL);
	} else if (weapon == WP_PLASMAGUN) {
		CG_GetWeaponColor(color, &cgs.clientinfo[clientNum], WPCOLOR_RAILTRAIL);
	} else {
		color[0] = 1.0f;
		color[1] = 1.0f;
		color[2] = 1.0f;
		color[3] = 1.0f;
	}

	// create the explosion
	if (mod) {
		le = CG_MakeExplosion(origin, dir, mod, shader, duration, isSprite);
		le->light = light;
		VectorCopy(lightColor, le->lightColor);

		SCR_SetRGBA(le->refEntity.shaderRGBA, color);
	}

	// impact mark
	alphaFade = (mark == cgs.media.energyMarkShader);	// plasma fades alpha, all others fade color
	CG_ImpactMark(mark, origin, dir, random()*360, color, alphaFade, radius, qfalse);
}

void CG_MissileHitPlayer(int weapon, vec3_t origin, vec3_t dir, int entityNum)
{
	CG_Bleed(origin, entityNum);

	// some weapons will make an explosion with the blood, while
	// others will just make the blood
	switch (weapon) {
	case WP_GRENADE_LAUNCHER:
	case WP_ROCKET_LAUNCHER:
	case WP_PLASMAGUN:
	case WP_BFG:
		CG_MissileHitWall(weapon, 0, origin, dir, IMPACTSOUND_FLESH);
		break;
	default:
		break;
	}
}

// SHOTGUN TRACING

static void CG_ShotgunPellet(vec3_t start, vec3_t end, int skipNum)
{
	trace_t		tr;
	int			sourceContentType, destContentType;
	int			mask;

	if (cgs.gametype == GT_DEFRAG) {
		mask = MASK_SOLID;
	} else {
		mask = MASK_SHOT;
	}

	CG_Trace(&tr, start, NULL, NULL, end, skipNum, mask);

	sourceContentType = CG_PointContents(start, 0);
	destContentType = CG_PointContents(tr.endpos, 0);

	// FIXME: should probably move this cruft into CG_BubbleTrail
	if (sourceContentType == destContentType) {
		if (sourceContentType & CONTENTS_WATER) {
			CG_BubbleTrail(start, tr.endpos, 32);
		}
	} else if (sourceContentType & CONTENTS_WATER) {
		trace_t trace;

		trap_CM_BoxTrace(&trace, end, start, NULL, NULL, 0, CONTENTS_WATER);
		CG_BubbleTrail(start, trace.endpos, 32);
	} else if (destContentType & CONTENTS_WATER) {
		trace_t trace;

		trap_CM_BoxTrace(&trace, start, end, NULL, NULL, 0, CONTENTS_WATER);
		CG_BubbleTrail(tr.endpos, trace.endpos, 32);
	}

	if (tr.surfaceFlags & SURF_NOIMPACT) {
		return;
	}

	if (cg_entities[tr.entityNum].currentState.eType == ET_PLAYER) {
		CG_MissileHitPlayer(WP_SHOTGUN, tr.endpos, tr.plane.normal, tr.entityNum);
	} else {
		if (tr.surfaceFlags & SURF_NOIMPACT) {
			// SURF_NOIMPACT will not make a flame puff or a mark
			return;
		}
		if (tr.surfaceFlags & SURF_METALSTEPS) {
			CG_MissileHitWall(WP_SHOTGUN, 0, tr.endpos, tr.plane.normal, IMPACTSOUND_METAL);
		} else {
			CG_MissileHitWall(WP_SHOTGUN, 0, tr.endpos, tr.plane.normal, IMPACTSOUND_DEFAULT);
		}
	}
}

/**
Perform the same traces the server did to locate the
hit splashes
*/
void CG_ShotgunPattern(vec3_t origin, vec3_t origin2, int seed, int otherEntNum)
{
	int			i;
	float		r, u;
	vec3_t		end;
	vec3_t		forward, right, up;

	// derive the right and up vectors from the forward vector, because
	// the client won't have any other information
	VectorNormalize2(origin2, forward);
	PerpendicularVector(right, forward);
	CrossProduct(forward, right, up);

	// generate the "random" spread pattern
	for (i = 0; i < DEFAULT_SHOTGUN_COUNT; i++) {
		r = Q_crandom(&seed) * DEFAULT_SHOTGUN_SPREAD * 16;
		u = Q_crandom(&seed) * DEFAULT_SHOTGUN_SPREAD * 16;
		VectorMA(origin, 8192 * 16, forward, end);
		VectorMA(end, r, right, end);
		VectorMA(end, u, up, end);

		CG_ShotgunPellet(origin, end, otherEntNum);
	}
}

void CG_ShotgunFire(entityState_t *es)
{
	vec3_t	v;

	VectorSubtract(es->origin2, es->pos.trBase, v);
	VectorNormalize(v);
	VectorScale(v, 32, v);
	VectorAdd(es->pos.trBase, v, v);
	CG_ShotgunPattern(es->pos.trBase, es->origin2, es->eventParm, es->otherEntityNum);
}

// BULLETS

static qboolean CG_CalcMuzzlePoint(int entityNum, vec3_t muzzle)
{
	vec3_t		forward;
	centity_t	*cent;
	int			anim;

	if (entityNum == cg.snap->ps.clientNum) {
		VectorCopy(cg.snap->ps.origin, muzzle);
		muzzle[2] += cg.snap->ps.viewheight;
		AngleVectors(cg.snap->ps.viewangles, forward, NULL, NULL);
		VectorMA(muzzle, 14, forward, muzzle);
		return qtrue;
	}

	cent = &cg_entities[entityNum];
	if (!cent->currentValid) {
		return qfalse;
	}

	VectorCopy(cent->currentState.pos.trBase, muzzle);

	AngleVectors(cent->currentState.apos.trBase, forward, NULL, NULL);
	anim = cent->currentState.legsAnim & ~ANIM_TOGGLEBIT;
	if (anim == LEGS_WALKCR || anim == LEGS_IDLECR) {
		muzzle[2] += CROUCH_VIEWHEIGHT;
	} else {
		muzzle[2] += DEFAULT_VIEWHEIGHT;
	}

	VectorMA(muzzle, 14, forward, muzzle);

	return qtrue;

}

void CG_Tracer(vec3_t source, vec3_t dest)
{
	vec3_t		forward, right;
	polyVert_t	verts[4];
	vec3_t		line;
	float		len, begin, end;
	vec3_t		start, finish;
	vec3_t		midpoint;

	// tracer
	VectorSubtract(dest, source, forward);
	len = VectorNormalize(forward);

	// start at least a little ways from the muzzle
	if (len < 100) {
		return;
	}
	begin = 50 + random() * (len - 60);
	end = begin + cg_tracerLength.value;
	if (end > len) {
		end = len;
	}
	VectorMA(source, begin, forward, start);
	VectorMA(source, end, forward, finish);

	line[0] = DotProduct(forward, cg.refdef.viewaxis[1]);
	line[1] = DotProduct(forward, cg.refdef.viewaxis[2]);

	VectorScale(cg.refdef.viewaxis[1], line[1], right);
	VectorMA(right, -line[0], cg.refdef.viewaxis[2], right);
	VectorNormalize(right);

	VectorMA(finish, cg_tracerWidth.value, right, verts[0].xyz);
	verts[0].st[0] = 0;
	verts[0].st[1] = 1;
	verts[0].modulate[0] = 255;
	verts[0].modulate[1] = 255;
	verts[0].modulate[2] = 255;
	verts[0].modulate[3] = 255;

	VectorMA(finish, -cg_tracerWidth.value, right, verts[1].xyz);
	verts[1].st[0] = 1;
	verts[1].st[1] = 0;
	verts[1].modulate[0] = 255;
	verts[1].modulate[1] = 255;
	verts[1].modulate[2] = 255;
	verts[1].modulate[3] = 255;

	VectorMA(start, -cg_tracerWidth.value, right, verts[2].xyz);
	verts[2].st[0] = 1;
	verts[2].st[1] = 1;
	verts[2].modulate[0] = 255;
	verts[2].modulate[1] = 255;
	verts[2].modulate[2] = 255;
	verts[2].modulate[3] = 255;

	VectorMA(start, cg_tracerWidth.value, right, verts[3].xyz);
	verts[3].st[0] = 0;
	verts[3].st[1] = 0;
	verts[3].modulate[0] = 255;
	verts[3].modulate[1] = 255;
	verts[3].modulate[2] = 255;
	verts[3].modulate[3] = 255;

	trap_R_AddPolyToScene(cgs.media.tracerShader, 4, verts);

	midpoint[0] = (start[0] + finish[0]) * 0.5;
	midpoint[1] = (start[1] + finish[1]) * 0.5;
	midpoint[2] = (start[2] + finish[2]) * 0.5;

	// add the tracer sound
	trap_S_StartSound(midpoint, ENTITYNUM_WORLD, CHAN_AUTO, cgs.media.tracerSound);
}

/**
Renders bullet effects.
*/
void CG_Bullet(vec3_t end, int sourceEntityNum, vec3_t normal, qboolean flesh, int fleshEntityNum)
{
	trace_t	trace;
	int		sourceContentType, destContentType;
	vec3_t	start;

	// if the shooter is currently valid, calc a source point and possibly
	// do trail effects
	if (sourceEntityNum >= 0 && cg_tracerChance.value > 0) {
		if (CG_CalcMuzzlePoint(sourceEntityNum, start)) {
			sourceContentType = CG_PointContents(start, 0);
			destContentType = CG_PointContents(end, 0);

			// do a complete bubble trail if necessary
			if ((sourceContentType == destContentType) && (sourceContentType & CONTENTS_WATER)) {
				CG_BubbleTrail(start, end, 32);
			}
			// bubble trail from water into air
			else if ((sourceContentType & CONTENTS_WATER)) {
				trap_CM_BoxTrace(&trace, end, start, NULL, NULL, 0, CONTENTS_WATER);
				CG_BubbleTrail(start, trace.endpos, 32);
			}
			// bubble trail from air into water
			else if ((destContentType & CONTENTS_WATER)) {
				trap_CM_BoxTrace(&trace, start, end, NULL, NULL, 0, CONTENTS_WATER);
				CG_BubbleTrail(trace.endpos, end, 32);
			}

			// draw a tracer
			if (random() < cg_tracerChance.value) {
				CG_Tracer(start, end);
			}
		}
	}

	// impact splash and mark
	if (flesh) {
		CG_Bleed(end, fleshEntityNum);
	} else {
		CG_MissileHitWall(WP_MACHINEGUN, 0, end, normal, IMPACTSOUND_DEFAULT);
	}
}


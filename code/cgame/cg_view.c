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
// cg_view.c -- setup all the parameters (position, angle, etc)
// for a 3D rendering

#include "cg_local.h"

#define FOCUS_DISTANCE	512
#define WAVE_AMPLITUDE	1
#define WAVE_FREQUENCY	0.4

/** MODEL TESTING

The viewthing and gun positioning tools from Q2 have been integrated and
enhanced into a single model testing facility.

Model viewing can begin with either "testmodel <modelname>" or "testgun <modelname>".

The names must be the full pathname after the basedir, like 
"models/weapons/v_launch/tris.md3" or "players/male/tris.md3"

Testmodel will create a fake entity 100 units in front of the current view
position, directly facing the viewer.  It will remain immobile, so you can
move around it to view it from different angles.

Testgun will cause the model to follow the player around and supress the real
view weapon model.  The default frame 0 of most guns is completely off screen,
so you will probably have to cycle a couple frames to see it.

"nextframe", "prevframe", "nextskin", and "prevskin" commands will change the
frame or skin of the testmodel.  These are bound to F5, F6, F7, and F8 in
q3default.cfg.

If a gun is being tested, the "gun_x", "gun_y", and "gun_z" variables will let
you adjust the positioning.

Note that none of the model testing features update while the game is paused, so
it may be convenient to test with deathmatch set to 1 so that bringing down the
console doesn't pause the game.
*/

/**
Creates an entity in front of the current position, which
can then be moved around
*/
void CG_TestModel_f(void)
{
	vec3_t		angles;

	memset(&cg.testModelEntity, 0, sizeof(cg.testModelEntity));
	if (trap_Argc() < 2) {
		return;
	}

	Q_strncpyz(cg.testModelName, BG_Argv(1), MAX_QPATH);
	cg.testModelEntity.hModel = trap_R_RegisterModel(cg.testModelName);

	if (trap_Argc() == 3) {
		cg.testModelEntity.backlerp = atof(BG_Argv(2));
		cg.testModelEntity.frame = 1;
		cg.testModelEntity.oldframe = 0;
	}
	if (!cg.testModelEntity.hModel) {
		CG_Printf("Can't register model\n");
		return;
	}

	VectorMA(cg.refdef.vieworg, 100, cg.refdef.viewaxis[0], cg.testModelEntity.origin);

	angles[PITCH] = 0;
	angles[YAW] = 180 + cg.refdefViewAngles[1];
	angles[ROLL] = 0;

	AnglesToAxis(angles, cg.testModelEntity.axis);
	cg.testGun = qfalse;
}

/**
Replaces the current view weapon with the given model
*/
void CG_TestGun_f(void)
{
	CG_TestModel_f();
	cg.testGun = qtrue;
	cg.testModelEntity.renderfx = RF_MINLIGHT | RF_DEPTHHACK | RF_FIRST_PERSON;
}

void CG_TestModelNextFrame_f(void)
{
	cg.testModelEntity.frame++;
	CG_Printf("frame %i\n", cg.testModelEntity.frame);
}

void CG_TestModelPrevFrame_f(void)
{
	cg.testModelEntity.frame--;
	if (cg.testModelEntity.frame < 0) {
		cg.testModelEntity.frame = 0;
	}
	CG_Printf("frame %i\n", cg.testModelEntity.frame);
}

void CG_TestModelNextSkin_f(void)
{
	cg.testModelEntity.skinNum++;
	CG_Printf("skin %i\n", cg.testModelEntity.skinNum);
}

void CG_TestModelPrevSkin_f(void)
{
	cg.testModelEntity.skinNum--;
	if (cg.testModelEntity.skinNum < 0) {
		cg.testModelEntity.skinNum = 0;
	}
	CG_Printf("skin %i\n", cg.testModelEntity.skinNum);
}

static void CG_AddTestModel(void)
{
	int		i;

	// re-register the model, because the level may have changed
	cg.testModelEntity.hModel = trap_R_RegisterModel(cg.testModelName);
	if (!cg.testModelEntity.hModel) {
		CG_Printf ("Can't register model\n");
		return;
	}

	// if testing a gun, set the origin relative to the view origin
	if (cg.testGun) {
		VectorCopy(cg.refdef.vieworg, cg.testModelEntity.origin);
		VectorCopy(cg.refdef.viewaxis[0], cg.testModelEntity.axis[0]);
		VectorCopy(cg.refdef.viewaxis[1], cg.testModelEntity.axis[1]);
		VectorCopy(cg.refdef.viewaxis[2], cg.testModelEntity.axis[2]);

		// allow the position to be adjusted
		for (i = 0; i < 3; i++) {
			cg.testModelEntity.origin[i] += cg.refdef.viewaxis[0][i] * cg_gun_x.value;
			cg.testModelEntity.origin[i] += cg.refdef.viewaxis[1][i] * cg_gun_y.value;
			cg.testModelEntity.origin[i] += cg.refdef.viewaxis[2][i] * cg_gun_z.value;
		}
	}

	trap_R_AddRefEntityToScene(&cg.testModelEntity);
}

/**
Sets the coordinates of the rendered window
*/
static void CG_CalcVrect(void)
{
	int		size;

	// the intermission should allways be full screen
	if (cg.snap->ps.pm_type == PM_INTERMISSION) {
		size = 100;
	} else {
		// bound normal viewsize
		if (cg_viewsize.integer < 30) {
			trap_Cvar_Set ("cg_viewsize","30");
			size = 30;
		} else if (cg_viewsize.integer > 100) {
			trap_Cvar_Set ("cg_viewsize","100");
			size = 100;
		} else {
			size = cg_viewsize.integer;
		}

	}
	cg.refdef.width = cgs.glconfig.vidWidth*size/100;
	cg.refdef.width &= ~1;

	cg.refdef.height = cgs.glconfig.vidHeight*size/100;
	cg.refdef.height &= ~1;

	cg.refdef.x = (cgs.glconfig.vidWidth - cg.refdef.width)/2;
	cg.refdef.y = (cgs.glconfig.vidHeight - cg.refdef.height)/2;
}

static void CG_OffsetThirdPersonView(void)
{
	vec3_t		forward, right, up;
	vec3_t		view;
	vec3_t		focusAngles;
	trace_t		trace;
	static vec3_t	mins = { -4, -4, -4 };
	static vec3_t	maxs = { 4, 4, 4 };
	vec3_t		focusPoint;
	float		focusDist;
	float		forwardScale, sideScale;

	cg.refdef.vieworg[2] += cg.predictedPlayerState.viewheight;

	VectorCopy(cg.refdefViewAngles, focusAngles);

	// if dead, look at killer
	if (cg.predictedPlayerState.stats[STAT_HEALTH] <= 0) {
		focusAngles[YAW] = cg.predictedPlayerState.stats[STAT_DEAD_YAW];
		cg.refdefViewAngles[YAW] = cg.predictedPlayerState.stats[STAT_DEAD_YAW];
	}

	if (focusAngles[PITCH] > 45) {
		focusAngles[PITCH] = 45;		// don't go too far overhead
	}
	AngleVectors(focusAngles, forward, NULL, NULL);

	VectorMA(cg.refdef.vieworg, FOCUS_DISTANCE, forward, focusPoint);

	VectorCopy(cg.refdef.vieworg, view);

	view[2] += 8;

	cg.refdefViewAngles[PITCH] *= 0.5;

	AngleVectors(cg.refdefViewAngles, forward, right, up);

	forwardScale = cos(cg_thirdPersonAngle.value / 180 * M_PI);
	sideScale = sin(cg_thirdPersonAngle.value / 180 * M_PI);
	VectorMA(view, -cg_thirdPersonRange.value * forwardScale, forward, view);
	VectorMA(view, -cg_thirdPersonRange.value * sideScale, right, view);

	// trace a ray from the origin to the viewpoint to make sure the view isn't
	// in a solid block.  Use an 8 by 8 block to prevent the view from near clipping anything

	if (!cg_cameraMode.integer) {
		CG_Trace(&trace, cg.refdef.vieworg, mins, maxs, view, cg.predictedPlayerState.clientNum, MASK_SOLID);

		if (trace.fraction != 1.0) {
			VectorCopy(trace.endpos, view);
			view[2] += (1.0 - trace.fraction) * 32;
			// try another trace to this position, because a tunnel may have the ceiling
			// close enough that this is poking out

			CG_Trace(&trace, cg.refdef.vieworg, mins, maxs, view, cg.predictedPlayerState.clientNum, MASK_SOLID);
			VectorCopy(trace.endpos, view);
		}
	}


	VectorCopy(view, cg.refdef.vieworg);

	// select pitch to look at focus point from vieword
	VectorSubtract(focusPoint, cg.refdef.vieworg, focusPoint);
	focusDist = sqrt(focusPoint[0] * focusPoint[0] + focusPoint[1] * focusPoint[1]);
	if (focusDist < 1) {
		focusDist = 1;	// should never happen
	}
	cg.refdefViewAngles[PITCH] = -180 / M_PI * atan2(focusPoint[2], focusDist);
	cg.refdefViewAngles[YAW] -= cg_thirdPersonAngle.value;
}

// this causes a compiler bug on mac MrC compiler
static void CG_StepOffset(void)
{
	int		timeDelta;
	
	// smooth out stair climbing
	timeDelta = cg.time - cg.stepTime;
	if (timeDelta < STEP_TIME) {
		cg.refdef.vieworg[2] -= cg.stepChange 
			* (STEP_TIME - timeDelta) / STEP_TIME;
	}
}

static void CG_OffsetFirstPersonView(void)
{
	float			*origin;
	float			*angles;
	float			delta;
	float			f;
	int				timeDelta;
	
	if (cg.snap->ps.pm_type == PM_INTERMISSION) {
		return;
	}

	origin = cg.refdef.vieworg;
	angles = cg.refdefViewAngles;

	// if dead, fix the angle and don't add any kick
	if (cg.snap->ps.stats[STAT_HEALTH] <= 0) {
		angles[ROLL] = 40;
		angles[PITCH] = -15;
		angles[YAW] = cg.snap->ps.stats[STAT_DEAD_YAW];
		origin[2] += cg.predictedPlayerState.viewheight;
		return;
	}

	// add view height
	origin[2] += cg.predictedPlayerState.viewheight;

	// smooth out duck height changes
	timeDelta = cg.time - cg.duckTime;
	if (timeDelta < DUCK_TIME) {
		cg.refdef.vieworg[2] -= cg.duckChange 
			* (DUCK_TIME - timeDelta) / DUCK_TIME;
	}

	// add fall height
	delta = cg.time - cg.landTime;
	if (delta < LAND_DEFLECT_TIME) {
		f = delta / LAND_DEFLECT_TIME;
		cg.refdef.vieworg[2] += cg.landChange * f;
	} else if (delta < LAND_DEFLECT_TIME + LAND_RETURN_TIME) {
		delta -= LAND_DEFLECT_TIME;
		f = 1.0 - (delta / LAND_RETURN_TIME);
		cg.refdef.vieworg[2] += cg.landChange * f;
	}

	// add step offset
	CG_StepOffset();

	// pivot the eye based on a neck length
#if 0
	{
#define	NECK_LENGTH		8
	vec3_t			forward, up;
 
	cg.refdef.vieworg[2] -= NECK_LENGTH;
	AngleVectors(cg.refdefViewAngles, forward, NULL, up);
	VectorMA(cg.refdef.vieworg, 3, forward, cg.refdef.vieworg);
	VectorMA(cg.refdef.vieworg, NECK_LENGTH, up, cg.refdef.vieworg);
	}
#endif
}

void CG_ZoomDown_f(void)
{
	if (cg_zoomToggle.integer) {
		cg.zoomTime = cg.time;
		cg.zoomed = !cg.zoomed;
	} else if (!cg.zoomed) {
		cg.zoomed = qtrue;
		cg.zoomTime = cg.time;
	}
}

void CG_ZoomUp_f(void)
{
	if (!cg_zoomToggle.integer && cg.zoomed) {
		cg.zoomed = qfalse;
		cg.zoomTime = cg.time;
	}
}

/**
Fixed fov at intermissions, otherwise account for fov variable and zooms.
*/
static int CG_CalcFov(void)
{
	float	x;
	float	phase;
	float	v;
	int		contents;
	float	fov_x, fov_y;
	int		inwater;
	float	aspect;

	if (cg.predictedPlayerState.pm_type == PM_INTERMISSION) {
		fov_x = 90;
	} else {
		float	f;

		fov_x = cg_fov.value;

		if (cg_zoomScaling.value > 0.0f) {
			f = (cg.time - cg.zoomTime) / (cg_zoomScaling.value * ZOOM_TIME);
		}
		else {
			f = 1.0f;
		}

		if (cg.zoomed) {
			fov_x = (f < 1.0f ? fov_x + f * (cg_zoomFov.value - fov_x) : cg_zoomFov.value);
		} else if (f < 1.0f) {
			fov_x = cg_zoomFov.value + f * (fov_x - cg_zoomFov.value);
		}
	}

	aspect = (float) cg.refdef.height / (float) cg.refdef.width;

	// Based on LordHavoc's code for Darkplaces
	// http://www.quakeworld.nu/forum/topic/53/what-does-your-qw-look-like/page/30
	fov_x = atan2(tan(fov_x * M_PI / 360.0f) * 0.75f / aspect, 1) * 360.0f / M_PI;

	if (cg_zoomSensitivity.value != 0.0f && fov_x != cg_fov.value) {
		float	fovarg1, fovarg2;
		fovarg1 = fov_x / 720.0f * M_PI;
		fovarg2 = cg_fov.value / 720.0f * M_PI;
		cg.zoomSensitivity = atan2(aspect * tan(fovarg1), 1) / atan2(aspect * tan(fovarg2), 1);
		cg.zoomSensitivity *= cg_zoomSensitivity.value;
	} else {
		cg.zoomSensitivity = 1.0f;
	}

	x = cg.refdef.width / tan(fov_x / 360.0f * M_PI);
	fov_y = atan2(cg.refdef.height, x);
	fov_y = fov_y * 360.0f / M_PI;

	// warp if underwater
	contents = CG_PointContents(cg.refdef.vieworg, -1);
	if (contents & (CONTENTS_WATER | CONTENTS_SLIME | CONTENTS_LAVA)){
		phase = cg.time / 1000.0f * WAVE_FREQUENCY * M_PI * 2.0f;
		v = WAVE_AMPLITUDE * sin(phase);
		fov_x += v;
		fov_y -= v;
		inwater = qtrue;
	} else {
		inwater = qfalse;
	}

	cg.refdef.fov_x = fov_x;
	cg.refdef.fov_y = fov_y;

	return inwater;
}

static void CG_DamageBlendBlob(void)
{
	int			t;
	int			maxTime;
	refEntity_t		ent;

	if (!cg_blood.integer) {
		return;
	}

	if (!cg.damageValue) {
		return;
	}

	//if (cg.cameraMode) {
	//	return;
	//}

	// ragePro systems can't fade blends, so don't obscure the screen
	if (cgs.glconfig.hardwareType == GLHW_RAGEPRO) {
		return;
	}

	maxTime = DAMAGE_TIME;
	t = cg.time - cg.damageTime;
	if (t <= 0 || t >= maxTime) {
		return;
	}


	memset(&ent, 0, sizeof(ent));
	ent.reType = RT_SPRITE;
	ent.renderfx = RF_FIRST_PERSON;

	VectorMA(cg.refdef.vieworg, 8, cg.refdef.viewaxis[0], ent.origin);
	VectorMA(ent.origin, cg.damageX * -8, cg.refdef.viewaxis[1], ent.origin);
	VectorMA(ent.origin, cg.damageY * 8, cg.refdef.viewaxis[2], ent.origin);

	ent.radius = cg.damageValue * 3;
	ent.customShader = cgs.media.viewBloodShader;
	ent.shaderRGBA[0] = 255;
	ent.shaderRGBA[1] = 255;
	ent.shaderRGBA[2] = 255;
	ent.shaderRGBA[3] = 200 * (1.0 - ((float)t / maxTime));
	trap_R_AddRefEntityToScene(&ent);
}

/**
Sets cg.refdef view values
*/
static int CG_CalcViewValues(void)
{
	playerState_t	*ps;

	memset(&cg.refdef, 0, sizeof(cg.refdef));

	// calculate size of 3D view
	CG_CalcVrect();

	ps = &cg.predictedPlayerState;

	// intermission view
	if (ps->pm_type == PM_INTERMISSION) {
		VectorCopy(ps->origin, cg.refdef.vieworg);
		VectorCopy(ps->viewangles, cg.refdefViewAngles);
		AnglesToAxis(cg.refdefViewAngles, cg.refdef.viewaxis);
		return CG_CalcFov();
	}

	cg.bobcycle = (ps->bobCycle & 128) >> 7;
	cg.bobfracsin = fabs(sin((ps->bobCycle & 127) / 127.0 * M_PI));
	cg.xyspeed = sqrt(ps->velocity[0] * ps->velocity[0] +
		ps->velocity[1] * ps->velocity[1]);

	VectorCopy(ps->origin, cg.refdef.vieworg);
	VectorCopy(ps->viewangles, cg.refdefViewAngles);

	if (cg_cameraOrbit.integer) {
		if (cg.time > cg.nextOrbitTime) {
			cg.nextOrbitTime = cg.time + cg_cameraOrbitDelay.integer;
			cg_thirdPersonAngle.value += cg_cameraOrbit.value;
		}
	}
	// add error decay
	if (cg_errorDecay.value > 0) {
		int		t;
		float	f;

		t = cg.time - cg.predictedErrorTime;
		f = (cg_errorDecay.value - t) / cg_errorDecay.value;
		if (f > 0 && f < 1) {
			VectorMA(cg.refdef.vieworg, f, cg.predictedError, cg.refdef.vieworg);
		} else {
			cg.predictedErrorTime = 0;
		}
	}

	if (cg.renderingThirdPerson) {
		// back away from character
		CG_OffsetThirdPersonView();
	} else {
		// offset for local bobbing and kicks
		CG_OffsetFirstPersonView();
	}

	// position eye relative to origin
	AnglesToAxis(cg.refdefViewAngles, cg.refdef.viewaxis);

	if (cg.hyperspace) {
		cg.refdef.rdflags |= RDF_NOWORLDMODEL | RDF_HYPERSPACE;
	}

	// field of view
	return CG_CalcFov();
}

static void CG_WarmupSounds(void)
{
	int	sec;

	if (cgs.gametype == GT_ELIMINATION && cg.time < cgs.roundStartTime) {
		sec = (cgs.roundStartTime - cg.time) / 1000;
	} else if (cg.warmup > 0) {
		sec = (cg.warmup - cg.time) / 1000;
		if (sec < 0) {
			sec = 0;
		}
	} else {
		return;
	}

	if (sec != cg.warmupCount) {
		cg.warmupCount = sec;
		switch (sec) {
		case 0:
			trap_S_StartLocalSound(cgs.media.count1Sound, CHAN_ANNOUNCER);
			break;
		case 1:
			trap_S_StartLocalSound(cgs.media.count2Sound, CHAN_ANNOUNCER);
			break;
		case 2:
			trap_S_StartLocalSound(cgs.media.count3Sound, CHAN_ANNOUNCER);
			break;
		default:
			break;
		}
	}
}

static void CG_PopReward(void)
{
	int	i;

	if (!cg_drawRewards.integer || cg.rewardStack <= 0) {
		return;
	}

	if (cg.time - cg.rewardTime > REWARD_TIME) {
		return;
	}

	for (i = 0; i < cg.rewardStack; i++) {
		cg.rewardSound[i] = cg.rewardSound[i + 1];
		cg.rewardShader[i] = cg.rewardShader[i + 1];
		cg.rewardCount[i] = cg.rewardCount[i + 1];
	}
	cg.rewardTime = cg.time;
	cg.rewardStack--;
	trap_S_StartLocalSound(cg.rewardSound[0], CHAN_ANNOUNCER);
}

static void CG_PowerupTimerSounds(void)
{
	int		i;
	int		t;

	// powerup timers going away
	for (i = 0; i < MAX_POWERUPS; i++) {
		t = cg.snap->ps.powerups[i];
		if (t <= cg.time) {
			continue;
		}
		if (t - cg.time >= POWERUP_BLINKS * POWERUP_BLINK_TIME) {
			continue;
		}
		if ((t - cg.time) / POWERUP_BLINK_TIME != (t - cg.oldTime) / POWERUP_BLINK_TIME) {
			trap_S_StartSound(NULL, cg.snap->ps.clientNum, CHAN_ITEM, cgs.media.wearOffSound);
		}
	}
}

void CG_AddBufferedSound(sfxHandle_t sfx)
{
	if (!sfx)
		return;
	cg.soundBuffer[cg.soundBufferIn] = sfx;
	cg.soundBufferIn = (cg.soundBufferIn + 1) % MAX_SOUNDBUFFER;
	if (cg.soundBufferIn == cg.soundBufferOut) {
		cg.soundBufferOut++;
	}
}

static void CG_PlayBufferedSounds(void)
{
	if (cg.soundTime < cg.time) {
		if (cg.soundBufferOut != cg.soundBufferIn && cg.soundBuffer[cg.soundBufferOut]) {
			trap_S_StartLocalSound(cg.soundBuffer[cg.soundBufferOut], CHAN_ANNOUNCER);
			cg.soundBuffer[cg.soundBufferOut] = 0;
			cg.soundBufferOut = (cg.soundBufferOut + 1) % MAX_SOUNDBUFFER;
			cg.soundTime = cg.time + 750;
		}
	}
}

static void CG_AddSpawnpoints(void)
{
	refEntity_t	re;
	int			i;
	vec3_t		mins, maxs;

	if (!cg_drawSpawnpoints.integer) {
		return;
	}

	if (!cg.warmup) {
		return;
	}

	memset(&re, 0, sizeof re);
	re.hModel = cgs.media.spawnpoint;
	re.customShader = cgs.media.spawnpointShader;

	if (cg_drawSpawnpoints.integer == 2) {
		re.renderfx |= RF_DEPTHHACK;
	}

	// move spawnpoint to the ground
	trap_R_ModelBounds(cgs.media.spawnpoint, mins, maxs);

	for (i = 0; i < cg.numSpawnpoints; i++) {
		VectorCopy(cg.spawnOrigin[i], re.origin);
		AnglesToAxis(cg.spawnAngle[i], re.axis);

		re.origin[2] += maxs[2] - mins[2];

		switch (cg.spawnTeam[i]) {
		case TEAM_FREE:
			CG_SetRGBA(re.shaderRGBA, colorGreen);
			break;
		case TEAM_RED:
			CG_SetRGBA(re.shaderRGBA, colorRed);
			break;
		case TEAM_BLUE:
			CG_SetRGBA(re.shaderRGBA, colorBlue);
			break;
		default:
			continue;
		}
		trap_R_AddRefEntityToScene(&re);
	}
}

/**
Generates and draws a game scene and status information at the given time.
*/
void CG_DrawActiveFrame(int serverTime, stereoFrame_t stereoView, qboolean demoPlayback)
{
	int		inwater;

	cg.time = serverTime;
	cg.demoPlayback = demoPlayback;

	// get the rendering configuration from the client system
	trap_GetGlconfig(&cgs.glconfig);
	cgs.screenXScale = cgs.glconfig.vidWidth / 640.0;
	cgs.screenYScale = cgs.glconfig.vidHeight / 480.0;

	// update cvars
	CG_UpdateCvars();

	// if we are only updating the screen as a loading
	// pacifier, don't even try to read snapshots
	if (cg.showInfoScreen) {
		CG_DrawInformation();
		return;
	}

	// any looped sounds will be respecified as entities
	// are added to the render list
	trap_S_ClearLoopingSounds(qfalse);

	// clear all the render lists
	trap_R_ClearScene();

	// set up cg.snap and possibly cg.nextSnap
	CG_ProcessSnapshots();

	// if we haven't received any snapshots yet, all
	// we can draw is the information screen
	if (!cg.snap || (cg.snap->snapFlags & SNAPFLAG_NOT_ACTIVE)) {
		CG_DrawInformation();
		return;
	}

	// let the client system know what our weapon and zoom settings are
	trap_SetUserCmdValue(cg.weaponSelect, cg.zoomSensitivity);

	// this counter will be bumped for every valid scene we generate
	cg.clientFrame++;

	// update cg.predictedPlayerState
	CG_PredictPlayerState();

	// decide on third person view
	cg.renderingThirdPerson = cg_thirdPerson.integer || (cg.snap->ps.stats[STAT_HEALTH] <= 0);

	// build cg.refdef
	inwater = CG_CalcViewValues();

	// first person blend blobs, done after AnglesToAxis
	if (!cg.renderingThirdPerson) {
		CG_DamageBlendBlob();
	}

	// build the render lists
	if (!cg.hyperspace) {
		CG_AddPacketEntities();			// adter calcViewValues, so predicted player state is correct
		CG_AddMarks();
		CG_AddParticles();
		CG_AddLocalEntities();
		CG_AddSpawnpoints();
	}
	CG_AddViewWeapon(&cg.predictedPlayerState);

	// add buffered sounds
	CG_PlayBufferedSounds();

	// finish up the rest of the refdef
	if (cg.testModelEntity.hModel) {
		CG_AddTestModel();
	}
	cg.refdef.time = cg.time;
	memcpy(cg.refdef.areamask, cg.snap->areamask, sizeof(cg.refdef.areamask));

	CG_WarmupSounds();

	CG_PopReward();

	// warning sounds when powerup is wearing off
	CG_PowerupTimerSounds();

	// update audio positions
	trap_S_Respatialize(cg.snap->ps.clientNum, cg.refdef.vieworg, cg.refdef.viewaxis, inwater);

	// make sure the lagometerSample and frame timing isn't done twice when in stereo
	if (stereoView != STEREO_RIGHT) {
		cg.frametime = cg.time - cg.oldTime;
		if (cg.frametime < 0) {
			cg.frametime = 0;
		}
		cg.oldTime = cg.time;
		CG_AddLagometerFrameInfo();
	}
	if (cg_timescale.value != cg_timescaleFadeEnd.value) {
		if (cg_timescale.value < cg_timescaleFadeEnd.value) {
			cg_timescale.value += cg_timescaleFadeSpeed.value * ((float)cg.frametime) / 1000;
			if (cg_timescale.value > cg_timescaleFadeEnd.value)
				cg_timescale.value = cg_timescaleFadeEnd.value;
		}
		else {
			cg_timescale.value -= cg_timescaleFadeSpeed.value * ((float)cg.frametime) / 1000;
			if (cg_timescale.value < cg_timescaleFadeEnd.value)
				cg_timescale.value = cg_timescaleFadeEnd.value;
		}
		if (cg_timescaleFadeSpeed.value) {
			trap_Cvar_Set("timescale", va("%f", cg_timescale.value));
		}
	}

	// actually issue the rendering calls
	CG_DrawActive(stereoView);

	if (cg_stats.integer) {
		CG_Printf("cg.clientFrame:%i\n", cg.clientFrame);
	}
}


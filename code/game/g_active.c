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
// g_active.c

#include "g_local.h"

/**
Called just before a snapshot is sent to the given player.
Totals up all damage and generates both the player_state_t
damage values to that client for pain blends and kicks, and
global pain sound events for all clients.
*/
void P_DamageFeedback(gentity_t *player)
{
	gclient_t	*client;
	float		count;
	vec3_t		angles;

	client = player->client;
	if (client->ps.pm_type == PM_DEAD) {
		return;
	}

	// total points of damage shot at the player this frame
	count = client->damage_blood + client->damage_armor;
	if (count == 0) {
		return;		// didn't take any damage
	}

	if (count > 255) {
		count = 255;
	}

	// send the information to the client

	// world damage (falling, slime, etc) uses a special code
	// to make the blend blob centered instead of positional
	if (client->damage_fromWorld) {
		client->ps.damagePitch = 255;
		client->ps.damageYaw = 255;

		client->damage_fromWorld = qfalse;
	} else {
		vectoangles(client->damage_from, angles);
		client->ps.damagePitch = angles[PITCH]/360.0 * 256;
		client->ps.damageYaw = angles[YAW]/360.0 * 256;
	}

	// play an apropriate pain sound
	if ((level.time > player->pain_debounce_time) && !(player->flags & FL_GODMODE)) {
		player->pain_debounce_time = level.time + 700;
		G_AddEvent(player, EV_PAIN, player->health);
		client->ps.damageEvent++;
	}


	client->ps.damageCount = count;

	//
	// clear totals
	//
	client->damage_blood = 0;
	client->damage_armor = 0;
}

/**
Check for lava / slime contents and drowning
*/
void P_WorldEffects(gentity_t *ent)
{
	qboolean	envirosuit;
	int			waterlevel;

	if (ent->client->noclip) {
		ent->client->airOutTime = level.time + 12000;	// don't need air
		return;
	}

	waterlevel = ent->waterlevel;

	envirosuit = ent->client->ps.powerups[PW_BATTLESUIT] > level.time;

	//
	// check for drowning
	//
	if (waterlevel == 3) {
		// envirosuit give air
		if (envirosuit) {
			ent->client->airOutTime = level.time + 10000;
		}

		// if out of air, start drowning
		if (ent->client->airOutTime < level.time) {
			// drown!
			ent->client->airOutTime += 1000;
			if (ent->health > 0) {
				// take more damage the longer underwater
				ent->damage += 2;
				if (ent->damage > 15)
					ent->damage = 15;

				// don't play a normal pain sound
				ent->pain_debounce_time = level.time + 200;

				G_Damage(ent, NULL, NULL, NULL, NULL,
					ent->damage, DAMAGE_NO_ARMOR, MOD_WATER);
			}
		}
	} else {
		ent->client->airOutTime = level.time + 12000;
		ent->damage = 2;
	}

	//
	// check for sizzle damage (move to pmove?)
	//
	if (waterlevel && (ent->watertype & (CONTENTS_LAVA|CONTENTS_SLIME)) &&
		ent->health > 0 && ent->pain_debounce_time <= level.time)
	{
		if (envirosuit) {
			G_AddEvent(ent, EV_POWERUP_BATTLESUIT, 0);
		} else if (ent->watertype & CONTENTS_LAVA) {
				G_Damage(ent, NULL, NULL, NULL, NULL,
					30*waterlevel, 0, MOD_LAVA);
		} else if (ent->watertype & CONTENTS_SLIME) {
				G_Damage(ent, NULL, NULL, NULL, NULL,
					10*waterlevel, 0, MOD_SLIME);
		}
	}
}

void G_SetClientSound(gentity_t *ent)
{
	if (ent->waterlevel && (ent->watertype&(CONTENTS_LAVA|CONTENTS_SLIME))) {
		ent->client->ps.loopSound = level.snd_fry;
	} else {
		ent->client->ps.loopSound = 0;
	}
}

void ClientImpacts(gentity_t *ent, pmove_t *pm)
{
	int			i, j;
	trace_t		trace;
	gentity_t	*other;

	memset(&trace, 0, sizeof(trace));
	for (i = 0; i < pm->numtouch; i++) {
		for (j = 0; j < i; j++) {
			if (pm->touchents[j] == pm->touchents[i]) {
				break;
			}
		}
		if (j != i) {
			continue;	// duplicated
		}
		other = &g_entities[ pm->touchents[i] ];

		if ((ent->r.svFlags & SVF_BOT) && (ent->touch)) {
			ent->touch(ent, other, &trace);
		}

		if (!other->touch) {
			continue;
		}

		other->touch(other, ent, &trace);
	}
}

/**
Find all trigger entities that ent's current position touches.
Spectators will only interact with teleporters.
*/
void G_TouchTriggers(gentity_t *ent)
{
	int			i, num;
	int			touch[MAX_GENTITIES];
	gentity_t	*hit;
	trace_t		trace;
	vec3_t		mins, maxs;
	static vec3_t	range = { 40, 40, 52 };

	if (!ent->client) {
		return;
	}

	// dead clients don't activate triggers!
	if (ent->client->ps.stats[STAT_HEALTH] <= 0) {
		return;
	}

	VectorSubtract(ent->client->ps.origin, range, mins);
	VectorAdd(ent->client->ps.origin, range, maxs);

	num = trap_EntitiesInBox(mins, maxs, touch, MAX_GENTITIES);

	// can't use ent->absmin, because that has a one unit pad
	VectorAdd(ent->client->ps.origin, ent->r.mins, mins);
	VectorAdd(ent->client->ps.origin, ent->r.maxs, maxs);

	for (i = 0; i<num; i++) {
		hit = &g_entities[touch[i]];

		if (!hit->touch && !ent->touch) {
			continue;
		}
		if (!(hit->r.contents & CONTENTS_TRIGGER)) {
			continue;
		}

		// ignore most entities if a spectator
		if (ent->client->sess.spectatorState != SPECTATOR_NOT) {
			if (hit->s.eType != ET_TELEPORT_TRIGGER &&
				// this is ugly but adding a new ET_? type will
				// most likely cause network incompatibilities
				hit->touch != Touch_DoorTrigger) {
				continue;
			}
		}

		// use separate code for determining if an item is picked up
		// so you don't have to actually contact its bounding box
		if (hit->s.eType == ET_ITEM) {
			if (!BG_PlayerTouchesItem(&ent->client->ps, &hit->s, level.time, g_newItemHeight.integer)) {
				continue;
			}
		} else {
			if (!trap_EntityContact(mins, maxs, hit)) {
				continue;
			}
		}

		memset(&trace, 0, sizeof(trace));

		if (hit->touch) {
			hit->touch (hit, ent, &trace);
		}

		if ((ent->r.svFlags & SVF_BOT) && (ent->touch)) {
			ent->touch(ent, hit, &trace);
		}
	}

	// if we didn't touch a jump pad this pmove frame
	if (ent->client->ps.jumppad_frame != ent->client->ps.pmove_framecount) {
		ent->client->ps.jumppad_frame = 0;
		ent->client->ps.jumppad_ent = 0;
	}
}

void SpectatorThink(gentity_t *ent, usercmd_t *ucmd)
{
	pmove_t	pm;
	gclient_t	*client;

	client = ent->client;

	// if playing and in freespec, follow the next player
	if (g_gametype.integer == GT_ELIMINATION && client->sess.sessionTeam != TEAM_SPECTATOR) {
		if (client->sess.spectatorState != SPECTATOR_FOLLOW || (client->sess.spectatorClient >= 0
			&& level.clients[client->sess.spectatorClient].sess.spectatorState != SPECTATOR_NOT))
		{
			Cmd_FollowCycle_f(ent);
		}
	}

	if (client->sess.spectatorState != SPECTATOR_FOLLOW) {
		client->ps.pm_type = PM_SPECTATOR;
		client->ps.speed = 400;	// faster than normal

		// set up for pmove
		memset(&pm, 0, sizeof(pm));
		pm.ps = &client->ps;
		pm.cmd = *ucmd;
		pm.tracemask = MASK_PLAYERSOLID & ~CONTENTS_BODY;	// spectators can fly through bodies
		pm.trace = trap_Trace;
		pm.pointcontents = trap_PointContents;

		// perform a pmove
		Pmove(&pm);
		// save results of pmove
		VectorCopy(client->ps.origin, ent->s.origin);

		G_TouchTriggers(ent);
		trap_UnlinkEntity(ent);
	}

	client->oldbuttons = client->buttons;
	client->buttons = ucmd->buttons;

	// attack button cycles through spectators
	if ((client->buttons & BUTTON_ATTACK) && ! (client->oldbuttons & BUTTON_ATTACK)) {
		Cmd_FollowCycle_f(ent);
	}
}

/**
Returns qfalse if the client is dropped
*/
qboolean ClientInactivityTimer(gclient_t *client)
{
	if (g_gametype.integer == GT_DEFRAG) {
		return qtrue;
	}

	if (! g_inactivity.integer) {
		// give everyone some time, so if the operator sets g_inactivity during
		// gameplay, everyone isn't kicked
		client->inactivityTime = level.time + 60 * 1000;
		client->inactivityWarning = qfalse;
	} else if (client->pers.cmd.forwardmove ||
		client->pers.cmd.rightmove ||
		client->pers.cmd.upmove ||
		(client->pers.cmd.buttons & BUTTON_ATTACK)) {
		client->inactivityTime = level.time + g_inactivity.integer * 1000;
		client->inactivityWarning = qfalse;
	} else if (!client->pers.localClient) {
		if (level.time > client->inactivityTime) {
			SetTeam(&g_entities[client - level.clients], "speconly");
			return qfalse;
		}
		if (level.time > client->inactivityTime - 10000 && !client->inactivityWarning) {
			client->inactivityWarning = qtrue;
			trap_SendServerCommand(client - level.clients, "cp \"Ten seconds until inactivity drop!\n\"");
		}
	}
	return qtrue;
}

/**
Actions that happen once a second
*/
void ClientTimerActions(gentity_t *ent, int msec)
{
	gclient_t	*client;

	if (level.pauseStart) {
		return;
	}

	client = ent->client;
	client->timeResidual += msec;

	while (client->timeResidual >= 1000) {
		client->timeResidual -= 1000;

		// regenerate
		if (client->ps.powerups[PW_REGEN]) {
			if (ent->health < client->ps.stats[STAT_MAX_HEALTH]) {
				ent->health += 15;
				if (ent->health > client->ps.stats[STAT_MAX_HEALTH] * 1.1) {
					ent->health = client->ps.stats[STAT_MAX_HEALTH] * 1.1;
				}
				G_AddEvent(ent, EV_POWERUP_REGEN, 0);
			} else if (ent->health < client->ps.stats[STAT_MAX_HEALTH] * 2) {
				ent->health += 5;
				if (ent->health > client->ps.stats[STAT_MAX_HEALTH] * 2) {
					ent->health = client->ps.stats[STAT_MAX_HEALTH] * 2;
				}
				G_AddEvent(ent, EV_POWERUP_REGEN, 0);
			}
		} else if (g_gametype.integer != GT_ELIMINATION
			&& ent->health > client->ps.stats[STAT_MAX_HEALTH])
		{
			// count down health when over max
			ent->health--;
		}

		// count down armor when over max
		if (g_gametype.integer != GT_ELIMINATION
			&& client->ps.stats[STAT_ARMOR] > client->ps.stats[STAT_MAX_HEALTH])
		{
			client->ps.stats[STAT_ARMOR]--;
		}
	}
}

void ClientIntermissionThink(gclient_t *client)
{
	client->ps.eFlags &= ~EF_TALK;
	client->ps.eFlags &= ~EF_FIRING;

	// the level will exit when everyone wants to or after timeouts

	// swap and latch button actions
	client->oldbuttons = client->buttons;
	client->buttons = client->pers.cmd.buttons;
	if (client->buttons & (BUTTON_ATTACK | BUTTON_USE_HOLDABLE) & (client->oldbuttons ^ client->buttons)) {
		client->pers.ready = qtrue;
		ClientUserinfoChanged(client - level.clients);
	}
}

/**
Events will be passed on to the clients for presentation,
but any server game effects are handled here
*/
void ClientEvents(gentity_t *ent, int oldEventSequence)
{
	int		i, j;
	int		event;
	gclient_t *client;
	int		damage;
	vec3_t	origin, angles;
	gitem_t *item;
	gentity_t *drop;

	client = ent->client;

	if (oldEventSequence < client->ps.eventSequence - MAX_PS_EVENTS) {
		oldEventSequence = client->ps.eventSequence - MAX_PS_EVENTS;
	}
	for (i = oldEventSequence; i < client->ps.eventSequence; i++) {
		event = client->ps.events[ i & (MAX_PS_EVENTS-1) ];

		switch (event) {
		case EV_FALL_MEDIUM:
		case EV_FALL_FAR:
			if (ent->s.eType != ET_PLAYER) {
				break;		// not in the player model
			}
			if (g_dmflags.integer & DF_NO_FALLING) {
				break;
			}
			if (event == EV_FALL_FAR) {
				damage = 10;
			} else {
				damage = 5;
			}
			ent->pain_debounce_time = level.time + 200;	// no normal pain sound
			G_Damage(ent, NULL, NULL, NULL, NULL, damage, 0, MOD_FALLING);
			break;

		case EV_FIRE_WEAPON:
			FireWeapon(ent);
			break;

		case EV_USE_ITEM1:		// teleporter
			// drop flags in CTF
			item = NULL;
			j = 0;

			if (ent->client->ps.powerups[ PW_REDFLAG ]) {
				item = BG_FindItemForPowerup(PW_REDFLAG);
				j = PW_REDFLAG;
			} else if (ent->client->ps.powerups[ PW_BLUEFLAG ]) {
				item = BG_FindItemForPowerup(PW_BLUEFLAG);
				j = PW_BLUEFLAG;
			}

			if (item) {
				drop = Drop_Item(ent, item, 0);
				// decide how many seconds it has left
				drop->count = (ent->client->ps.powerups[ j ] - level.time) / 1000;
				if (drop->count < 1) {
					drop->count = 1;
				}

				ent->client->ps.powerups[ j ] = 0;
			}

			SelectSpawnPoint(ent->client->ps.origin, origin, angles, qfalse);
			TeleportPlayer(ent, origin, angles);
			break;

		case EV_USE_ITEM2:		// medkit
			ent->health = ent->client->ps.stats[STAT_MAX_HEALTH] + 25;

			break;
		default:
			break;
		}
	}
}

void BotTestSolid(vec3_t origin);

void SendPendingPredictableEvents(playerState_t *ps)
{
	gentity_t *t;
	int event, seq;
	int extEvent, number;

	// if there are still events pending
	if (ps->entityEventSequence < ps->eventSequence) {
		// create a temporary entity for this event which is sent to everyone
		// except the client who generated the event
		seq = ps->entityEventSequence & (MAX_PS_EVENTS-1);
		event = ps->events[ seq ] | ((ps->entityEventSequence & 3) << 8);
		// set external event to zero before calling BG_PlayerStateToEntityState
		extEvent = ps->externalEvent;
		ps->externalEvent = 0;
		// create temporary entity for event
		t = G_TempEntity(ps->origin, event);
		number = t->s.number;
		BG_PlayerStateToEntityState(ps, &t->s, qtrue);
		t->s.number = number;
		t->s.eType = ET_EVENTS + event;
		t->s.eFlags |= EF_PLAYER_EVENT;
		t->s.otherEntityNum = ps->clientNum;
		// send to everyone except the client who generated the event
		t->r.svFlags |= SVF_NOTSINGLECLIENT;
		t->r.singleClient = ps->clientNum;
		// set back external event
		ps->externalEvent = extEvent;
	}
}

static void ClientCalcPing(gclient_t *client)
{
	int	i, sum;

	// unlagged - backward reconciliation #4
	// frameOffset should be about the number of milliseconds into a frame
	// this command packet was received, depending on how fast the server
	// does a G_RunFrame()
	client->frameOffset = trap_Milliseconds() - level.frameStartTime;

	client->pers.pingSamples[client->pers.pingSampleHead] =
		level.previousTime + client->frameOffset - client->pers.cmd.serverTime;
	client->pers.pingSampleHead++;

	if (client->pers.pingSampleHead >= NUM_PING_SAMPLES) {
		client->pers.pingSampleHead -= NUM_PING_SAMPLES;
	}

	// get an average of the samples we saved up
	sum = 0;
	for (i = 0; i < NUM_PING_SAMPLES; i++) {
		sum += client->pers.pingSamples[i];
	}

	client->pers.ping = sum / NUM_PING_SAMPLES;
	if (client->pers.ping > 999) {
		client->pers.ping = 999;
	}
}

/**
This will be called once for each client frame, which will
usually be a couple times for each server frame on fast clients.
*/
void ClientThink_real(gentity_t *ent)
{
	gclient_t	*client;
	pmove_t		pm;
	int			oldEventSequence;
	int			msec;
	usercmd_t	*ucmd;

	client = ent->client;

	// don't think if the client is not yet connected (and thus not yet spawned in)
	if (client->pers.connected != CON_CONNECTED) {
		return;
	}
	// mark the time, so the connection sprite can be removed
	ucmd = &ent->client->pers.cmd;

	// sanity check the command time to prevent speedup cheating
	if (ucmd->serverTime > level.totalTime + 200) {
		ucmd->serverTime = level.totalTime + 200;
//		G_Printf("serverTime <<<<<\n");
	}
	if (ucmd->serverTime < level.totalTime - 1000) {
		ucmd->serverTime = level.totalTime - 1000;
//		G_Printf("serverTime >>>>>\n");
	}

	ClientCalcPing(client);

	// unlagged - backward reconciliation #4
	// save the command time *before* pmove_fixed messes with the serverTime,
	// and *after* lag simulation messes with it
	// attackTime will be used for backward reconciliation later (time shift)
	client->attackTime = ucmd->serverTime;

	// unlagged - smooth clients #1
	// keep track of this for later - we'll use this to decide whether or not
	// to send extrapolated positions for this client
	client->lastUpdateFrame = level.framenum;

	msec = ucmd->serverTime - client->ps.commandTime;
	// following others may result in bad times, but we still want
	// to check for follow toggles
	if (msec < 1 && client->sess.spectatorState != SPECTATOR_FOLLOW) {
		return;
	}
	if (msec > 200) {
		msec = 200;
	}

	if (pmove_msec.integer < 8) {
		trap_Cvar_Set("pmove_msec", "8");
	}
	else if (pmove_msec.integer > 33) {
		trap_Cvar_Set("pmove_msec", "33");
	}

	if (pmove_fixed.integer || client->pers.pmoveFixed) {
		ucmd->serverTime = ((ucmd->serverTime + pmove_msec.integer-1) / pmove_msec.integer) * pmove_msec.integer;
		//if (ucmd->serverTime - client->ps.commandTime <= 0)
		//	return;
	}

	//
	// check for exiting intermission
	//
	if (level.intermissiontime) {
		ClientIntermissionThink(client);
		return;
	}

	// spectators don't do much
	if (client->sess.spectatorState != SPECTATOR_NOT) {
		if (client->sess.spectatorState == SPECTATOR_SCOREBOARD) {
			return;
		}
		SpectatorThink(ent, ucmd);
		return;
	}

	// check for inactivity timer
	if (!ClientInactivityTimer(client)) {
		return;
	}

	// clear the rewards if time
	if (level.time > client->rewardTime) {
		client->ps.eFlags &= ~(EF_AWARD_IMPRESSIVE | EF_AWARD_EXCELLENT | EF_AWARD_GAUNTLET | EF_AWARD_ASSIST | EF_AWARD_DEFEND | EF_AWARD_CAP);
	}

	if (client->noclip) {
		client->ps.pm_type = PM_NOCLIP;
	} else if (client->ps.stats[STAT_HEALTH] <= 0) {
		client->ps.pm_type = PM_DEAD;
	} else {
		client->ps.pm_type = PM_NORMAL;
	}

	client->ps.gravity = g_gravity.value;

	// set speed
	client->ps.speed = g_speed.value;

	if (client->ps.powerups[PW_HASTE]) {
		client->ps.speed *= 1.3;
	}

	// Let go of the hook if we aren't firing
	if (client->ps.weapon == WP_GRAPPLING_HOOK &&
		client->hook && !(ucmd->buttons & BUTTON_ATTACK)) {
		Weapon_HookFree(client->hook);
	}

	// set up for pmove
	oldEventSequence = client->ps.eventSequence;

	memset(&pm, 0, sizeof(pm));

	// check for the hit-scan gauntlet, don't let the action
	// go through as an attack unless it actually hits something
	if (client->ps.weapon == WP_GAUNTLET && !(ucmd->buttons & BUTTON_TALK) &&
		(ucmd->buttons & BUTTON_ATTACK) && client->ps.weaponTime <= 0) {
		pm.gauntletHit = CheckGauntletAttack(ent);
	}

	if (ent->flags & FL_FORCE_GESTURE) {
		ent->flags &= ~FL_FORCE_GESTURE;
		ent->client->pers.cmd.buttons |= BUTTON_GESTURE;
	}

	pm.ps = &client->ps;
	pm.cmd = *ucmd;
	if (g_gametype.integer == GT_DEFRAG) {
		pm.tracemask = MASK_SOLID;
	} else if (pm.ps->pm_type == PM_DEAD) {
		pm.tracemask = MASK_PLAYERSOLID & ~CONTENTS_BODY;
	}
	else if (ent->r.svFlags & SVF_BOT) {
		pm.tracemask = MASK_PLAYERSOLID | CONTENTS_BOTCLIP;
	}
	else {
		pm.tracemask = MASK_PLAYERSOLID;
	}
	pm.trace = trap_Trace;
	pm.pointcontents = trap_PointContents;
	pm.debugLevel = g_debugMove.integer;
	pm.noFootsteps = (g_dmflags.integer & DF_NO_FOOTSTEPS) > 0;

	pm.pmove_fixed = pmove_fixed.integer | client->pers.pmoveFixed;
	pm.pmove_msec = pmove_msec.integer;

	VectorCopy(client->ps.origin, client->oldOrigin);

	if (level.pauseStart) {
		// We freeze the client while paused and run the pmove code
		// to set the command times correctly. If we just ignored usercmds,
		// the lagometer would not work anymore.
		pm.ps->pm_type |= PM_FREEZE;
	}

	Pmove(&pm);

	// save results of pmove
	if (ent->client->ps.eventSequence != oldEventSequence) {
		ent->eventTime = level.time;
	}

	// unlagged - smooth clients #2
	// Clients no longer do extrapolation, because skip correction is all handled server-side now.
	// Since that's the case, it makes no sense to store the extra info using
	// BG_PlayerStateToEntityStateExtraPolate in the client's snapshot entity,
	// so let's save a little bandwidth.

	BG_PlayerStateToEntityState(&ent->client->ps, &ent->s, qtrue);

	SendPendingPredictableEvents(&ent->client->ps);

	if (!(ent->client->ps.eFlags & EF_FIRING)) {
		client->fireHeld = qfalse;		// for grapple
	}

	// use the snapped origin for linking so it matches client predicted versions
	VectorCopy(ent->s.pos.trBase, ent->r.currentOrigin);

	VectorCopy(pm.mins, ent->r.mins);
	VectorCopy(pm.maxs, ent->r.maxs);

	ent->waterlevel = pm.waterlevel;
	ent->watertype = pm.watertype;

	// execute client events
	ClientEvents(ent, oldEventSequence);

	// link entity now, after any personal teleporters have been used
	trap_LinkEntity (ent);
	if (!ent->client->noclip) {
		G_TouchTriggers(ent);
	}

	// NOTE: now copy the exact origin over otherwise clients can be snapped into solid
	VectorCopy(ent->client->ps.origin, ent->r.currentOrigin);

	//test for solid areas in the AAS file
	BotTestAAS(ent->r.currentOrigin);

	// touch other objects
	ClientImpacts(ent, &pm);

	// save results of triggers and client events
	if (ent->client->ps.eventSequence != oldEventSequence) {
		ent->eventTime = level.time;
	}

	// swap and latch button actions
	client->oldbuttons = client->buttons;
	client->buttons = ucmd->buttons;
	client->latched_buttons |= client->buttons & ~client->oldbuttons;

	// check for respawning
	if (client->ps.stats[STAT_HEALTH] <= 0) {
		// wait for the attack button to be pressed
		if (level.time > client->respawnTime) {
			// respawn immediately in elimination
			if (g_gametype.integer == GT_ELIMINATION && !level.warmupTime
				&& ent->client->sess.spectatorState == SPECTATOR_NOT)
			{
				ClientRespawn(ent);
				return;
			}

			// forcerespawn is to prevent users from waiting out powerups
			if (level.time - client->respawnTime > 5000) {
				ClientRespawn(ent);
				return;
			}

			// pressing attack or use is the normal respawn method
			if (ucmd->buttons & (BUTTON_ATTACK | BUTTON_USE_HOLDABLE)) {
				ClientRespawn(ent);
			}
		}
		return;
	}

	if (client->ps.groundEntityNum == ENTITYNUM_NONE && client->lastGroundTime == 0) {
		client->lastGroundTime = level.time;
	} else if (client->ps.groundEntityNum != ENTITYNUM_NONE) {
		client->lastGroundTime = 0;
		client->lastSentFlying = -1;
	}

	// perform once-a-second actions
	ClientTimerActions(ent, msec);
}

/**
A new command has arrived from the client
*/
void ClientThink(int clientNum)
{
	gentity_t *ent;

	ent = g_entities + clientNum;
	trap_GetUsercmd(clientNum, &ent->client->pers.cmd);

	if (!(ent->r.svFlags & SVF_BOT)) {
		ClientThink_real(ent);
	}
}

void G_RunClient(gentity_t *ent)
{
	if (!(ent->r.svFlags & SVF_BOT)) {
		return;
	}
	ent->client->pers.cmd.serverTime = level.totalTime;
	ClientThink_real(ent);
}

void SpectatorClientEndFrame(gentity_t *ent)
{
	gclient_t	*cl;

	// if we are doing a chase cam or a remote view, grab the latest info
	if (ent->client->sess.spectatorState == SPECTATOR_FOLLOW) {
		int		clientNum;

		clientNum = ent->client->sess.spectatorClient;

		// team follow1 and team follow2 go to whatever clients are playing
		if (clientNum == -1) {
			clientNum = level.follow1;
		} else if (clientNum == -2) {
			clientNum = level.follow2;
		}
		if (clientNum >= 0) {
			cl = &level.clients[ clientNum ];
			if (cl->pers.connected == CON_CONNECTED && cl->sess.sessionTeam != TEAM_SPECTATOR) {
				ent->client->ps = cl->ps;
				ent->client->ps.pm_flags |= PMF_FOLLOW;
				return;
			} else {
				// drop them to free spectators unless they are dedicated camera followers
				if (ent->client->sess.spectatorClient >= 0) {
					ent->client->sess.spectatorState = SPECTATOR_FREE;
					ClientBegin(ent->client - level.clients);
				}
			}
		}
	}

	if (ent->client->sess.spectatorState == SPECTATOR_SCOREBOARD) {
		ent->client->ps.pm_flags |= PMF_SCOREBOARD;
	} else {
		ent->client->ps.pm_flags &= ~PMF_SCOREBOARD;
	}
}

/**
Called at the end of each server frame for each connected client
A fast client will have multiple ClientThink for each ClientEdFrame,
while a slow client may have multiple ClientEndFrame between ClientThink.
*/
void ClientEndFrame(gentity_t *ent)
{
	int			i;
	int			frames;

	if (ent->client->sess.spectatorState != SPECTATOR_NOT) {
		SpectatorClientEndFrame(ent);
		return;
	}

	// turn off any expired powerups
	for (i = 0; i < MAX_POWERUPS; i++) {
		if (ent->client->ps.powerups[ i ] < level.time) {
			ent->client->ps.powerups[ i ] = 0;
		}
	}

	ent->client->ps.persistant[PERS_SCORE] = ent->client->pers.score;

	// If the end of unit layout is displayed, don't give
	// the player any normal movement attributes
	if (level.intermissiontime) {
		return;
	}

	// burn from lava, etc
	P_WorldEffects(ent);

	// apply all the damage taken this frame
	P_DamageFeedback(ent);

	ent->client->ps.stats[STAT_HEALTH] = ent->health;	// FIXME: get rid of ent->health...

	G_SetClientSound(ent);

	// unlagged - smooth clients #1
	// always use BG_PlayerStateToEntityState instead of BG_PlayerStateToEntityStateExtraPolate
	BG_PlayerStateToEntityState(&ent->client->ps, &ent->s, qtrue);
	SendPendingPredictableEvents(&ent->client->ps);

	// unlagged - smooth clients #1
	// mark as not missing updates initially
	ent->client->ps.eFlags &= ~EF_CONNECTION;

	frames = level.framenum - ent->client->lastUpdateFrame - 1;

	// don't extrapolate more than two frames
	if (frames > 2) {
		frames = 2;

		// if they missed more than two in a row, show the phone jack
		ent->client->ps.eFlags |= EF_CONNECTION;
		ent->s.eFlags |= EF_CONNECTION;
	}

	// did the client miss any frames?
	if (frames > 0) {
		// yep, missed one or more, so extrapolate the player's movement
		G_PredictPlayerMove(ent, (float) frames / sv_fps.integer);
		// save network bandwidth
		SnapVector(ent->s.pos.trBase);
	}

	// unlagged - backward reconciliation #1
	// store the client's position for backward reconciliation later
	G_StoreHistory(ent);
}


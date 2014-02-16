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
// cg_players.c -- handle the media and animation for player entities

#include "cg_local.h"

#define SHADOW_DISTANCE		128
#define DEFAULT_MODEL		"sarge"
#define DEFAULT_MODEL_SKIN	"pm"

char	*cg_customSoundNames[MAX_CUSTOM_SOUNDS] = {
	"*death1.wav",
	"*death2.wav",
	"*death3.wav",
	"*jump1.wav",
	"*pain25_1.wav",
	"*pain50_1.wav",
	"*pain75_1.wav",
	"*pain100_1.wav",
	"*falling1.wav",
	"*gasp.wav",
	"*drown.wav",
	"*fall1.wav",
	"*taunt.wav"
};

sfxHandle_t CG_CustomSound(int clientNum, const char *soundName)
{
	clientInfo_t *ci;
	int			i;

	if (soundName[0] != '*') {
		return trap_S_RegisterSound(soundName, qfalse);
	}

	if (clientNum < 0 || clientNum >= MAX_CLIENTS) {
		clientNum = 0;
	}
	ci = &cgs.clientinfo[clientNum];

	for (i = 0; i < MAX_CUSTOM_SOUNDS && cg_customSoundNames[i]; i++) {
		if (!strcmp(soundName, cg_customSoundNames[i])) {
			return ci->model->sounds[i];
		}
	}

	CG_Error("Unknown custom sound: %s", soundName);
	return 0;
}

/**
Read a configuration file containing animation counts and rates
models/players/visor/animation.cfg, etc
*/
static qboolean CG_ParseAnimationFile(const char *filename, model_t *model)
{
	char		*text_p, *prev;
	int			len;
	int			i;
	char		*token;
	float		fps;
	int			skip;
	char		text[20000];
	fileHandle_t	f;

	// load the file
	len = trap_FS_FOpenFile(filename, &f, FS_READ);
	if (len <= 0) {
		return qfalse;
	}
	if (len >= sizeof(text) - 1) {
		CG_Printf("File %s too long\n", filename);
		trap_FS_FCloseFile(f);
		return qfalse;
	}
	trap_FS_Read(text, len, f);
	text[len] = 0;
	trap_FS_FCloseFile(f);

	// parse the text
	text_p = text;
	skip = 0;	// quite the compiler warning

	model->footsteps = FOOTSTEP_NORMAL;
	VectorClear(model->headOffset);
	model->gender = GENDER_MALE;
	model->fixedlegs = qfalse;
	model->fixedtorso = qfalse;

	// read optional parameters
	while (1) {
		prev = text_p;	// so we can unget
		token = COM_Parse(&text_p);
		if (!token) {
			break;
		}
		if (!Q_stricmp(token, "footsteps")) {
			token = COM_Parse(&text_p);
			if (!token) {
				break;
			}
			if (!Q_stricmp(token, "default") || !Q_stricmp(token, "normal")) {
				model->footsteps = FOOTSTEP_NORMAL;
			} else if (!Q_stricmp(token, "boot")) {
				model->footsteps = FOOTSTEP_BOOT;
			} else if (!Q_stricmp(token, "flesh")) {
				model->footsteps = FOOTSTEP_FLESH;
			} else if (!Q_stricmp(token, "mech")) {
				model->footsteps = FOOTSTEP_MECH;
			} else if (!Q_stricmp(token, "energy")) {
				model->footsteps = FOOTSTEP_ENERGY;
			} else {
				CG_Printf("Bad footsteps parm in %s: %s\n", filename, token);
			}
			continue;
		} else if (!Q_stricmp(token, "headoffset")) {
			for (i = 0; i < 3; i++) {
				token = COM_Parse(&text_p);
				if (!token) {
					break;
				}
				model->headOffset[i] = atof(token);
			}
			continue;
		} else if (!Q_stricmp(token, "sex")) {
			token = COM_Parse(&text_p);
			if (!token) {
				break;
			}
			if (token[0] == 'f' || token[0] == 'F') {
				model->gender = GENDER_FEMALE;
			} else if (token[0] == 'n' || token[0] == 'N') {
				model->gender = GENDER_NEUTER;
			} else {
				model->gender = GENDER_MALE;
			}
			continue;
		} else if (!Q_stricmp(token, "fixedlegs")) {
			model->fixedlegs = qtrue;
			continue;
		} else if (!Q_stricmp(token, "fixedtorso")) {
			model->fixedtorso = qtrue;
			continue;
		}

		// if it is a number, start parsing animations
		if (token[0] >= '0' && token[0] <= '9') {
			text_p = prev;	// unget the token
			break;
		}
		Com_Printf("unknown token '%s' in %s\n", token, filename);
	}

	// read information for each frame
	for (i = 0; i < MAX_ANIMATIONS; i++) {

		token = COM_Parse(&text_p);
		if (!*token) {
			if (i >= TORSO_GETFLAG && i <= TORSO_NEGATIVE) {
				model->animations[i].firstFrame = model->animations[TORSO_GESTURE].firstFrame;
				model->animations[i].frameLerp = model->animations[TORSO_GESTURE].frameLerp;
				model->animations[i].initialLerp = model->animations[TORSO_GESTURE].initialLerp;
				model->animations[i].loopFrames = model->animations[TORSO_GESTURE].loopFrames;
				model->animations[i].numFrames = model->animations[TORSO_GESTURE].numFrames;
				model->animations[i].reversed = qfalse;
				model->animations[i].flipflop = qfalse;
				continue;
			}
			break;
		}
		model->animations[i].firstFrame = atoi(token);
		// leg only frames are adjusted to not count the upper body only frames
		if (i == LEGS_WALKCR) {
			skip = model->animations[LEGS_WALKCR].firstFrame - model->animations[TORSO_GESTURE].firstFrame;
		}
		if (i >= LEGS_WALKCR && i<TORSO_GETFLAG) {
			model->animations[i].firstFrame -= skip;
		}

		token = COM_Parse(&text_p);
		if (!*token) {
			break;
		}
		model->animations[i].numFrames = atoi(token);

		model->animations[i].reversed = qfalse;
		model->animations[i].flipflop = qfalse;
		// if numFrames is negative the animation is reversed
		if (model->animations[i].numFrames < 0) {
			model->animations[i].numFrames = -model->animations[i].numFrames;
			model->animations[i].reversed = qtrue;
		}

		token = COM_Parse(&text_p);
		if (!*token) {
			break;
		}
		model->animations[i].loopFrames = atoi(token);

		token = COM_Parse(&text_p);
		if (!*token) {
			break;
		}
		fps = atof(token);
		if (fps == 0) {
			fps = 1;
		}
		model->animations[i].frameLerp = 1000 / fps;
		model->animations[i].initialLerp = 1000 / fps;
	}

	if (i != MAX_ANIMATIONS) {
		CG_Printf("Error parsing animation file: %s\n", filename);
		return qfalse;
	}

	// crouch backward animation
	memcpy(&model->animations[LEGS_BACKCR], &model->animations[LEGS_WALKCR], sizeof(animation_t));
	model->animations[LEGS_BACKCR].reversed = qtrue;
	// walk backward animation
	memcpy(&model->animations[LEGS_BACKWALK], &model->animations[LEGS_WALK], sizeof(animation_t));
	model->animations[LEGS_BACKWALK].reversed = qtrue;
	// flag moving fast
	model->animations[FLAG_RUN].firstFrame = 0;
	model->animations[FLAG_RUN].numFrames = 16;
	model->animations[FLAG_RUN].loopFrames = 16;
	model->animations[FLAG_RUN].frameLerp = 1000 / 15;
	model->animations[FLAG_RUN].initialLerp = 1000 / 15;
	model->animations[FLAG_RUN].reversed = qfalse;
	// flag not moving or moving slowly
	model->animations[FLAG_STAND].firstFrame = 16;
	model->animations[FLAG_STAND].numFrames = 5;
	model->animations[FLAG_STAND].loopFrames = 0;
	model->animations[FLAG_STAND].frameLerp = 1000 / 20;
	model->animations[FLAG_STAND].initialLerp = 1000 / 20;
	model->animations[FLAG_STAND].reversed = qfalse;
	// flag speeding up
	model->animations[FLAG_STAND2RUN].firstFrame = 16;
	model->animations[FLAG_STAND2RUN].numFrames = 5;
	model->animations[FLAG_STAND2RUN].loopFrames = 1;
	model->animations[FLAG_STAND2RUN].frameLerp = 1000 / 15;
	model->animations[FLAG_STAND2RUN].initialLerp = 1000 / 15;
	model->animations[FLAG_STAND2RUN].reversed = qtrue;
	//
	// new anims changes
	//
// TODO remove flipflop and defines
//	model->animations[TORSO_GETFLAG].flipflop = qtrue;
//	model->animations[TORSO_GUARDBASE].flipflop = qtrue;
//	model->animations[TORSO_PATROL].flipflop = qtrue;
//	model->animations[TORSO_AFFIRMATIVE].flipflop = qtrue;
//	model->animations[TORSO_NEGATIVE].flipflop = qtrue;
	//
	return qtrue;
}

void CG_SetModelColor(clientInfo_t *ci, refEntity_t *head, refEntity_t *torso,
	refEntity_t *legs, int state)
{
	if ((state & EF_DEAD) && cg_deadBodyDarken.integer) {
		CG_SetRGBA(head->shaderRGBA, cgs.media.deadBodyColor);
		CG_SetRGBA(torso->shaderRGBA, cgs.media.deadBodyColor);
		CG_SetRGBA(legs->shaderRGBA, cgs.media.deadBodyColor);
		return;
	}

	CG_SetRGBA(head->shaderRGBA, ci->model->headColor);
	CG_SetRGBA(torso->shaderRGBA, ci->model->torsoColor);
	CG_SetRGBA(legs->shaderRGBA, ci->model->legsColor);
}

static void CG_ParseModelString(char *str, char **model, char **skin)
{
	*model = str;
	if ((*skin = strchr(str, '/')) == NULL) {
		*skin = "default";
	} else {
		*skin[0] = '\0';
		++(*skin);
	}
}

static int CG_RegisterSkin(model_t *model, const char *modelName, const char *skinName)
{
	char	filename[MAX_QPATH];

	Com_sprintf(filename, sizeof filename, "models/players/%s/lower_%s.skin", modelName, skinName);
	model->legsSkin = trap_R_RegisterSkin(filename);
	if (!model->legsSkin) {
		return 1;
	}

	Com_sprintf(filename, sizeof filename, "models/players/%s/upper_%s.skin", modelName, skinName);
	model->torsoSkin = trap_R_RegisterSkin(filename);
	if (!model->torsoSkin) {
		return 1;
	}

	Com_sprintf(filename, sizeof filename, "models/players/%s/head_%s.skin", modelName, skinName);
	model->headSkin = trap_R_RegisterSkin(filename);
	if (!model->headSkin) {
		return 1;
	}

	Com_sprintf(filename, sizeof filename, "models/players/%s/icon_%s.tga", modelName, skinName);
	model->modelIcon = trap_R_RegisterShaderNoMip(filename);
	if (!model->modelIcon) {
		return 1;
	}

	return 0;
}

static int CG_RegisterClientModelname(model_t *model, char *modelName)
{
	char	filename[MAX_QPATH];

	Com_sprintf(filename, sizeof filename, "models/players/%s/lower.md3", modelName);
	model->legsModel = trap_R_RegisterModel(filename);
	if (!model->legsModel) {
		return 1;
	}

	Com_sprintf(filename, sizeof filename, "models/players/%s/upper.md3", modelName);
	model->torsoModel = trap_R_RegisterModel(filename);
	if (!model->torsoModel) {
		return 1;
	}

	Com_sprintf(filename, sizeof filename, "models/players/%s/head.md3", modelName);
	model->headModel = trap_R_RegisterModel(filename);
	if (!model->headModel) {
		return 1;
	}

	// load the animations
	Com_sprintf(filename, sizeof filename, "models/players/%s/animation.cfg", modelName);
	if (!CG_ParseAnimationFile(filename, model)) {
		Com_Printf("Failed to load animation file %s\n", filename);
		return 1;
	}

	model->modelName = modelName;
	return 0;
}

/**
Load model and skin from files
If return value is not zero, default model and skin are used.
*/
static int CG_RegisterModel(model_t *model, char *modelString)
{
	char		*modelName, *skin;
	qboolean	useDefault;

	useDefault = qfalse;
	CG_ParseModelString(modelString, &modelName, &skin);

	if (CG_RegisterClientModelname(model, modelName)) {
		Com_Printf("WARNING: Failed to load model %s - using default.\n", modelName);
		modelName = DEFAULT_MODEL;
		if (CG_RegisterClientModelname(model, DEFAULT_MODEL)) {
			Com_Error(ERR_DROP, "Failed to load default model.");
		}

		useDefault = qtrue;
	}

	if (!useDefault && CG_RegisterSkin(model, modelName, skin)) {
		Com_Printf("WARNING: Failed to load skin %s - using default.\n", skin);
		useDefault = qtrue;
	}

	if (useDefault && CG_RegisterSkin(model, modelName, DEFAULT_MODEL_SKIN)) {
		Com_Error(ERR_DROP, "Failed to load default skin.");
	}

	return useDefault;
}

/**
Load player sounds from files.
If return value is non-zero, sounds from default model are used.
*/
static int CG_RegisterSoundModel(model_t *model, char *modelString, const char *soundModelString)
{
	int			i;
	qboolean	oneSoundFound;
	char		*modelName, *skin, *filename, *soundName;

	oneSoundFound = qfalse;

	if (soundModelString[0] == '\0') {
		CG_ParseModelString(modelString, &modelName, &skin);
		soundModelString = modelName;
	}

	for (i = 0; i < MAX_CUSTOM_SOUNDS; i++) {
		soundName = cg_customSoundNames[i];
		if (!soundName) {
			break;
		}
		
		filename = va("sound/player/%s/%s", soundModelString, soundName + 1);
		if (trap_FS_FOpenFile(filename, NULL, FS_READ)) {
			model->sounds[i] = trap_S_RegisterSound(va("sound/player/%s/%s",
				soundModelString, soundName + 1), qfalse);
			oneSoundFound = qtrue;
		}

		if (!model->sounds[i]) {
			model->sounds[i] = trap_S_RegisterSound(va("sound/player/%s/%s",
				DEFAULT_MODEL, soundName + 1), qfalse);
		}
	}

	if (!oneSoundFound) {
		Com_Printf("WARNING: Sound model %s not found - using default.\n", soundModelString);
		return 1;
	}

	return 0;
}

/**
If no snapshot has been reveived and player is in team spectator,
assume that he does not follow (spectate) another player.
*/
static void CG_SetPlayerModel(int clientNum)
{
	clientInfo_t	*ci, *curci, *localci;
	qboolean		follow;
	int				i;

	ci = &cgs.clientinfo[clientNum];
	localci = &cgs.clientinfo[cg.clientNum];
	curci = &cgs.clientinfo[cg.snap ? cg.snap->ps.clientNum : cg.clientNum];
	follow = (cg.snap ? cg.snap->ps.pm_flags & PMF_FOLLOW : qfalse);

	if (cgs.gametype >= GT_TEAM) {
		if (cg_forceTeamModels.integer == 1
			|| (localci->team == TEAM_SPECTATOR && !follow)
			|| (localci->team == TEAM_SPECTATOR && follow && cg_forceTeamModels.integer == 2))
		{
			ci->model = (ci->team == TEAM_RED ? &cgs.media.redTeamModel : &cgs.media.blueTeamModel);
		} else if (ci->team == curci->team) {
			ci->model = &cgs.media.teamModel;
		} else {
			ci->model = &cgs.media.enemyModel;
		}
	} else {
		ci->model = (ci == curci ? &cgs.media.teamModel : &cgs.media.enemyModel);
	}

	// reset any existing players and bodies, because they might be in bad
	// frames for this new model
	for (i = 0; i < MAX_GENTITIES; i++) {
		if (cg_entities[i].currentState.clientNum == clientNum
			&& cg_entities[i].currentState.eType == ET_PLAYER) {
			CG_ResetPlayerEntity(&cg_entities[i]);
		}
	}
}

static void CG_PrintTeamChange(clientInfo_t *cl)
{
	const char	*str;

	switch (cl->team) {
	case TEAM_FREE:
		str = va("%s joined the battle", cl->name);
		break;
	case TEAM_RED:
		str = va("%s joined the red team", cl->name);
		break;
	case TEAM_BLUE:
		str = va("%s joined the blue team", cl->name);
		break;
	case TEAM_SPECTATOR:
		str = va("%s joined the spectators", cl->name);
		break;
	default:
		return;
	}

	CG_CenterPrint(str);
}

/**
If cvar is a variable that specifies a model, load that model and
return zero. Otherwise, return value is non-zero.
*/
int CG_LoadCvarModel(const char *cvarName, vmCvar_t *cvar)
{
	model_t		*model;
	vmCvar_t	*soundCvar;

	if (cgs.gametype < GT_TEAM && (cvar == &cg_redTeamModel || cvar == &cg_blueTeamModel
		|| cvar == &cg_redTeamSoundModel || cvar == &cg_blueTeamSoundModel))
	{
		return 0;
	}

	if (cvar == &cg_teamModel || cvar == &cg_teamSoundModel) {
		model = &cgs.media.teamModel;
		soundCvar = &cg_teamSoundModel;
	} else if (cvar == &cg_enemyModel || cvar == &cg_enemySoundModel) {
		model = &cgs.media.enemyModel;
		soundCvar = &cg_enemySoundModel;
	} else if (cvar == &cg_redTeamModel || cvar == &cg_redTeamSoundModel) {
		model = &cgs.media.redTeamModel;
		soundCvar = &cg_redTeamSoundModel;
	} else if (cvar == &cg_blueTeamModel || cvar == &cg_blueTeamSoundModel) {
		model = &cgs.media.blueTeamModel;
		soundCvar = &cg_blueTeamSoundModel;
	} else {
		return 1;
	}

	if (cvar != soundCvar) {
		if (CG_RegisterModel(model, cvar->string)) {
			trap_Cvar_Set(cvarName, DEFAULT_MODEL);
			trap_Cvar_Update(cvar);
		}

		if (soundCvar->string[0] != '\0') {
			return 0;
		}
	}

	if (CG_RegisterSoundModel(model, model->modelName, cvar->string)) {
		trap_Cvar_Set(cvarName, DEFAULT_MODEL);
		trap_Cvar_Update(cvar);
	}

	return 0;
}

void CG_ForceModelChange(void)
{
	int	i;
	for (i = 0; i < MAX_CLIENTS; ++i) {
		if (cgs.clientinfo[i].infoValid) {
			CG_SetPlayerModel(i);
		}
	}
}

/**
Parse the player info received from server and set the client info.
If clientNum is the local client number and he switches to/from
spectator, adjust models of all players if needed.
*/
void CG_NewClientInfo(int clientNum)
{
	clientInfo_t	*ci;
	const char		*configstring;
	const char		*v;
	int				oldTeam;
	qboolean		oldReady, oldValid;

	ci = &cgs.clientinfo[clientNum];
	oldTeam = ci->team;
	oldReady = ci->ready;
	oldValid = ci->infoValid;

	configstring = CG_ConfigString(clientNum + CS_PLAYERS);
	if (!configstring[0]) {
		memset(ci, 0, sizeof(*ci));
		ci->ping = -1;
		return;		// player just left
	}

	ci->infoValid = qtrue;

	// isolate the player's name
	v = Info_ValueForKey(configstring, "n");
	if (oldValid && strcmp(v, ci->name)) {
		CG_Printf("%s" S_COLOR_WHITE " renamed to %s\n", ci->name, v);
	}
	Q_strncpyz(ci->name, v, sizeof ci->name);

	v = Info_ValueForKey(configstring, "skill");
	ci->botSkill = atoi(v);

	v = Info_ValueForKey(configstring, "hc");
	ci->handicap = atoi(v);

	v = Info_ValueForKey(configstring, "w");
	ci->wins = atoi(v);

	v = Info_ValueForKey(configstring, "l");
	ci->losses = atoi(v);

	v = Info_ValueForKey(configstring, "tt");
	ci->teamTask = atoi(v);

	v = Info_ValueForKey(configstring, "tl");
	ci->teamLeader = atoi(v);

	v = Info_ValueForKey(configstring, "r");
	ci->ready = !!atoi(v);
	if (cgs.startWhenReady && cg.warmup < 0 && oldValid && ci->ready != oldReady) {
		if (ci->ready) {
			CG_CenterPrint(va("%s ^2is ready", ci->name));
		} else {
			CG_CenterPrint(va("%s ^1is not ready", ci->name));
		}
	}

	v = Info_ValueForKey(configstring, "t");
	ci->team = atoi(v);

	if ((!oldValid && ci->team != TEAM_SPECTATOR) || (oldValid && oldTeam != ci->team)) {
		CG_PrintTeamChange(ci);
	}

	if (oldValid && clientNum == cg.clientNum && oldTeam != ci->team
		&& (ci->team == TEAM_SPECTATOR || oldTeam == TEAM_SPECTATOR))
	{
		CG_ForceModelChange();
	} else {
		CG_SetPlayerModel(clientNum);
	}

	if (!oldValid) {
		ci->enterTime = cg.time;
	}
}

// PLAYER ANIMATION

/**
May include ANIM_TOGGLEBIT
*/
static void CG_SetLerpFrameAnimation(clientInfo_t *ci, lerpFrame_t *lf, int newAnimation)
{
	animation_t	*anim;

	lf->animationNumber = newAnimation;
	newAnimation &= ~ANIM_TOGGLEBIT;

	if (newAnimation < 0 || newAnimation >= MAX_TOTALANIMATIONS) {
		CG_Error("Bad animation number: %i", newAnimation);
	}

	anim = &ci->model->animations[ newAnimation ];

	lf->animation = anim;
	lf->animationTime = lf->frameTime + anim->initialLerp;

	if (cg_debugAnim.integer) {
		CG_Printf("Anim: %i\n", newAnimation);
	}
}

/**
Sets cg.snap, cg.oldFrame, and cg.backlerp
cg.time should be between oldFrameTime and frameTime after exit
*/
static void CG_RunLerpFrame(clientInfo_t *ci, lerpFrame_t *lf, int newAnimation, float speedScale)
{
	int			f, numFrames;
	animation_t	*anim;

	// debugging tool to get no animations
	if (cg_animSpeed.integer == 0) {
		lf->oldFrame = lf->frame = lf->backlerp = 0;
		return;
	}

	// see if the animation sequence is switching
	if (newAnimation != lf->animationNumber || !lf->animation) {
		CG_SetLerpFrameAnimation(ci, lf, newAnimation);
	}

	// if we have passed the current frame, move it to
	// oldFrame and calculate a new frame
	if (cg.time >= lf->frameTime) {
		lf->oldFrame = lf->frame;
		lf->oldFrameTime = lf->frameTime;

		// get the next frame based on the animation
		anim = lf->animation;
		if (!anim->frameLerp) {
			return;		// shouldn't happen
		}
		if (cg.time < lf->animationTime) {
			lf->frameTime = lf->animationTime;		// initial lerp
		} else {
			lf->frameTime = lf->oldFrameTime + anim->frameLerp;
		}
		f = (lf->frameTime - lf->animationTime) / anim->frameLerp;
		f *= speedScale;		// adjust for haste, etc

		numFrames = anim->numFrames;
		if (anim->flipflop) {
			numFrames *= 2;
		}
		if (f >= numFrames) {
			f -= numFrames;
			if (anim->loopFrames) {
				f %= anim->loopFrames;
				f += anim->numFrames - anim->loopFrames;
			} else {
				f = numFrames - 1;
				// the animation is stuck at the end, so it
				// can immediately transition to another sequence
				lf->frameTime = cg.time;
			}
		}
		if (anim->reversed) {
			lf->frame = anim->firstFrame + anim->numFrames - 1 - f;
		}
		else if (anim->flipflop && f>=anim->numFrames) {
			lf->frame = anim->firstFrame + anim->numFrames - 1 - (f%anim->numFrames);
		}
		else {
			lf->frame = anim->firstFrame + f;
		}
		if (cg.time > lf->frameTime) {
			lf->frameTime = cg.time;
			if (cg_debugAnim.integer) {
				CG_Printf("Clamp lf->frameTime\n");
			}
		}
	}

	if (lf->frameTime > cg.time + 200) {
		lf->frameTime = cg.time;
	}

	if (lf->oldFrameTime > cg.time) {
		lf->oldFrameTime = cg.time;
	}
	// calculate current lerp value
	if (lf->frameTime == lf->oldFrameTime) {
		lf->backlerp = 0;
	} else {
		lf->backlerp = 1.0 - (float)(cg.time - lf->oldFrameTime) / (lf->frameTime - lf->oldFrameTime);
	}
}

static void CG_ClearLerpFrame(clientInfo_t *ci, lerpFrame_t *lf, int animationNumber)
{
	lf->frameTime = lf->oldFrameTime = cg.time;
	CG_SetLerpFrameAnimation(ci, lf, animationNumber);
	lf->oldFrame = lf->frame = lf->animation->firstFrame;
}

static void CG_PlayerAnimation(centity_t *cent, int *legsOld, int *legs, float *legsBackLerp, int *torsoOld, int *torso, float *torsoBackLerp)
{
	clientInfo_t	*ci;
	int				clientNum;
	float			speedScale;

	clientNum = cent->currentState.clientNum;

	if (cg_noPlayerAnims.integer) {
		*legsOld = *legs = *torsoOld = *torso = 0;
		return;
	}

	if (cent->currentState.powerups & (1 << PW_HASTE)) {
		speedScale = 1.5;
	} else {
		speedScale = 1;
	}

	ci = &cgs.clientinfo[ clientNum ];

	// do the shuffle turn frames locally
	if (cent->pe.legs.yawing && (cent->currentState.legsAnim & ~ANIM_TOGGLEBIT) == LEGS_IDLE) {
		CG_RunLerpFrame(ci, &cent->pe.legs, LEGS_TURN, speedScale);
	} else {
		CG_RunLerpFrame(ci, &cent->pe.legs, cent->currentState.legsAnim, speedScale);
	}

	*legsOld = cent->pe.legs.oldFrame;
	*legs = cent->pe.legs.frame;
	*legsBackLerp = cent->pe.legs.backlerp;

	CG_RunLerpFrame(ci, &cent->pe.torso, cent->currentState.torsoAnim, speedScale);

	*torsoOld = cent->pe.torso.oldFrame;
	*torso = cent->pe.torso.frame;
	*torsoBackLerp = cent->pe.torso.backlerp;
}

// PLAYER ANGLES

static void CG_SwingAngles(float destination, float swingTolerance, float clampTolerance, float speed, float *angle, qboolean *swinging)
{
	float	swing;
	float	move;
	float	scale;

	if (!*swinging) {
		// see if a swing should be started
		swing = AngleSubtract(*angle, destination);
		if (swing > swingTolerance || swing < -swingTolerance) {
			*swinging = qtrue;
		}
	}

	if (!*swinging) {
		return;
	}
	
	// modify the speed depending on the delta
	// so it doesn't seem so linear
	swing = AngleSubtract(destination, *angle);
	scale = fabs(swing);
	if (scale < swingTolerance * 0.5) {
		scale = 0.5;
	} else if (scale < swingTolerance) {
		scale = 1.0;
	} else {
		scale = 2.0;
	}

	// swing towards the destination angle
	if (swing >= 0) {
		move = cg.frametime * scale * speed;
		if (move >= swing) {
			move = swing;
			*swinging = qfalse;
		}
		*angle = AngleMod(*angle + move);
	} else if (swing < 0) {
		move = cg.frametime * scale * -speed;
		if (move <= swing) {
			move = swing;
			*swinging = qfalse;
		}
		*angle = AngleMod(*angle + move);
	}

	// clamp to no more than tolerance
	swing = AngleSubtract(destination, *angle);
	if (swing > clampTolerance) {
		*angle = AngleMod(destination - (clampTolerance - 1));
	} else if (swing < -clampTolerance) {
		*angle = AngleMod(destination + (clampTolerance - 1));
	}
}

static void CG_AddPainTwitch(centity_t *cent, vec3_t torsoAngles)
{
	int		t;
	float	f;

	t = cg.time - cent->pe.painTime;
	if (t >= PAIN_TWITCH_TIME) {
		return;
	}

	f = 1.0 - (float)t / PAIN_TWITCH_TIME;

	if (cent->pe.painDirection) {
		torsoAngles[ROLL] += 20 * f;
	} else {
		torsoAngles[ROLL] -= 20 * f;
	}
}

/**
Handles seperate torso motion
  Legs pivot is based on direction of movement.
  Head always looks exactly at cent->lerpAngles.
  If motion < 20 degrees, show in head only.
  If < 45 degrees, also show in torso.
*/
static void CG_PlayerAngles(centity_t *cent, vec3_t legs[3], vec3_t torso[3], vec3_t head[3])
{
	vec3_t		legsAngles, torsoAngles, headAngles;
	float		dest;
	static	int	movementOffsets[8] = { 0, 22, 45, -22, 0, 22, -45, -22 };
	vec3_t		velocity;
	float		speed;
	int			dir, clientNum;
	clientInfo_t	*ci;

	VectorCopy(cent->lerpAngles, headAngles);
	headAngles[YAW] = AngleMod(headAngles[YAW]);
	VectorClear(legsAngles);
	VectorClear(torsoAngles);

	// --------- yaw -------------

	// allow yaw to drift a bit
	if ((cent->currentState.legsAnim & ~ANIM_TOGGLEBIT) != LEGS_IDLE 
		|| ((cent->currentState.torsoAnim & ~ANIM_TOGGLEBIT) != TORSO_STAND 
		&& (cent->currentState.torsoAnim & ~ANIM_TOGGLEBIT) != TORSO_STAND2)) {
		// if not standing still, always point all in the same direction
		cent->pe.torso.yawing = qtrue;	// always center
		cent->pe.torso.pitching = qtrue;	// always center
		cent->pe.legs.yawing = qtrue;	// always center
	}

	// adjust legs for movement dir
	if (cent->currentState.eFlags & EF_DEAD) {
		// don't let dead bodies twitch
		dir = 0;
	} else {
		dir = cent->currentState.angles2[YAW];
		if (dir < 0 || dir > 7) {
			CG_Error("Bad player movement angle");
		}
	}
	legsAngles[YAW] = headAngles[YAW] + movementOffsets[ dir ];
	torsoAngles[YAW] = headAngles[YAW] + 0.25 * movementOffsets[ dir ];

	// torso
	CG_SwingAngles(torsoAngles[YAW], 25, 90, cg_swingSpeed.value, &cent->pe.torso.yawAngle, &cent->pe.torso.yawing);
	CG_SwingAngles(legsAngles[YAW], 40, 90, cg_swingSpeed.value, &cent->pe.legs.yawAngle, &cent->pe.legs.yawing);

	torsoAngles[YAW] = cent->pe.torso.yawAngle;
	legsAngles[YAW] = cent->pe.legs.yawAngle;


	// --------- pitch -------------

	// only show a fraction of the pitch angle in the torso
	if (headAngles[PITCH] > 180) {
		dest = (-360 + headAngles[PITCH]) * 0.75f;
	} else {
		dest = headAngles[PITCH] * 0.75f;
	}
	CG_SwingAngles(dest, 15, 30, 0.1f, &cent->pe.torso.pitchAngle, &cent->pe.torso.pitching);
	torsoAngles[PITCH] = cent->pe.torso.pitchAngle;

	//
	clientNum = cent->currentState.clientNum;
	if (clientNum >= 0 && clientNum < MAX_CLIENTS) {
		ci = &cgs.clientinfo[ clientNum ];
		if (ci->model->fixedtorso) {
			torsoAngles[PITCH] = 0.0f;
		}
	}

	// --------- roll -------------


	// lean towards the direction of travel
	VectorCopy(cent->currentState.pos.trDelta, velocity);
	speed = VectorNormalize(velocity);
	if (speed) {
		vec3_t	axis[3];
		float	side;

		speed *= 0.05f;

		AnglesToAxis(legsAngles, axis);
		side = speed * DotProduct(velocity, axis[1]);
		legsAngles[ROLL] -= side;

		side = speed * DotProduct(velocity, axis[0]);
		legsAngles[PITCH] += side;
	}

	//
	clientNum = cent->currentState.clientNum;
	if (clientNum >= 0 && clientNum < MAX_CLIENTS) {
		ci = &cgs.clientinfo[ clientNum ];
		if (ci->model->fixedlegs) {
			legsAngles[YAW] = torsoAngles[YAW];
			legsAngles[PITCH] = 0.0f;
			legsAngles[ROLL] = 0.0f;
		}
	}

	// pain twitch
	CG_AddPainTwitch(cent, torsoAngles);

	// pull the angles back out of the hierarchial chain
	AnglesSubtract(headAngles, torsoAngles, headAngles);
	AnglesSubtract(torsoAngles, legsAngles, torsoAngles);
	AnglesToAxis(legsAngles, legs);
	AnglesToAxis(torsoAngles, torso);
	AnglesToAxis(headAngles, head);
}

static void CG_HasteTrail(centity_t *cent)
{
	localEntity_t	*smoke;
	vec3_t			origin;
	int				anim;

	if (cent->trailTime > cg.time) {
		return;
	}
	anim = cent->pe.legs.animationNumber & ~ANIM_TOGGLEBIT;
	if (anim != LEGS_RUN && anim != LEGS_BACK) {
		return;
	}

	cent->trailTime += 100;
	if (cent->trailTime < cg.time) {
		cent->trailTime = cg.time;
	}

	VectorCopy(cent->lerpOrigin, origin);
	origin[2] -= 16;

	smoke = CG_SmokePuff(origin, vec3_origin, 8, (vec4_t) { 1, 1, 1, 1 },
		500, cg.time, 0, 0, cgs.media.hastePuffShader);

	// use the optimized local entity add
	smoke->leType = LE_SCALE_FADE;
}

static void CG_TrailItem(centity_t *cent, qhandle_t hModel)
{
	refEntity_t		ent;
	vec3_t			angles;
	vec3_t			axis[3];

	VectorCopy(cent->lerpAngles, angles);
	angles[PITCH] = 0;
	angles[ROLL] = 0;
	AnglesToAxis(angles, axis);

	memset(&ent, 0, sizeof(ent));
	VectorMA(cent->lerpOrigin, -16, axis[0], ent.origin);
	ent.origin[2] += 16;
	angles[YAW] += 90;
	AnglesToAxis(angles, ent.axis);

	ent.hModel = hModel;
	trap_R_AddRefEntityToScene(&ent);
}

static void CG_PlayerPowerups(centity_t *cent, refEntity_t *torso)
{
	int		powerups;

	powerups = cent->currentState.powerups;
	if (!powerups) {
		return;
	}

	// quad gives a dlight
	if (powerups & (1 << PW_QUAD)) {
		trap_R_AddLightToScene(cent->lerpOrigin, 200 + (rand()&31), 0.2f, 0.2f, 1);
	}

	// redflag
	if (powerups & (1 << PW_REDFLAG)) {
		CG_TrailItem(cent, cgs.media.redFlagModel);
		trap_R_AddLightToScene(cent->lerpOrigin, 200 + (rand()&31), 1.0, 0.2f, 0.2f);
	}

	// blueflag
	if (powerups & (1 << PW_BLUEFLAG)) {
		CG_TrailItem(cent, cgs.media.blueFlagModel);
		trap_R_AddLightToScene(cent->lerpOrigin, 200 + (rand()&31), 0.2f, 0.2f, 1.0);
	}

	// haste leaves smoke trails
	if (powerups & (1 << PW_HASTE)) {
		CG_HasteTrail(cent);
	}
}

/**
Float a sprite over the player's head
*/
static void CG_PlayerFloatSprite(centity_t *cent, qhandle_t shader)
{
	int				rf;
	refEntity_t		ent;

	if (cent->currentState.number == cg.snap->ps.clientNum && !cg.renderingThirdPerson) {
		rf = RF_THIRD_PERSON;		// only show in mirrors
	} else {
		rf = 0;
	}

	memset(&ent, 0, sizeof(ent));
	VectorCopy(cent->lerpOrigin, ent.origin);
	ent.origin[2] += 48;
	ent.reType = RT_SPRITE;
	ent.customShader = shader;
	ent.radius = 10;
	ent.renderfx = rf;
	ent.shaderRGBA[0] = 255;
	ent.shaderRGBA[1] = 255;
	ent.shaderRGBA[2] = 255;
	ent.shaderRGBA[3] = 255;
	trap_R_AddRefEntityToScene(&ent);
}

/**
Float sprites over the player's head
*/
static void CG_PlayerSprites(centity_t *cent)
{
	int		team;

	team = cgs.clientinfo[cent->currentState.clientNum].team;

	if (cent->currentState.eFlags & EF_CONNECTION) {
		CG_PlayerFloatSprite(cent, cgs.media.connectionShader);
		return;
	}

	if (cent->currentState.eFlags & EF_TALK) {
		if (!(cent->currentState.eFlags & EF_DEAD) && cg.snap->ps.persistant[PERS_TEAM] == team
			&& cgs.gametype > GT_TEAM && cgs.friendsThroughWalls)
		{
			CG_PlayerFloatSprite(cent, cgs.media.balloonShaderVisible);
		} else {
			CG_PlayerFloatSprite(cent, cgs.media.balloonShader);
		}
		return;
	}

	if (cent->currentState.eFlags & EF_AWARD_IMPRESSIVE) {
		CG_PlayerFloatSprite(cent, cgs.media.medalImpressive);
		return;
	}

	if (cent->currentState.eFlags & EF_AWARD_EXCELLENT) {
		CG_PlayerFloatSprite(cent, cgs.media.medalExcellent);
		return;
	}

	if (cent->currentState.eFlags & EF_AWARD_GAUNTLET) {
		CG_PlayerFloatSprite(cent, cgs.media.medalGauntlet);
		return;
	}

	if (cent->currentState.eFlags & EF_AWARD_DEFEND) {
		CG_PlayerFloatSprite(cent, cgs.media.medalDefend);
		return;
	}

	if (cent->currentState.eFlags & EF_AWARD_ASSIST) {
		CG_PlayerFloatSprite(cent, cgs.media.medalAssist);
		return;
	}

	if (cent->currentState.eFlags & EF_AWARD_CAP) {
		CG_PlayerFloatSprite(cent, cgs.media.medalCapture);
		return;
	}

	if (!(cent->currentState.eFlags & EF_DEAD) && cg_drawFriend.integer
		&& cg.snap->ps.persistant[PERS_TEAM] == team && cgs.gametype >= GT_TEAM)
	{
		if (cgs.friendsThroughWalls) {
			CG_PlayerFloatSprite(cent, cgs.media.friendShaderVisible);
		} else {
			CG_PlayerFloatSprite(cent, cgs.media.friendShader);
		}
		return;
	}
}

/**
Returns the Z component of the surface being shadowed
Should it return a full plane instead of a Z?
*/
static qboolean CG_PlayerShadow(centity_t *cent, float *shadowPlane)
{
	vec3_t		end, mins = {-15, -15, 0}, maxs = {15, 15, 2};
	trace_t		trace;
	float		alpha;

	*shadowPlane = 0;

	if (cg_shadows.integer == 0) {
		return qfalse;
	}

	// no shadows when invisible
	if (cent->currentState.powerups & (1 << PW_INVIS)) {
		return qfalse;
	}

	// send a trace down from the player to the ground
	VectorCopy(cent->lerpOrigin, end);
	end[2] -= SHADOW_DISTANCE;

	trap_CM_BoxTrace(&trace, cent->lerpOrigin, end, mins, maxs, 0, MASK_PLAYERSOLID);

	// no shadow if too high
	if (trace.fraction == 1.0 || trace.startsolid || trace.allsolid) {
		return qfalse;
	}

	*shadowPlane = trace.endpos[2] + 1;

	if (cg_shadows.integer != 1) {	// no mark for stencil or projection shadows
		return qtrue;
	}

	// fade the shadow out with height
	alpha = (1.0 - trace.fraction);

	// hack / FPE - bogus planes?
	//assert(DotProduct(trace.plane.normal, trace.plane.normal) != 0.0f) 

	// add the mark as a temporary, so it goes directly to the renderer
	// without taking a spot in the cg_marks array
	CG_ImpactMark(cgs.media.shadowMarkShader, trace.endpos, trace.plane.normal,
		cent->pe.legs.yawAngle, (vec4_t) { alpha, alpha, alpha, 1}, qfalse, 24, qtrue);

	return qtrue;
}

/**
Draw a mark at the water surface
*/
static void CG_PlayerSplash(centity_t *cent)
{
	vec3_t		start, end;
	trace_t		trace;
	int			contents;
	polyVert_t	verts[4];

	if (!cg_shadows.integer) {
		return;
	}

	VectorCopy(cent->lerpOrigin, end);
	end[2] -= 24;

	// if the feet aren't in liquid, don't make a mark
	// this won't handle moving water brushes, but they wouldn't draw right anyway...
	contents = CG_PointContents(end, 0);
	if (!(contents & (CONTENTS_WATER | CONTENTS_SLIME | CONTENTS_LAVA))) {
		return;
	}

	VectorCopy(cent->lerpOrigin, start);
	start[2] += 32;

	// if the head isn't out of liquid, don't make a mark
	contents = CG_PointContents(start, 0);
	if (contents & (CONTENTS_SOLID | CONTENTS_WATER | CONTENTS_SLIME | CONTENTS_LAVA)) {
		return;
	}

	// trace down to find the surface
	trap_CM_BoxTrace(&trace, start, end, NULL, NULL, 0, (CONTENTS_WATER | CONTENTS_SLIME | CONTENTS_LAVA));

	if (trace.fraction == 1.0) {
		return;
	}

	// create a mark polygon
	VectorCopy(trace.endpos, verts[0].xyz);
	verts[0].xyz[0] -= 32;
	verts[0].xyz[1] -= 32;
	verts[0].st[0] = 0;
	verts[0].st[1] = 0;
	verts[0].modulate[0] = 255;
	verts[0].modulate[1] = 255;
	verts[0].modulate[2] = 255;
	verts[0].modulate[3] = 255;

	VectorCopy(trace.endpos, verts[1].xyz);
	verts[1].xyz[0] -= 32;
	verts[1].xyz[1] += 32;
	verts[1].st[0] = 0;
	verts[1].st[1] = 1;
	verts[1].modulate[0] = 255;
	verts[1].modulate[1] = 255;
	verts[1].modulate[2] = 255;
	verts[1].modulate[3] = 255;

	VectorCopy(trace.endpos, verts[2].xyz);
	verts[2].xyz[0] += 32;
	verts[2].xyz[1] += 32;
	verts[2].st[0] = 1;
	verts[2].st[1] = 1;
	verts[2].modulate[0] = 255;
	verts[2].modulate[1] = 255;
	verts[2].modulate[2] = 255;
	verts[2].modulate[3] = 255;

	VectorCopy(trace.endpos, verts[3].xyz);
	verts[3].xyz[0] += 32;
	verts[3].xyz[1] -= 32;
	verts[3].st[0] = 1;
	verts[3].st[1] = 0;
	verts[3].modulate[0] = 255;
	verts[3].modulate[1] = 255;
	verts[3].modulate[2] = 255;
	verts[3].modulate[3] = 255;

	trap_R_AddPolyToScene(cgs.media.wakeMarkShader, 4, verts);
}

/**
Adds a piece with modifications or duplications for powerups
Also called by CG_Missile for quad rockets, but nobody can tell...
*/
void CG_AddRefEntityWithPowerups(refEntity_t *ent, entityState_t *state, int team)
{
	if (state->powerups & (1 << PW_INVIS)) {
		return;
	}

	trap_R_AddRefEntityToScene(ent);

	if (state->powerups & (1 << PW_QUAD))
	{
		if (team == TEAM_RED)
			ent->customShader = cgs.media.redQuadShader;
		else
			ent->customShader = cgs.media.quadShader;
		trap_R_AddRefEntityToScene(ent);
	}
	if (state->powerups & (1 << PW_REGEN)) {
		if (((cg.time / 100) % 10) == 1) {
			ent->customShader = cgs.media.regenShader;
			trap_R_AddRefEntityToScene(ent);
		}
	}
	if (state->powerups & (1 << PW_BATTLESUIT)) {
		ent->customShader = cgs.media.battleSuitShader;
		trap_R_AddRefEntityToScene(ent);
	}
}

int CG_LightVerts(vec3_t normal, int numVerts, polyVert_t *verts)
{
	int				i, j;
	float			incoming;
	vec3_t			ambientLight;
	vec3_t			lightDir;
	vec3_t			directedLight;

	trap_R_LightForPoint(verts[0].xyz, ambientLight, directedLight, lightDir);

	for (i = 0; i < numVerts; i++) {
		incoming = DotProduct (normal, lightDir);
		if (incoming <= 0) {
			verts[i].modulate[0] = ambientLight[0];
			verts[i].modulate[1] = ambientLight[1];
			verts[i].modulate[2] = ambientLight[2];
			verts[i].modulate[3] = 255;
			continue;
		} 
		j = (ambientLight[0] + incoming * directedLight[0]);
		if (j > 255) {
			j = 255;
		}
		verts[i].modulate[0] = j;

		j = (ambientLight[1] + incoming * directedLight[1]);
		if (j > 255) {
			j = 255;
		}
		verts[i].modulate[1] = j;

		j = (ambientLight[2] + incoming * directedLight[2]);
		if (j > 255) {
			j = 255;
		}
		verts[i].modulate[2] = j;

		verts[i].modulate[3] = 255;
	}
	return qtrue;
}

void CG_Player(centity_t *cent)
{
	clientInfo_t	*ci;
	refEntity_t		legs;
	refEntity_t		torso;
	refEntity_t		head;
	int				clientNum;
	int				renderfx;
	qboolean		shadow;
	float			shadowPlane;

	// the client number is stored in clientNum.  It can't be derived
	// from the entity number, because a single client may have
	// multiple corpses on the level using the same clientinfo
	clientNum = cent->currentState.clientNum;
	if (clientNum < 0 || clientNum >= MAX_CLIENTS) {
		CG_Error("Bad clientNum on player entity");
	}
	ci = &cgs.clientinfo[clientNum];

	// it is possible to see corpses from disconnected players that may
	// not have valid clientinfo
	if (!ci->infoValid) {
		return;
	}

	// get the player model information
	renderfx = 0;
	if (cent->currentState.number == cg.snap->ps.clientNum) {
		if (!cg.renderingThirdPerson) {
			renderfx = RF_THIRD_PERSON;			// only draw in mirrors
		} else {
			if (cg_cameraMode.integer) {
				return;
			}
		}
	}

	memset(&legs, 0, sizeof(legs));
	memset(&torso, 0, sizeof(torso));
	memset(&head, 0, sizeof(head));

	CG_SetModelColor(ci, &head, &torso, &legs, cent->currentState.eFlags);

	// get the rotation information
	CG_PlayerAngles(cent, legs.axis, torso.axis, head.axis);
	
	// get the animation state (after rotation, to allow feet shuffle)
	CG_PlayerAnimation(cent, &legs.oldframe, &legs.frame, &legs.backlerp,
		 &torso.oldframe, &torso.frame, &torso.backlerp);

	// add the talk baloon or disconnect icon
	CG_PlayerSprites(cent);

	// add the shadow
	shadow = CG_PlayerShadow(cent, &shadowPlane);

	// add a water splash if partially in and out of water
	CG_PlayerSplash(cent);

	if (cg_shadows.integer == 3 && shadow) {
		renderfx |= RF_SHADOW_PLANE;
	}
	renderfx |= RF_LIGHTING_ORIGIN;			// use the same origin for all

	//
	// add the legs
	//
	legs.hModel = ci->model->legsModel;
	legs.customSkin = ci->model->legsSkin;

	VectorCopy(cent->lerpOrigin, legs.origin);

	VectorCopy(cent->lerpOrigin, legs.lightingOrigin);
	legs.shadowPlane = shadowPlane;
	legs.renderfx = renderfx;
	VectorCopy (legs.origin, legs.oldorigin);	// don't positionally lerp at all

	CG_AddRefEntityWithPowerups(&legs, &cent->currentState, ci->team);

	// if the model failed, allow the default nullmodel to be displayed
	if (!legs.hModel) {
		return;
	}

	//
	// add the torso
	//
	torso.hModel = ci->model->torsoModel;
	if (!torso.hModel) {
		return;
	}

	torso.customSkin = ci->model->torsoSkin;

	VectorCopy(cent->lerpOrigin, torso.lightingOrigin);

	CG_PositionRotatedEntityOnTag(&torso, &legs, ci->model->legsModel, "tag_torso");

	torso.shadowPlane = shadowPlane;
	torso.renderfx = renderfx;

	CG_AddRefEntityWithPowerups(&torso, &cent->currentState, ci->team);

	//
	// add the head
	//
	head.hModel = ci->model->headModel;
	if (!head.hModel) {
		return;
	}

	head.customSkin = ci->model->headSkin;

	VectorCopy(cent->lerpOrigin, head.lightingOrigin);

	CG_PositionRotatedEntityOnTag(&head, &torso, ci->model->torsoModel, "tag_head");

	head.shadowPlane = shadowPlane;
	head.renderfx = renderfx;

	CG_AddRefEntityWithPowerups(&head, &cent->currentState, ci->team);

	//
	// add the gun / barrel / flash
	//
	CG_AddPlayerWeapon(&torso, NULL, cent, ci->team);

	// add powerups floating behind the player
	CG_PlayerPowerups(cent, &torso);

	CG_AddBoundingBox(cent);
}

/**
A player just came into view or teleported, so reset all animation info
*/
void CG_ResetPlayerEntity(centity_t *cent)
{
	cent->errorTime = -99999;		// guarantee no error decay added
	cent->extrapolated = qfalse;	

	CG_ClearLerpFrame(&cgs.clientinfo[ cent->currentState.clientNum ], &cent->pe.legs, cent->currentState.legsAnim);
	CG_ClearLerpFrame(&cgs.clientinfo[ cent->currentState.clientNum ], &cent->pe.torso, cent->currentState.torsoAnim);

	BG_EvaluateTrajectory(&cent->currentState.pos, cg.time, cent->lerpOrigin);
	BG_EvaluateTrajectory(&cent->currentState.apos, cg.time, cent->lerpAngles);

	VectorCopy(cent->lerpOrigin, cent->rawOrigin);
	VectorCopy(cent->lerpAngles, cent->rawAngles);

	memset(&cent->pe.legs, 0, sizeof(cent->pe.legs));
	cent->pe.legs.yawAngle = cent->rawAngles[YAW];
	cent->pe.legs.yawing = qfalse;
	cent->pe.legs.pitchAngle = 0;
	cent->pe.legs.pitching = qfalse;

	memset(&cent->pe.torso, 0, sizeof(cent->pe.torso));
	cent->pe.torso.yawAngle = cent->rawAngles[YAW];
	cent->pe.torso.yawing = qfalse;
	cent->pe.torso.pitchAngle = cent->rawAngles[PITCH];
	cent->pe.torso.pitching = qfalse;

	if (cg_debugPosition.integer) {
		CG_Printf("%i ResetPlayerEntity yaw=%f\n", cent->currentState.number, cent->pe.torso.yawAngle);
	}
}


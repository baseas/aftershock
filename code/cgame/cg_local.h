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
// cg_local.c

#include "../qcommon/q_shared.h"
#include "../renderercommon/tr_types.h"
#include "../game/bg_public.h"
#include "cg_public.h"

// The entire cgame module is unloaded and reloaded on each level change,
// so there is NO persistant data between levels on the client side.
// If you absolutely need something stored, it can either be kept
// by the server in the server stored userinfos, or stashed in a cvar.

#define POWERUP_BLINKS		5

#define POWERUP_BLINK_TIME	1000
#define FADE_TIME			200
#define PULSE_TIME			200
#define DAMAGE_DEFLECT_TIME	100
#define DAMAGE_RETURN_TIME	400
#define DAMAGE_TIME			500
#define LAND_DEFLECT_TIME	150
#define LAND_RETURN_TIME	300
#define STEP_TIME			200
#define DUCK_TIME			100
#define PAIN_TWITCH_TIME	200
#define WEAPON_SELECT_TIME	1400
#define ITEM_SCALEUP_TIME	1000
#define ZOOM_TIME			150
#define ITEM_BLOB_TIME		200
#define MUZZLE_FLASH_TIME	50
#define SINK_TIME			1000		// time for fragments to sink into ground before going away
#define ATTACKER_HEAD_TIME	10000
#define REWARD_TIME			3000

#define PULSE_SCALE			1.5			// amount to scale up the icons when activating

#define MAX_STEP_CHANGE		32

#define MAX_VERTS_ON_POLY	10
#define MAX_MARK_POLYS		256

#define STAT_MINUS			10			// num frame for '-' stats digit

#define ICON_SIZE			48
#define CHAR_WIDTH			32
#define CHAR_HEIGHT			48
#define TEXT_ICON_SPACE		4

#define CHAT_WIDTH			80
#define CHAT_HEIGHT			8
#define DEATHNOTICE_HEIGHT	5

// very large characters
#define GIANT_WIDTH			32
#define GIANT_HEIGHT		48

#define NUM_CROSSHAIRS		19

#define MAX_SPAWNPOINTS		32

#define LAG_SAMPLES			128

#define TEAM_OVERLAY_MAXNAME_WIDTH	12
#define TEAM_OVERLAY_MAXLOCATION_WIDTH	16

typedef enum {
	WPCOLOR_GRENADE = (1 << 0),
	WPCOLOR_GRENADETRAIL = (1 << 1),
	WPCOLOR_ROCKETTRAIL = (1 << 2),
	WPCOLOR_RAILTRAIL = (1 << 3),
	WPCOLOR_PLASMA = (1 << 4),
	WPCOLOR_LIGHTNING = (1 << 5)
} weaponColor_t;

typedef enum {
	FOOTSTEP_NORMAL,
	FOOTSTEP_BOOT,
	FOOTSTEP_FLESH,
	FOOTSTEP_MECH,
	FOOTSTEP_ENERGY,
	FOOTSTEP_METAL,
	FOOTSTEP_SPLASH,
	FOOTSTEP_TOTAL
} footstep_t;

typedef enum {
	IMPACTSOUND_DEFAULT,
	IMPACTSOUND_METAL,
	IMPACTSOUND_FLESH
} impactSound_t;

// player entities need to track more information
// than any other type of entity.
// note that not every player entity is a client entity,
// because corpses after respawn are outside the normal
// client numbering range
// when changing animation, set animationTime to frameTime + lerping time
// The current lerp will finish out, then it will lerp to the new animation
typedef struct {
	int			oldFrame;
	int			oldFrameTime;		// time when ->oldFrame was exactly on

	int			frame;
	int			frameTime;			// time when ->frame will be exactly on

	float		backlerp;

	float		yawAngle;
	qboolean	yawing;
	float		pitchAngle;
	qboolean	pitching;

	int			animationNumber;	// may include ANIM_TOGGLEBIT
	animation_t	*animation;
	int			animationTime;		// time when the first frame of the animation will be exact
} lerpFrame_t;

typedef struct {
	lerpFrame_t		legs, torso, flag;
	int				painTime;
	int				painDirection;		// flip from 0 to 1
	int				lightningFiring;

	int				railFireTime;

	// machinegun spinning
	float			barrelAngle;
	int				barrelTime;
	qboolean		barrelSpinning;
} playerEntity_t;

// centity_t have a direct corespondence with gentity_t in the game, but
// only the entityState_t is directly communicated to the cgame
typedef struct centity_s {
	entityState_t	currentState;		// from cg.frame
	entityState_t	nextState;			// from cg.nextFrame, if available
	qboolean		interpolate;		// true if next is valid to interpolate to
	qboolean		currentValid;		// true if cg.frame holds this entity

	int				muzzleFlashTime;	// move to playerEntity?
	int				previousEvent;
	int				teleportFlag;

	int				trailTime;			// so missile trails can handle dropped initial packets
	int				dustTrailTime;
	int				miscTime;

	int				snapShotTime;		// last time this entity was found in a snapshot

	playerEntity_t	pe;

	int				errorTime;			// decay the error from this time
	vec3_t			errorOrigin;
	vec3_t			errorAngles;
	
	qboolean		extrapolated;		// false if origin / angles is an interpolation
	vec3_t			rawOrigin;
	vec3_t			rawAngles;

	vec3_t			beamEnd;

	// exact interpolated position of entity on this frame
	vec3_t			lerpOrigin;
	vec3_t			lerpAngles;
} centity_t;

// local entities are created as a result of events or predicted actions,
// and live independantly from all server transmitted entities

typedef struct markPoly_s {
	struct markPoly_s	*prevMark, *nextMark;
	int			time;
	qhandle_t	markShader;
	qboolean	alphaFade;		// fade alpha instead of rgb
	byte		color[4];
	poly_t		poly;
	polyVert_t	verts[MAX_VERTS_ON_POLY];
} markPoly_t;

typedef enum {
	LE_MARK,
	LE_EXPLOSION,
	LE_SPRITE_EXPLOSION,
	LE_FRAGMENT,
	LE_MOVE_SCALE_FADE,
	LE_FALL_SCALE_FADE,
	LE_FADE_RGB,
	LE_SCALE_FADE,
	LE_SCOREPLUM
} leType_t;

typedef enum {
	LEF_PUFF_DONT_SCALE  = 0x0001,			// do not scale size over time
	LEF_TUMBLE			 = 0x0002,			// tumble over time, used for ejecting shells
	LEF_SOUND1			 = 0x0004,			// sound 1 for kamikaze
	LEF_SOUND2			 = 0x0008			// sound 2 for kamikaze
} leFlag_t;

typedef enum {
	LEMT_NONE,
	LEMT_BURN,
	LEMT_BLOOD
} leMarkType_t;			// fragment local entities can leave marks on walls

typedef enum {
	LEBS_NONE,
	LEBS_BLOOD,
	LEBS_BRASS
} leBounceSoundType_t;	// fragment local entities can make sounds on impacts

typedef struct localEntity_s {
	struct localEntity_s	*prev, *next;
	leType_t		leType;
	int				leFlags;

	int				startTime;
	int				endTime;
	int				fadeInTime;

	float			lifeRate;			// 1.0 / (endTime - startTime)

	trajectory_t	pos;
	trajectory_t	angles;

	float			bounceFactor;		// 0.0 = no bounce, 1.0 = perfect

	float			color[4];

	float			radius;

	float			light;
	vec3_t			lightColor;

	leMarkType_t		leMarkType;		// mark to leave on fragment impact
	leBounceSoundType_t	leBounceSoundType;

	refEntity_t		refEntity;		
} localEntity_t;

#define MAX_CUSTOM_SOUNDS	32

typedef struct {
	char		*modelName;
	qhandle_t	modelIcon;

	qhandle_t	headModel;
	qhandle_t	headSkin;
	vec4_t		headColor;

	qhandle_t	torsoModel;
	qhandle_t	torsoSkin;
	vec4_t		torsoColor;

	qhandle_t	legsModel;
	qhandle_t	legsSkin;
	vec4_t		legsColor;

	vec4_t		weaponColor;

	qboolean	fixedlegs;              // true if legs yaw is always the same as torso yaw
	qboolean	fixedtorso;             // true if torso never changes yaw

	vec3_t		headOffset;             // move head in icon views
	footstep_t	footsteps;
	gender_t	gender;                 // from model

	animation_t	animations[MAX_TOTALANIMATIONS];
	sfxHandle_t	sounds[MAX_CUSTOM_SOUNDS];
} model_t;

// each client has an associated clientInfo_t
// that contains media references necessary to present the
// client model and other color coded effects
// this is regenerated each time a client's configstring changes,
// usually as a result of a userinfo (name, model, etc) change
typedef struct {
	qboolean		infoValid;
    qboolean		ready;

	char			name[MAX_QPATH];
	team_t			team;

	int				ping;
	int				enterTime;

	int				botSkill;		// 0 = not bot, 1-5 = bot

	int				handicap;
	int				wins, losses;	// in tourney mode
	qboolean		specOnly;

	int				teamTask;		// task in teamplay (offence/defence)
	qboolean		teamLeader;		// true when this is a team leader

	int				medkitUsageTime;
	model_t			*model;

	playerStats_t	stats;

	// scoreboard
	int			score;
	int			powerups;
	int			damageDone;
	int			damageTaken;
	int			yellowArmor;
	int			redArmor;
	int			megaHealth;

	// team overlay
	int			location;
	int			health;
	int			armor;
	int			weapon;
} clientInfo_t;

// each WP_* weapon enum has an associated weaponInfo_t
// that contains media references necessary to present the
// weapon and its effects
typedef struct weaponInfo_s {
	qboolean		registered;
	gitem_t			*item;

	qhandle_t		handsModel;			// the hands don't actually draw, they just position the weapon
	qhandle_t		weaponModel;
	qhandle_t		barrelModel;
	qhandle_t		flashModel;

	vec3_t			weaponMidpoint;		// so it will rotate centered instead of by tag

	float			flashDlight;
	vec3_t			flashDlightColor;
	sfxHandle_t		flashSound[4];		// fast firing weapons randomly choose

	qhandle_t		weaponIcon;
	qhandle_t		ammoIcon;

	qhandle_t		ammoModel;

	qhandle_t		missileModel;
	sfxHandle_t		missileSound;
	void			(*missileTrailFunc)(centity_t *, const struct weaponInfo_s *wi);
	float			missileDlight;
	vec3_t			missileDlightColor;
	int				missileRenderfx;

	sfxHandle_t		readySound;
	sfxHandle_t		firingSound;
} weaponInfo_t;


// each IT_* item has an associated itemInfo_t
// that constains media references necessary to present the
// item and its effects
typedef struct {
	qboolean		registered;
	qhandle_t		models[MAX_ITEM_MODELS];
	qhandle_t		icon;
	qhandle_t		pickupSound;
} itemInfo_t;

typedef struct {
	int				itemNum;
} powerupInfo_t;

#define MAX_REWARDSTACK		10
#define MAX_SOUNDBUFFER		20

// all cg.stepTime, cg.duckTime, cg.landTime, etc are set to cg.time when the action
// occurs, and they will have visible effects for #define STEP_TIME or whatever msec after

#define MAX_PREDICTED_EVENTS	16
 
typedef struct {
	int			clientFrame;		// incremented each frame

	int			clientNum;
	
	qboolean	demoPlayback;
	qboolean	levelShot;			// taking a level menu screenshot
	qboolean	intermissionStarted;	// don't play voice rewards, because game will end shortly

	// there are only one or two snapshot_t that are relevent at a time
	int			latestSnapshotNum;	// the number of snapshots the client system has received
	int			latestSnapshotTime;	// the time from latestSnapshotNum, so we don't need to read the snapshot yet

	snapshot_t	*snap;				// cg.snap->serverTime <= cg.time
	snapshot_t	*nextSnap;			// cg.nextSnap->serverTime > cg.time, or NULL
	snapshot_t	activeSnapshots[2];

	struct {
		int		frameSamples[LAG_SAMPLES];
		int		frameCount;
		int		snapshotFlags[LAG_SAMPLES];
		int		snapshotSamples[LAG_SAMPLES];
		int		snapshotCount;
	} lagometer;

	float		frameInterpolation;	// (float)(cg.time - cg.frame->serverTime) / (cg.nextFrame->serverTime - cg.frame->serverTime)

	qboolean	thisFrameTeleport;
	qboolean	nextFrameTeleport;

	int			frametime;		// cg.time - cg.oldTime

	int			time;			// this is the time value that the client
								// is rendering at.
	int			oldTime;		// time at last frame, used for missile trails and prediction checking

	int			physicsTime;	// either cg.snap->time or cg.nextSnap->time

	int			timelimitWarnings;	// 5 min, 1 min, overtime
	int			fraglimitWarnings;

	qboolean	mapRestart;			// set on a map restart to set back the weapon

	qboolean	renderingThirdPerson;		// during deaths, chasecams, etc

	// prediction state
	qboolean	hyperspace;				// true if prediction has hit a trigger_teleport
	playerState_t	predictedPlayerState;
	centity_t		predictedPlayerEntity;
	qboolean	validPPS;				// clear until the first call to CG_PredictPlayerState
	int			predictedErrorTime;
	vec3_t		predictedError;

	int			eventSequence;
	int			predictableEvents[MAX_PREDICTED_EVENTS];

	float		stepChange;				// for stair up smoothing
	int			stepTime;

	float		duckChange;				// for duck viewheight smoothing
	int			duckTime;

	float		landChange;				// for landing hard
	int			landTime;

	// input state sent to server
	int			weaponSelect;

	// auto rotating items
	vec3_t		autoAngles;
	vec3_t		autoAxis[3];
	vec3_t		autoAnglesFast;
	vec3_t		autoAxisFast[3];

	// view rendering
	refdef_t	refdef;
	vec3_t		refdefViewAngles;		// will be converted to refdef.viewaxis

	// zoom key
	qboolean	zoomed;
	int			zoomTime;
	float		zoomSensitivity;

	qboolean	drawDisconnect;

	// information screen text during loading
	qboolean	showInfoScreen;

	qboolean	showChat;

	// scoreboard
	qboolean	showScores;
	qboolean	showInfo;
	qboolean	showStats;
	qboolean	scoreBoardShowing;
	char		killerName[MAX_NAME_LENGTH];

	// centerprinting
	int			centerPrintTime;
	char		centerPrint[1024];
	int			centerPrintLines;

	// low ammo warning state
	int			lowAmmoWarning;		// 1 = low, 2 = empty

	// crosshair client ID
	int			crosshairClientNum;
	int			crosshairClientTime;

	// powerup active flashing
	int			powerupActive;
	int			powerupTime;

	// attacking player
	int			attackerTime;
	int			voiceTime;

	// reward medals
	int			rewardStack;
	int			rewardTime;
	int			rewardCount[MAX_REWARDSTACK];
	qhandle_t	rewardShader[MAX_REWARDSTACK];
	qhandle_t	rewardSound[MAX_REWARDSTACK];

	// sound buffer mainly for announcer sounds
	int			soundBufferIn;
	int			soundBufferOut;
	int			soundTime;
	qhandle_t	soundBuffer[MAX_SOUNDBUFFER];

	// warmup countdown
	int			warmup;
	int			warmupCount;

	int			itemPickup;
	int			itemPickupTime;
	int			itemPickupBlendTime;	// the pulse around the crosshair is timed seperately

	int			weaponSelectTime;
	int			weaponAnimation;
	int			weaponAnimationTime;

	// blend blobs
	float		damageTime;
	float		damageX, damageY, damageValue;

	// status bar head
	float		headYaw;
	float		headEndPitch;
	float		headEndYaw;
	int			headEndTime;
	float		headStartPitch;
	float		headStartYaw;
	int			headStartTime;

	// view movement
	float		v_dmg_time;
	float		v_dmg_pitch;
	float		v_dmg_roll;

	// temp working variables for player view
	float		bobfracsin;
	int			bobcycle;
	float		xyspeed;
	int			nextOrbitTime;

	// development tool
	refEntity_t		testModelEntity;
	char			testModelName[MAX_QPATH];
	qboolean		testGun;

	// spawnpoints
	vec3_t		spawnOrigin[MAX_SPAWNPOINTS];
	vec3_t		spawnAngle[MAX_SPAWNPOINTS];
	int			spawnTeam[MAX_SPAWNPOINTS];
	int			numSpawnpoints;

	int			lastHitDamage;
	int			lastHitTime;

	int			sortedClients[MAX_CLIENTS];

	playerStats_t	statsOwn;
	playerStats_t	statsFollow;
	playerStats_t	statsEnemy;		// for tourney enemy
} cg_t;

#define MAX_LGSTYLES	4

// all of the model, shader, and sound references that are
// loaded at gamestate time are stored in cgMedia_t
// Other media that can be tied to clients, weapons, or items are
// stored in the clientInfo_t, itemInfo_t, weaponInfo_t, and powerupInfo_t
typedef struct {
	qhandle_t	charsetShader;
	qhandle_t	charsetShader32;
	qhandle_t	charsetShader64;
	qhandle_t	charsetShader128;
	qhandle_t	charsetProp;
	qhandle_t	charsetPropGlow;
	qhandle_t	charsetPropB;
	qhandle_t	whiteShader;

	qhandle_t	redFlagModel;
	qhandle_t	blueFlagModel;
	qhandle_t	neutralFlagModel;
	qhandle_t	redFlagShader[3];
	qhandle_t	blueFlagShader[3];
	qhandle_t	flagShader[4];

	qhandle_t	flagPoleModel;
	qhandle_t	flagFlapModel;

	qhandle_t	redFlagFlapSkin;
	qhandle_t	blueFlagFlapSkin;
	qhandle_t	neutralFlagFlapSkin;

	qhandle_t	redFlagBaseModel;
	qhandle_t	blueFlagBaseModel;
	qhandle_t	neutralFlagBaseModel;

	qhandle_t	armorRed;
	qhandle_t	armorBlue;
	qhandle_t	armorYellow;

	qhandle_t	healthRed;
	qhandle_t	healthBlue;
	qhandle_t	healthYellow;

	qhandle_t	deferShader;

	// gib explosions
	qhandle_t	gibAbdomen;
	qhandle_t	gibArm;
	qhandle_t	gibChest;
	qhandle_t	gibFist;
	qhandle_t	gibFoot;
	qhandle_t	gibForearm;
	qhandle_t	gibIntestine;
	qhandle_t	gibLeg;
	qhandle_t	gibSkull;
	qhandle_t	gibBrain;

	qhandle_t	smoke2;

	qhandle_t	railCoreShader;

	qhandle_t	lightningShader[MAX_LGSTYLES];

	qhandle_t	friendShader;
	qhandle_t	friendShaderVisible;

	qhandle_t	balloonShader;
	qhandle_t	balloonShaderVisible;
	qhandle_t	connectionShader;

	qhandle_t	viewBloodShader;
	qhandle_t	tracerShader;
	qhandle_t	crosshairShader[NUM_CROSSHAIRS];
	qhandle_t	lagometerShader;
	qhandle_t	backTileShader;
	qhandle_t	noammoShader;

	qhandle_t	smokePuffShader;
	qhandle_t	smokePuffRageProShader;
	qhandle_t	plasmaBallShader;
	qhandle_t	waterBubbleShader;
	qhandle_t	bloodTrailShader;
	qhandle_t	grenadeShader;

	qhandle_t	numberShaders[11];

	qhandle_t	shadowMarkShader;

	qhandle_t	botSkillShaders[5];

	// wall mark shaders
	qhandle_t	wakeMarkShader;
	qhandle_t	bloodMarkShader;
	qhandle_t	bulletMarkShader;
	qhandle_t	burnMarkShader;
	qhandle_t	holeMarkShader;
	qhandle_t	energyMarkShader;

	// powerup shaders
	qhandle_t	quadShader;
	qhandle_t	quadWeaponShader;
	qhandle_t	regenShader;
	qhandle_t	battleSuitShader;
	qhandle_t	battleWeaponShader;
	qhandle_t	hastePuffShader;

	qhandle_t	bboxShader;
	qhandle_t	bboxShaderNocull;

	// weapon effect models
	qhandle_t	bulletFlashModel;
	qhandle_t	ringFlashModel;
	qhandle_t	dishFlashModel;
	qhandle_t	lightningExplosionModel;

	// weapon effect shaders
	qhandle_t	railExplosionShader;
	qhandle_t	plasmaExplosionShader;
	qhandle_t	bulletExplosionShader;
	qhandle_t	rocketExplosionShader;
	qhandle_t	grenadeExplosionShader;
	qhandle_t	bfgExplosionShader;
	qhandle_t	bloodExplosionShader;

	// special effects models
	qhandle_t	teleportEffectModel;
	qhandle_t	teleportEffectShader;

	// medals shown during gameplay
	qhandle_t	medalImpressive;
	qhandle_t	medalExcellent;
	qhandle_t	medalGauntlet;
	qhandle_t	medalDefend;
	qhandle_t	medalAssist;
	qhandle_t	medalCapture;
	qhandle_t	medalAirrocket;
	qhandle_t	medalAirgrenade;
	qhandle_t	medalRocketrail;
	qhandle_t	medalFullSg;
	qhandle_t	medalItemdenied;
	qhandle_t	medalLgAccuracy;
	qhandle_t	medalDoubleAirrocket;

	// sounds
	sfxHandle_t	quadSound;
	sfxHandle_t	tracerSound;
	sfxHandle_t	selectSound;
	sfxHandle_t	useNothingSound;
	sfxHandle_t	wearOffSound;
	sfxHandle_t	footsteps[FOOTSTEP_TOTAL][4];
	sfxHandle_t	sfx_lghit1;
	sfxHandle_t	sfx_lghit2;
	sfxHandle_t	sfx_lghit3;
	sfxHandle_t	sfx_ric1;
	sfxHandle_t	sfx_ric2;
	sfxHandle_t	sfx_ric3;
	sfxHandle_t	sfx_rockexp;
	sfxHandle_t	sfx_plasmaexp;
	sfxHandle_t	gibSound;
	sfxHandle_t	gibBounce1Sound;
	sfxHandle_t	gibBounce2Sound;
	sfxHandle_t	gibBounce3Sound;
	sfxHandle_t	teleInSound;
	sfxHandle_t	teleOutSound;
	sfxHandle_t	noAmmoSound;
	sfxHandle_t	respawnSound;
	sfxHandle_t	talkSound;
	sfxHandle_t	landSound;
	sfxHandle_t	fallSound;
	sfxHandle_t	jumpPadSound;

	sfxHandle_t	oneMinuteSound;
	sfxHandle_t	fiveMinuteSound;
	sfxHandle_t	suddenDeathSound;

	sfxHandle_t	threeFragSound;
	sfxHandle_t	twoFragSound;
	sfxHandle_t	oneFragSound;

	sfxHandle_t	hitSound0;
	sfxHandle_t	hitSound1;
	sfxHandle_t	hitSound2;
	sfxHandle_t	hitSound3;
	sfxHandle_t	hitSound4;
	sfxHandle_t	hitTeamSound;
	sfxHandle_t	impressiveSound;
	sfxHandle_t	excellentSound;
	sfxHandle_t	deniedSound;
	sfxHandle_t	humiliationSound;
	sfxHandle_t	assistSound;
	sfxHandle_t	defendSound;
	sfxHandle_t	airrocketSound;
	sfxHandle_t	airgrenadeSound;
	sfxHandle_t	lgAccuracySound;

	sfxHandle_t	takenLeadSound;
	sfxHandle_t	tiedLeadSound;
	sfxHandle_t	lostLeadSound;

	sfxHandle_t	voteNow;
	sfxHandle_t	votePassed;
	sfxHandle_t	voteFailed;

	sfxHandle_t	watrInSound;
	sfxHandle_t	watrOutSound;
	sfxHandle_t	watrUnSound;

	sfxHandle_t	medkitSound;

	// teamplay sounds
	sfxHandle_t	captureAwardSound;
	sfxHandle_t	redScoredSound;
	sfxHandle_t	blueScoredSound;
	sfxHandle_t	redLeadsSound;
	sfxHandle_t	blueLeadsSound;
	sfxHandle_t	teamsTiedSound;

	sfxHandle_t	captureYourTeamSound;
	sfxHandle_t	captureOpponentSound;
	sfxHandle_t	returnYourTeamSound;
	sfxHandle_t	returnOpponentSound;

	sfxHandle_t	redFlagReturnedSound;
	sfxHandle_t	blueFlagReturnedSound;
	sfxHandle_t	enemyTookYourFlagSound;
	sfxHandle_t	yourTeamTookEnemyFlagSound;
	sfxHandle_t	youHaveFlagSound;
	sfxHandle_t	holyShitSound;

	// voice sounds
	sfxHandle_t	count3Sound;
	sfxHandle_t	count2Sound;
	sfxHandle_t	count1Sound;
	sfxHandle_t	countFightSound;
	sfxHandle_t	countPrepareSound;

	sfxHandle_t	regenSound;
	sfxHandle_t	protectSound;
	sfxHandle_t	n_healthSound;
	sfxHandle_t	hgrenb1aSound;
	sfxHandle_t	hgrenb2aSound;
	sfxHandle_t	wstbimplSound;
	sfxHandle_t	wstbimpmSound;
	sfxHandle_t	wstbimpdSound;
	sfxHandle_t	wstbactvSound;

	sfxHandle_t	gurp1Sound;
	sfxHandle_t	gurp2Sound;
	sfxHandle_t	killbeep;

	model_t	teamModel;
	model_t	enemyModel;
	model_t	redTeamModel;
	model_t	blueTeamModel;

	vec4_t	deadBodyColor;
	vec4_t	crosshairColor;
	vec4_t	crosshairHitColor;

	// scoreboard
	qhandle_t	sbBackground;
	qhandle_t	sbClock;
	qhandle_t	sbPing;
	qhandle_t	sbReady;
	qhandle_t	sbNotReady;
	qhandle_t	sbSkull;
	qhandle_t	sbLocked;

	qhandle_t	accBackground;

	qhandle_t	skull;
	qhandle_t	directHit;

	qhandle_t	netgraph;
	qhandle_t	spawnpoint;
	qhandle_t	spawnpointShader;
} cgMedia_t;

enum {
	HUD_AMMOWARNING,
	HUD_ATTACKERICON,
	HUD_ATTACKERNAME,
	HUD_CHAT1,
	HUD_CHAT2,
	HUD_CHAT3,
	HUD_CHAT4,
	HUD_CHAT5,
	HUD_CHAT6,
	HUD_CHAT7,
	HUD_CHAT8,
	HUD_FS_OWN,
	HUD_FS_NME,
	HUD_FOLLOW,
	HUD_FPS,
	HUD_FRAGMSG,
	HUD_GAMETIME,
	HUD_ROUNDTIME,
	HUD_REALTIME,
	HUD_GAMETYPE,
	HUD_ITEMPICKUPNAME,
	HUD_ITEMPICKUPTIME,
	HUD_ITEMPICKUPICON,
	HUD_NETGRAPH,
	HUD_NETGRAPHPING,
	HUD_SPEED,
	HUD_ACCEL,
	HUD_PU1,
	HUD_PU2,
	HUD_PU3,
	HUD_PU4,
	HUD_PU1ICON,
	HUD_PU2ICON,
	HUD_PU3ICON,
	HUD_PU4ICON,
	HUD_RANKMSG,
	HUD_SCORELIMIT,
	HUD_SCORENME,
	HUD_SCOREOWN,
	HUD_SPECMESSAGE,
	HUD_ARMORCOUNT,
	HUD_ARMORICON,
	HUD_AMMOCOUNT,
	HUD_AMMOICON,
	HUD_HEALTHCOUNT,
	HUD_HEALTHICON,
	HUD_TARGETNAME,
	HUD_TARGETSTATUS,
	HUD_TC_NME,
	HUD_TC_OWN,
	HUD_TI_NME,
	HUD_TI_OWN,
	HUD_TEAMCHAT1,
	HUD_TEAMCHAT2,
	HUD_TEAMCHAT3,
	HUD_TEAMCHAT4,
	HUD_TEAMCHAT5,
	HUD_TEAMCHAT6,
	HUD_TEAMCHAT7,
	HUD_TEAMCHAT8,
	HUD_VOTEMSG,
	HUD_WARMUP,
	HUD_WEAPONLIST,
	HUD_READYSTATUS,
	HUD_DEATHNOTICE1,
	HUD_DEATHNOTICE2,
	HUD_DEATHNOTICE3,
	HUD_DEATHNOTICE4,
	HUD_DEATHNOTICE5,
	HUD_COUNTDOWN,
	HUD_RESPAWNTIMER,
	HUD_STATUSBARFLAG,
	HUD_TEAMOVERLAY1,
	HUD_TEAMOVERLAY2,
	HUD_TEAMOVERLAY3,
	HUD_TEAMOVERLAY4,
	HUD_TEAMOVERLAY5,
	HUD_TEAMOVERLAY6,
	HUD_TEAMOVERLAY7,
	HUD_TEAMOVERLAY8,
	HUD_REWARD,
	HUD_REWARDCOUNT,
	HUD_HOLDABLE,
	HUD_HELP,
	HUD_MAX
};

typedef struct {
	qboolean	inuse;
	float		xpos;
	float		ypos;
	float		width;
	float		height;
	vec4_t		color;
	vec4_t		bgcolor;
	qboolean	fill;
	float		fontWidth;
	float		fontHeight;
	char		*image;
	char		*text;
	int			textAlign;
	int			textStyle;
	int			time;
	int			teamColor;
	int			teamBgColor;
} hudElement_t;

typedef struct {
	// chat width is *3 because of embedded color codes
	char	message[3 * CHAT_WIDTH + 1];
	int		time;
} msgItem_t;

typedef struct {
	// need to store names instead of clientNumber,
	// because the clientInfo is lost on disconnect
	char		target[MAX_NAME_LENGTH];
	char		attacker[MAX_NAME_LENGTH];
	qhandle_t	icon;
	qboolean	directHit;
	int			time;
} deathNotice_t;

// The client game static (cgs) structure hold everything
// loaded or calculated from the gamestate.  It will NOT
// be cleared when a tournement restart is done, allowing
// all clients to begin playing instantly
typedef struct {
	gameState_t		gameState;			// gamestate from server
	glconfig_t		glconfig;			// rendering configuration
	float			screenXScale;		// derived from glconfig
	float			screenYScale;
	float			screenXBias;

	int				serverCommandSequence;	// reliable command stream counter
	int				processedSnapshotNum;	// the number of snapshots cgame has requested

	qboolean		localServer;			// detected on startup by checking sv_running

	// parsed from serverinfo
	gametype_t		gametype;
	char			*gametypeName;
	char			*gametypeShortName;
	int				dmflags;
	int				teamflags;
	int				fraglimit;
	int				capturelimit;
	int				timelimit;
	int				maxclients;
	qboolean		newItemHeight;
	char			mapname[MAX_QPATH];
	char			redTeam[MAX_QPATH];
	char			blueTeam[MAX_QPATH];

	int				voteTime;
	int				voteYes;
	int				voteNo;
	char			voteString[MAX_STRING_TOKENS];

	int				teamVoteTime[2];
	int				teamVoteYes[2];
	int				teamVoteNo[2];
	char			teamVoteString[2][MAX_STRING_TOKENS];

	int				levelStartTime;
	int				roundStartTime;			// for elimination gametype
	int				roundTimelimit;

	int				scores1, scores2;		// from configstrings
	int				redflag, blueflag;		// flag status from configstrings
	int				flagStatus;

	qboolean		startWhenReady;

	//
	// locally derived information from gamestate
	//
	qhandle_t		gameModels[MAX_MODELS];
	sfxHandle_t		gameSounds[MAX_SOUNDS];

	int				numInlineModels;
	qhandle_t		inlineDrawModel[MAX_MODELS];
	vec3_t			inlineModelMidpoints[MAX_MODELS];

	clientInfo_t	clientinfo[MAX_CLIENTS];

	msgItem_t		chatMessages[CHAT_HEIGHT];
	msgItem_t		teamChatMessages[CHAT_HEIGHT];
	deathNotice_t	deathNotices[DEATHNOTICE_HEIGHT];

	int			cursorX;
	int			cursorY;
	qboolean	eventHandling;
	qboolean	mouseCaptured;
	qboolean	sizingHud;
	void		*capturedItem;
	qhandle_t	activeCursor;

	// orders
	int			currentOrder;
	qboolean	orderPending;
	int			orderTime;
	int			currentVoiceClient;
	int			acceptOrderTime;
	int			acceptTask;
	int			acceptLeader;
	char		acceptVoice[MAX_NAME_LENGTH];

	qboolean	redLocked, blueLocked;
	qboolean	friendsThroughWalls;

	// media
	cgMedia_t		media;
	hudElement_t	hud[HUD_MAX];
} cgs_t;

extern cgs_t			cgs;
extern cg_t				cg;
extern centity_t		cg_entities[MAX_GENTITIES];
extern weaponInfo_t		cg_weapons[MAX_WEAPONS];
extern itemInfo_t		cg_items[MAX_ITEMS];
extern markPoly_t		cg_markPolys[MAX_MARK_POLYS];

extern vmCvar_t		cg_centertime;
extern vmCvar_t		cg_swingSpeed;
extern vmCvar_t		cg_shadows;
extern vmCvar_t		cg_gibs;
extern vmCvar_t		cg_drawSnapshot;
extern vmCvar_t		cg_drawAmmoWarning;
extern vmCvar_t		cg_drawCrosshair;
extern vmCvar_t		cg_drawCrosshairNames;
extern vmCvar_t		cg_drawRewards;
extern vmCvar_t		cg_drawTeamOverlay;
extern vmCvar_t		cg_teamOverlayUserinfo;
extern vmCvar_t		cg_crosshairX;
extern vmCvar_t		cg_crosshairY;
extern vmCvar_t		cg_crosshairSize;
extern vmCvar_t		cg_crosshairHealth;
extern vmCvar_t		cg_crosshairHitColor;
extern vmCvar_t		cg_crosshairHitColorTime;
extern vmCvar_t		cg_crosshairHitColorStyle;
extern vmCvar_t		cg_crosshairHitPulse;
extern vmCvar_t		cg_crosshairPickupPulse;
extern vmCvar_t		cg_drawHud;
extern vmCvar_t		cg_drawScoreboard;
extern vmCvar_t		cg_animSpeed;
extern vmCvar_t		cg_debugAnim;
extern vmCvar_t		cg_debugPosition;
extern vmCvar_t		cg_debugEvents;
extern vmCvar_t		cg_errorDecay;
extern vmCvar_t		cg_nopredict;
extern vmCvar_t		cg_noPlayerAnims;
extern vmCvar_t		cg_showmiss;
extern vmCvar_t		cg_footsteps;
extern vmCvar_t		cg_marks;
extern vmCvar_t		cg_gun_x;
extern vmCvar_t		cg_gun_y;
extern vmCvar_t		cg_gun_z;
extern vmCvar_t		cg_drawGun;
extern vmCvar_t		cg_viewsize;
extern vmCvar_t		cg_tracerChance;
extern vmCvar_t		cg_tracerWidth;
extern vmCvar_t		cg_tracerLength;
extern vmCvar_t		cg_autoswitch;
extern vmCvar_t		cg_simpleItems;
extern vmCvar_t		cg_fov;
extern vmCvar_t		cg_zoomFov;
extern vmCvar_t		cg_thirdPersonRange;
extern vmCvar_t		cg_thirdPersonAngle;
extern vmCvar_t		cg_thirdPerson;
extern vmCvar_t		cg_lagometer;
extern vmCvar_t		cg_drawAttacker;
extern vmCvar_t		cg_synchronousClients;
extern vmCvar_t		cg_chatTime;
extern vmCvar_t		cg_teamChatTime;
extern vmCvar_t		cg_deathNoticeTime;
extern vmCvar_t		cg_stats;
extern vmCvar_t		cg_buildScript;
extern vmCvar_t		cg_paused;
extern vmCvar_t		cg_blood;
extern vmCvar_t		cg_predictItems;
extern vmCvar_t		cg_drawFriend;
extern vmCvar_t		cg_teamChatsOnly;
extern vmCvar_t		cg_scorePlum;
extern vmCvar_t		cg_smoothClients;
extern vmCvar_t		pmove_fixed;
extern vmCvar_t		pmove_msec;
extern vmCvar_t		cg_cameraOrbit;
extern vmCvar_t		cg_cameraOrbitDelay;
extern vmCvar_t		cg_timescaleFadeEnd;
extern vmCvar_t		cg_timescaleFadeSpeed;
extern vmCvar_t		cg_timescale;
extern vmCvar_t		cg_cameraMode;
extern vmCvar_t		cg_trueLightning;
extern vmCvar_t		cg_crosshairColor;
extern vmCvar_t		cg_teamModel;
extern vmCvar_t		cg_teamSoundModel;
extern vmCvar_t		cg_enemyModel;
extern vmCvar_t		cg_enemySoundModel;
extern vmCvar_t		cg_redTeamModel;
extern vmCvar_t		cg_redTeamSoundModel;
extern vmCvar_t		cg_blueTeamModel;
extern vmCvar_t		cg_blueTeamSoundModel;
extern vmCvar_t		cg_forceTeamModels;
extern vmCvar_t		cg_deadBodyDarken;
extern vmCvar_t		cg_deadBodyColor;
extern vmCvar_t		cg_teamHeadColor;
extern vmCvar_t		cg_teamTorsoColor;
extern vmCvar_t		cg_teamLegsColor;
extern vmCvar_t		cg_enemyHeadColor;
extern vmCvar_t		cg_enemyTorsoColor;
extern vmCvar_t		cg_enemyLegsColor;
extern vmCvar_t		cg_redHeadColor;
extern vmCvar_t		cg_redTorsoColor;
extern vmCvar_t		cg_redLegsColor;
extern vmCvar_t		cg_blueHeadColor;
extern vmCvar_t		cg_blueTorsoColor;
extern vmCvar_t		cg_blueLegsColor;
extern vmCvar_t		cg_zoomToggle;
extern vmCvar_t		cg_zoomOutOnDeath;
extern vmCvar_t		cg_zoomScaling;
extern vmCvar_t		cg_zoomSensitivity;
extern vmCvar_t		s_ambient;
extern vmCvar_t		cg_weaponConfig;
extern vmCvar_t		cg_weaponConfig_g;
extern vmCvar_t		cg_weaponConfig_mg;
extern vmCvar_t		cg_weaponConfig_sg;
extern vmCvar_t		cg_weaponConfig_gl;
extern vmCvar_t		cg_weaponConfig_rl;
extern vmCvar_t		cg_weaponConfig_lg;
extern vmCvar_t		cg_weaponConfig_rg;
extern vmCvar_t		cg_weaponConfig_pg;
extern vmCvar_t		cg_weaponConfig_bfg;
extern vmCvar_t		cg_weaponConfig_gh;
extern vmCvar_t		cg_forceWeaponColor;
extern vmCvar_t		cg_teamWeaponColor;
extern vmCvar_t		cg_enemyWeaponColor;
extern vmCvar_t		cg_flatGrenades;
extern vmCvar_t		cg_rocketTrail;
extern vmCvar_t		cg_rocketTrailTime;
extern vmCvar_t		cg_rocketTrailRadius;
extern vmCvar_t		cg_grenadeTrail;
extern vmCvar_t		cg_grenadeTrailTime;
extern vmCvar_t		cg_grenadeTrailRadius;
extern vmCvar_t		cg_railTrail;
extern vmCvar_t		cg_railTrailTime;
extern vmCvar_t		cg_railTrailRadius;
extern vmCvar_t		cg_lightningStyle;
extern vmCvar_t		cg_muzzleFlash;
extern vmCvar_t		cg_lightningExplosion;
extern vmCvar_t		cg_weaponBobbing;
extern vmCvar_t		cg_switchOnEmpty;
extern vmCvar_t		cg_switchToEmpty;
extern vmCvar_t		cg_killbeep;
extern vmCvar_t		cg_drawSpawnpoints;
extern vmCvar_t		cg_drawBBox;
extern vmCvar_t		cg_itemFX;
extern vmCvar_t		cg_wallhack;
extern vmCvar_t		cg_hitBeep;

//
// cg_main.c
//
const char	*CG_ConfigString(int index);
void QDECL	CG_Printf(const char *msg, ...) __attribute__ ((format (printf, 1, 2)));
void QDECL	CG_Error(const char *msg, ...) __attribute__ ((noreturn, format (printf, 1, 2)));
void		CG_UpdateCvars(void);
int			CG_CrosshairPlayer(void);
int			CG_LastAttacker(void);
void		CG_LoadMenus(const char *menuFile);
void		CG_EventHandling(int type);
void		CG_BuildSpectatorString(void);

//
// cg_view.c
//
void	CG_TestModel_f (void);
void	CG_TestGun_f (void);
void	CG_TestModelNextFrame_f (void);
void	CG_TestModelPrevFrame_f (void);
void	CG_TestModelNextSkin_f (void);
void	CG_TestModelPrevSkin_f (void);
void	CG_ZoomDown_f(void);
void	CG_ZoomUp_f(void);
void	CG_AddBufferedSound(sfxHandle_t sfx);
void	CG_DrawActiveFrame(int serverTime, stereoFrame_t stereoView, qboolean demoPlayback);

//
// cg_drawtools.c
//
void	CG_AdjustFrom640(float *x, float *y, float *w, float *h);
float	CG_AdjustWidth(float width);
void	CG_FillRect(float x, float y, float width, float height, const float *color);
void	CG_DrawPic(float x, float y, float width, float height, qhandle_t hShader);
void	CG_DrawAdjustPic(float x, float y, float width, float height, qhandle_t hShader);
void	CG_DrawString(float x, float y, const char *string, 
			float charWidth, float charHeight, const float *modulate);
void	CG_DrawStringExt(float x, float y, const char *string, const float *setColor, 
			qboolean forceColor, qboolean shadow, float charWidth, float charHeight, int maxChars);
void	CG_DrawBigString(float x, float y, const char *s, float alpha);
void	CG_DrawBigStringColor(float x, float y, const char *s, vec4_t color);
void	CG_DrawSmallString(float x, float y, const char *s, float alpha);
void	CG_DrawSmallStringColor(float x, float y, const char *s, vec4_t color);
float	CG_StringWidth(float size, const char *str);
int		CG_DrawStrlen(const char *str);
float	*CG_FadeColor(int startMsec, int totalMsec);
float	*CG_TeamColor(int team);
void	CG_TileClear(void);
void	UI_DrawProportionalString(int x, int y, const char* str, int style, vec4_t color);
void	CG_DrawRect(float x, float y, float width, float height, float size, const float *color);
void	CG_SetRGBA(byte incolor[4], vec4_t color);
int		CG_ParseColor(vec4_t incolor, const char *str);

//
// cg_draw.c
//
extern int		sortedTeamPlayers[TEAM_MAXOVERLAY];
extern int		numSortedTeamPlayers;
extern int		drawTeamOverlayModificationCount;
extern char		systemChat[256];
extern char		teamChat1[256];
extern char		teamChat2[256];

void		CG_AddLagometerFrameInfo(void);
void		CG_AddLagometerSnapshotInfo(snapshot_t *snap);
void		CG_CenterPrint(const char *str);
void		CG_DrawActive(stereoFrame_t stereoView);
void		CG_DrawFlagModel(float x, float y, float w, float h, int team, qboolean force2D);
void		CG_OwnerDraw(float x, float y, float w, float h, float text_x, float text_y, int ownerDraw, int ownerDrawFlags,
				int align, float special, float scale, vec4_t color, qhandle_t shader, int textStyle);
void		CG_Text_Paint(float x, float y, float scale, vec4_t color, const char *text, float adjust, int limit, int style);
int			CG_Text_Width(const char *text, float scale, int limit);
int			CG_Text_Height(const char *text, float scale, int limit);
void		CG_SelectPrevPlayer(void);
void		CG_SelectNextPlayer(void);
float		CG_GetValue(int ownerDraw);
qboolean	CG_OwnerDrawVisible(int flags);
void		CG_RunMenuScript(char **args);
void		CG_ShowResponseHead(void);
void		CG_SetPrintString(int type, const char *p);
void		CG_InitTeamChat(void);
void		CG_GetTeamColor(vec4_t *color);
const char	*CG_GetGameStatusText(void);
const char	*CG_GetKillerText(void);
void		CG_Draw3DModel(float x, float y, float w, float h, qhandle_t model, qhandle_t skin, vec3_t origin, vec3_t angles);
void		CG_Text_PaintChar(float x, float y, float width, float height, float scale,
				float s, float t, float s2, float t2, qhandle_t hShader);
void		CG_CheckOrderPending(void);
const char	*CG_GameTypeString(void);
qboolean	CG_YourTeamHasFlag(void);
qboolean	CG_OtherTeamHasFlag(void);
qhandle_t	CG_StatusHandle(int task);

//
// cg_hud.c, cg_hudparser.c
//
void		CG_DrawHud(void);
void		CG_HudEdit_f( void );
void		CG_ClearHud( void );
void		CG_HudInit(void);
void		CG_Hud_f(void);

//
// cg_players.c
//
void		CG_Player(centity_t *cent);
void		CG_ResetPlayerEntity(centity_t *cent);
void		CG_AddRefEntityWithPowerups(refEntity_t *ent, entityState_t *state, int team);
int			CG_LoadCvarModel(const char *cvarName, vmCvar_t *cvar);
void		CG_NewClientInfo(int clientNum);
void		CG_ForceModelChange(void);
sfxHandle_t	CG_CustomSound(int clientNum, const char *soundName);

//
// cg_predict.c
//
void	CG_BuildSolidList(void);
int		CG_PointContents(const vec3_t point, int passEntityNum);
void	CG_Trace(trace_t *result, const vec3_t start, const vec3_t mins, const vec3_t maxs, const vec3_t end, int skipNumber, int mask);
void	CG_PredictPlayerState(void);

//
// cg_events.c
//
void		CG_CheckEvents(centity_t *cent);
const char	*CG_PlaceString(int rank);
void		CG_PainEvent(centity_t *cent, int health);
void		CG_EntityEvent(centity_t *cent, vec3_t position);

//
// cg_ents.c
//
void	CG_SetEntitySoundPosition(centity_t *cent);
void	CG_AddPacketEntities(void);
void	CG_Beam(centity_t *cent);
void	CG_AdjustPositionForMover(const vec3_t in, int moverNum, int fromTime, int toTime, vec3_t out, vec3_t angles_in, vec3_t angles_out);
void	CG_PositionEntityOnTag(refEntity_t *entity, const refEntity_t *parent, qhandle_t parentModel, char *tagName);
void	CG_PositionRotatedEntityOnTag(refEntity_t *entity, const refEntity_t *parent, qhandle_t parentModel, char *tagName);

//
// cg_weapons.c
//
void	CG_GetWeaponColor(vec4_t color, clientInfo_t *ci, weaponColor_t wpcolor);
void	CG_RunWeaponScript(void);
void	CG_NextWeapon_f(void);
void	CG_PrevWeapon_f(void);
void	CG_Weapon_f(void);

void	CG_RegisterItem(int itemNum);

void	CG_FireWeapon(centity_t *cent);
void	CG_MissileHitWall(int weapon, int clientNum, vec3_t origin, vec3_t dir, impactSound_t soundType);
void	CG_MissileHitPlayer(int weapon, vec3_t origin, vec3_t dir, int entityNum);
void	CG_ShotgunFire(entityState_t *es);
void	CG_ShotgunPattern(vec3_t origin, vec3_t origin2, int seed, int otherEntNum);
void	CG_Bullet(vec3_t origin, int sourceEntityNum, vec3_t normal, qboolean flesh, int fleshEntityNum);

void	CG_RailTrail(clientInfo_t *ci, vec3_t start, vec3_t end);
void	CG_GrappleTrail(centity_t *ent, const weaponInfo_t *wi);
void	CG_AddViewWeapon (playerState_t *ps);
void	CG_AddPlayerWeapon(refEntity_t *parent, playerState_t *ps, centity_t *cent, int team);
void	CG_OutOfAmmoChange(void);

//
// cg_marks.c
//
void	CG_InitMarkPolys(void);
void	CG_AddMarks(void);
void	CG_ImpactMark(qhandle_t markShader, const vec3_t origin, const vec3_t dir,
			float orientation, vec4_t color, qboolean alphaFade, float radius, qboolean temporary);

//
// cg_localents.c
//
void			CG_InitLocalEntities(void);
localEntity_t	*CG_AllocLocalEntity(void);
void			CG_AddLocalEntities(void);

//
// cg_effects.c
//
localEntity_t	*CG_SmokePuff(const vec3_t p, const vec3_t vel, float radius, vec4_t color,
	float duration, int startTime, int fadeInTime, int leFlags, qhandle_t hShader);
localEntity_t	*CG_MakeExplosion(vec3_t origin, vec3_t dir,
	qhandle_t hModel, qhandle_t shader, int msec, qboolean isSprite);

void	CG_BubbleTrail(vec3_t start, vec3_t end, float spacing);
void	CG_SpawnEffect(vec3_t org);
void	CG_ScorePlum(int client, vec3_t org, int score);
void	CG_GibPlayer(vec3_t playerOrigin);
void	CG_BigExplode(vec3_t playerOrigin);
void	CG_Bleed(vec3_t origin, int entityNum);

//
// cg_snapshot.c
//
void	CG_ProcessSnapshots(void);

//
// cg_info.c
//
void	CG_LoadingItem(int itemNum);
void	CG_DrawInformation(void);

//
// cg_scoreboard.c
//
qboolean	CG_DrawScoreboard(void);
qboolean	CG_DrawStatboard(void);

//
// cg_consolecmds.c
//
qboolean	CG_ConsoleCommand(void);
void		CG_InitConsoleCommands(void);

//
// cg_servercmds.c
//
void	CG_ExecuteNewServerCommands(int latestSequence);
void	CG_ParseServerinfo(void);
void	CG_SetConfigValues(void);
void	CG_ShaderStateChanged(void);

//
// cg_playerstate.c
//
void	CG_Respawn(void);
void	CG_TransitionPlayerState(playerState_t *ps, playerState_t *ops);
void	CG_CheckChangedPredictableEvents(playerState_t *ps);

//
// cg_particles.c
//
void	CG_ClearParticles (void);
void	CG_AddParticles (void);
void	CG_ParticleSnow (qhandle_t pshader, vec3_t origin, vec3_t origin2, int turb, float range, int snum);
void	CG_ParticleSmoke (qhandle_t pshader, centity_t *cent);
void	CG_AddParticleShrapnel (localEntity_t *le);
void	CG_ParticleSnowFlurry (qhandle_t pshader, centity_t *cent);
void	CG_ParticleBulletDebris (vec3_t	org, vec3_t vel, int duration);
void	CG_ParticleSparks (vec3_t org, vec3_t vel, int duration, float x, float y, float speed);
void	CG_ParticleDust (centity_t *cent, vec3_t origin, vec3_t dir);
void	CG_ParticleMisc (qhandle_t pshader, vec3_t origin, int size, int duration, float alpha);
void	CG_ParticleExplosion (char *animStr, vec3_t origin, vec3_t vel, int duration, int sizeStart, int sizeEnd);
int		CG_NewParticleArea (int num);

//
// cg_unlagged.c
//
void CG_AddBoundingBox(centity_t *cent);
void CG_PredictWeaponEffects(centity_t *cent);

//
// system traps
// These functions are how the cgame communicates with the main game system
//

// print message on the local console
void		trap_Print(const char *fmt);

// abort the game
void		trap_Error(const char *fmt) __attribute__((noreturn));

// milliseconds should only be used for performance tuning, never
// for anything game related.  Get time from the CG_DrawActiveFrame parameter
int			trap_Milliseconds(void);

// console variable interaction
void	trap_Cvar_Register(vmCvar_t *vmCvar, const char *varName, const char *defaultValue, int flags);
void	trap_Cvar_Update(vmCvar_t *vmCvar);
void	trap_Cvar_Set(const char *var_name, const char *value);
void	trap_Cvar_VariableStringBuffer(const char *var_name, char *buffer, int bufsize);

// ServerCommand and ConsoleCommand parameter access
int			trap_Argc(void);
void		trap_Argv(int n, char *buffer, int bufferLength);
void		trap_Args(char *buffer, int bufferLength);

// filesystem access
// returns length of file
int			trap_FS_FOpenFile(const char *qpath, fileHandle_t *f, fsMode_t mode);
void		trap_FS_Read(void *buffer, int len, fileHandle_t f);
void		trap_FS_Write(const void *buffer, int len, fileHandle_t f);
void		trap_FS_FCloseFile(fileHandle_t f);
int			trap_FS_Seek(fileHandle_t f, long offset, fsOrigin_t origin);
int			trap_Ini_Section(iniSection_t *section, fileHandle_t fp);

// add commands to the local console as if they were typed in
// for map changing, etc.  The command is not executed immediately,
// but will be executed in order the next time console commands
// are processed
void		trap_SendConsoleCommand(const char *text);

// register a command name so the console can perform command completion.
// FIXME: replace this with a normal console command "defineCommand"?
void		trap_AddCommand(const char *cmdName);
void		trap_RemoveCommand(const char *cmdName);

// send a string to the server over the network
void		trap_SendClientCommand(const char *s);

// force a screen update, only used during gamestate load
void		trap_UpdateScreen(void);

// model collision
void		trap_CM_LoadMap(const char *mapname);
int			trap_CM_NumInlineModels(void);
clipHandle_t	trap_CM_InlineModel(int index);		// 0 = world, 1+ = bmodels
clipHandle_t	trap_CM_TempBoxModel(const vec3_t mins, const vec3_t maxs);
int			trap_CM_PointContents(const vec3_t p, clipHandle_t model);
int			trap_CM_TransformedPointContents(const vec3_t p, clipHandle_t model, const vec3_t origin, const vec3_t angles);
void		trap_CM_BoxTrace(trace_t *results, const vec3_t start, const vec3_t end,
				const vec3_t mins, const vec3_t maxs, clipHandle_t model, int brushmask);
void		trap_CM_CapsuleTrace(trace_t *results, const vec3_t start, const vec3_t end,
				const vec3_t mins, const vec3_t maxs, clipHandle_t model, int brushmask);
void		trap_CM_TransformedBoxTrace(trace_t *results, const vec3_t start, const vec3_t end,
				const vec3_t mins, const vec3_t maxs, clipHandle_t model,
				int brushmask, const vec3_t origin, const vec3_t angles);
void		trap_CM_TransformedCapsuleTrace(trace_t *results, const vec3_t start, const vec3_t end,
				const vec3_t mins, const vec3_t maxs, clipHandle_t model,
				int brushmask, const vec3_t origin, const vec3_t angles);

// Returns the projection of a polygon onto the solid brushes in the world
int			trap_CM_MarkFragments(int numPoints, const vec3_t *points, 
				const vec3_t projection, int maxPoints, vec3_t pointBuffer,
				int maxFragments, markFragment_t *fragmentBuffer);

// normal sounds will have their volume dynamically changed as their entity
// moves and the listener moves
void		trap_S_StartSound(vec3_t origin, int entityNum, int entchannel, sfxHandle_t sfx);
void		trap_S_StopLoopingSound(int entnum);

// a local sound is always played full volume
void		trap_S_StartLocalSound(sfxHandle_t sfx, int channelNum);
void		trap_S_ClearLoopingSounds(qboolean killall);
void		trap_S_AddLoopingSound(int entityNum, const vec3_t origin, const vec3_t velocity, sfxHandle_t sfx);
void		trap_S_AddRealLoopingSound(int entityNum, const vec3_t origin, const vec3_t velocity, sfxHandle_t sfx);
void		trap_S_UpdateEntityPosition(int entityNum, const vec3_t origin);

// respatialize recalculates the volumes of sound as they should be heard by the
// given entityNum and position
void		trap_S_Respatialize(int entityNum, const vec3_t origin, vec3_t axis[3], int inwater);
sfxHandle_t	trap_S_RegisterSound(const char *sample, qboolean compressed);		// returns buzz if not found

void		trap_R_LoadWorldMap(const char *mapname);

// all media should be registered during level startup to prevent
// hitches during gameplay
qhandle_t	trap_R_RegisterModel(const char *name);			// returns rgb axis if not found
qhandle_t	trap_R_RegisterSkin(const char *name);			// returns all white if not found
qhandle_t	trap_R_RegisterShader(const char *name);			// returns all white if not found
qhandle_t	trap_R_RegisterShaderNoMip(const char *name);			// returns all white if not found

// a scene is built up by calls to R_ClearScene and the various R_Add functions.
// Nothing is drawn until R_RenderScene is called.
void		trap_R_ClearScene(void);
void		trap_R_AddRefEntityToScene(const refEntity_t *re);

// polys are intended for simple wall marks, not really for doing
// significant construction
void		trap_R_AddPolyToScene(qhandle_t hShader , int numVerts, const polyVert_t *verts);
void		trap_R_AddPolysToScene(qhandle_t hShader , int numVerts, const polyVert_t *verts, int numPolys);
void		trap_R_AddLightToScene(const vec3_t org, float intensity, float r, float g, float b);
void		trap_R_AddAdditiveLightToScene(const vec3_t org, float intensity, float r, float g, float b);
int			trap_R_LightForPoint(vec3_t point, vec3_t ambientLight, vec3_t directedLight, vec3_t lightDir);
void		trap_R_RenderScene(const refdef_t *fd);
void		trap_R_SetColor(const float *rgba);	// NULL = 1,1,1,1
void		trap_R_DrawStretchPic(float x, float y, float w, float h, 
				float s1, float t1, float s2, float t2, qhandle_t hShader);
void		trap_R_ModelBounds(clipHandle_t model, vec3_t mins, vec3_t maxs);
int			trap_R_LerpTag(orientation_t *tag, clipHandle_t mod, int startFrame, int endFrame, 
				float frac, const char *tagName);
void		trap_R_RemapShader(const char *oldShader, const char *newShader, const char *timeOffset);
qboolean	trap_R_inPVS(const vec3_t p1, const vec3_t p2);

// The glconfig_t will not change during the life of a cgame.
// If it needs to change, the entire cgame will be restarted, because
// all the qhandle_t are then invalid.
void		trap_GetGlconfig(glconfig_t *glconfig);

// the gamestate should be grabbed at startup, and whenever a
// configstring changes
void		trap_GetGameState(gameState_t *gamestate);

// cgame will poll each frame to see if a newer snapshot has arrived
// that it is interested in.  The time is returned seperately so that
// snapshot latency can be calculated.
void		trap_GetCurrentSnapshotNumber(int *snapshotNumber, int *serverTime);

// a snapshot get can fail if the snapshot (or the entties it holds) is so
// old that it has fallen out of the client system queue
qboolean	trap_GetSnapshot(int snapshotNumber, snapshot_t *snapshot);

// retrieve a text command from the server stream
// the current snapshot will hold the number of the most recent command
// qfalse can be returned if the client system handled the command
// argc() / argv() can be used to examine the parameters of the command
qboolean	trap_GetServerCommand(int serverCommandNumber);

// returns the most recent command number that can be passed to GetUserCmd
// this will always be at least one higher than the number in the current
// snapshot, and it may be quite a few higher if it is a fast computer on
// a lagged connection
int			trap_GetCurrentCmdNumber(void);	

qboolean	trap_GetUserCmd(int cmdNumber, usercmd_t *ucmd);

// used for the weapon select and zoom
void		trap_SetUserCmdValue(int stateValue, float sensitivityScale);

// aids for VM testing
void		testPrintInt(char *string, int i);
void		testPrintFloat(char *string, float f);

int			trap_MemoryRemaining(void);
void		trap_R_RegisterFont(const char *fontName, int pointSize, fontInfo_t *font);
qboolean	trap_Key_IsDown(int keynum);
int			trap_Key_GetCatcher(void);
void		trap_Key_SetCatcher(int catcher);
int			trap_Key_GetKey(const char *binding);

int			trap_CIN_PlayCinematic(const char *arg0, int xpos, int ypos, int width, int height, int bits);
e_status	trap_CIN_StopCinematic(int handle);
e_status	trap_CIN_RunCinematic (int handle);
void		trap_CIN_DrawCinematic (int handle);
void		trap_CIN_SetExtents (int handle, int x, int y, int w, int h);

int			trap_RealTime(qtime_t *qtime);
void		trap_SnapVector(float *v);

qboolean	trap_loadCamera(const char *name);
void		trap_startCamera(int time);
qboolean	trap_getCameraInfo(int time, vec3_t *origin, vec3_t *angles);

qboolean	trap_GetEntityToken(char *buffer, int bufferSize);


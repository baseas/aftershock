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
// g_user.c

#include "g_local.h"

// referee rights
#define REF_RIGHTS "allready passvote cancelvote put allready lock unlock mute unmute kick"

typedef struct {
	char	name[32];
	char	password[32];
	char	rights[64];
} user_t;

static int		userCount;
static user_t	users[32];

static qboolean G_UserExists(const char *name)
{
	int	i;
	for (i = 0; i < userCount; ++i) {
		if (!strcmp(users[i].name, name)) {
			return qtrue;
		}
	}
	return qfalse;
}

static qboolean G_UserActionAllowed(const char *rights, const char *action)
{
	char		*token;
	qboolean	allowed;

	allowed = qfalse;
	while ((token = COM_Parse((char **) action))) {
		if (!strcmp(token, "all")) {
			allowed = qtrue;
		} else if (!strcmp(token, "referee") && G_UserActionAllowed(REF_RIGHTS, action)) {
			allowed = qtrue;
		}

		if (!strcmp(token, action)) {
			allowed = qtrue;
		} else if (token[0] == '-' && !strcmp(token + 1, action)) {
			allowed = qfalse;
		}
	}
	return allowed;
}

/**
AdjustUser only allows to append rights to require less typing.
So we need to clean up the rights string.
*/
static void G_UserCleanRights(user_t *user)
{
	char		cleanRights[MAX_STRING_CHARS];
	char		*token;
	qboolean	cleanAllowed, dirtyAllowed;

	cleanRights[0] = '\0';

	while ((token = COM_Parse((char **) &user->rights))) {
		if (token[0] == '-') {
			token++;
		}

		cleanAllowed = G_UserActionAllowed(cleanRights, token);
		dirtyAllowed = G_UserActionAllowed(user->rights, token);

		if (!cleanAllowed && dirtyAllowed) {
			Q_strcat(cleanRights, sizeof cleanRights, token);
		} else if (cleanAllowed && !dirtyAllowed) {
			Q_strcat(cleanRights, sizeof cleanRights, "-");
			Q_strcat(cleanRights, sizeof cleanRights, token);
		}
	}
}

static void G_UserWrite(void)
{
	int				i;
	const int		pad = 10;
	fileHandle_t	fp;

	trap_FS_FOpenFile("user.ini", &fp, FS_WRITE);

	for (i = 0; i < userCount; ++i) {
		Ini_WriteLabel("user", i == 0, fp);
		Ini_WriteString("name", users[i].name, pad, fp);
		Ini_WriteString("password", users[i].password, pad, fp);
		Ini_WriteString("rights", users[i].rights, pad, fp);
	}

	trap_FS_Write("\n", 1, fp);
	trap_FS_FCloseFile(fp);
}

static int G_UserId(gentity_t *ent, const char *id)
{
	int	i;
	int	userid;

	if (Q_isanumber(id)) {
		userid = atoi(id);
		if (userid < 0 || userid >= userCount) {
			ClientPrint(ent, "Invalid user id.");
			return -1;
		}
		return userid;
	}

	for (i = 0; i < userCount; ++i) {
		if (!strcmp(id, users[i].name)) {
			return i;
		}
	}

	ClientPrint(ent, "There is no user with the name %s^7", id);
	return -1;
}

static void G_UserList(gentity_t *ent)
{
	int	i;

	// TODO last seen column

	if (!userCount) {
		ClientPrint(ent, "There are no registered users.");
		return;
	}

	ClientPrint(ent, "User list:");
	ClientPrint(ent, "name               rights");
	ClientPrint(ent, "--------------------------");
	for (i = 0; i < userCount; ++i) {
		ClientPrint(ent, "%-16s %-16s", users[i].name, users[i].rights);
	}
}

static void G_UserAdd(gentity_t *ent)
{
	int			i;
	const char	*right;

	if (userCount == ARRAY_LEN(users)) {
		ClientPrint(ent, "Too many users.");
		return;
	}

	if (trap_Argc() < 4) {
		ClientPrint(ent, "Usage: user add <username> <password> [rights]");
		return;
	}

	if (G_UserExists(users[userCount].name)) {
		ClientPrint(ent, "There already exists a user with this name.");
		return;
	}

	Q_strncpyz(users[userCount].name, BG_Argv(2), sizeof users[0].name);
	Q_strncpyz(users[userCount].password, BG_Argv(3), sizeof users[0].password);

	// concat all following words to the rights string
	users[userCount].rights[0] = '\0';
	i = 4;
	while (*(right = BG_Argv(i++))) {
		if (users[userCount].rights[0]) {
			Q_strcat(users[userCount].rights, sizeof users[0].rights, " ");
		}
		Q_strcat(users[userCount].rights, sizeof users[0].rights, right);
	}

	ClientPrint(ent, "The user has been added, user id: %d", userCount);
	userCount++;
	G_UserWrite();
}

static void G_UserAdjust(gentity_t *ent)
{
	int			userid;
	const char	*field, *value;

	if (trap_Argc() < 3) {
		ClientPrint(ent, "Usage: user adjust <userid> (name | password | rights) <value>");
		return;
	}

	if ((userid = G_UserId(ent, BG_Argv(2)) < 0)) {
		return;
	}

	field = BG_Argv(3);
	value = BG_Argv(4);

	if (!strcmp(field, "name")) {
		if (!*value) {
			ClientPrint(ent, "%s", users[userid].name);
			return;
		}
		Q_strncpyz(users[userid].name, value, sizeof users[0].name);
	} else if (!strcmp(field, "password")) {
		if (!*value) {
			ClientPrint(ent, "Specify a password.");
			return;
		}
		Q_strncpyz(users[userid].password, value, sizeof users[0].password);
	} else if (!strcmp(field, "rights")) {
		if (!*value) {
			ClientPrint(ent, "%s", users[userid].rights);
			return;
		}
		Q_strncpyz(users[userid].rights, value, sizeof users[0].rights);
		G_UserCleanRights(&users[userid]);
	} else {
		ClientPrint(ent, "Valid fields are: name, password, rights.");
		return;
	}

	G_UserWrite();
}

static void G_UserDel(gentity_t *ent)
{
	int	i;
	int	userid;

	if ((userid = G_UserId(ent, BG_Argv(2))) < 0) {
		return;
	}

	for (i = userid; i < userCount - 1; ++i) {
		users[i] = users[i + 1];
	}

	userCount--;
	G_UserWrite();
}

void G_UserRead(void)
{
	fileHandle_t	fp;
	iniSection_t	section;
	user_t			*user;
	char			*value;

	userCount = 0;

	trap_FS_FOpenFile("user.ini", &fp, FS_READ);
	if (!fp) {
		return;
	} else {
		G_LogPrintf("Reading users ...\n");
	}

	while (!trap_Ini_Section(&section, fp)) {
		if (strcmp(section.label, "user")) {
			G_Printf("Users: Invalid section in vote.ini.\n");
			continue;
		}

		if (userCount >= ARRAY_LEN(users) - 1) {
			G_Printf("Users: Too many users.\n");
			break;
		}

		user = &users[userCount];

		value = Ini_GetValue(&section, "name");

		if (G_UserExists(value)) {
			G_Printf("Users: Duplicate name %s^7.\n", value);
			continue;
		}

		if (!value) {
			G_Printf("Users: No name specified.\n");
			continue;
		}
		Q_strncpyz(user->name, value, sizeof user->name);

		value = Ini_GetValue(&section, "password");
		if (!value) {
			G_Printf("Users: No password specified.\n");
			continue;
		}
		Q_strncpyz(user->password, value, sizeof user->password);

		userCount++;
	}

	trap_FS_FCloseFile(fp);
}

qboolean G_UserAllowed(gentity_t *ent, const char *action)
{
	const char	*rights;

	if (ent->client->pers.localClient) {
		return qtrue;
	}

	if (ent->client->userid < 1 || ent->client->userid > userCount) {
		rights = NULL;
	} else {
		rights = users[ent->client->userid - 1].rights;
	}

	if ((rights && !G_UserActionAllowed(rights, action)) && !G_UserActionAllowed("", action)) {
		ClientPrint(ent, "You are not allowed to use the command \"%s\".", action);
		return qfalse;
	}

	return qfalse;
}

const char *G_UserName(gentity_t *ent)
{
	if (ent == g_entities) {
		return "^2console^7";
	}

	if (!ent->client->userid) {
		return va("%s^7", ent->client->pers.netname);
	}

	return va("%s^7 (logged in as %s^7)", ent->client->pers.netname, users[ent->client->userid - 1].name);
}

void Cmd_UserTest_f(gentity_t *ent)
{
	if (!ent->client->userid) {
		ClientPrint(NULL, "^2%s ^2wants to brag with his rights, but is not logged in.",
			ent->client->pers.netname);
		return;
	}

	if (ent->client->pers.localClient) {
		ClientPrint(NULL, "^2%s ^7is a local client and has all administrator rights.", G_UserName(ent));
		return;
	}

	ClientPrint(NULL, "^2%s ^2has the following rights: %s",
		G_UserName(ent), users[ent->client->userid].rights);
}

void Cmd_Login_f(gentity_t *ent)
{
	int	i;
	const char	*name, *password;

	if (trap_Argc() != 3) {
		ClientPrint(ent, "Usage: login <username> <password>");
		return;
	}

	name = BG_Argv(1);
	password = BG_Argv(2);

	if (!G_UserExists(name)) {
		ClientPrint(ent, "No user with this name exists.");
		return;
	}

	for (i = 0; i < userCount; ++i) {
		if (strcmp(users[i].password, password)) {
			ClientPrint(ent, "Wrong password.");
			return;
		}

		// userid is shifted by 1, because for user who are not logged in, we have userid = 0
		ent->client->userid = i + 1;

		ClientPrint(ent, "Welcome, %s.", name);
		return;
	}
}

void Cmd_User_f(gentity_t *ent)
{
	const char	*cmd;

	cmd = BG_Argv(1);

	if (!strcmp(cmd, "list")) {
		G_UserList(ent);
	} else if (!strcmp(cmd, "add")) {
		G_UserAdd(ent);
	} else if (!strcmp(cmd, "adjust")) {
		G_UserAdjust(ent);
	} else if (!strcmp(cmd, "del")) {
		G_UserDel(ent);
	} else {
		ClientPrint(ent, "Usage: user (list | add | adjust | del) <player or userid>");
	}
}


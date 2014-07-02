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

#define REF_PERMISSION	"passvote kick"

typedef struct {
	char	name[32];
	char	password[32];
	char	permissions[64];
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

void G_UserRead(void)
{
	fileHandle_t	fp;
	iniSection_t	section;
	user_t			*user;
	char			*value;

	userCount = 0;

	trap_FS_FOpenFile("users.ini", &fp, FS_READ);
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

qboolean G_UserHasPermission(gclient_t *client, const char *perm)
{
	char	*token;

	if (client->userid < 0 || client->userid > userCount) {
		return qfalse;
	}

	while ((token = COM_Parse((char **) &users[client->userid - 1].permissions))) {
		if (!strcmp(token, perm)) {
			return qtrue;
		}
	}

	return qfalse;
}

void G_UserLogin(gclient_t *client, const char *name, const char *password)
{
	int	i;
	for (i = 0; i < userCount; ++i) {
		if (!G_UserExists(name)) {
			ClientPrint(&g_entities[client - level.clients], "No user with this name exists.");
			return;
		}
		if (!strcmp(users[i].password, password)) {
			ClientPrint(&g_entities[client - level.clients], "Wrong password.");
			return;
		}

		// userid is shifted by 1, because for user who are not logged in, we have userid = 0
		client->userid = i + 1;
	}
}

void G_UserList(void)
{
	int	i;

	// TODO last seen column

	G_Printf("User list:\n");
	G_Printf("name           permissions\n");
	G_Printf("--------------------------\n");
	for (i = 0; i < userCount; ++i) {
		G_Printf("%s %s", users[i].name, users[i].permissions);
	}
}

void G_UserWrite(void)
{
	int				i;
	const int		pad = 10;
	fileHandle_t	fp;

	trap_FS_FOpenFile("user.ini", &fp, FS_WRITE);

	for (i = 0; i < userCount; ++i) {
		Ini_WriteLabel("user", i == 0, fp);
		Ini_WriteString("name", users[i].name, pad, fp);
		Ini_WriteString("password", users[i].password, pad, fp);
		Ini_WriteString("permissions", users[i].permissions, pad, fp);
	}

	trap_FS_Write("\n", 1, fp);
	trap_FS_FCloseFile(fp);
}

void G_UserDel(int userid)
{
	int	i;

	if (userid < 1 || userid > userCount) {
		return;
	}

	for (i = userid; i < userCount - 1; ++i) {
		users[i] = users[i + 1];
	}

	userCount--;
}


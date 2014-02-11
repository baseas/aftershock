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
// ini.c

#include "../qcommon/q_shared.h"
#include "../qcommon/qcommon.h"

#define INI_LINE_LENGTH		256

/**
INI file parser
Only uses stack memory and uses Ini_ReadLine instead of fgets.

Format:
	# comment
	[section-label]
	key:	value
			line2

Comments start with # and there may not be other characters before
the # on that line.
Multi line values are supported.
*/

/**
Seek end of line and return length of line.
*/
static int Ini_SeekLineEnd(fileHandle_t fp)
{
	int		read;
	int		lineLength;
	char	tmp[256];
	char	*nl;

	while ((read = FS_Read(tmp, sizeof tmp, fp))) {
		nl = strchr(tmp, '\n');
		if (!nl) {
			lineLength += read;
			continue;
		}
		FS_Seek(fp, nl - tmp, FS_SEEK_CUR);
	}

	return lineLength;
}

/**
Read one line into buffer and seek to end of line.
Lines longer than length are cut.  Returns length of line.
*/
static int Ini_ReadLine(char *buffer, int length, fileHandle_t fp)
{
	int	read;
	char *nl;

	read = FS_Read(buffer, length, fp);
	if (!read) {
		return -1;
	}

	buffer[MIN(read, length)] = '\0';

	nl = strchr(buffer, '\n');
	if (!nl) {
		Com_Printf("Ini parser: line '%.10s ...' too long.\n", buffer);
		return read + Ini_SeekLineEnd(fp);
	}
	*nl = '\0';

	// seek new line start
	FS_Seek(fp, nl - buffer - read + 1, FS_SEEK_CUR);
	return nl - buffer + 1;
}

/**
Check if the line if empty, i.e. only contains whitespaces.
*/
static qboolean Ini_EmptyLine(const char *line)
{
	const char	*c;
	for (c = line; *c; ++c) {
		if (!isspace(*c)) {
			return qfalse;
		}
	}
	return qtrue;
}

/**
Remove whitespace at end of line.
*/
static void Ini_TrimEnd(char *line)
{
	int	i;
	for (i = strlen(line) - 1; i > 0; --i) {
		if (!isspace(line[i])) {
			return;
		}
		line[i] = '\0';
	}
}

/**
Trim leading and trailing spaces from buffer and append it to the last value of the section.
*/
static void Ini_AppendValue(iniSection_t *section, const char *line)
{
	char	*valp;

	if (section->numItems == 0) {
		Com_Printf("Ini parser: Invalid line with leading spaces: '%10s'.\n", line);
		return;
	}

	while (isspace(*line)) {
		++line;
	}

	valp = section->vals[section->numItems - 1];
	Q_strcat(valp, INI_VAL_LENGTH, "\n");
	Q_strcat(valp, INI_VAL_LENGTH, line);
	Ini_TrimEnd(valp);
}

/**
Parses `line` and appends the key, value pair to the section.
*/
static void Ini_ReadPair(iniSection_t *section, const char *line)
{
	char	*colon;
	char	*index;

	if (section->numItems >= INI_MAX_ITEMS) {
		return;
	}

	colon = strchr(line, ':');
	if (!colon) {
		Com_Printf("Ini parser: No colon found in section '%s': '%10s'.\n", section->label, line);
		return;
	}

	index = colon;

	do {
		--index;
	} while (isspace(*index));

	if (colon == line) {
		Com_Printf("Ini parser: Empty key in section '%s': '%10s'", section->label, line);
		return;
	}

	// need +2 instead of +1 because of forced string terminator
	Q_strncpyz(section->keys[section->numItems], line, MIN(index - line + 2, INI_KEY_LENGTH));

	if (Ini_GetValue(section, section->keys[section->numItems])) {
		Com_Printf("Ini parser: Duplicate key '%s' in section '%s'.\n",
			section->keys[section->numItems], section->label);
		return;
	}

	index = colon;
	do {
		++index;
	} while (isspace(*index));

	Q_strncpyz(section->vals[section->numItems], index, INI_VAL_LENGTH);
	Ini_TrimEnd(section->vals[section->numItems]);

	++section->numItems;
}

/**
Read the section label from `line` into `section`.
Format is [label]. In case of error, set label to empty string.
*/
static void Ini_ReadLabel(iniSection_t *section, const char *line)
{
	const char	*start, *end;

	for (start = line + 1; *start; ++start) {
		if (!isspace(*start)) {
			break;
		}
	}

	end = strchr(start, ']');

	if (!end) {
		Com_Printf("Ini parser: Expected ] not found in '%s'.\n", line);
		section->label[0] = '\0';
		return;
	}

	if (end == start) {
		Com_Printf("Ini parser: Empty section label found in '%s'.\n", line);
		section->label[0] = '\0';
		return;
	}

	Q_strncpyz(section->label, start, sizeof section->label);
	section->label[end - start] = '\0';
	Ini_TrimEnd(section->label);

	if (!Ini_EmptyLine(end + 1)) {
		Com_Printf("Ini parser: Invalid characters after label in '%s'.\n", line);
	}
}

/**
Parse one section and advance the file pointer to the end
of the section. Return value is zero is the end of file has not been reached.
*/
int Ini_Section(iniSection_t *section, fileHandle_t fp)
{
	qboolean	sectionFound;
	char		buffer[INI_LINE_LENGTH];
	int			lineLength;

	memset(section, 0, sizeof (iniSection_t));

	sectionFound = qfalse;

	while ((lineLength = Ini_ReadLine(buffer, sizeof buffer, fp)) >= 0) {
		if (buffer[0] == '#' || Ini_EmptyLine(buffer)) {
			continue;
		} else if (buffer[0] == '[' && !sectionFound) {
			sectionFound = qtrue;
			Ini_ReadLabel(section, buffer);
		} else if (buffer[0] == '[' && sectionFound) {
			// go back to start of line
			FS_Seek(fp, -lineLength, FS_SEEK_CUR);
			return 0;
		} else if (!sectionFound) {
			Com_Printf("Ini parser: Invalid line '%10s'.\n", buffer);
		} else if (!isspace(buffer[0])) {
			Ini_ReadPair(section, buffer);
		} else {
			Ini_AppendValue(section, buffer);
		}

		if (section->numItems >= INI_MAX_ITEMS) {
			Com_Printf("Ini parser: Too many items in section '%s'.\n", section->label);
			break;
		}
	}

	// no section found and reached end of file
	if (!sectionFound) {
		return 1;
	}

	return 0;
}


/*  This file is part of PKSM
>	Copyright (C) 2016/2017 Bernardo Giordano
>
>   This program is free software: you can redistribute it and/or modify
>   it under the terms of the GNU General Public License as published by
>   the Free Software Foundation, either version 3 of the License, or
>   (at your option) any later version.
>
>   This program is distributed in the hope that it will be useful,
>   but WITHOUT ANY WARRANTY; without even the implied warranty of
>   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
>   GNU General Public License for more details.
>
>   You should have received a copy of the GNU General Public License
>   along with this program.  If not, see <http://www.gnu.org/licenses/>.
>   See LICENSE for information.
*/

#include "scripts.h"

static const char* magic = "PKSMSCRIPT";

static char* getBaseScriptPath(void)
{
	if (game_getisUSUM())
	{
		return "/3ds/PKSM/scripts/usum/";
	}
	else if (game_getisSUMO())
	{
		return "/3ds/PKSM/scripts/sm/";
	}
	else if (game_getisORAS())
	{
		return "/3ds/PKSM/scripts/oras/";
	}
	else if (game_getisXY())
	{
		return "/3ds/PKSM/scripts/xy/";
	}
	else if (game_getisB2W2())
	{
		return "/3ds/PKSM/scripts/b2w2/";
	}
	else if (game_getisBW())
	{
		return "/3ds/PKSM/scripts/bw/";
	}
	else if (game_getisHGSS())
	{
		return "/3ds/PKSM/scripts/hgss/";
	}
	else if (game_getisPT())
	{
		return "/3ds/PKSM/scripts/pt/";
	}
	else if (game_getisDP())
	{
		return "/3ds/PKSM/scripts/dp/";
	}
	
	return " ";
}

static Result getScriptList(Script_s **scriptList, int *count)
{
	Result res = 0;
	Handle handle;
	FS_Archive archiveSDMC;
	char* basepath = getBaseScriptPath();
	
	res = FSUSER_OpenArchive(&archiveSDMC, ARCHIVE_SDMC, fsMakePath(PATH_EMPTY, ""));
	if (R_FAILED(res))
	{
		return res;
	}
	res = FSUSER_OpenDirectory(&handle, archiveSDMC, fsMakePath(PATH_ASCII, basepath));
	
	u32 entries_read = 1;
	while(entries_read)
	{
		FS_DirectoryEntry entry = {0};
		res = FSDIR_Read(handle, &entries_read, 1, &entry);
		if (R_FAILED(res) || entries_read == 0)
		{
			break;
		}
		
		*count += entries_read;
		*scriptList = realloc(*scriptList, (*count)*sizeof(Script_s));
		
		Script_s* script = &(*scriptList)[*count - 1];
		memset(script, 0, sizeof(Script_s));
		
		u8 name[256] = {0};
		utf16_to_utf8(name, entry.name, 256);
		memcpy(script->name, name, 256);
		
		u16 u16path[256] = {0};
		struacat(u16path, basepath);
		strucat(u16path, entry.name);
		char path[256] = {0};
		utf16_to_utf8((u8*)path, u16path, 256);
		memcpy(script->path, path, 256);
		
		char* ext;
		if ((ext = strrchr(path, '.')) == NULL)
		{
			*count -= 1;
		} 
		else if (strcmp(ext, ".pksm") != 0)
		{
			*count -= 1;
		}
		else
		{
			FILE *fptr = fopen(path, "rt");
			fseek(fptr, 0, SEEK_END);
			u32 size = ftell(fptr);
			fclose(fptr);
			script->size = size;
		}
	}
	
	FSUSER_CloseArchive(archiveSDMC);
	FSDIR_Close(handle);
	return res;
}

static void printScriptMenu(Script_s* scriptList, int count, int entry)
{
	pp2d_begin_draw(GFX_TOP, GFX_LEFT);
		pp2d_draw_rectangle(0, 0, 400, 240, PALEBLUE);
		pp2d_draw_rectangle(0, 0, 400, 20, MENUBLUE);
		pp2d_draw_rectangle(0, 220, 400, 20, MENUBLUE);
		
		char counter[10] = {0};
		sprintf(counter, "%d/%d", entry + 1, count);
		pp2d_draw_text(10, 3, FONT_SIZE_11, FONT_SIZE_11, YELLOW, getBaseScriptPath());
		pp2d_draw_text(390 - pp2d_get_text_width(counter, FONT_SIZE_11, FONT_SIZE_11), 3, FONT_SIZE_11, FONT_SIZE_11, YELLOW, counter);
		
		int offset = entry < 7 ? 0 : entry - 7;
		int total = offset + 8 < count ? offset + 8 : count;
		for (int i = offset; i < total; i++)
		{
			char sizestr[20] = {0};
			const u32 div = scriptList[i].size / 1024;
			if (div < 1)
			{
				sprintf(sizestr, "%ld B", scriptList[i].size);
			}
			else
			{
				sprintf(sizestr, "%ld KiB", div);
			}
			
			if (i == entry)
			{
				printSelector(0, 20 + (i - offset)*25, 400, 25);
			}
			
			pp2d_draw_text(10, 24 + (i - offset)*25, FONT_SIZE_12, FONT_SIZE_12, WHITE, scriptList[i].name);
			pp2d_draw_text(390 - pp2d_get_text_width(sizestr, FONT_SIZE_11, FONT_SIZE_11), 24 + (i - offset) * 25, FONT_SIZE_11, FONT_SIZE_11, WHITE, sizestr);
		}
		
		pp2d_draw_wtext_center(GFX_TOP, 225, FONT_SIZE_9, FONT_SIZE_9, WHITE, i18n(S_SCRIPT_EXECUTE_STRING));
		pp2d_draw_on(GFX_BOTTOM, GFX_LEFT);
		pp2d_draw_rectangle(0, 0, 320, 240, PALEBLUE);
		pp2d_draw_rectangle(0, 0, 320, 20, MENUBLUE);	
		pp2d_draw_rectangle(0, 220, 320, 20, MENUBLUE);	

		printBottomIndications(i18n(S_GRAPHIC_CREDITS_INDICATIONS));
	pp2d_end_draw();
}

bool scriptExecute(u8* mainbuf, const char* path)
{
	FILE *fptr = fopen(path, "rt");
	fseek(fptr, 0, SEEK_END);
	u32 size = ftell(fptr);
	u8 *buf = (u8*)malloc(size);
	rewind(fptr);
	fread(buf, size, 1, fptr);
	fclose(fptr);
	
	if (strstr((char*)buf, magic) != NULL)
	{
		u32 i = strlen(magic);
		while (i < size)
		{
			u32 offset = *(u32*)(buf + i);
			u32 length = *(u32*)(buf + i + 4);
			u32 repeat = *(u32*)(buf + i + 8 + length);
			for (u32 k = 0; k < repeat; k++)
			{
				memcpy(mainbuf + offset + k*length, buf + i + 8, length);
			}
			i += 12 + length;
		}
		
		free(buf);
		return true;
	}
	
	free(buf);
	return false;
}

void scriptMenu(u8* mainbuf)
{
	int entry = 0, count = 0;
	Script_s* scriptList = NULL;
	getScriptList(&scriptList, &count);

	if (count < 0)
	{
		count = 0;
	}
	
	while(aptMainLoop() & !(hidKeysDown() & KEY_B))
	{
		hidScanInput();

		if (hidKeysDown() & KEY_DOWN)
		{
			entry = entry + 1 >= count ? 0 : entry + 1;
		}
		else if (hidKeysDown() & KEY_UP)
		{
			entry = entry - 1 < 0 ? count - 1 : entry - 1;
		}
		else if (hidKeysDown() & KEY_LEFT)
		{
			entry = entry - 4 < 0 ? 0 : entry - 4;
		}
		else if (hidKeysDown() & KEY_RIGHT)
		{
			entry = entry + 4 >= count ? count - 1 : entry + 4;
		}
		
		if ((hidKeysDown() & KEY_A) && entry >= 0 && entry < count)
		{
			if (scriptExecute(mainbuf, scriptList[entry].path))
			{
				infoDisp(i18n(S_SCRIPT_EXECUTED_CORRECTLY));
			}
			else
			{
				infoDisp(i18n(S_SCRIPT_EXECUTION_FAILED));
			}
		}
		
		printScriptMenu(scriptList, count, entry);
	}
	
	free(scriptList);
}
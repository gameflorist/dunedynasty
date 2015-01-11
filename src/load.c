/** @file src/load.c Load routines. */

#include <stdio.h>
#include <stdlib.h>
#include "enum_string.h"
#include "errorlog.h"
#include "multichar.h"
#include "types.h"
#include "os/endian.h"
#include "os/strings.h"

#include "load.h"

#include "ai.h"
#include "audio/audio.h"
#include "file.h"
#include "map.h"
#include "mods/skirmish.h"
#include "newui/menubar.h"
#include "opendune.h"
#include "saveload/saveload.h"
#include "scenario.h"
#include "sprites.h"
#include "string.h"
#include "structure.h"
#include "team.h"
#include "unit.h"


static uint32 Load_FindChunk(FILE *fp, uint32 chunk)
{
	uint32 header;
	uint32 length;

	while (fread(&header, sizeof(uint32), 1, fp) == 1) {
		if (fread(&length, sizeof(uint32), 1, fp) != 1) return 0;
		length = BETOH32(length);
		if (BETOH32(header) != chunk) {
			fseek(fp, length + (length & 1), SEEK_CUR);
			continue;
		}
		return length;
	}
	return 0;
}

static bool Load_Main(FILE *fp)
{
	uint32 position;
	uint32 length;
	uint32 header;
	uint16 version;

	/* All Dune II savegames should start with 'FORM'. */
	if (fread(&header, sizeof(uint32), 1, fp) != 1) return false;
	if (BETOH32(header) != CC_FORM) {
		Warning("Invalid magic header in savegame.  Not a Dune II savegame.");
		return false;
	}

	/* The total length field, which is ignored */
	if (fread(&length, sizeof(uint32), 1, fp) != 1) return false;

	/* The next 'chunk' is fake, and has no length field */
	if (fread(&header, sizeof(uint32), 1, fp) != 1) return false;
	if (BETOH32(header) != CC_SCEN) return false;

	if (g_campaign_selected == CAMPAIGNID_SKIRMISH) {
		for (enum HouseType h = HOUSE_HARKONNEN; h < HOUSE_MAX; h++) {
			g_skirmish.brain[h] = BRAIN_NONE;
		}
	}

	position = ftell(fp);

	/* Find the 'INFO' chunk, as it contains the savegame version */
	version = 0;
	length = Load_FindChunk(fp, CC_INFO);
	if (length == 0) return false;

	/* Read the savegame version */
	if (!fread_le_uint16(&version, fp)) return false;
	length -= 2;
	if (version == 0) return false;

	if (version != 0x0290) {
		/* Get the scenarioID / campaignID */
		if (!Info_LoadOld(fp, length)) return false;

		g_gameMode = GM_RESTART;

		/* Find the 'PLYR' chunk */
		fseek(fp, position, SEEK_SET);
		length = Load_FindChunk(fp, CC_PLYR);
		if (length == 0) return false;

		/* Find the human player */
		if (!House_LoadOld(fp, length)) return false;

		GUI_DisplayModalMessage(String_Get_ByIndex(STR_WARNING_ORIGINAL_SAVED_GAMES_ARE_INCOMPATIBLE_WITH_THE_NEW_VERSION_THE_BATTLE_WILL_BE_RESTARTED), 0xFFFF);

		return true;
	}

	/* Load the 'INFO' chunk'. It has to be the first chunk loaded */
	if (!Info_Load(fp, length)) return false;

	/* Rewind, and read other chunks */
	bool load_bldg = false;
	bool load_plyr = false;
	bool load_unit = false;
	bool load_map  = false;
	fseek(fp, position, SEEK_SET);
	while (fread(&header, sizeof(uint32), 1, fp) == 1) {
		bool abort = false;
		bool skip = false;

		if (fread(&length, sizeof(uint32), 1, fp) != 1) return false;
		length = BETOH32(length);

		switch (BETOH32(header)) {
			case CC_NAME: break; /* 'NAME' chunk is of no interest to us */
			case CC_INFO: break; /* 'INFO' chunk is already read */
			case CC_MAP : if (!Map_Load      (fp, length)) return false; load_map  = true; Map_Load2Fallback(); break;
			case CC_PLYR: if (!House_Load    (fp, length)) return false; load_plyr = true; break;
			case CC_UNIT: if (!Unit_Load     (fp, length)) return false; load_unit = true; break;
			case CC_BLDG: if (!Structure_Load(fp, length)) return false; load_bldg = true; break;
			case CC_TEAM: if (!Team_Load     (fp, length)) return false; break;
			case CC_ODUN: if (!UnitNew_Load  (fp, length)) return false; break;

			/* Dune Dynasty extensions.  Note: must come AFTER CC_BLDG, CC_UNIT, etc. */
			case CC_DDAI: if (!BrutalAI_Load (fp, length)) return false; break;

			case CC_DDB2:
				skip  = !load_bldg;
				abort = !skip && !Structure_Load2(fp, length);
				break;

			case CC_DDH2:
				skip  = !load_plyr;
				abort = !skip && !House_Load2(fp, length);
				break;

			case CC_DDI2:
				skip  = !load_unit;
				abort = !skip && !Info_Load2(fp, length);
				break;

			case CC_DDM2:
				skip  = !load_map;
				abort = !skip && !Map_Load2(fp, length);
				break;

			case CC_DDS2:
				skip  = g_campaign_selected != CAMPAIGNID_SKIRMISH;
				abort = !skip && !Scenario_Load2(fp, length);
				break;

			case CC_DDS3:
				skip  = !load_plyr;
				abort = !skip && !Scenario_Load3(fp, length);
				break;

			case CC_DDU2:
				skip  = !load_unit;
				abort = !skip && !Unit_Load2(fp, length);
				break;

			default:
				skip  = true;
				abort = false;
				break;
		}

		if (skip) {
			Error("Unknown chunk in savegame: %c%c%c%c (length: %d). Skipped.\n",
					header, header >> 8, header >> 16, header >> 24, length);
		} else if (abort) {
			Error("Faulty chunk in savegame: %c%c%c%c (length: %d).\n",
					header, header >> 8, header >> 16, header >> 24, length);
			return false;
		}

		/* Savegames are word aligned */
		position += length + 8 + (length & 1);
		fseek(fp, position, SEEK_SET);
	}

	if (g_campaign_selected == CAMPAIGNID_SKIRMISH) {
		if (Skirmish_IsPlayable()) {
			Skirmish_Prepare();
		} else {
			GUI_DisplayModalMessage("Missing skirmish saved game data!", 0xFFFF);
			return false;
		}
	}

	return true;
}

bool
LoadFile(const char *filename)
{
	FILE *fp;
	bool res;

	Audio_PlayVoice(VOICE_STOP);

	Game_Init();

	fp = File_Open_CaseInsensitive(SEARCHDIR_PERSONAL_DATA_DIR, filename, "rb");
	if (fp == NULL) {
		Error("Failed to open file '%s' for reading.\n", filename);

		/* TODO -- Load failures should not result in termination */
		exit(0);

		return false;
	}

	Sprites_LoadTiles();

	g_validateStrictIfZero++;
	res = Load_Main(fp);
	g_validateStrictIfZero--;

	fclose(fp);

	if (!res) {
		Error("Error while loading savegame.\n");

		/* TODO -- Load failures should not result in termination */
		exit(0);

		return false;
	}

	if (g_gameMode != GM_RESTART) Game_Prepare();

	return true;
}

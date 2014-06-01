/** @file src/save.c Save routines. */

#include <stdio.h>
#include <string.h>
#include "types.h"

#include "save.h"

#include "ai.h"
#include "file.h"
#include "house.h"
#include "map.h"
#include "newui/menubar.h"
#include "opendune.h"
#include "os/endian.h"
#include "os/error.h"
#include "os/math.h"
#include "os/strings.h"
#include "pool/pool.h"
#include "pool/pool_structure.h"
#include "pool/pool_unit.h"
#include "saveload/saveload.h"
#include "scenario.h"
#include "shape.h"
#include "sprites.h"
#include "structure.h"
#include "team.h"
#include "unit.h"

/**
 * Save a chunk of data.
 * @param fp The file to save to.
 * @param header The chunk identification string (4 chars, always).
 * @param saveProc The proc to call to generate the content of the chunk.
 * @return True if and only if all bytes were written successful.
 */
static bool Save_Chunk(FILE *fp, const char *header, bool (*saveProc)(FILE *fp))
{
	uint32 position;
	uint32 length;
	uint32 lengthSwapped;

	if (fwrite(header, 4, 1, fp) != 1) return false;

	/* Reserve the length field */
	length = 0;
	if (fwrite(&length, 4, 1, fp) != 1) return false;

	/* Store the content of the chunk, and remember the length */
	position = ftell(fp);
	if (!saveProc(fp)) return false;
	length = ftell(fp) - position;

	/* Ensure we are word aligned */
	if ((length & 1) == 1) {
		uint8 empty = 0;
		if (fwrite(&empty, 1, 1, fp) != 1) return false;
	}

	/* Write back the chunk size */
	fseek(fp, position - 4, SEEK_SET);
	lengthSwapped = HTOBE32(length);
	if (fwrite(&lengthSwapped, 4, 1, fp) != 1) return false;
	fseek(fp, 0, SEEK_END);

	return true;
}

/**
 * Save the game for real. It creates all the required chunks and stores them
 *  to the file. It updates the field lengths where needed.
 *
 * @param fp The file to save to.
 * @param description The description of the savegame.
 * @return True if and only if all bytes were written successful.
 */
static bool
Save_Main(FILE *fp, const char *description)
{
	uint32 length;
	uint32 lengthSwapped;

	/* Write the 'FORM' chunk (in which all other chunks are) */
	if (fwrite("FORM", 4, 1, fp) != 1) return false;
	/* Write zero length for now. We come back to this value before closing */
	length = 0;
	if (fwrite(&length, 4, 1, fp) != 1) return false;

	/* Write the 'SCEN' chunk. Never contains content. */
	if (fwrite("SCEN", 4, 1, fp) != 1) return false;

	/* Write the 'NAME' chunk. Keep ourself word-aligned. */
	if (fwrite("NAME", 4, 1, fp) != 1) return false;
	length = min(255, strlen(description) + 1);
	lengthSwapped = HTOBE32(length);
	if (fwrite(&lengthSwapped, 4, 1, fp) != 1) return false;
	if (fwrite(description, length, 1, fp) != 1) return false;
	/* Ensure we are word aligned */
	if ((length & 1) == 1) {
		uint8 empty = 0;
		if (fwrite(&empty, 1, 1, fp) != 1) return false;
	}

	/* Store all additional chunks */
	if (!Save_Chunk(fp, "INFO", &Info_Save)) return false;
	if (!Save_Chunk(fp, "PLYR", &House_Save)) return false;
	if (!Save_Chunk(fp, "UNIT", &Unit_Save)) return false;
	if (!Save_Chunk(fp, "BLDG", &Structure_Save)) return false;
	if (!Save_Chunk(fp, "MAP ", &Map_Save)) return false;
	if (!Save_Chunk(fp, "TEAM", &Team_Save)) return false;
	if (!Save_Chunk(fp, "ODUN", &UnitNew_Save)) return false;

	/* Store Dune Dynasty extensions. */
	if (g_campaign_selected == CAMPAIGNID_SKIRMISH) {
		if (!Save_Chunk(fp, "DDS2", &Scenario_Save2)) return false;
	}

	if (!Save_Chunk(fp, "DDS3", &Scenario_Save3)) return false;
	if (!Save_Chunk(fp, "DDI2", &Info_Save2)) return false;
	if (!Save_Chunk(fp, "DDH2", &House_Save2)) return false;
	if (!Save_Chunk(fp, "DDM2", &Map_Save2)) return false;
	if (!Save_Chunk(fp, "DDB2", &Structure_Save2)) return false;
	if (!Save_Chunk(fp, "DDU2", &Unit_Save2)) return false;
	if (!Save_Chunk(fp, "DDAI", &BrutalAI_Save)) return false;

	/* Write the total length of all data in the FORM chunk */
	length = ftell(fp) - 8;
	fseek(fp, 4, SEEK_SET);
	lengthSwapped = HTOBE32(length);
	if (fwrite(&lengthSwapped, 4, 1, fp) != 1) return false;

	return true;
}

/**
 * Save the game to a filename
 *
 * @param fp The filename of the savegame.
 * @param description The description of the savegame.
 * @return True if and only if all bytes were written successful.
 */
bool
SaveFile(const char *filename, const char *description)
{
	FILE *fp;
	bool res;

	/* In debug-scenario mode, the whole map is uncovered. Cover it now in
	 *  the savegame based on the current position of the units and
	 *  structures. */
	if (g_debugScenario) {
		PoolFindStruct find;
		uint16 i;

		/* Add fog of war for all tiles on the map */
		for (i = 0; i < 0x1000; i++) {
			Tile *tile = &g_map[i];
			tile->isUnveiled_ = false;
			tile->overlaySpriteID = g_veiledSpriteID;
		}

		/* Remove the fog of war for all units */
		for (const Unit *u = Unit_FindFirst(&find, HOUSE_INVALID, UNIT_INVALID);
				u != NULL;
				u = Unit_FindNext(&find)) {
			Unit_RemoveFog(UNVEILCAUSE_INITIALISATION, u);
		}

		/* Remove the fog of war for all structures */
		for (const Structure *s = Structure_FindFirst(&find, HOUSE_INVALID, STRUCTURE_INVALID);
				s != NULL;
				s = Structure_FindNext(&find)) {
			if (Structure_SharesPoolElement(s->o.type))
				continue;

			Structure_RemoveFog(UNVEILCAUSE_INITIALISATION, s);
		}
	}

	fp = File_Open_CaseInsensitive(SEARCHDIR_PERSONAL_DATA_DIR, filename, "wb");
	if (fp == NULL) {
		GUI_DisplayModalMessage("Failed to open file '%s' for writing.", SHAPE_INVALID, filename);
		return false;
	}

	g_validateStrictIfZero++;
	res = Save_Main(fp, description);
	g_validateStrictIfZero--;

	fclose(fp);

	if (!res) {
		/* TODO -- Also remove the savegame now */

		Error("Error while writing savegame.\n");
		return false;
	}

	return true;
}

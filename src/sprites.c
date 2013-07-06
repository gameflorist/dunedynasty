/** @file src/sprites.c Sprite routines. */

#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "multichar.h"
#include "types.h"
#include "os/common.h"
#include "os/endian.h"
#include "os/math.h"
#include "os/strings.h"

#include "sprites.h"

#include "codec/format80.h"
#include "file.h"
#include "gfx.h"
#include "gui/gui.h"
#include "house.h"
#include "ini.h"
#include "scenario.h"
#include "script/script.h"
#include "string.h"
#include "video/video.h"


uint8 *g_sprites[SHAPE_MAX];
uint8 *g_spriteBuffer;
uint8 *g_iconRTBL = NULL;
uint8 *g_iconRPAL = NULL;
uint8 *g_spriteInfo = NULL;
uint16 *g_iconMap = NULL;

uint8 *g_fileRgnclkCPS = NULL;
void *g_fileRegionINI = NULL;
uint16 *g_regions = NULL;

uint16 g_veiledSpriteID;
uint16 g_bloomSpriteID;
uint16 g_landscapeSpriteID;
uint16 g_builtSlabSpriteID;
uint16 g_wallSpriteID;

static bool s_iconLoaded = false;

/**
 * Gets the given sprite inside the given buffer.
 *
 * @param buffer The buffer containing sprites.
 * @param index The index of the sprite to get.
 * @return The sprite.
 */
static const uint8 *Sprites_GetSprite(const uint8 *buffer, uint16 index)
{
	uint32 offset;

	if (buffer == NULL) return NULL;
	if (READ_LE_UINT16(buffer) <= index) return NULL;

	buffer += 2;

	offset = READ_LE_UINT32(buffer + 4 * index);

	if (offset == 0) return NULL;

	return buffer + offset;
}

/**
 * Loads the sprites.
 *
 * @param index The index of the list of sprite files to load.
 * @param sprites The array where to store CSIP for each loaded sprite.
 */
static void
Sprites_Load(enum SearchDirectory dir, const char *filename, int start, int end)
{
	uint8 *buffer;
	uint16 count;
	uint16 i;

	buffer = File_ReadWholeFile_Ex(dir, filename);
	count = READ_LE_UINT16(buffer);

	assert(count == end - start + 1);
	count = min(count, end - start + 1);

	for (i = 0; i < count; i++) {
		const uint8 *src = Sprites_GetSprite(buffer, i);
		uint8 *dst = NULL;

		if (src != NULL) {
			uint16 size = READ_LE_UINT16(src + 6);
			dst = (uint8 *)malloc(size);
			memcpy(dst, src, size);
		}

		free(g_sprites[start + i]);
		g_sprites[start + i] = dst;
	}

	free(buffer);
}

/**
 * Gets the width of the given sprite.
 *
 * @param sprite The sprite.
 * @return The width.
 */
uint8 Sprite_GetWidth(const uint8 *sprite)
{
	if (sprite == NULL) return 0;

	return sprite[3];
}

/**
 * Gets the height of the given sprite.
 *
 * @param sprite The sprite.
 * @return The height.
 */
uint8 Sprite_GetHeight(const uint8 *sprite)
{
	if (sprite == NULL) return 0;

	return sprite[2];
}

#if 0
extern uint16 Sprites_GetType(uint8 *sprite);
#endif

/**
 * Decodes an image.
 *
 * @param source The encoded image.
 * @param dest The place the decoded image will be.
 * @return The size of the decoded image.
 */
static uint32 Sprites_Decode(const uint8 *source, uint8 *dest)
{
	uint32 size = 0;

	switch(*source) {
		case 0x0:
			source += 2;
			size = READ_LE_UINT32(source);
			source += 4;
			source += READ_LE_UINT16(source);
			source += 2;
			memmove(dest, source, size);
			break;

		case 0x4:
			source += 6;
			source += READ_LE_UINT16(source);
			source += 2;
			size = Format80_Decode(dest, source, 0xFFFF);
			break;

		default: break;
	}

	return size;
}

/**
 * Loads an ICN file.
 *
 * @param filename The name of the file to load.
 * @param screenID The index of a memory block where to store loaded sprites.
 */
static void Sprites_LoadICNFile(const char *filename)
{
	uint8  fileIndex;

	uint32 spriteInfoLength;
	uint32 tableLength;
	uint32 paletteLength;
	int8   info[4];

	fileIndex = ChunkFile_Open(filename);

	/* Get the length of the chunks */
	spriteInfoLength = ChunkFile_Seek(fileIndex, HTOBE32(CC_SSET));
	tableLength      = ChunkFile_Seek(fileIndex, HTOBE32(CC_RTBL));
	paletteLength    = ChunkFile_Seek(fileIndex, HTOBE32(CC_RPAL));

	/* Read the header information */
	ChunkFile_Read(fileIndex, HTOBE32(CC_SINF), info, 4);
	GFX_Init_SpriteInfo(info[0], info[1]);

	/* Get the SpriteInfo chunk */
	free(g_spriteInfo);
	g_spriteInfo = calloc(1, spriteInfoLength);
	ChunkFile_Read(fileIndex, HTOBE32(CC_SSET), g_spriteInfo, spriteInfoLength);
	Sprites_Decode(g_spriteInfo, g_spriteInfo);

	/* Get the Table chunk */
	free(g_iconRTBL);
	g_iconRTBL = calloc(1, tableLength);
	ChunkFile_Read(fileIndex, HTOBE32(CC_RTBL), g_iconRTBL, tableLength);

	/* Get the Palette chunk */
	free(g_iconRPAL);
	g_iconRPAL = calloc(1, paletteLength);
	ChunkFile_Read(fileIndex, HTOBE32(CC_RPAL), g_iconRPAL, paletteLength);

	ChunkFile_Close(fileIndex);
}

/**
 * Loads the sprites for tiles.
 */
void Sprites_LoadTiles(void)
{
	if (s_iconLoaded) return;

	s_iconLoaded = true;

	Sprites_LoadICNFile("ICON.ICN");

	free(g_iconMap);
	g_iconMap = File_ReadWholeFileLE16("ICON.MAP");

	g_veiledSpriteID    = g_iconMap[g_iconMap[ICM_ICONGROUP_FOG_OF_WAR] + 16];
	g_bloomSpriteID     = g_iconMap[g_iconMap[ICM_ICONGROUP_SPICE_BLOOM]];
	g_builtSlabSpriteID = g_iconMap[g_iconMap[ICM_ICONGROUP_CONCRETE_SLAB] + 2];
	g_landscapeSpriteID = g_iconMap[g_iconMap[ICM_ICONGROUP_LANDSCAPE]];
	g_wallSpriteID      = g_iconMap[g_iconMap[ICM_ICONGROUP_WALLS]];

	Script_LoadFromFile("UNIT.EMC", g_scriptUnit, g_scriptFunctionsUnit, GFX_Screen_Get_ByIndex(SCREEN_2));
}

/**
 * Unloads the sprites for tiles.
 */
void Sprites_UnloadTiles(void)
{
	s_iconLoaded = false;
}

/**
 * Loads a CPS file.
 *
 * @param filename The name of the file to load.
 * @param screenID The index of a memory block where to store loaded data.
 * @param palette Where to store the palette, if any.
 * @return The size of the loaded image.
 */
static uint32
Sprites_LoadCPSFile(enum SearchDirectory dir, const char *filename,
		Screen screenID, uint8 *palette)
{
	uint8 index;
	uint16 size;
	uint8 *buffer;
	uint8 *buffer2;
	uint16 paletteSize;

	buffer = GFX_Screen_Get_ByIndex(screenID);

	index = File_Open_Ex(dir, filename, FILE_MODE_READ);
	size = File_Read_LE16(index);

	File_Read(index, buffer, 8);

	size -= 8;

	paletteSize = READ_LE_UINT16(buffer + 6);

	if (palette != NULL && paletteSize != 0) {
		File_Read(index, palette, paletteSize);
	} else {
		File_Seek(index, paletteSize, 1);
	}

	buffer[6] = 0;	/* dont read palette next time */
	buffer[7] = 0;
	size -= paletteSize;

	buffer2 = GFX_Screen_Get_ByIndex(screenID);
	buffer2 += GFX_Screen_GetSize_ByIndex(screenID) - size - 8;

	memmove(buffer2, buffer, 8);
	File_Read(index, buffer2 + 8, size);

	File_Close(index);

	return Sprites_Decode(buffer2, buffer);
}

/**
 * Loads an image.
 *
 * @param filename The name of the file to load.
 * @param memory1 The index of a memory block where to store loaded data.
 * @param memory2 The index of a memory block where to store loaded data.
 * @param palette Where to store the palette, if any.
 * @return The size of the loaded image.
 */
uint16
Sprites_LoadImage(enum SearchDirectory dir, const char *filename,
		Screen screenID, uint8 *palette)
{
	uint8 index;
	uint32 header;

	index = File_Open_Ex(dir, filename, FILE_MODE_READ);
	if (index == FILE_INVALID) return 0;

	File_Read(index, &header, 4);
	File_Close(index);

	return Sprites_LoadCPSFile(dir, filename, screenID, palette) / 8000;
}

#if 0
extern void Sprites_SetMouseSprite(uint16 hotSpotX, uint16 hotSpotY, uint8 *sprite);
#endif

static void InitRegions(void)
{
	uint16 *regions = g_regions;
	uint16 i;
	char textBuffer[81];

	Ini_GetString("INFO", "TOTAL REGIONS", NULL, textBuffer, lengthof(textBuffer) - 1, g_fileRegionINI);

	sscanf(textBuffer, "%hu", &regions[0]);

	for (i = 0; i < regions[0]; i++) regions[i + 1] = 0xFFFF;
}

void Sprites_CPS_LoadRegionClick(void)
{
	uint8 *buf;
	uint8 i;
	char filename[16];

	buf = GFX_Screen_Get_ByIndex(SCREEN_2);

	g_fileRgnclkCPS = buf;
	Sprites_LoadCPSFile(SEARCHDIR_GLOBAL_DATA_DIR, "RGNCLK.CPS", SCREEN_2, NULL);
	for (i = 0; i < 120; i++) memcpy(buf + (i * 304), buf + 7688 + (i * 320), 304);
	buf += 120 * 304;

	g_fileRegionINI = buf;
	snprintf(filename, sizeof(filename), "REGION%c.INI", g_table_houseInfo[g_playerHouseID].name[0]);
	buf += File_ReadFile_Ex(SEARCHDIR_CAMPAIGN_DIR, filename, buf);

	g_regions = (uint16 *)buf;

	InitRegions();
}

/**
 * Check if a spriteID is part of the veiling sprites.
 * @param spriteID The sprite to check for.
 * @return True if and only if the spriteID is part of the veiling sprites.
 */
bool Sprite_IsUnveiled(uint16 spriteID)
{
	if (spriteID > g_veiledSpriteID) return true;
	if (spriteID < g_veiledSpriteID - 15) return true;

	return false;
}

void Sprites_Init(void)
{
	g_spriteBuffer = calloc(1, 20000);

	Sprites_Load(SEARCHDIR_GLOBAL_DATA_DIR, "MOUSE.SHP", 0, 6);
	Sprites_Load(SEARCHDIR_GLOBAL_DATA_DIR, String_GenerateFilename("BTTN"), 7, 11);
	Sprites_Load(SEARCHDIR_GLOBAL_DATA_DIR, "SHAPES.SHP",  12, 110);
	Sprites_Load(SEARCHDIR_GLOBAL_DATA_DIR, "UNITS2.SHP", 111, 150);
	Sprites_Load(SEARCHDIR_GLOBAL_DATA_DIR, "UNITS1.SHP", 151, 237);
	Sprites_Load(SEARCHDIR_GLOBAL_DATA_DIR, "UNITS.SHP",  238, 354);
	Sprites_Load(SEARCHDIR_GLOBAL_DATA_DIR, String_GenerateFilename("CHOAM"), 355, 372);
	Sprites_Load(SEARCHDIR_GLOBAL_DATA_DIR, String_GenerateFilename("MENTAT"), 373, 386);
	Sprites_Load(SEARCHDIR_GLOBAL_DATA_DIR, "PIECES.SHP", 477, 504);
	Sprites_Load(SEARCHDIR_GLOBAL_DATA_DIR, "ARROWS.SHP", 505, 513);
}

void
Sprites_InitCHOAM(const char *bttn, const char *choam)
{
	if (bttn != NULL)
		Sprites_Load(SEARCHDIR_GLOBAL_DATA_DIR, bttn, 7, 11);

	if (choam != NULL)
		Sprites_Load(SEARCHDIR_GLOBAL_DATA_DIR, choam, 355, 372);
}

void
Sprites_InitMentat(enum MentatID mentatID)
{
	static enum MentatID l_mentatID = MENTAT_MAX;
	static enum HouseType l_houseID = HOUSE_INVALID;
	static int l_campaign_selected = -1;

	const char *shapes[HOUSE_MAX] = {
		"MENSHPH.SHP", "MENSHPA.SHP", "MENSHPO.SHP",
		"MENSHPF.SHP", "MENSHPS.SHP", "MENSHPM.SHP"
	};

	if (mentatID == l_mentatID) {
		if ((mentatID != MENTAT_CUSTOM) || (g_playerHouseID == l_houseID && g_campaign_selected == l_campaign_selected))
			return;
	}

	if (mentatID == MENTAT_CUSTOM) {
		Sprites_Load(SEARCHDIR_CAMPAIGN_DIR, shapes[g_playerHouseID], SHAPE_MENTAT_EYES, SHAPE_MENTAT_EYES + 15 - 1);
	}
	else {
		const enum HouseType houseID = (mentatID == MENTAT_BENE_GESSERIT) ? HOUSE_MERCENARY : (enum HouseType)mentatID;
		Sprites_Load(SEARCHDIR_GLOBAL_DATA_DIR, shapes[houseID], SHAPE_MENTAT_EYES, SHAPE_MENTAT_EYES + 15 - 1);
	}

	const bool use_benepal = (mentatID == MENTAT_BENE_GESSERIT);

	Video_InitMentatSprites(use_benepal);
	l_mentatID = mentatID;
	l_houseID = g_playerHouseID;
	l_campaign_selected = g_campaign_selected;
}

void
Sprites_InitCredits(void)
{
	static bool l_loaded = false;

	if (l_loaded)
		return;

	Sprites_Load(SEARCHDIR_GLOBAL_DATA_DIR, "CREDIT1.SHP",  514, 514);
	Sprites_Load(SEARCHDIR_GLOBAL_DATA_DIR, "CREDIT2.SHP",  515, 515);
	Sprites_Load(SEARCHDIR_GLOBAL_DATA_DIR, "CREDIT3.SHP",  516, 516);
	Sprites_Load(SEARCHDIR_GLOBAL_DATA_DIR, "CREDIT4.SHP",  517, 517);
	Sprites_Load(SEARCHDIR_GLOBAL_DATA_DIR, "CREDIT5.SHP",  518, 518);
	Sprites_Load(SEARCHDIR_GLOBAL_DATA_DIR, "CREDIT6.SHP",  519, 519);
	Sprites_Load(SEARCHDIR_GLOBAL_DATA_DIR, "CREDIT7.SHP",  520, 520);
	Sprites_Load(SEARCHDIR_GLOBAL_DATA_DIR, "CREDIT8.SHP",  521, 521);
	Sprites_Load(SEARCHDIR_GLOBAL_DATA_DIR, "CREDIT9.SHP",  522, 522);
	Sprites_Load(SEARCHDIR_GLOBAL_DATA_DIR, "CREDIT10.SHP", 523, 523);
	Sprites_Load(SEARCHDIR_GLOBAL_DATA_DIR, "CREDIT11.SHP", 524, 524);

	l_loaded = true;
}

void Sprites_Uninit(void)
{
	for (int i = 0; i < SHAPE_MAX; i++)
		free(g_sprites[i]);

	free(g_spriteBuffer); g_spriteBuffer = NULL;

	free(g_spriteInfo); g_spriteInfo = NULL;
	free(g_iconRTBL); g_iconRTBL = NULL;
	free(g_iconRPAL); g_iconRPAL = NULL;

	free(g_iconMap); g_iconMap = NULL;
}

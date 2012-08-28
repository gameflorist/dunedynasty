/* $Id$ */

/** @file src/gui/gui.c Generic GUI definitions. */

#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
#include <stdarg.h>
#include <string.h>
#include "../os/common.h"
#include "../os/math.h"
#include "../os/sleep.h"
#include "../os/strings.h"

#include "gui.h"

#include "font.h"
#include "mentat.h"
#include "widget.h"
#include "../animation.h"
#include "../audio/audio.h"
#include "../codec/format80.h"
#include "../config.h"
#include "../explosion.h"
#include "../file.h"
#include "../gfx.h"
#include "../house.h"
#include "../ini.h"
#include "../input/input.h"
#include "../input/mouse.h"
#include "../load.h"
#include "../map.h"
#include "../newui/halloffame.h"
#include "../newui/menu.h"
#include "../newui/menubar.h"
#include "../opendune.h"
#include "../pool/pool.h"
#include "../pool/house.h"
#include "../pool/structure.h"
#include "../pool/unit.h"
#include "../sprites.h"
#include "../string.h"
#include "../structure.h"
#include "../table/strings.h"
#include "../table/widgetinfo.h"
#include "../tile.h"
#include "../timer/timer.h"
#include "../tools.h"
#include "../unit.h"
#include "../video/video.h"
#include "../wsa.h"

/** Coupling between score and rank name. */
typedef struct RankScore {
	uint16 rankString; /*!< StringID of the name of the rank. */
	uint16 score;      /*!< Score needed to obtain the rank. */
} RankScore;

/** Mapping of scores to rank names. */
static const RankScore _rankScores[] = {
	{271,   25}, /* "Sand Flea" */
	{272,   50}, /* "Sand Snake" */
	{273,  100}, /* "Desert Mongoose" */
	{274,  150}, /* "Sand Warrior" */
	{275,  200}, /* "Dune Trooper" */
	{276,  300}, /* "Squad Leader" */
	{277,  400}, /* "Outpost Commander" */
	{278,  500}, /* "Base Commander" */
	{279,  700}, /* "Warlord" */
	{280, 1000}, /* "Chief Warlord" */
	{281, 1400}, /* "Ruler of Arrakis" */
	{282, 1800}  /* "Emperor" */
};

static uint8 g_colours[16];
uint8 g_palette_998A[3 * 256];
uint8 g_remap[256];
FactoryWindowItem g_factoryWindowItems[25];
uint16 g_factoryWindowOrdered = 0;
uint16 g_factoryWindowBase = 0;
uint16 g_factoryWindowTotal = 0;
uint16 g_factoryWindowSelected = 0;
uint16 g_factoryWindowUpgradeCost = 0;
bool g_factoryWindowConstructionYard = false;
FactoryResult g_factoryWindowResult = FACTORY_RESUME;
bool g_factoryWindowStarport = false;
static uint8 s_factoryWindowGraymapTbl[256];
static Widget s_factoryWindowWidgets[13];
static uint8 s_factoryWindowWsaBuffer[64000];
static uint16 s_temporaryColourBorderSchema[5][4];          /*!< Temporary storage for the #s_colourBorderSchema. */
uint16 g_productionStringID;                                /*!< Descriptive text of activity of the active structure. */
bool g_textDisplayNeedsUpdate;                              /*!< If set, text display needs to be updated. */
static uint32 s_ticksPlayed;

uint16 g_cursorSpriteID;

uint16 g_variable_37B2;
bool g_var_37B8;

uint16 g_viewportMessageCounter;                            /*!< Countdown counter for displaying #g_viewportMessageText, bit 0 means 'display the text'. */
char *g_viewportMessageText;                                /*!< If not \c NULL, message text displayed in the viewport. */

uint16 g_viewportPosition;                                  /*!< Top-left tile of the viewport. */
uint16 g_minimapPosition;                                   /*!< Top-left tile of the border in the minimap. */
uint16 g_selectionRectanglePosition;                        /*!< Position of the structure selection rectangle. */
uint16 g_selectionPosition;                                 /*!< Current selection position (packed). */
uint16 g_selectionWidth;                                    /*!< Width of the selection. */
uint16 g_selectionHeight;                                   /*!< Height of the selection. */
int16  g_selectionState = 1;                                /*!< State of the selection (\c 1 is valid, \c 0 is not valid, \c <0 valid but missing some slabs. */


/*!< Colours used for the border of widgets. */
static uint16 s_colourBorderSchema[5][4] = {
	{ 26,  29,  29,  29},
	{ 20,  26,  16,  20},
	{ 20,  16,  26,  20},
	{233, 235, 232, 233},
	{233, 232, 235, 233}
};

/** Colours used for the border of widgets in the hall of fame. */
static const uint16 s_HOF_ColourBorderSchema[5][4] = {
	{226, 228, 228, 228},
	{116, 226, 105, 116},
	{116, 105, 226, 116},
	{233, 235, 232, 233},
	{233, 232, 235, 233}
};

assert_compile(lengthof(s_colourBorderSchema) == lengthof(s_temporaryColourBorderSchema));
assert_compile(lengthof(s_colourBorderSchema) == lengthof(s_HOF_ColourBorderSchema));

#if 0
/* Moved to video/video_opendune.c */
extern void GUI_DrawWiredRectangle(uint16 left, uint16 top, uint16 right, uint16 bottom, uint8 colour);
extern void GUI_DrawFilledRectangle(int16 left, int16 top, int16 right, int16 bottom, uint8 colour);
#endif

/**
 * Display a text.
 * @param str The text to display. If \c NULL, update the text display (scroll text, and/or remove it on time out).
 * @param importance Importance of the new text. Value \c -1 means remove all text lines, \c -2 means drop all texts in buffer but not yet displayed.
 *                   Otherwise, it is the importance of the message (if supplied). Higher numbers mean displayed sooner.
 * @param ... The args for the text.
 */
void GUI_DisplayText(const char *str, int16 importance, ...)
{
	char buffer[80];                 /* Formatting buffer of new message. */
	static int64_t displayTimer = 0; /* Timeout value for next update of the display. */
	static uint16 textOffset;        /* Vertical position of text being scrolled. */
	static bool scrollInProgress;    /* Text is being scrolled (and partly visible to the user). */

	static char displayLine1[80];    /* Current line being displayed. */
	static char displayLine2[80];    /* Next line (if scrollInProgress, it is scrolled up). */
	static char displayLine3[80];    /* Next message to display (after scrolling next line has finished). */
	static int16 line1Importance;    /* Importance of the displayed line of text. */
	static int16 line2Importance;    /* Importance of the next line of text. */
	static int16 line3Importance;    /* Importance of the next message. */
	static uint8 fgColour1;          /* Foreground colour current line. */
	static uint8 fgColour2;          /* Foreground colour next line. */
	static uint8 fgColour3;          /* Foreground colour next message. */

	buffer[0] = '\0';

	if (str != NULL) {
		va_list ap;

		va_start(ap, importance);
		vsnprintf(buffer, sizeof(buffer), str, ap);
		va_end(ap);
	}

	if (importance == -1) { /* Remove all displayed lines. */
		line1Importance = -1;
		line2Importance = -1;
		line3Importance = -1;

		displayLine1[0] = '\0';
		displayLine2[0] = '\0';
		displayLine3[0] = '\0';

		scrollInProgress = false;
		displayTimer = 0;
		return;
	}

	if (importance == -2) { /* Remove next line and next message. */
		if (!scrollInProgress) {
			line2Importance = -1;
			displayLine2[0] = '\0';
		}
		line3Importance = -1;
		displayLine3[0] = '\0';
	}

	MenuBar_DrawStatusBar(displayLine1, displayLine2, fgColour1, fgColour2, textOffset);

	if (scrollInProgress) {
		if (buffer[0] != '\0') {
			if (strcasecmp(buffer, displayLine2) != 0 && importance >= line3Importance) {
				strcpy(displayLine3, buffer);
				line3Importance = importance;
			}
		}
		if (displayTimer > Timer_GetTicks()) return;

#if 0
		uint16 oldValue_07AE_0000 = Widget_SetCurrentWidget(7);
		uint16 height;

		if (g_textDisplayNeedsUpdate) {
			uint16 oldScreenID = GFX_Screen_SetActive(2);

			GUI_DrawFilledRectangle(0, 0, SCREEN_WIDTH - 1, 23, g_curWidgetFGColourNormal);

			GUI_DrawText_Wrapper(displayLine2, g_curWidgetXBase << 3,  2, fgColour2, 0, 0x012);
			GUI_DrawText_Wrapper(displayLine1, g_curWidgetXBase << 3, 13, fgColour1, 0, 0x012);

			g_textDisplayNeedsUpdate = false;

			GFX_Screen_SetActive(oldScreenID);
		}

		GUI_Mouse_Hide_InWidget(7);

		if (textOffset + g_curWidgetHeight > 24) {
			height = 24 - textOffset;
		} else {
			height = g_curWidgetHeight;
		}

		GUI_Screen_Copy(g_curWidgetXBase, textOffset, g_curWidgetXBase, g_curWidgetYBase, g_curWidgetWidth, height, 2, 0);
		GUI_Mouse_Show_InWidget();

		Widget_SetCurrentWidget(oldValue_07AE_0000);
#endif

		if (textOffset != 0) {
			if (line3Importance <= line2Importance) {
				displayTimer = Timer_GetTicks() + 1;
			}
			textOffset--;
			return;
		}

		/* Finished scrolling, move line 2 to line 1. */
		strcpy(displayLine1, displayLine2);
		fgColour1 = fgColour2;
		line1Importance = (line2Importance != 0) ? line2Importance - 1 : 0;

		/* And move line 3 to line 2. */
		strcpy(displayLine2, displayLine3);
		line2Importance = line3Importance;
		fgColour2 = fgColour3;
		displayLine3[0] = '\0';

		line3Importance = -1;
		g_textDisplayNeedsUpdate = true;
		displayTimer = Timer_GetTicks() + (line2Importance <= line1Importance ? 900 : 1);
		scrollInProgress = false;
		return;
	}

	if (buffer[0] != '\0') {
		/* If new line arrived, different from every line that is in the display buffers, and more important than existing messages,
		 * insert it at the right place.
		 */
		if (strcasecmp(buffer, displayLine1) != 0 && strcasecmp(buffer, displayLine2) != 0 && strcasecmp(buffer, displayLine3) != 0) {
			if (importance >= line2Importance) {
				/* Move line 2 to line 2 to make room for the new line. */
				strcpy(displayLine3, displayLine2);
				fgColour3 = fgColour2;
				line3Importance = line2Importance;
				/* Copy new line to line 2. */
				strcpy(displayLine2, buffer);
				fgColour2 = 12;
				line2Importance = importance;

			} else if (importance >= line3Importance) {
				/* Copy new line to line 3. */
				strcpy(displayLine3, buffer);
				line3Importance = importance;
				fgColour3 = 12;
			}
		}
	} else {
		if (displayLine1[0] == '\0' && displayLine2[0] == '\0') return;
	}

	if (line2Importance <= line1Importance && displayTimer >= Timer_GetTicks()) return;

	scrollInProgress = true;
	textOffset = 10;
	displayTimer = 0;
}

/**
 * Draw a char on the screen.
 *
 * @param c The char to draw.
 * @param x The most left position where to draw the string.
 * @param y The most top position where to draw the string.
 */
static void GUI_DrawChar(unsigned char c, uint16 x, uint16 y)
{
	Video_DrawChar(c, g_colours, x, y);
}

void GUI_DrawChar_(unsigned char c, int x, int y)
{
	const int width = (g_curWidgetIndex == WINDOWID_RENDER_TEXTURE) ? g_widgetProperties[WINDOWID_RENDER_TEXTURE].width*8 : SCREEN_WIDTH;
	const int height = (g_curWidgetIndex == WINDOWID_RENDER_TEXTURE) ? g_widgetProperties[WINDOWID_RENDER_TEXTURE].height : SCREEN_WIDTH;

	uint8 *screen = GFX_Screen_GetActive();

	FontChar *fc;

	uint16 remainingWidth;
	uint8 i;
	uint8 j;

	if (g_fontCurrent == NULL) return;

	fc = &g_fontCurrent->chars[c];
	if (fc->data == NULL) return;

	if (x >= width || (x + fc->width) > width) return;
	if (y >= height || (y + g_fontCurrent->height) > height) return;

	x += width * y;
	remainingWidth = width - fc->width;

	if (g_colours[0] != 0) {
		for (j = 0; j < fc->unusedLines; j++) {
			for (i = 0; i < fc->width; i++) screen[x++] = g_colours[0];
			x += remainingWidth;
		}
	} else {
		x += width * fc->unusedLines;
	}

	if (fc->usedLines == 0) return;

	for (j = 0; j < fc->usedLines; j++) {
		for (i = 0; i < fc->width; i++) {
			uint8 data = fc->data[j * fc->width + i];

			if (g_colours[data & 0xF] != 0) screen[x] = g_colours[data & 0xF];
			x++;
		}
		x += remainingWidth;
	}

	if (g_colours[0] == 0) return;

	for (j = fc->unusedLines + fc->usedLines; j < g_fontCurrent->height; j++) {
		for (i = 0; i < fc->width; i++) screen[x++] = g_colours[0];
		x += remainingWidth;
	}
}

/**
 * Draw a string to the screen.
 *
 * @param string The string to draw.
 * @param left The most left position where to draw the string.
 * @param top The most top position where to draw the string.
 * @param fgColour The foreground colour of the text.
 * @param bgColour The background colour of the text.
 */
void GUI_DrawText(const char *string, int16 left, int16 top, uint8 fgColour, uint8 bgColour)
{
	uint8 colours[2];
	uint16 x;
	uint16 y;
	const char *s;

	if (g_fontCurrent == NULL) return;

	if (left < 0) left = 0;
	if (top  < 0) top  = 0;
	if (left > TRUE_DISPLAY_WIDTH) return;
	if (top  > TRUE_DISPLAY_HEIGHT) return;

	colours[0] = bgColour;
	colours[1] = fgColour;

	GUI_InitColors(colours, 0, 1);

	s = string;
	x = left;
	y = top;
	while (*s != '\0') {
		uint16 width;

		if (*s == '\n' || *s == '\r') {
			x = left;
			y += g_fontCurrent->height;

			while (*s == '\n' || *s == '\r') s++;
		}

		width = Font_GetCharWidth(*s);

		if (x + width > TRUE_DISPLAY_WIDTH) {
			x = left;
			y += g_fontCurrent->height;
		}
		if (y > TRUE_DISPLAY_HEIGHT) break;

		GUI_DrawChar(*s, x, y);

		x += width;
		s++;
	}
}

/**
 * Draw a string to the screen, and so some magic.
 *
 * @param string The string to draw.
 * @param left The most left position where to draw the string.
 * @param top The most top position where to draw the string.
 * @param fgColour The foreground colour of the text.
 * @param bgColour The background colour of the text.
 * @param flags The flags of the string.
 */
void GUI_DrawText_Wrapper(const char *string, int16 left, int16 top, uint8 fgColour, uint8 bgColour, uint16 flags, ...)
{
	static char textBuffer[240];
	static uint16 displayedarg12low = -1;
	static uint16 displayedarg2mid  = -1;

	uint8 arg12low = flags & 0xF;
	uint8 arg2mid  = flags & 0xF0;

	if ((arg12low != displayedarg12low && arg12low != 0) || string == NULL) {
		switch (arg12low) {
			case 1:  Font_Select(g_fontNew6p); break;
			case 2:  Font_Select(g_fontNew8p); break;
			default: Font_Select(g_fontNew8p); break;
		}

		displayedarg12low = arg12low;
	}

	if ((arg2mid != displayedarg2mid && arg2mid != 0) || string == NULL) {
		uint8 colours[16];
		memset(colours, 0, sizeof(colours));

		switch (arg2mid) {
			case 0x0010:
				colours[2] = 0;
				colours[3] = 0;
				g_fontCharOffset = -2;
				break;

			case 0x0020:
				colours[2] = 12;
				colours[3] = 0;
				g_fontCharOffset = -1;
				break;

			case 0x0030:
				colours[2] = 12;
				colours[3] = 12;
				g_fontCharOffset = -1;
				break;

			case 0x0040:
				colours[2] = 232;
				colours[3] = 0;
				g_fontCharOffset = -1;
				break;
		}

		colours[0] = bgColour;
		colours[1] = fgColour;
		colours[4] = 6;

		GUI_InitColors(colours, 0, lengthof(colours) - 1);

		displayedarg2mid = arg2mid;
	}

	if (string == NULL) return;

	if (string != textBuffer) {
		char buf[256];
		va_list ap;

		strncpy(buf, string, sizeof(buf));

		va_start(ap, flags);
		vsnprintf(textBuffer, sizeof(textBuffer), buf, ap);
		va_end(ap);
	}

	switch (flags & 0x0F00) {
		case 0x100:
			left -= Font_GetStringWidth(textBuffer) / 2;
			break;

		case 0x200:
			left -= Font_GetStringWidth(textBuffer);
			break;
	}

	GUI_DrawText(textBuffer, left, top, fgColour, bgColour);
}

/**
 * Do something on the given colour in the given palette.
 *
 * @param palette The palette to work on.
 * @param colour The colour to modify.
 * @param reference The colour to use as reference.
 */
static bool GUI_Palette_2BA5_00A2(uint8 *palette, uint16 colour, uint16 reference)
{
	bool ret = false;
	uint16 i;

	colour *= 3;
	reference *= 3;

	for (i = 0; i < 3; i++) {
		if (palette[reference] != palette[colour]) {
			ret = true;
			palette[colour] += (palette[colour] > palette[reference]) ? -1 : 1;
		}
		colour++;
		reference++;
	}

	return ret;
}

/**
 * Animate the palette. Only works for some colours or something
 */
void GUI_PaletteAnimate(void)
{
	static int64_t timerAnimation = 0;
	static int64_t timerSelection = 0;
	static int64_t timerToggle = 0;

	if (timerAnimation < Timer_GetTicks()) {
		static bool animationToggle = false;

		uint16 colour;

		colour = (g_variable_37B2 == 0 && animationToggle) ? 6 : 15;
		memcpy(g_palette1 + 3 * 239, g_palette1 + 3 * colour, 3);

		GFX_SetPalette(g_palette1);

		animationToggle = !animationToggle;
		timerAnimation = Timer_GetTicks() + 60;
	}

	if (timerSelection < Timer_GetTicks() && g_selectionType != SELECTIONTYPE_MENTAT) {
		static uint16 selectionStateColour = 15;

		GUI_Palette_2BA5_00A2(g_palette1, 255, selectionStateColour);
		GUI_Palette_2BA5_00A2(g_palette1, 255, selectionStateColour);
		GUI_Palette_2BA5_00A2(g_palette1, 255, selectionStateColour);

		if (!GUI_Palette_2BA5_00A2(g_palette1, 255, selectionStateColour)) {
			if (selectionStateColour == 13) {
				selectionStateColour = 15;

				if (g_selectionType == SELECTIONTYPE_PLACE) {
					if (g_selectionState != 0) {
						selectionStateColour = (g_selectionState < 0) ? 5 : 15;
					} else {
						selectionStateColour = 6;
					}
				}
			} else {
				selectionStateColour = 13;
			}
		}

		GFX_SetPalette(g_palette1);

		timerSelection = Timer_GetTicks() + 3;
	}

	if (timerToggle < Timer_GetTicks()) {
		static uint16 toggleColour = 12;

		GUI_Palette_2BA5_00A2(g_palette1, 223, toggleColour);

		if (!GUI_Palette_2BA5_00A2(g_palette1, 223, toggleColour)) {
			toggleColour = (toggleColour == 12) ? 10 : 12;
		}

		GFX_SetPalette(g_palette1);

		timerToggle = Timer_GetTicks() + 5;
	}

	/* Sound_StartSpeech(); */
}

/**
 * Sets the activity description to the correct string for the active structure.
 * @see g_productionStringID
 */
void GUI_UpdateProductionStringID(void)
{
	Structure *s = NULL;

	s = Structure_Get_ByPackedTile(g_selectionPosition);

	g_productionStringID = STR_NULL;

	if (s == NULL) return;

	if (!g_table_structureInfo[s->o.type].o.flags.factory) {
		if (s->o.type == STRUCTURE_PALACE) g_productionStringID = g_table_houseInfo[s->o.houseID].specialWeapon + 0x29;
		return;
	}

	if (s->o.flags.s.upgrading) {
		g_productionStringID = STR_UPGRADINGD_DONE;
		return;
	}

	if (s->o.linkedID == 0xFF) {
		g_productionStringID = STR_BUILD_IT;
		return;
	}

	if (s->o.flags.s.onHold) {
		g_productionStringID = STR_ON_HOLD;
		return;
	}

	if (s->countDown != 0) {
		g_productionStringID = STR_D_DONE;
		return;
	}

	if (s->o.type == STRUCTURE_CONSTRUCTION_YARD) {
		g_productionStringID = STR_PLACE_IT;
		return;
	}

	g_productionStringID = STR_COMPLETED;
}

#if 0
/* Moved to gui/menu_opendune.c. */
static void GUI_Widget_SetProperties(uint16 index, uint16 xpos, uint16 ypos, uint16 width, uint16 height);
extern uint16 GUI_DisplayModalMessage(const char *str, uint16 spriteID, ...);
#endif

/**
 * Splits the given text in lines of maxwidth width using the given delimiter.
 * @param str The text to split.
 * @param maxwidth The maximum width the text will have.
 * @param delimiter The char used as delimiter.
 * @return The number of lines.
 */
uint16 GUI_SplitText(char *str, uint16 maxwidth, char delimiter)
{
	uint16 lines = 0;

	if (str == NULL) return 0;

	while (*str != '\0') {
		uint16 width = 0;

		lines++;

		while (width < maxwidth && *str != delimiter && *str != '\r' && *str != '\0') width += Font_GetCharWidth(*str++);

		if (width >= maxwidth) {
			while (*str != 0x20 && *str != delimiter && *str != '\r' && *str != '\0') width -= Font_GetCharWidth(*str--);
		}

		if (*str != '\0') *str++ = delimiter;
	}

	return lines;
}

/**
 * Draws a sprite.
 * @param screenID On which screen to draw the sprite.
 * @param sprite The sprite to draw.
 * @param posX ??.
 * @param posY ??.
 * @param windowID The ID of the window where the drawing is done.
 * @param flags The flags.
 * @param ... The extra args, flags dependant.
 *
 * flags  0x0001 = flip horizontal
 *        0x0002 = flip vertical
 *        0x0004 = ?
 *        0x0100 = remap colours: uint8 remap[256], int16 remap_count
 *        0x0200 = blur tile (sandworm, sonic tank)
 *        0x1000 = ?: uint16
 *        0x2000 = remap colours: uint8 remap[16]
 *        0x4000 = position relative to window
 *        0x8000 = centre sprite
 */
void GUI_DrawSprite(uint16 screenID, uint8 *sprite, int16 posX, int16 posY, uint16 windowID, uint16 flags, ...)
{
	int16 x = posX;
	int16 y = posY;
	int spriteID = -1;
	uint8 houseID = HOUSE_HARKONNEN;
	VARIABLE_NOT_USED(screenID);

	for (int i = 0; i < 512; i++) {
		if (sprite == g_sprites[i]) {
			spriteID = i;
			break;
		}
	}

	/* XXX: Attempt to find house by remap colour. */
	{
		va_list ap;

		va_start(ap, flags);

		if ((flags & 0x2000) != 0) {
			uint8 *loc3E = va_arg(ap, uint8*);
			for (int i = 15; i >= 0; i--) {
				if ((loc3E[i] >= 0x90) && ((loc3E[i] - 0x90) % 16 <= 0x08)) {
					houseID = (loc3E[i] - 0x90) / 16;
					break;
				}
			}
		}
		else if (flags & 0x100) {
			uint8 *remap = va_arg(ap, uint8*);

			if (remap[0x90] >= 0x90)
				houseID = (remap[0x90] - 0x90) / 16;
		}

		va_end(ap);
	}

	if (flags & 0x4000) {
		x += g_widgetProperties[windowID].xBase*8;
		y += g_widgetProperties[windowID].yBase;
	}

	if (flags & 0x8000) {
		x -= Sprite_GetWidth(sprite) / 2;
		y -= Sprite_GetHeight(sprite) / 2;
	}

	if (spriteID >= 0) {
		Video_DrawShape(spriteID, houseID, x, y, flags & 0x03);
	}
}

void GUI_DrawSprite_(uint16 screenID, uint8 *sprite, int16 posX, int16 posY, uint16 windowID, uint16 flags, ...)
{
	const uint16 s_variable_60[8]   = {1, 3, 2, 5, 4, 3, 2, 1};

	static uint16 s_variable_5E     = 0;
	static uint16 s_variable_70     = 1;
	static uint16 s_variable_72     = 0x8B55;
	static uint16 s_variable_74     = 0x51EC;

	va_list ap;

	int16  top;
	int16  bottom;
	uint16 width;
	uint16 loc10;
	int16  loc12;
	int16  loc14;
	int16  loc16;
	int16  loc1A;
	int16  loc1C;
	int16  loc1E;
	int16  loc20;
	uint16 loc22;
	uint8 *remap = NULL;
	int16  remapCount = 0;
	int16  loc2A;
	uint16 loc30 = 0;
	uint16 loc32;
	uint16 loc34;
	uint8 *loc38 = NULL;
	int16  loc3A;
	uint8 *loc3E = NULL;
	uint16 loc44;
	uint16 locbx;

	uint8 *buf = NULL;
	uint8 *b = NULL;
	int16  count;

	if (sprite == NULL) return;

	if ((*sprite & 0x1) != 0) flags |= 0x400;

	va_start(ap, flags);

	if ((flags & 0x2000) != 0) loc3E = va_arg(ap, uint8*);

	/* Remap */
	if ((flags & 0x100) != 0) {
		remap = va_arg(ap, uint8*);
		remapCount = (int16)va_arg(ap, int);
		if (remapCount == 0) flags &= 0xFEFF;
	}

	if ((flags & 0x200) != 0) {
		s_variable_5E = (s_variable_5E + 1) % 8;
		s_variable_70 = s_variable_60[s_variable_5E];
		s_variable_74 = 0x0;
		s_variable_72 = 0x100;
	}

	if ((flags & 0x1000) != 0) s_variable_72 = (uint16)va_arg(ap, int);

	if ((flags & 0x4) != 0) {
		loc30 = (uint16)va_arg(ap, int);
		loc32 = (uint16)va_arg(ap, int);
	} else {
		loc32 = 0x100;
	}

	va_end(ap);

	loc34 = 0;

	buf = GFX_Screen_Get_ByIndex(screenID);
	buf += g_widgetProperties[windowID].xBase << 3;

	if ((flags & 0x4000) == 0) posX -= g_widgetProperties[windowID].xBase << 3;

	width = g_widgetProperties[windowID].width << 3;
	top = g_widgetProperties[windowID].yBase;

	if ((flags & 0x4000) != 0) posY += g_widgetProperties[windowID].yBase;

	bottom = g_widgetProperties[windowID].yBase + g_widgetProperties[windowID].height;

	loc10 = *(uint16 *)sprite;
	sprite += 2;

	loc12 = *sprite++;

	if ((flags & 0x4) != 0) {
		loc12 *= loc32;
		loc12 >>= 8;
		if (loc12 == 0) return;
	}

	if ((flags & 0x8000) != 0) posY -= loc12 / 2;

	loc1A = *(uint16 *)sprite;
	sprite += 2;

	loc14 = loc1A;

	if ((flags & 0x4) != 0) {
		loc14 += loc30;
		loc14 >>= 8;
		if (loc14 == 0) return;
	}

	if ((flags & 0x8000) != 0) posX -= loc14 / 2;

	loc16 = loc14;

	sprite += 3;

	locbx = *(uint16 *)sprite;
	sprite += 2;

	if ((loc10 & 0x1) != 0 && (flags & 0x2000) == 0) loc3E = sprite;

	if ((flags & 0x400) != 0) {
		sprite += 16;
	}

	if ((loc10 & 0x2) == 0) {
		Format80_Decode(g_spriteBuffer, sprite, locbx);

		sprite = g_spriteBuffer;
	}

	if ((flags & 0x2) == 0) {
		loc2A = posY - top;
	} else {
		loc2A = bottom - posY - loc12;
	}

	if (loc2A < 0) {
		loc12 += loc2A;
		if (loc12 <= 0) return;

		loc2A = -loc2A;

		while (loc2A > 0) {
			loc38 = sprite;
			count = loc1A;
			loc1C = loc1A;

			assert((flags & 0xFF) < 4);

			while (count > 0) {
				while (count != 0) {
					count--;
					if (*sprite++ == 0) break;
				}
				if (sprite[-1] != 0 && count == 0) break;

				count -= *sprite++ - 1;
			}

			buf += count * (((flags & 0xFF) == 0 || (flags & 0xFF) == 2) ? -1 : 1);

			loc34 += loc32;
			if ((loc34 & 0xFF00) == 0) continue;

			loc2A -= loc34 >> 8;
			loc34 &= 0xFF;
		}

		if (loc2A < 0) {
			sprite = loc38;

			loc2A = -loc2A;
			loc34 += loc2A << 8;
		}

		if ((flags & 0x2) == 0) posY = top;
	}

	if ((flags & 0x2) == 0) {
		loc1E = bottom - posY;
	} else {
		loc1E = posY + loc12 - top;
	}

	if (loc1E <= 0) return;

	if (loc1E < loc12) {
		loc12 = loc1E;
		if ((flags & 0x2) != 0) posY = top;
	}

	loc1E = 0;
	if (posX < 0) {
		loc14 += posX;
		loc1E = -posX;
		if (loc1E >= loc16) return;
		posX = 0;
	}

	loc20 = 0;
	loc3A = width - posX;
	if (loc3A <= 0) return;

	if (loc3A < loc14) {
		loc14 = loc3A;
		loc20 = loc16 - loc1E - loc14;
	}

	loc3A = SCREEN_WIDTH;
	loc22 = posY;

	if ((flags & 0x2) != 0) {
		loc3A = - loc3A;
		loc22 += loc12 - 1;
	}

	if (windowID == WINDOWID_RENDER_TEXTURE) {
		buf += width * loc22 + posX;
	}
	else {
		buf += SCREEN_WIDTH * loc22 + posX;
	}

	if ((flags & 0x1) != 0) {
		uint16 tmp = loc1E;
		loc1E = loc20;
		loc20 = tmp;
		buf += loc14 - 1;
	}

	b = buf;

	if ((flags & 0x4) != 0) {
		loc20 = 0;
		loc44 = loc1E;
		loc1E = (loc44 << 8) / loc30;
	}

	if ((loc34 & 0xFF00) == 0) {
	l__04A4:
		while (true) {
			loc34 += loc32;

			if ((loc34 & 0xFF00) != 0) break;
			count = loc1A;
			loc1C = loc1A;

			assert((flags & 0xFF) < 4);

			while (count > 0) {
				while (count != 0) {
					count--;
					if (*sprite++ == 0) break;
				}
				if (sprite[-1] != 0 && count == 0) break;

				count -= *sprite++ - 1;
			}

			buf += count * (((flags & 0xFF) == 0 || (flags & 0xFF) == 2) ? -1 : 1);
		}
		loc38 = sprite;
	}

	if (windowID == WINDOWID_RENDER_TEXTURE)
		loc3A = width;

	while (true) {
		loc1C = loc1A;
		count = loc1E;

		assert((flags & 0xFF) < 4);

		while (count > 0) {
			while (count != 0) {
				count--;
				if (*sprite++ == 0) break;
			}
			if (sprite[-1] != 0 && count == 0) break;

			count -= *sprite++ - 1;
		}

		buf += count * (((flags & 0xFF) == 0 || (flags & 0xFF) == 2) ? -1 : 1);

		if (loc1C != 0) {
			count += loc14;
			if (count > 0) {
				uint8 v;

				while (count > 0) {
					v = *sprite++;
					if (v == 0) {
						buf += *sprite * (((flags & 0xFF) == 0 || (flags & 0xFF) == 2) ? 1 : -1);
						count -= *sprite++;
						continue;
					}

					assert(((flags >> 8) & 0xF) < 8);
					switch ((flags >> 8) & 0xF) {
						case 0:
							*buf = v;
							break;

						case 1: {
							int16 i;

							for(i = 0; i < remapCount; i++) v = remap[v];

							*buf = v;

							break;
						}

						case 2:
							s_variable_74 += s_variable_72;

							if ((s_variable_74 & 0xFF00) == 0) {
								*buf = v;
							} else {
								s_variable_74 &= 0xFF;
								*buf = buf[s_variable_70];
							}
							break;

						case 3: case 7: {
							int16 i;

							v = *buf;

							for(i = 0; i < remapCount; i++) v = remap[v];

							*buf = v;

							break;
						}

						case 4:
							*buf = loc3E[v];
							break;

						case 5: {
							int16 i;

							v = loc3E[v];

							for(i = 0; i < remapCount; i++) v = remap[v];

							*buf = v;

							break;
						}

						case 6:
							s_variable_74 += s_variable_72;

							if ((s_variable_74 & 0xFF00) == 0) {
								*buf = loc3E[v];
							} else {
								s_variable_74 &= 0xFF;
								*buf = buf[s_variable_70];
							}
							break;
					}

					buf += (((flags & 0xFF) == 0 || (flags & 0xFF) == 2) ? 1 : -1);
					count--;
				}
			}

			count += loc20;
			if (count != 0) {
				while (count > 0) {
					while (count != 0) {
						count--;
						if (*sprite++ == 0) break;
					}
					if (sprite[-1] != 0 && count == 0) break;

					count -= *sprite++ - 1;
				}

				buf += count * (((flags & 0xFF) == 0 || (flags & 0xFF) == 2) ? -1 : 1);
			}
		}

		b += loc3A;
		buf = b;

		if (--loc12 == 0) return;

		loc34 -= 0x100;
		if ((loc34 & 0xFF00) == 0) goto l__04A4;
		sprite = loc38;
	}
}

/**
 * Updates the score.
 * @param score The base score.
 * @param harvestedAllied Pointer to the total amount of spice harvested by allies.
 * @param harvestedEnemy Pointer to the total amount of spice harvested by enemies.
 * @param houseID The houseID of the player.
 */
uint16 Update_Score(int16 score, uint16 *harvestedAllied, uint16 *harvestedEnemy, uint8 houseID)
{
	PoolFindStruct find;
	uint16 locdi = 0;
	uint16 targetTime;
	uint16 loc0C = 0;
	uint32 tmp;

	if (score < 0) score = 0;

	find.houseID = houseID;
	find.type    = 0xFFFF;
	find.index   = 0xFFFF;

	while (true) {
		Structure *s;

		s = Structure_Find(&find);
		if (s == NULL) break;

		score += g_table_structureInfo[s->o.type].o.buildCredits / 100;
	}

	g_var_38BC++;

	find.houseID = HOUSE_INVALID;
	find.type    = UNIT_HARVESTER;
	find.index   = 0xFFFF;

	while (true) {
		Unit *u;

		u = Unit_Find(&find);
		if (u == NULL) break;

		if (House_AreAllied(Unit_GetHouseID(u), g_playerHouseID)) {
			locdi += u->amount * 7;
		} else {
			loc0C += u->amount * 7;
		}
	}

	g_var_38BC--;

	tmp = *harvestedEnemy + loc0C;
	*harvestedEnemy = (tmp > 65000) ? 65000 : (tmp & 0xFFFF);

	tmp = *harvestedAllied + locdi;
	*harvestedAllied = (tmp > 65000) ? 65000 : (tmp & 0xFFFF);

	score += House_Get_ByIndex(houseID)->credits / 100;

	if (score < 0) score = 0;

	targetTime = g_campaignID * 45;

	if (s_ticksPlayed < targetTime) {
		score += targetTime - s_ticksPlayed;
	}

	return score;
}

/**
 * Draws a string on a filled rectangle.
 * @param string The string to draw.
 * @param top The most top position where to draw the string.
 */
void GUI_DrawTextOnFilledRectangle(const char *string, uint16 top)
{
	uint16 halfWidth;

	GUI_DrawText_Wrapper(NULL, 0, 0, 0, 0, 0x121);

	halfWidth = (Font_GetStringWidth(string) / 2) + 4;

	GUI_DrawFilledRectangle(SCREEN_WIDTH / 2 - halfWidth, top, SCREEN_WIDTH / 2 + halfWidth, top + 6, 116);
	GUI_DrawText_Wrapper(string, SCREEN_WIDTH / 2, top, 0xF, 0, 0x121);
}

static uint16 GUI_HallOfFame_GetRank(uint16 score)
{
	uint8 i;

	for (i = 0; i < lengthof(_rankScores); i++) {
		if (_rankScores[i].score > score) break;
	}

	return min(i, lengthof(_rankScores) - 1);
}

#if 0
/* Moved to gui/menu_opendune.c */
static void GUI_HallOfFame_DrawRank(uint16 score, bool fadeIn);
static void GUI_HallOfFame_DrawBackground(uint16 score, bool hallOfFame);
static void GUI_EndStats_Sleep(uint16 delay);
extern void GUI_EndStats_Show(uint16 killedAllied, uint16 killedEnemy, uint16 destroyedAllied, uint16 destroyedEnemy, uint16 harvestedAllied, uint16 harvestedEnemy, int16 score, uint8 houseID);
extern uint8 GUI_PickHouse(void);
#endif

/**
 * Creates a palette mapping: colour -> colour + reference * intensity.
 *
 * @param palette The palette to create the mapping for.
 * @param colours The resulting mapping.
 * @param reference The colour to use as reference.
 * @param intensity The intensity to use.
 */
void GUI_Palette_CreateMapping(uint8 *palette, uint8 *colours, uint8 reference, uint8 intensity)
{
	uint16 index;

	if (palette == NULL || colours == NULL) return;

	colours[0] = 0;

	for (index = 1; index < 256; index++) {
		uint16 i;
		uint8 red   = palette[3 * index + 0] - (((palette[3 * index + 0] - palette[3 * reference + 0]) * (intensity / 2)) >> 7);
		uint8 blue  = palette[3 * index + 1] - (((palette[3 * index + 1] - palette[3 * reference + 1]) * (intensity / 2)) >> 7);
		uint8 green = palette[3 * index + 2] - (((palette[3 * index + 2] - palette[3 * reference + 2]) * (intensity / 2)) >> 7);
		uint8 colour = reference;
		uint16 sumMin = 0xFFFF;

		for (i = 1; i < 256; i++) {
			uint16 sum = 0;

			sum += (palette[3 * i + 0] - red)   * (palette[3 * i + 0] - red);
			sum += (palette[3 * i + 1] - blue)  * (palette[3 * i + 1] - blue);
			sum += (palette[3 * i + 2] - green) * (palette[3 * i + 2] - green);

			if (sum > sumMin) continue;
			if ((i != reference) && (i == index)) continue;

			sumMin = sum;
			colour = i & 0xFF;
		}

		colours[index] = colour;
	}
}

/**
 * Draw a border.
 *
 * @param left Left position of the border.
 * @param top Top position of the border.
 * @param width Width of the border.
 * @param height Height of the border.
 * @param colourSchemaIndex Index of the colourSchema used.
 * @param fill True if you want the border to be filled.
 */
void GUI_DrawBorder(uint16 left, uint16 top, uint16 width, uint16 height, uint16 colourSchemaIndex, bool fill)
{
	uint16 *colourSchema;

	width  -= 1;
	height -= 1;

	colourSchema = s_colourBorderSchema[colourSchemaIndex];

	if (fill) GUI_DrawFilledRectangle(left, top, left + width, top + height, colourSchema[0] & 0xFF);

	GUI_DrawLine(left, top + height, left + width, top + height, colourSchema[1] & 0xFF);
	GUI_DrawLine(left + width, top, left + width, top + height, colourSchema[1] & 0xFF);
	GUI_DrawLine(left, top, left + width, top, colourSchema[2] & 0xFF);
	GUI_DrawLine(left, top, left, top + height, colourSchema[2] & 0xFF);

	GFX_PutPixel(left, top + height, colourSchema[3] & 0xFF);
	GFX_PutPixel(left + width, top, colourSchema[3] & 0xFF);
}

/**
 * Display a hint to the user. Only show each hint exactly once.
 *
 * @param stringID The string of the hint to show.
 * @param spriteID The sprite to show with the hint.
 * @return Zero or the return value of GUI_DisplayModalMessage.
 */
uint16 GUI_DisplayHint(uint16 stringID, uint16 spriteID)
{
	uint32 *hintsShown;
	uint32 mask;
	uint16 hint;

	if (g_debugGame || stringID == STR_NULL || !g_gameConfig.hints || g_selectionType == SELECTIONTYPE_MENTAT) return 0;

	hint = stringID - STR_YOU_MUST_BUILD_A_WINDTRAP_TO_PROVIDE_POWER_TO_YOUR_BASE_WITHOUT_POWER_YOUR_STRUCTURES_WILL_DECAY;

	assert(hint < 64);

	if (hint < 32) {
		mask = (1 << hint);
		hintsShown = &g_hintsShown1;
	} else {
		mask = (1 << (hint - 32));
		hintsShown = &g_hintsShown2;
	}

	if ((*hintsShown & mask) != 0) return 0;
	*hintsShown |= mask;

	return GUI_DisplayModalMessage(String_Get_ByIndex(stringID), spriteID);
}

#if 0
/* Moved to video/video_opendune.c. */
extern void GUI_DrawProgressbar(uint16 current, uint16 max);
#endif

/**
 * Draw the interface (borders etc etc) and radar on the screen.
 * @param screenID The screen to draw the radar on.
 */
void GUI_DrawInterfaceAndRadar(uint16 screenID)
{
	PoolFindStruct find;
	uint16 oldScreenID;

	oldScreenID = GFX_Screen_SetActive((screenID == 0) ? 2 : screenID);

	g_viewport_forceRedraw = true;

	MenuBar_Draw(g_playerHouseID);

	g_textDisplayNeedsUpdate = true;

	GUI_Widget_Viewport_RedrawMap(g_screenActiveID);

	GUI_DrawScreen(g_screenActiveID);

	GUI_Widget_ActionPanel_Draw(true);

#if 0
	find.houseID = HOUSE_INVALID;
	find.index   = 0xFFFF;
	find.type    = 0xFFFF;

	while (true) {
		Structure *s;

		s = Structure_Find(&find);
		if (s == NULL) break;

		Structure_UpdateMap(s);
	}
#endif

	find.houseID = HOUSE_INVALID;
	find.index   = 0xFFFF;
	find.type    = 0xFFFF;

	while (true) {
		Unit *u;

		u = Unit_Find(&find);
		if (u == NULL) break;

		Unit_UpdateMap(1, u);
	}

	GUI_DrawCredits(g_playerHouseID, (g_playerCredits == 0xFFFF) ? 2 : 1);
	/* XXX: what is this for? */
	/* GUI_SetPaletteAnimated(g_palette1, 15); */

	MenuBar_DrawRadarAnimation();

	GFX_Screen_SetActive(oldScreenID);
	Input_History_Clear();
}

/**
 * Draw the credits on the screen, and animate it when the value is changing.
 * @param houseID The house to display the credits from.
 * @param mode The mode of displaying. 0 = animate, 1 = force draw, 2 = reset.
 */
void GUI_DrawCredits(uint8 houseID, uint16 mode)
{
	static int64_t l_tickCreditsAnimation = 0;    /*!< Next tick when credits animation needs an update. */
	static uint16 creditsAnimation = 0;           /* How many credits are shown in current animation of credits. */
	static int16  creditsAnimationOffset = 0;     /* Offset of the credits for the animation of credits. */

	House *h;
	int16 creditsDiff;
	int16 creditsNew;
	int16 creditsOld;
	int16 offset;

	if (l_tickCreditsAnimation > Timer_GetTicks() && mode == 0) return;
	l_tickCreditsAnimation = Timer_GetTicks() + 1;

	h = House_Get_ByIndex(houseID);

	if (mode == 2) {
		g_playerCredits = h->credits;
		creditsAnimation = h->credits;
	}

	if (mode == 0 && h->credits == creditsAnimation && creditsAnimationOffset == 0) return;

	creditsDiff = h->credits - creditsAnimation;
	if (creditsDiff != 0) {
		int16 diff = creditsDiff / 4;
		if (diff == 0)   diff = (creditsDiff < 0) ? -1 : 1;
		if (diff > 128)  diff = 128;
		if (diff < -128) diff = -128;
		creditsAnimationOffset += diff;
	} else {
		creditsAnimationOffset = 0;
	}

	if (creditsDiff != 0 && (creditsAnimationOffset < -7 || creditsAnimationOffset > 7)) {
		Audio_PlayEffect(creditsDiff > 0 ? EFFECT_CREDITS_INCREASE : EFFECT_CREDITS_DECREASE);
	}

	if (creditsAnimationOffset < 0 && creditsAnimation == 0) creditsAnimationOffset = 0;

	creditsAnimation += creditsAnimationOffset / 8;

	if (creditsAnimationOffset > 0) creditsAnimationOffset &= 7;
	if (creditsAnimationOffset < 0) creditsAnimationOffset = -((-creditsAnimationOffset) & 7);

	creditsOld = creditsAnimation;
	creditsNew = creditsAnimation;
	offset = 1;

	if (creditsAnimationOffset < 0) {
		creditsOld -= 1;
		if (creditsOld < 0) creditsOld = 0;

		offset -= 8;
	}

	if (creditsAnimationOffset > 0) {
		creditsNew += 1;
	}

	g_playerCredits = creditsOld;

#if 0
	uint16 oldScreenID = GFX_Screen_SetActive(2);
	uint16 oldValue_07AE_0000 = Widget_SetCurrentWidget(4);
	char charCreditsOld[7];
	char charCreditsNew[7];
	int i;

	snprintf(charCreditsOld, sizeof(charCreditsOld), "%6d", creditsOld);
	snprintf(charCreditsNew, sizeof(charCreditsNew), "%6d", creditsNew);

	for (i = 0; i < 6; i++) {
		uint16 left = i * 10 + 4;
		uint16 spriteID;

		spriteID = (charCreditsOld[i] == ' ') ? 13 : charCreditsOld[i] - 34;

		if (charCreditsOld[i] != charCreditsNew[i]) {
			GUI_DrawSprite(g_screenActiveID, g_sprites[spriteID], left, offset - creditsAnimationOffset, 4, 0x4000);
			if (creditsAnimationOffset == 0) continue;

			spriteID = (charCreditsNew[i] == ' ') ? 13 : charCreditsNew[i] - 34;

			GUI_DrawSprite(g_screenActiveID, g_sprites[spriteID], left, offset + 8 - creditsAnimationOffset, 4, 0x4000);
		} else {
			GUI_DrawSprite(g_screenActiveID, g_sprites[spriteID], left, 1, 4, 0x4000);
		}
	}

	if (oldScreenID != g_screenActiveID) {
		GUI_Mouse_Hide_InWidget(5);
		GUI_Screen_Copy(g_curWidgetXBase, g_curWidgetYBase, g_curWidgetXBase, g_curWidgetYBase - 40, g_curWidgetWidth, g_curWidgetHeight, g_screenActiveID, oldScreenID);
		GUI_Mouse_Show_InWidget();
	}

	GFX_Screen_SetActive(oldScreenID);

	Widget_SetCurrentWidget(oldValue_07AE_0000);
#else
	MenuBar_DrawCredits(creditsNew, creditsOld, offset - creditsAnimationOffset);
#endif
}

/**
 * Change the selection type.
 * @param selectionType The new selection type.
 */
void GUI_ChangeSelectionType(uint16 selectionType)
{
	uint16 oldScreenID;

	if (selectionType == SELECTIONTYPE_UNIT && !Unit_AnySelected()) {
		selectionType = SELECTIONTYPE_STRUCTURE;
	}

	if (selectionType == SELECTIONTYPE_STRUCTURE && Unit_AnySelected()) {
		Unit_UnselectAll();
	}

	oldScreenID = GFX_Screen_SetActive(2);

	if (g_selectionType != selectionType) {
		uint16 oldSelectionType = g_selectionType;

		Timer_SetTimer(TIMER_GAME, false);

		g_selectionType = selectionType;
		g_selectionTypeNew = selectionType;
		g_var_37B8 = true;

		switch (oldSelectionType) {
			case SELECTIONTYPE_TARGET:
			case SELECTIONTYPE_PLACE:
				Map_SetSelection(g_structureActivePosition);
				/* Fall-through */
			case SELECTIONTYPE_STRUCTURE:
				GUI_DisplayText(NULL, -1);
				break;

			case SELECTIONTYPE_UNIT:
				if (Unit_AnySelected() && selectionType != SELECTIONTYPE_TARGET && selectionType != SELECTIONTYPE_UNIT) {
					for (Unit *u = Unit_FirstSelected(); u != NULL; u = Unit_NextSelected(u)) {
						Unit_UpdateMap(2, u);
					}

					Unit_UnselectAll();
				}
				break;

			default:
				break;
		}

		if (g_table_selectionType[oldSelectionType].variable_04 && g_table_selectionType[selectionType].variable_06) {
			g_viewport_forceRedraw = true;
			g_var_3A14 = true;

			GUI_DrawInterfaceAndRadar(0);
		}

		Widget_SetCurrentWidget(g_table_selectionType[selectionType].defaultWidget);

		if (g_curWidgetIndex != 0) {
			GUI_Widget_DrawBorder(g_curWidgetIndex, 0, false);
		}

		if (selectionType != SELECTIONTYPE_MENTAT) {
			Widget *w = g_widgetLinkedListHead;

			while (w != NULL) {
				const int8 *s = g_table_selectionType[selectionType].visibleWidgets;

				w->state.s.selected = false;
				w->flags.s.invisible = true;

				for (; *s != -1; s++) {
					if (*s == w->index) {
						w->flags.s.invisible = false;
						break;
					}
				}

				GUI_Widget_Draw(w);
				w = GUI_Widget_GetNext(w);
			}

			GUI_Widget_DrawAll(g_widgetLinkedListHead);
			g_textDisplayNeedsUpdate = true;
		}

		switch (g_selectionType) {
			case SELECTIONTYPE_MENTAT:
				if (oldSelectionType != SELECTIONTYPE_INTRO) {
					Video_SetCursor(SHAPE_CURSOR_NORMAL);
				}

				Widget_SetCurrentWidget(g_table_selectionType[selectionType].defaultWidget);
				break;

			case SELECTIONTYPE_TARGET:
				g_structureActivePosition = g_selectionPosition;
				GUI_Widget_ActionPanel_Draw(true);
				Timer_SetTimer(TIMER_GAME, true);
				break;

			case SELECTIONTYPE_PLACE:
				Unit_UnselectAll();
				GUI_Widget_ActionPanel_Draw(true);

				Map_SetSelectionSize(g_table_structureInfo[g_structureActiveType].layout);

				Timer_SetTimer(TIMER_GAME, true);
				break;

			case SELECTIONTYPE_UNIT:
				GUI_Widget_ActionPanel_Draw(true);

				Timer_SetTimer(TIMER_GAME, true);
				break;

			case SELECTIONTYPE_STRUCTURE:
				GUI_Widget_ActionPanel_Draw(true);

				Timer_SetTimer(TIMER_GAME, true);
				break;

			default: break;
		}
	}

	GFX_Screen_SetActive(oldScreenID);
}

/**
 * Sets the colours to be used when drawing chars.
 * @param colours The colours to use.
 * @param min The index of the first colour to set.
 * @param max The index of the last colour to set.
 */
void GUI_InitColors(const uint8 *colours, uint8 first, uint8 last)
{
	uint8 i;

	first &= 0xF;
	last &= 0xF;

	if (last < first || colours == NULL) return;

	for (i = first; i < last + 1; i++) g_colours[i] = *colours++;
}

#if 0
/* Moved to video/video_opendune.c. */
static uint16 GetNeededClipping(int16 x, int16 y);
static void ClipTop(int16 *x1, int16 *y1, int16 x2, int16 y2);
static void ClipBottom(int16 *x1, int16 *y1, int16 x2, int16 y2);
static void ClipLeft(int16 *x1, int16 *y1, int16 x2, int16 y2);
static void ClipRight(int16 *x1, int16 *y1, int16 x2, int16 y2);
extern void GUI_DrawLine(int16 x1, int16 y1, int16 x2, int16 y2, uint8 colour);
extern void GUI_SetClippingArea(uint16 left, uint16 top, uint16 right, uint16 bottom);
#endif

/**
 * Wrapper around GFX_Screen_Copy. Protects against wrong input values.
 * @param xSrc The X-coordinate on the source divided by 8.
 * @param ySrc The Y-coordinate on the source.
 * @param xDst The X-coordinate on the destination divided by 8.
 * @param yDst The Y-coordinate on the destination.
 * @param width The width divided by 8.
 * @param height The height.
 * @param screenSrc The ID of the source screen.
 * @param screenDst The ID of the destination screen.
 */
void GUI_Screen_Copy(int16 xSrc, int16 ySrc, int16 xDst, int16 yDst, int16 width, int16 height, int16 screenSrc, int16 screenDst)
{
	if (width  > SCREEN_WIDTH / 8) width  = SCREEN_WIDTH / 8;
	if (height > SCREEN_HEIGHT)    height = SCREEN_HEIGHT;

	if (xSrc < 0) {
		xDst -= xSrc;
		width += xSrc;
		xSrc = 0;
	}

	if (xSrc >= SCREEN_WIDTH / 8 || xDst >= SCREEN_WIDTH / 8) return;

	if (xDst < 0) {
		xSrc -= xDst;
		width += xDst;
		xDst = 0;
	}

	if (ySrc < 0) {
		yDst -= ySrc;
		height += ySrc;
		ySrc = 0;
	}

	if (yDst < 0) {
		ySrc -= yDst;
		height += yDst;
		yDst = 0;
	}

	GFX_Screen_Copy(xSrc * 8, ySrc, xDst * 8, yDst, width * 8, height, screenSrc, screenDst);
}

static uint32 GUI_FactoryWindow_CreateWidgets(void)
{
	uint16 i;
	uint16 count = 0;
	WidgetInfo *wi = g_table_factoryWidgetInfo;
	Widget *w = s_factoryWindowWidgets;

	memset(w, 0, 13 * sizeof(Widget));

	for (i = 0; i < 13; i++, wi++) {
		if ((i == 8 || i == 9 || i == 10 || i == 12) && !g_factoryWindowStarport) continue;
		if (i == 11 && g_factoryWindowStarport) continue;
		if (i == 7 && g_factoryWindowUpgradeCost == 0) continue;

		count++;

		w->index     = i + 46;
		w->state.all = 0x0;
		w->offsetX   = wi->offsetX;
		w->offsetY   = wi->offsetY;
		w->flags.all = wi->flags;
		w->shortcut  = (wi->shortcut < 0) ? abs(wi->shortcut) : GUI_Widget_GetShortcut(*String_Get_ByIndex(wi->shortcut));
		w->clickProc = wi->clickProc;
		w->width     = wi->width;
		w->height    = wi->height;

		if (wi->spriteID < 0) {
			w->drawModeNormal   = DRAW_MODE_NONE;
			w->drawModeSelected = DRAW_MODE_NONE;
			w->drawModeDown     = DRAW_MODE_NONE;
		} else {
			w->drawModeNormal   = DRAW_MODE_SPRITE;
			w->drawModeSelected = DRAW_MODE_SPRITE;
			w->drawModeDown     = DRAW_MODE_SPRITE;
			w->drawParameterNormal.sprite   = g_sprites[wi->spriteID];
			w->drawParameterSelected.sprite = g_sprites[wi->spriteID + 1];
			w->drawParameterDown.sprite     = g_sprites[wi->spriteID + 1];
		}

		if (i != 0) {
			g_widgetInvoiceTail = GUI_Widget_Link(g_widgetInvoiceTail, w);
		} else {
			g_widgetInvoiceTail = w;
		}

		w++;
	}

	GUI_Widget_DrawAll(g_widgetInvoiceTail);

	return count * sizeof(Widget);
}

static uint32 GUI_FactoryWindow_LoadGraymapTbl(void)
{
	uint8 fileID;

	fileID = File_Open("GRAYRMAP.TBL", 1);
	File_Read(fileID, s_factoryWindowGraymapTbl, 256);
	File_Close(fileID);

	return 256;
}

static uint16 GUI_FactoryWindow_CalculateStarportPrice(uint16 credits)
{
	credits = (credits / 10) * 4 + (credits / 10) * (Tools_RandomRange(0, 6) + Tools_RandomRange(0, 6));

	return min(credits, 999);
}

static int GUI_FactoryWindow_Sorter(const void *a, const void *b)
{
	const FactoryWindowItem *pa = a;
	const FactoryWindowItem *pb = b;

	return pb->sortPriority - pa->sortPriority;
}

void GUI_FactoryWindow_InitItems(enum StructureType s)
{
	g_factoryWindowTotal = 0;
	g_factoryWindowSelected = 0;
	g_factoryWindowBase = 0;

	memset(g_factoryWindowItems, 0, 25 * sizeof(FactoryWindowItem));

	if (s == STRUCTURE_STARPORT) {
		uint16 seconds = (g_timerGame - g_tickScenarioStart) / 60;
		uint16 seed = (seconds / 60) + g_scenarioID + g_playerHouseID;
		seed *= seed;

		srand(seed);
	}

	if (s != STRUCTURE_CONSTRUCTION_YARD) {
		uint16 i;

		for (i = 0; i < UNIT_MAX; i++) {
			ObjectInfo *oi = &g_table_unitInfo[i].o;

			if (oi->available == 0) continue;

			g_factoryWindowItems[g_factoryWindowTotal].objectInfo = oi;
			g_factoryWindowItems[g_factoryWindowTotal].objectType = i;

			if (s == STRUCTURE_STARPORT) {
				g_factoryWindowItems[g_factoryWindowTotal].credits = GUI_FactoryWindow_CalculateStarportPrice(oi->buildCredits);
			} else {
				g_factoryWindowItems[g_factoryWindowTotal].credits = oi->buildCredits;
			}

			g_factoryWindowItems[g_factoryWindowTotal].sortPriority = oi->sortPriority;

			g_factoryWindowTotal++;
		}
	} else {
		uint16 i;

		for (i = 0; i < STRUCTURE_MAX; i++) {
			ObjectInfo *oi = &g_table_structureInfo[i].o;

			if (oi->available == 0) continue;

			g_factoryWindowItems[g_factoryWindowTotal].objectInfo    = oi;
			g_factoryWindowItems[g_factoryWindowTotal].objectType    = i;
			g_factoryWindowItems[g_factoryWindowTotal].credits       = oi->buildCredits;
			g_factoryWindowItems[g_factoryWindowTotal].sortPriority  = oi->sortPriority;

			if (i == 0 || i == 1) g_factoryWindowItems[g_factoryWindowTotal].sortPriority = 0x64;

			g_factoryWindowTotal++;
		}
	}

	if (g_factoryWindowTotal == 0) {
#if 0
		GUI_DisplayModalMessage("ERROR: No items in construction list!", 0xFFFF);
		PrepareEnd();
		exit(0);
#else
		return;
#endif
	}

	qsort(g_factoryWindowItems, g_factoryWindowTotal, sizeof(FactoryWindowItem), GUI_FactoryWindow_Sorter);
}

static void GUI_FactoryWindow_Init(Structure *s)
{
	static uint8 xSrc[HOUSE_MAX] = { 0, 0, 16, 0, 0, 0 };
	static uint8 ySrc[HOUSE_MAX] = { 8, 152, 48, 0, 0, 0 };
	uint16 oldScreenID;
	void *wsa;
	int16 i;
	ObjectInfo *oi;

	oldScreenID = GFX_Screen_SetActive(2);

	Sprites_LoadImage("CHOAM.CPS", 3, NULL);
	GUI_DrawSprite(2, g_sprites[11], 192, 0, 0, 0); /* "Credits" */

	GUI_Palette_RemapScreen(0, 0, SCREEN_WIDTH, SCREEN_HEIGHT, 2, g_remap);

	GUI_Screen_Copy(xSrc[g_playerHouseID], ySrc[g_playerHouseID], 0, 8, 7, 40, 2, 2);
	GUI_Screen_Copy(xSrc[g_playerHouseID], ySrc[g_playerHouseID], 0, 152, 7, 40, 2, 2);

	GUI_FactoryWindow_CreateWidgets();
	GUI_FactoryWindow_LoadGraymapTbl();
	GUI_FactoryWindow_InitItems(s->o.type);

	for (i = g_factoryWindowTotal; i < 4; i++) GUI_Widget_MakeInvisible(GUI_Widget_Get_ByIndex(g_widgetInvoiceTail, i + 46));

	for (i = 0; i < 4; i++) {
		FactoryWindowItem *item = GUI_FactoryWindow_GetItem(i);

		if (item == NULL) continue;

		oi = item->objectInfo;
		if (oi->available == -1) {
			GUI_DrawSprite(2, g_sprites[oi->spriteID], 72, 24 + i * 32, 0, 0x100, s_factoryWindowGraymapTbl, 1);
		} else {
			GUI_DrawSprite(2, g_sprites[oi->spriteID], 72, 24 + i * 32, 0, 0);
		}
	}

	g_factoryWindowBase = 0;
	g_factoryWindowSelected = 0;

	oi = g_factoryWindowItems[0].objectInfo;

	wsa = WSA_LoadFile(oi->wsa, s_factoryWindowWsaBuffer, sizeof(s_factoryWindowWsaBuffer), false);
	WSA_DisplayFrame(wsa, 0, 128, 48, 2);
	WSA_Unload(wsa);

	GUI_Mouse_Hide_Safe();
	GUI_Screen_Copy(0, 0, 0, 0, SCREEN_WIDTH / 8, SCREEN_HEIGHT, 2, 0);
	GUI_Mouse_Show_Safe();

	GUI_DrawFilledRectangle(64, 0, 112, SCREEN_HEIGHT - 1, GFX_GetPixel(72, 23));

	GUI_FactoryWindow_PrepareScrollList();

	GFX_Screen_SetActive(0);

	GUI_FactoryWindow_DrawDetails();

	GUI_DrawCredits(g_playerHouseID, 1);

	GFX_Screen_SetActive(oldScreenID);
}

/**
 * Display the window where you can order/build stuff for a structure.
 * @param var06 Unknown.
 * @param isStarPort True if this is for a starport.
 * @param var0A Unknown.
 * @return Unknown value.
 */
FactoryResult GUI_DisplayFactoryWindow(Structure *s, uint16 upgradeCost)
{
	uint16 oldScreenID = GFX_Screen_SetActive(0);
	uint8 backup[3];

	memcpy(backup, g_palette1 + 255 * 3, 3);

	g_factoryWindowConstructionYard = (s->o.type == STRUCTURE_CONSTRUCTION_YARD);
	g_factoryWindowStarport = (s->o.type == STRUCTURE_STARPORT);
	g_factoryWindowUpgradeCost = upgradeCost;
	g_factoryWindowOrdered = 0;

	GUI_FactoryWindow_Init(s);

	GUI_FactoryWindow_UpdateSelection(true);

	g_factoryWindowResult = FACTORY_CONTINUE;
	while (g_factoryWindowResult == FACTORY_CONTINUE) {
		uint16 event;

		GUI_DrawCredits(g_playerHouseID, 0);

		GUI_FactoryWindow_UpdateSelection(false);

		event = GUI_Widget_HandleEvents(g_widgetInvoiceTail);

		if (event == 0x6E) GUI_Production_ResumeGame_Click(NULL);

		GUI_PaletteAnimate();
		Video_Tick();
		sleepIdle();
	}

	GUI_DrawCredits(g_playerHouseID, 1);

	GFX_Screen_SetActive(oldScreenID);

	GUI_FactoryWindow_B495_0F30();

	memcpy(g_palette1 + 255 * 3, backup, 3);

	GFX_SetPalette(g_palette1);

	/* Visible credits have to be reset, as it might not be the real value */
	g_playerCredits = 0xFFFF;

	return g_factoryWindowResult;
}

char *GUI_String_Get_ByIndex(int16 stringID)
{
	extern char g_savegameDesc[5][51];

	switch (stringID) {
		case -5: case -4: case -3: case -2: case -1: {
			char *s = g_savegameDesc[abs((int16)stringID + 1)];
			if (*s == '\0') return NULL;
			return s;
		}

		case -10:
			stringID = (g_enable_music ? STR_ON : STR_OFF);
			break;

		case -11:
			stringID = (g_enable_sounds ? STR_ON : STR_OFF);
			break;

		case -12: {
			static uint16 gameSpeedStrings[] = {
				STR_SLOWEST,
				STR_SLOW,
				STR_NORMAL,
				STR_FAST,
				STR_FASTEST
			};

			stringID = gameSpeedStrings[g_gameConfig.gameSpeed];
		} break;

		case -13:
			stringID = (g_gameConfig.hints != 0) ? STR_ON : STR_OFF;
			break;

		case -14:
			stringID = (g_gameConfig.autoScroll != 0) ? STR_ON : STR_OFF;
			break;

		default: break;
	}

	return String_Get_ByIndex(stringID);
}

#if 0
/* Moved to gui/menu_opendune.c. */
static void GUI_StrategicMap_AnimateArrows(void);
static void GUI_StrategicMap_AnimateSelected(uint16 selected, StrategicMapData *data);
static bool GUI_StrategicMap_GetRegion(uint16 region);
static void GUI_StrategicMap_SetRegion(uint16 region, bool set);
static int16 GUI_StrategicMap_ClickedRegion(void);
static bool GUI_StrategicMap_FastForwardToggleWithESC(void);
static void GUI_StrategicMap_DrawText(const char *string)
static uint16 GUI_StrategicMap_ScenarioSelection(uint16 campaignID);
static void GUI_StrategicMap_ReadHouseRegions(uint8 houseID, uint16 campaignID);
static void GUI_StrategicMap_DrawRegion(uint8 houseId, uint16 region, bool progressive);
static void GUI_StrategicMap_PrepareRegions(uint16 campaignID);
static void GUI_StrategicMap_ShowProgression(uint16 campaignID);
extern uint16 GUI_StrategicMap_Show(uint16 campaignID, bool win);
#endif

/**
 * Clear the screen.
 * @param screenID Which screen to clear.
 */
void GUI_ClearScreen(uint16 screenID)
{
	uint16 oldScreenID = GFX_Screen_SetActive(screenID);

	GFX_ClearScreen();

	GFX_Screen_SetActive(oldScreenID);
}

/**
 * Draw a string to the screen using a fixed width for each char.
 *
 * @param string The string to draw.
 * @param left The most left position where to draw the string.
 * @param top The most top position where to draw the string.
 * @param fgColour The foreground colour of the text.
 * @param bgColour The background colour of the text.
 * @param charWidth The width of a char.
 */
void GUI_DrawText_Monospace(const char *string, uint16 left, uint16 top, uint8 fgColour, uint8 bgColour, uint16 charWidth)
{
	char s[2] = " ";

	while (*string != '\0') {
		*s = *string++;
		GUI_DrawText(s, left, top, fgColour, bgColour);
		left += charWidth;
	}
}

void GUI_FactoryWindow_B495_0F30(void)
{
	GUI_Mouse_Hide_Safe();
	GFX_Screen_Copy2(69, ((g_factoryWindowSelected + 1) * 32) + 5, 69, (g_factoryWindowSelected * 32) + 21, 38, 30, 2, 0, false);
	GUI_Mouse_Show_Safe();
}

FactoryWindowItem *GUI_FactoryWindow_GetItem(int16 offset)
{
	offset += g_factoryWindowBase;

	if (offset < 0 || offset >= g_factoryWindowTotal) return NULL;

	return &g_factoryWindowItems[offset];
}

void GUI_FactoryWindow_DrawDetails(void)
{
	uint16 oldScreenID;
	FactoryWindowItem *item = GUI_FactoryWindow_GetItem(g_factoryWindowSelected);
	ObjectInfo *oi = item->objectInfo;
	void *wsa;

	oldScreenID = GFX_Screen_SetActive(2);

	wsa = WSA_LoadFile(oi->wsa, s_factoryWindowWsaBuffer, sizeof(s_factoryWindowWsaBuffer), false);
	WSA_DisplayFrame(wsa, 0, 128, 48, 2);
	WSA_Unload(wsa);

	if (g_factoryWindowConstructionYard) {
		const StructureInfo *si;
		int16 x = 288;
		int16 y = 136;
		uint8 *sprite;
		uint16 width;
		uint16 i;
		uint16 j;

		GUI_DrawSprite(g_screenActiveID, g_sprites[64], x, y, 0, 0);
		x++;
		y++;

		sprite = g_sprites[24];
		width = Sprite_GetWidth(sprite) + 1;
		si = &g_table_structureInfo[item->objectType];

		for (j = 0; j < g_table_structure_layoutSize[si->layout].height; j++) {
			for (i = 0; i < g_table_structure_layoutSize[si->layout].width; i++) {
				GUI_DrawSprite(g_screenActiveID, sprite, x + i * width, y + j * width, 0, 0);
			}
		}
	}

	if (oi->available == -1) {
		GUI_Palette_RemapScreen(128, 48, 184, 112, 2, s_factoryWindowGraymapTbl);

		if (g_factoryWindowStarport) {
			GUI_DrawText_Wrapper(String_Get_ByIndex(STR_OUT_OF_STOCK), 220, 99, 6, 0, 0x132);
		} else {
			GUI_DrawText_Wrapper(String_Get_ByIndex(STR_NEED_STRUCTURE_UPGRADE), 220, 94, 6, 0, 0x132);

			if (g_factoryWindowUpgradeCost != 0) {
				GUI_DrawText_Wrapper(String_Get_ByIndex(STR_UPGRADE_COST_D), 220, 104, 6, 0, 0x132, g_factoryWindowUpgradeCost);
			} else {
				GUI_DrawText_Wrapper(String_Get_ByIndex(STR_REPAIR_STRUCTURE_FIRST), 220, 104, 6, 0, 0x132);
			}
		}
	} else {
		if (g_factoryWindowStarport) {
			GUI_Screen_Copy(16, 99, 16, 160, 23, 9, 2, 2);
			GUI_Screen_Copy(16, 99, 16, 169, 23, 9, 2, 2);
			GUI_DrawText_Wrapper(String_Get_ByIndex(STR_OUT_OF_STOCK), 220, 169, 6, 0, 0x132);

			GUI_FactoryWindow_UpdateDetails();
		}
	}

	GUI_Mouse_Hide_Safe();
	GUI_Screen_Copy(16, 48, 16, 48, 23, 112, 2, oldScreenID);
	GUI_Mouse_Show_Safe();

	GFX_Screen_SetActive(oldScreenID);

	GUI_FactoryWindow_DrawCaption(NULL);
}

void GUI_FactoryWindow_DrawCaption(const char *caption)
{
	uint16 oldScreenID;

	oldScreenID = GFX_Screen_SetActive(2);

	GUI_DrawFilledRectangle(128, 21, 310, 35, 116);

	if (caption != NULL && *caption != '\0') {
		GUI_DrawText_Wrapper(caption, 128, 23, 12, 0, 0x12);
	} else {
		FactoryWindowItem *item = GUI_FactoryWindow_GetItem(g_factoryWindowSelected);
		ObjectInfo *oi = item->objectInfo;
		uint16 width;

		GUI_DrawText_Wrapper(String_Get_ByIndex(oi->stringID_full), 128, 23, 12, 0, 0x12);

		width = Font_GetStringWidth(String_Get_ByIndex(STR_COST_999));
		GUI_DrawText_Wrapper(String_Get_ByIndex(STR_COST_3D), 310 - width, 23, 12, 0, 0x12, item->credits);

		if (g_factoryWindowStarport) {
			width += Font_GetStringWidth(String_Get_ByIndex(STR_QTY_99)) + 2;
			GUI_DrawText_Wrapper(String_Get_ByIndex(STR_QTY_2D), 310 - width, 23, 12, 0, 0x12, item->amount);
		}
	}

	GUI_Mouse_Hide_Safe();
	if (oldScreenID == 0) GFX_Screen_Copy2(128, 21, 128, 21, 182, 14, 3, oldScreenID, false);
	GUI_Mouse_Show_Safe();

	GFX_Screen_SetActive(oldScreenID);
}

void GUI_FactoryWindow_UpdateDetails(void)
{
	FactoryWindowItem *item = GUI_FactoryWindow_GetItem(g_factoryWindowSelected);
	ObjectInfo *oi = item->objectInfo;

	if (oi->available == -1) return;

	GUI_Mouse_Hide_Safe();
	GUI_Screen_Copy(16, (oi->available == item->amount) ? 169 : 160, 16, 99, 23, 9, 2, g_screenActiveID);
	GUI_Mouse_Show_Safe();
}

/**
 * Update the selection in the factory window.
 * If \a selectionChanged, it draws the rectangle around the new entry.
 * In addition, the palette colour of the rectangle is slowly changed back and
 * forth between white and the house colour by palette changes, thus giving it
 * the appearance of glowing.
 * @param selectionChanged User has selected a new thing to build.
 */
void GUI_FactoryWindow_UpdateSelection(bool selectionChanged)
{
	static int64_t paletteChangeTimer;
	static int8 paletteColour;
	static int8 paletteChange;

	if (selectionChanged) {
		uint16 y;

		memset(g_palette1 + 255 * 3, 0x3F, 3);

		GFX_SetPalette(g_palette1);

		paletteChangeTimer = 0;
		paletteColour = 0;
		paletteChange = 8;

		y = g_factoryWindowSelected * 32 + 24;

		GUI_Mouse_Hide_Safe();
		GUI_DrawWiredRectangle(71, y - 1, 104, y + 24, 255);
		GUI_DrawWiredRectangle(72, y, 103, y + 23, 255);
		GUI_Mouse_Show_Safe();
	} else {
		if (paletteChangeTimer > Timer_GetTicks()) return;
	}

	paletteChangeTimer = Timer_GetTicks() + 3;
	paletteColour += paletteChange;

	if (paletteColour < 0 || paletteColour > 63) {
		paletteChange = -paletteChange;
		paletteColour += paletteChange;
	}

	switch (g_playerHouseID) {
		case HOUSE_HARKONNEN:
			*(g_palette1 + 255 * 3 + 1) = paletteColour;
			*(g_palette1 + 255 * 3 + 2) = paletteColour;
			break;

		case HOUSE_ATREIDES:
			*(g_palette1 + 255 * 3 + 0) = paletteColour;
			*(g_palette1 + 255 * 3 + 1) = paletteColour;
			break;

		case HOUSE_ORDOS:
			*(g_palette1 + 255 * 3 + 0) = paletteColour;
			*(g_palette1 + 255 * 3 + 2) = paletteColour;
			break;

		default: break;
	}

	GFX_SetPalette(g_palette1);
}

/**
 * Fade in parts of the screen from one screenbuffer to the other screenbuffer.
 * @param xSrc The X-position to start in the source screenbuffer divided by 8.
 * @param ySrc The Y-position to start in the source screenbuffer.
 * @param xDst The X-position to start in the destination screenbuffer divided by 8.
 * @param yDst The Y-position to start in the destination screenbuffer.
 * @param width The width of the screen to copy divided by 8.
 * @param height The height of the screen to copy.
 * @param screenSrc The ID of the source screen.
 * @param screenDst The ID of the destination screen.
 */
void GUI_Screen_FadeIn(uint16 xSrc, uint16 ySrc, uint16 xDst, uint16 yDst, uint16 width, uint16 height, uint16 screenSrc, uint16 screenDst)
{
	uint16 offsetsY[100];
	uint16 offsetsX[40];
	int x, y;

	if (screenDst == 0) {
		GUI_Mouse_Hide_InRegion(xDst << 3, yDst, (xDst + width) << 3, yDst + height);
	}

	height /= 2;

	for (x = 0; x < width;  x++) offsetsX[x] = x;
	for (y = 0; y < height; y++) offsetsY[y] = y;

	for (x = 0; x < width; x++) {
		uint16 index;
		uint16 temp;

		index = Tools_RandomRange(0, width - 1);

		temp = offsetsX[index];
		offsetsX[index] = offsetsX[x];
		offsetsX[x] = temp;
	}

	for (y = 0; y < height; y++) {
		uint16 index;
		uint16 temp;

		index = Tools_RandomRange(0, height - 1);

		temp = offsetsY[index];
		offsetsY[index] = offsetsY[y];
		offsetsY[y] = temp;
	}

	for (y = 0; y < height; y++) {
		uint16 y2 = y;
		for (x = 0; x < width; x++) {
			uint16 offsetX, offsetY;

			offsetX = offsetsX[x];
			offsetY = offsetsY[y2];

			GUI_Screen_Copy(xSrc + offsetX, ySrc + offsetY * 2, xDst + offsetX, yDst + offsetY * 2, 1, 2, screenSrc, screenDst);

			y2++;
			if (y2 == height) y2 = 0;
		}

		/* XXX -- This delays the system so you can in fact see the animation */
		if ((y % 4) == 0) {
			Video_Tick();
			Timer_Wait();
		}
	}

	if (screenDst == 0) {
		GUI_Mouse_Show_InRegion();
	}
}

void GUI_FactoryWindow_PrepareScrollList(void)
{
	FactoryWindowItem *item;

	GUI_Mouse_Hide_Safe();
	GUI_Screen_Copy(9, 24, 9, 40, 4, 128, 0, 2);
	GUI_Mouse_Show_Safe();

	item = GUI_FactoryWindow_GetItem(-1);

	if (item != NULL) {
		ObjectInfo *oi = item->objectInfo;

		if (oi->available == -1) {
			GUI_DrawSprite(2, g_sprites[oi->spriteID], 72, 8, 0, 0x100, s_factoryWindowGraymapTbl, 1);
		} else {
			GUI_DrawSprite(2, g_sprites[oi->spriteID], 72, 8, 0, 0);
		}
	} else {
		GUI_Screen_Copy(9, 32, 9, 24, 4, 8, 2, 2);
	}

	item = GUI_FactoryWindow_GetItem(4);

	if (item != NULL) {
		ObjectInfo *oi = item->objectInfo;

		if (oi->available == -1) {
			GUI_DrawSprite(2, g_sprites[oi->spriteID], 72, 168, 0, 0x100, s_factoryWindowGraymapTbl, 1);
		} else {
			GUI_DrawSprite(2, g_sprites[oi->spriteID], 72, 168, 0, 0);
		}
	} else {
		GUI_Screen_Copy(9, 0, 9, 168, 4, 8, 2, 2);
	}
}

/**
 * Fade in parts of the screen from one screenbuffer to the other screenbuffer.
 * @param x The X-position in the source and destination screenbuffers.
 * @param y The Y-position in the source and destination screenbuffers.
 * @param width The width of the screen to copy.
 * @param height The height of the screen to copy.
 * @param screenSrc The ID of the source screen.
 * @param screenDst The ID of the destination screen.
 * @param delay The delay.
 * @param skipNull Wether to copy pixels with colour 0.
 */
void GUI_Screen_FadeIn2(int16 x, int16 y, int16 width, int16 height, uint16 screenSrc, uint16 screenDst, uint16 delay, bool skipNull)
{
	uint16 oldScreenID;
	uint16 i;
	uint16 j;

	uint16 columns[SCREEN_WIDTH];
	uint16 rows[SCREEN_HEIGHT];

	assert(width <= SCREEN_WIDTH);
	assert(height <= SCREEN_HEIGHT);

	if (screenDst == 0) {
		GUI_Mouse_Hide_InRegion(x, y, x + width, y + height);
	}

	for (i = 0; i < width; i++)  columns[i] = i;
	for (i = 0; i < height; i++) rows[i] = i;

	for (i = 0; i < width; i++) {
		uint16 tmp;

		j = Tools_RandomRange(0, width - 1);

		tmp = columns[j];
		columns[j] = columns[i];
		columns[i] = tmp;
	}

	for (i = 0; i < height; i++) {
		uint16 tmp;

		j = Tools_RandomRange(0, height - 1);

		tmp = rows[j];
		rows[j] = rows[i];
		rows[i] = tmp;
	}

	oldScreenID = GFX_Screen_SetActive(screenDst);

	for (j = 0; j < height; j++) {
		uint16 j2 = j;

		for (i = 0; i < width; i++) {
			uint8 colour;
			uint16 curX = x + columns[i];
			uint16 curY = y + rows[j2];

			if (++j2 >= height) j2 = 0;

			GFX_Screen_SetActive(screenSrc);

			colour = GFX_GetPixel(curX, curY);

			GFX_Screen_SetActive(screenDst);

			if (skipNull && colour == 0) continue;

			GFX_PutPixel(curX, curY, colour);
		}

		Video_Tick();
		Timer_Sleep(delay);
	}

	if (screenDst == 0) {
		GUI_Mouse_Show_InRegion();
	}

	GFX_Screen_SetActive(oldScreenID);
}

#if 0
/* Moved to video/video_opendune.c. */
extern void GUI_Mouse_Show(void);
extern void GUI_Mouse_Hide(void);
extern void GUI_Mouse_Hide_Safe(void);
extern void GUI_Mouse_Show_Safe(void);
extern void GUI_Mouse_Show_InRegion(void);
extern void GUI_Mouse_Hide_InRegion(uint16 left, uint16 top, uint16 right, uint16 bottom);
extern void GUI_Mouse_Show_InWidget(void);
extern void GUI_Mouse_Hide_InWidget(uint16 widgetIndex);
#endif

/**
 * Draws a chess-pattern filled rectangle.
 * @param left The X-position of the rectangle.
 * @param top The Y-position of the rectangle.
 * @param width The width of the rectangle.
 * @param height The height of the rectangle.
 * @param colour The colour of the rectangle.
 */
void GUI_DrawBlockedRectangle(int16 left, int16 top, int16 width, int16 height, uint8 colour)
{
	uint8 *screen;

	if (width <= 0) return;
	if (height <= 0) return;
	if (left >= SCREEN_WIDTH) return;
	if (top >= SCREEN_HEIGHT) return;

	if (left < 0) {
		if (left + width <= 0) return;
		width += left;
		left = 0;
	}
	if (top < 0) {
		if (top + height <= 0) return;
		height += top;
		top = 0;
	}

	if (left + width >= SCREEN_WIDTH) {
		width = SCREEN_WIDTH - left;
	}
	if (top + height >= SCREEN_HEIGHT) {
		height = SCREEN_HEIGHT - top;
	}

	screen = GFX_Screen_GetActive();
	screen += top * SCREEN_WIDTH + left;

	for (; height > 0; height--) {
		int i = width;

		if ((height & 1) != (width & 1)) {
			screen++;
			i--;
		}

		for (; i > 0; i -= 2) {
			*screen = colour;
			screen += 2;
		}

		screen += SCREEN_WIDTH - width - (height & 1);
	}
}

#if 0
/* Moved to video/video_opendune.c. */
extern void GUI_Mouse_SetPosition(uint16 x, uint16 y);
#endif

/**
 * Remap all the colours in the region with the ones indicated by the remap palette.
 * @param left The left of the region to remap.
 * @param top The top of the region to remap.
 * @param width The width of the region to remap.
 * @param height The height of the region to remap.
 * @param screenID The screen to do the remapping on.
 * @param remap The pointer to the remap palette.
 */
void GUI_Palette_RemapScreen(uint16 left, uint16 top, uint16 width, uint16 height, uint16 screenID, uint8 *remap)
{
	uint8 *screen = GFX_Screen_Get_ByIndex(screenID);

	screen += top * SCREEN_WIDTH + left;
	for (; height > 0; height--) {
		int i;
		for (i = width; i > 0; i--) {
			uint8 pixel = *screen;
			*screen++ = remap[pixel];
		}
		screen += SCREEN_WIDTH - width;
	}
}

#if 0
/* Moved to gui/menu_opendune.c. */
extern uint16 GUI_HallOfFame_Tick(void);
#endif

static Widget *GUI_HallOfFame_CreateButtons(HallOfFameStruct *data)
{
	char *resumeString;
	char *clearString;
	Widget *wClear;
	Widget *wResume;
	uint16 width;

	memcpy(s_temporaryColourBorderSchema, s_colourBorderSchema, sizeof(s_colourBorderSchema));
	memcpy(s_colourBorderSchema, s_HOF_ColourBorderSchema, sizeof(s_colourBorderSchema));

	resumeString = String_Get_ByIndex(STR_RESUME_GAME2);
	clearString  = String_Get_ByIndex(STR_CLEAR_LIST);

	width = max(Font_GetStringWidth(resumeString), Font_GetStringWidth(clearString)) + 6;

	/* "Clear List" */
	wClear = GUI_Widget_Allocate(100, *clearString, 160 - width - 18, 180, 0xFFFE, 0x147);
	wClear->width     = width;
	wClear->height    = 10;
	wClear->flags.all = 0x44C5;
	wClear->data      = data;

	/* "Resume Game" */
	wResume = GUI_Widget_Allocate(101, *resumeString, 178, 180, 0xFFFE, 0x146);
	wResume->width     = width;
	wResume->height    = 10;
	wResume->flags.all = 0x44C5;
	wResume->data      = data;

	return GUI_Widget_Insert(wClear, wResume);
}

static void GUI_HallOfFame_DeleteButtons(Widget *w)
{
	while (w != NULL) {
		Widget *next = w->next;

		free(w);

		w = next;
	}

	memcpy(s_colourBorderSchema, s_temporaryColourBorderSchema, sizeof(s_temporaryColourBorderSchema));
}

static void GUI_HallOfFame_Encode(HallOfFameStruct *data)
{
	uint8 i;
	uint8 *d;

	for (d = (uint8 *)data, i = 0; i < 128; i++, d++) *d = (*d + i) ^ 0xA7;
}

static void GUI_HallOfFame_Decode(HallOfFameStruct *data)
{
	uint8 i;
	uint8 *d;

	for (d = (uint8 *)data, i = 0; i < 128; i++, d++) *d = (*d ^ 0xA7) - i;
}

static uint16 GUI_HallOfFame_InsertScore(HallOfFameStruct *data, uint16 score)
{
	uint16 i;
	for (i = 0; i < 8; i++, data++) {
		if (data->score >= score) continue;

		memmove(data + 1, data, 128);
		memset(data->name, 0, 6);
		data->score = score;
		data->houseID = g_playerHouseID;
		data->rank = GUI_HallOfFame_GetRank(score);
		data->campaignID = g_campaignID;

		return i + 1;
	}

	return 0;
}

void GUI_HallOfFame_Show(uint16 score)
{
	HallOfFameData *fame = &g_hall_of_fame_state;
	const uint16 old_widget = g_curWidgetIndex;

	uint16 width;
	uint16 editLine;
	Widget *w;
	uint8 fileID;
	HallOfFameStruct *data;

	if (score == 0xFFFF) {
		if (!File_Exists("SAVEFAME.DAT")) {
			return;
		}
		s_ticksPlayed = 0;
	}

	data = (HallOfFameStruct *)GFX_Screen_Get_ByIndex(5);

	if (!File_Exists("SAVEFAME.DAT")) {
		uint16 written;

		memset(data, 0, 128);

		GUI_HallOfFame_Encode(data);

		fileID = File_Open("SAVEFAME.DAT", 2);
		written = File_Write(fileID, data, 128);
		File_Close(fileID);

		if (written != 128) return;
	}

	File_ReadBlockFile("SAVEFAME.DAT", data, 128);

	GUI_HallOfFame_Decode(data);

	if (score == 0xFFFF) {
		editLine = 0;
	} else {
		editLine = GUI_HallOfFame_InsertScore(data, score);
	}

	width = GUI_HallOfFame_DrawData(data, false);

	if (editLine != 0) {
		WidgetProperties backupProperties;
		char *name;

		name = data[editLine - 1].name;

		memcpy(&backupProperties, &g_widgetProperties[19], sizeof(WidgetProperties));

		g_widgetProperties[19].xBase = 4;
		g_widgetProperties[19].yBase = (editLine - 1) * 11 + 90;
		g_widgetProperties[19].width = width / 8;
		g_widgetProperties[19].height = 11;
		g_widgetProperties[19].fgColourBlink = 6;
		g_widgetProperties[19].fgColourNormal = 116;

		GUI_DrawText_Wrapper(NULL, 0, 0, 0, 0, 0x22);

		while (true) {
			char *nameEnd;
			uint16 oldScreenID;

			oldScreenID = GFX_Screen_SetActive(0);
			Widget_SetAndPaintCurrentWidget(19);
			GFX_Screen_SetActive(oldScreenID);

			HallOfFame_DrawBackground(g_playerHouseID, true);
			HallOfFame_DrawScoreTime(fame->score, fame->time);
			HallOfFame_DrawRank(fame);

			const char backup = name[0];
			name[0] = '\0';
			GUI_HallOfFame_DrawData(data, false);
			name[0] = backup;

			Input_Tick(true);
			int ret = GUI_EditBox(name, 5, 19, NULL, NULL, 0);
			if (ret == SCANCODE_ENTER) {
				if (*name == '\0')
					continue;

				nameEnd = name + strlen(name) - 1;

				while (*nameEnd <= ' ' && nameEnd >= name)
					*nameEnd-- = '\0';

				break;
			}

			Video_Tick();
			sleepIdle();
		}

		memcpy(&g_widgetProperties[19], &backupProperties, sizeof(WidgetProperties));

		GUI_HallOfFame_Encode(data);
		fileID = File_Open("SAVEFAME.DAT", 2);
		File_Write(fileID, data, 128);
		File_Close(fileID);
		GUI_HallOfFame_Decode(data);
	}

	w = GUI_HallOfFame_CreateButtons(data);

	Input_History_Clear();

	GFX_Screen_SetActive(0);

	g_yesNoWindowDesc.stringID = STR_ARE_YOU_SURE_YOU_WANT_TO_CLEAR_THE_HIGH_SCORES;
	GUI_Window_Create(&g_yesNoWindowDesc);
	GUI_Widget_Get_ByIndex(g_widgetLinkedListTail, 30)->data = data;

	bool confirm_clear = false;
	while (true) {
		HallOfFame_DrawBackground(g_playerHouseID, true);
		if (score == 0xFFFF) {
			GUI_DrawText_Wrapper(String_Get_ByIndex(STR_HALL_OF_FAME2), SCREEN_WIDTH / 2, 15, 15, 0, 0x122);
		}
		else {
			HallOfFame_DrawScoreTime(fame->score, fame->time);
			HallOfFame_DrawRank(fame);
		}
		GUI_HallOfFame_DrawData(data, true);

		Input_Tick(true);
		if (confirm_clear) {
			const int ret = GUI_Widget_HOF_ClearList_Click(g_widgetLinkedListTail);

			GUI_Widget_DrawWindow(&g_yesNoWindowDesc);
			GUI_Widget_DrawAll(g_widgetLinkedListTail);

			if (ret == -1) {
				confirm_clear = false;
			}
			else if (ret == 1) {
				break;
			}
		}
		else {
			const uint16 key = GUI_Widget_HandleEvents(w);

			if (key == (0x8000 | 100)) { /* Clear list */
				confirm_clear = true;
			}
			else if (key == (0x8000 | 101)) { /* Resume */
				break;
			}
		}

		GUI_Widget_DrawAll(w);

		Video_Tick();
		sleepIdle();
	}

	GUI_HallOfFame_DeleteButtons(w);

	Input_History_Clear();
	Widget_SetCurrentWidget(old_widget);

	if (score == 0xFFFF) return;

	memset(g_palette1 + 255 * 3, 0, 3);
}

uint16 GUI_HallOfFame_DrawData(HallOfFameStruct *data, bool show)
{
	VARIABLE_NOT_USED(show);

	uint16 oldScreenID;
	char *scoreString;
	char *battleString;
	uint16 width = 0;
	uint16 offsetY;
	uint16 scoreX;
	uint16 battleX;
	uint8 i;

	oldScreenID = GFX_Screen_SetActive(2);
	GUI_DrawFilledRectangle(8, 80, 311, 178, 116);
	GUI_DrawText_Wrapper(NULL, 0, 0, 0, 0, 0x22);

	battleString = String_Get_ByIndex(STR_BATTLE);
	scoreString = String_Get_ByIndex(STR_SCORE);

	scoreX = 320 - Font_GetStringWidth(scoreString) / 2 - 12;
	battleX = scoreX - Font_GetStringWidth(scoreString) / 2 - 8 - Font_GetStringWidth(battleString) / 2;
	offsetY = 80;

	GUI_DrawText_Wrapper(String_Get_ByIndex(STR_NAME_AND_RANK), 32, offsetY, 8, 0, 0x22);
	GUI_DrawText_Wrapper(battleString, battleX, offsetY, 8, 0, 0x122);
	GUI_DrawText_Wrapper(scoreString, scoreX, offsetY, 8, 0, 0x122);

	offsetY = 90;
	for (i = 0; i < 8; i++, offsetY += 11) {
		char buffer[81];
		const char *p1, *p2;

		if (data[i].score == 0) break;

		if (g_config.language == LANGUAGE_FRENCH) {
			p1 = String_Get_ByIndex(_rankScores[data[i].rank].rankString);
			p2 = g_table_houseInfo[data[i].houseID].name;
		} else {
			p1 = g_table_houseInfo[data[i].houseID].name;
			p2 = String_Get_ByIndex(_rankScores[data[i].rank].rankString);
		}
		snprintf(buffer, sizeof(buffer), "%s, %s %s", data[i].name, p1, p2);

		if (*data[i].name == '\0') {
			width = battleX - 36 - Font_GetStringWidth(buffer);
		} else {
			GUI_DrawText_Wrapper(buffer, 32, offsetY, 15, 0, 0x22);
		}

		GUI_DrawText_Wrapper("%u.", 24, offsetY, 15, 0, 0x222, i + 1);
		GUI_DrawText_Wrapper("%u", battleX, offsetY, 15, 0, 0x122, data[i].campaignID);
		GUI_DrawText_Wrapper("%u", scoreX, offsetY, 15, 0, 0x122, data[i].score);
	}

	GFX_Screen_SetActive(oldScreenID);

	return width;
}

/**
 * Draw a filled rectangle using xor.
 * @param left The left position of the rectangle.
 * @param top The top position of the rectangle.
 * @param right The right position of the rectangle.
 * @param bottom The bottom position of the rectangle.
 * @param colour The colour of the rectangle.
 */
void GUI_DrawXorFilledRectangle(int16 left, int16 top, int16 right, int16 bottom, uint8 colour)
{
	uint16 x;
	uint16 y;
	uint16 height;
	uint16 width;

	uint8 *screen = GFX_Screen_GetActive();

	if (left >= SCREEN_WIDTH) return;
	if (left < 0) left = 0;

	if (top >= SCREEN_HEIGHT) return;
	if (top < 0) top = 0;

	if (right >= SCREEN_WIDTH) right = SCREEN_WIDTH - 1;
	if (right < 0) right = 0;

	if (bottom >= SCREEN_HEIGHT) bottom = SCREEN_HEIGHT - 1;
	if (bottom < 0) bottom = 0;

	if (left > right) return;
	if (top > bottom) return;

	screen += left + top * SCREEN_WIDTH;
	width = right - left + 1;
	height = bottom - top + 1;
	for (y = 0; y < height; y++) {
		for (x = 0; x < width; x++) {
			*screen++ ^= colour;
		}
		screen += SCREEN_WIDTH - width;
	}
}

/**
 * Create the remap palette for the givern house.
 * @param houseID The house ID.
 */
void GUI_Palette_CreateRemap(uint8 houseID)
{
	int16 i;
	int16 loc4;
	int16 loc6;
	uint8 *remap;

	remap = g_remap;
	for (i = 0; i < 0x100; i++, remap++) {
		*remap = i & 0xFF;

		loc6 = i / 16;
		loc4 = i % 16;
		if (loc6 == 9 && loc4 <= 6) {
			*remap = (houseID << 4) + 0x90 + loc4;
		}
	}
}

/**
 * Draw the screen.
 * This also handles animation tick and other viewport related activity.
 * @param screenID The screen to draw on.
 */
void GUI_DrawScreen(uint16 screenID)
{
	const WidgetInfo *wi = &g_table_gameWidgetInfo[GAME_WIDGET_VIEWPORT];

	static uint32 s_timerViewportMessage = 0;
	bool loc10;
	uint16 oldScreenID;
	uint16 xpos;

	if (g_selectionType == SELECTIONTYPE_MENTAT) return;
	if (g_selectionType == SELECTIONTYPE_DEBUG) return;
	if (g_selectionType == SELECTIONTYPE_UNKNOWN6) return;
	if (g_selectionType == SELECTIONTYPE_INTRO) return;

	loc10 = false;

	oldScreenID = GFX_Screen_SetActive(screenID);

	if (screenID != 0) g_viewport_forceRedraw = true;

	Explosion_Tick();
	Animation_Tick();
	Unit_Sort();

	if (!g_viewport_forceRedraw && g_viewportPosition != g_minimapPosition) {
		uint16 viewportX = Tile_GetPackedX(g_viewportPosition);
		uint16 viewportY = Tile_GetPackedY(g_viewportPosition);
		int16 xOffset = Tile_GetPackedX(g_minimapPosition) - viewportX; /* Horizontal offset between viewport and minimap. */
		int16 yOffset = Tile_GetPackedY(g_minimapPosition) - viewportY; /* Vertical offset between viewport and minmap. */

		/* Overlap remaining in tiles. */
		int16 xOverlap = 15 - abs(xOffset);
		int16 yOverlap = 10 - abs(yOffset);

		int16 x, y;

		if (xOverlap < 1 || yOverlap < 1) g_viewport_forceRedraw = true;

		if (!g_viewport_forceRedraw && (xOverlap != 15 || yOverlap != 10)) {
			Map_SetSelectionObjectPosition(0xFFFF);
			loc10 = true;

			GUI_Mouse_Hide_InWidget(2);

			GUI_Screen_Copy(max(-xOffset << 1, 0), 40 + max(-yOffset << 4, 0), max(0, xOffset << 1), 40 + max(0, yOffset << 4), xOverlap << 1, yOverlap << 4, 0, 2);
		} else {
			g_viewport_forceRedraw = true;
		}

		xOffset = max(0, xOffset);
		yOffset = max(0, yOffset);

		for (y = 0; y < 10; y++) {
			uint16 mapYBase = (y + viewportY) << 6;

			for (x = 0; x < 15; x++) {
				if (x >= xOffset && (xOffset + xOverlap) > x && y >= yOffset && (yOffset + yOverlap) > y && !g_viewport_forceRedraw) continue;

				Map_Update(x + viewportX + mapYBase, 0, true);
			}
		}
	}

	if (loc10) {
		Map_SetSelectionObjectPosition(0xFFFF);

		for (xpos = 0; xpos < 14; xpos++) {
			uint16 v = g_minimapPosition + xpos + 6*64;

			BitArray_Set(g_dirtyViewport, v);
			BitArray_Set(g_dirtyMinimap, v);

			g_dirtyViewportCount++;
		}
	}

	g_minimapPosition = g_viewportPosition;
	g_selectionRectanglePosition = g_selectionPosition;

	if (g_viewportMessageCounter != 0 && s_timerViewportMessage < Timer_GetTicks()) {
		g_viewportMessageCounter--;
		s_timerViewportMessage = Timer_GetTicks() + 60;

		for (xpos = 0; xpos < 14; xpos++) {
			Map_Update(g_viewportPosition + xpos + 6 * 64, 0, true);
		}
	}

	Video_SetClippingArea(wi->offsetX, wi->offsetY, wi->width, wi->height);
	GUI_Widget_Viewport_Draw(g_viewport_forceRedraw, loc10, screenID != 0);
	Video_SetClippingArea(0, 0, TRUE_DISPLAY_WIDTH, TRUE_DISPLAY_HEIGHT);

	g_viewport_forceRedraw = false;

	GFX_Screen_SetActive(oldScreenID);

	Map_SetSelectionObjectPosition(g_selectionRectanglePosition);
	Map_UpdateMinimapPosition(g_minimapPosition, false);

	GUI_Mouse_Show_InWidget();
}

/**
 * Set a new palette, but animate it in slowly.
 * @param palette The new palette.
 * @param ticksOfAnimation The amount of ticks it should take.
 */
void GUI_SetPaletteAnimated(uint8 *palette, int16 ticksOfAnimation)
{
	bool progress;
	int16 diffPerTick;
	int16 tickSlice;
	int16 highestDiff;
	int16 ticks;
	uint16 tickCurrent;
	uint8 data[256 * 3];
	int i;

	if (g_paletteActive == NULL || palette == NULL) return;

	memcpy(data, g_paletteActive, 256 * 3);

	highestDiff = 0;
	for (i = 0; i < 256 * 3; i++) {
		int16 diff = (int16)palette[i] - (int16)data[i];
		highestDiff = max(highestDiff, abs(diff));
	}

	ticks = ticksOfAnimation << 8;
	if (highestDiff != 0) ticks /= highestDiff;

	/* Find a nice value to change every timeslice */
	tickSlice = ticks;
	diffPerTick = 1;
	while (diffPerTick <= highestDiff && ticks < (2 << 8)) {
		ticks += tickSlice;
		diffPerTick++;
	}

	tickCurrent = 0;

	do {
		progress = false;

		tickCurrent  += (uint16)ticks;
		const int delay = tickCurrent >> 8;
		tickCurrent  &= 0xFF;

		for (i = 0; i < 256 * 3; i++) {
			int16 current = palette[i];
			int16 goal = data[i];

			if (goal == current) continue;

			if (goal < current) {
				goal = min(goal + diffPerTick, current);
				progress = true;
			}

			if (goal > current) {
				goal = max(goal - diffPerTick, current);
				progress = true;
			}

			data[i] = goal & 0xFF;
		}

		if (progress) {
			GFX_SetPalette(data);

			Video_Tick();
			Timer_Sleep(delay);
		}
	} while (progress);
}

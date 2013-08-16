/** @file src/gui/gui.c Generic GUI definitions. */

#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
#include <stdarg.h>
#include <string.h>
#include "enum_string.h"
#include "../os/common.h"
#include "../os/math.h"
#include "../os/sleep.h"
#include "../os/strings.h"
#include "../os/endian.h"

#include "gui.h"

#include "font.h"
#include "mentat.h"
#include "widget.h"
#include "../animation.h"
#include "../audio/audio.h"
#include "../codec/format80.h"
#include "../common_a5.h"
#include "../config.h"
#include "../enhancement.h"
#include "../explosion.h"
#include "../file.h"
#include "../gfx.h"
#include "../house.h"
#include "../ini.h"
#include "../input/input.h"
#include "../input/mouse.h"
#include "../load.h"
#include "../map.h"
#include "../newui/actionpanel.h"
#include "../newui/halloffame.h"
#include "../newui/menu.h"
#include "../newui/menubar.h"
#include "../newui/savemenu.h"
#include "../opendune.h"
#include "../pool/pool.h"
#include "../pool/house.h"
#include "../pool/structure.h"
#include "../pool/unit.h"
#include "../scenario.h"
#include "../sprites.h"
#include "../string.h"
#include "../structure.h"
#include "../table/locale.h"
#include "../table/widgetinfo.h"
#include "../timer/timer.h"
#include "../tools/random_lcg.h"
#include "../unit.h"
#include "../video/video.h"
#include "../wsa.h"

static uint8 g_colours[16];
uint8 g_palette_998A[3 * 256];
uint8 g_remap[256];
static uint8 s_temporaryColourBorderSchema[5][4];           /*!< Temporary storage for the #s_colourBorderSchema. */
uint16 g_productionStringID;                                /*!< Descriptive text of activity of the active structure. */
static uint32 s_ticksPlayed;

uint16 g_cursorSpriteID;

bool g_structureHighHealth;

uint16 g_viewportMessageCounter;                            /*!< Countdown counter for displaying #g_viewportMessageText, bit 0 means 'display the text'. */
const char *g_viewportMessageText;                          /*!< If not \c NULL, message text displayed in the viewport. */

uint16 g_viewportPosition;                                  /*!< Top-left tile of the viewport. */
int g_viewport_scrollOffsetX;
int g_viewport_scrollOffsetY;
float g_viewport_desiredDX;
float g_viewport_desiredDY;

uint16 g_selectionRectanglePosition;                        /*!< Position of the structure selection rectangle. */
uint16 g_selectionPosition;                                 /*!< Current selection position (packed). */
uint16 g_selectionWidth;                                    /*!< Width of the selection. */
uint16 g_selectionHeight;                                   /*!< Height of the selection. */
int16  g_selectionState = 1;                                /*!< State of the selection (\c 1 is valid, \c 0 is not valid, \c <0 valid but missing some slabs. */

/*!< Colours used for the border of widgets. */
uint8 s_colourBorderSchema[5][4] = {
	{ 26,  29,  29,  29},
	{ 20,  26,  16,  20},
	{ 20,  16,  26,  20},
	{233, 235, 232, 233},
	{233, 232, 235, 233}
};

/** Colours used for the border of widgets in the hall of fame. */
static const uint8 s_HOF_ColourBorderSchema[5][4] = {
	{226, 228, 228, 228},
	{116, 226, 105, 116},
	{116, 105, 226, 116},
	{233, 235, 232, 233},
	{233, 232, 235, 233}
};

assert_compile(lengthof(s_colourBorderSchema) == lengthof(s_temporaryColourBorderSchema));
assert_compile(lengthof(s_colourBorderSchema) == lengthof(s_HOF_ColourBorderSchema));

#if 0
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
static bool scrollInProgress;    /* Text is being scrolled (and partly visible to the user). */
static char displayLine1[80];    /* Current line being displayed. */
static char displayLine2[80];    /* Next line (if scrollInProgress, it is scrolled up). */
static uint16 textOffset;        /* Vertical position of text being scrolled. */

void GUI_DisplayText(const char *str, int16 importance, ...)
{
	char buffer[80];                 /* Formatting buffer of new message. */
	static int64_t displayTimer = 0; /* Timeout value for next update of the display. */

	static char displayLine3[80];    /* Next message to display (after scrolling next line has finished). */
	static int16 line1Importance;    /* Importance of the displayed line of text. */
	static int16 line2Importance;    /* Importance of the next line of text. */
	static int16 line3Importance;    /* Importance of the next message. */

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

	if (scrollInProgress) {
		if (buffer[0] != '\0') {
			if (strcasecmp(buffer, displayLine2) != 0 && importance >= line3Importance) {
				strncpy(displayLine3, buffer, sizeof(displayLine3));
				line3Importance = importance;
			}
		}
		if (displayTimer > Timer_GetTicks()) return;

#if 0
		uint16 oldValue_07AE_0000 = Widget_SetCurrentWidget(7);
		uint16 height;

		if (g_textDisplayNeedsUpdate) {
			Screen oldScreenID = GFX_Screen_SetActive(SCREEN_1);

			GUI_DrawFilledRectangle(0, 0, SCREEN_WIDTH - 1, 23, g_curWidgetFGColourNormal);

			GUI_DrawText_Wrapper(displayLine2, g_curWidgetXBase, fgColour2, 0, 0x012);
			GUI_DrawText_Wrapper(displayLine1, g_curWidgetXBase, 13, fgColour1, 0, 0x012);

			GFX_Screen_SetActive(oldScreenID);
		}

		GUI_Mouse_Hide_InWidget(7);

		if (textOffset + g_curWidgetHeight > 24) {
			height = 24 - textOffset;
		} else {
			height = g_curWidgetHeight;
		}

		GUI_Screen_Copy(g_curWidgetXBase/8, textOffset, g_curWidgetXBase/8, g_curWidgetYBase, g_curWidgetWidth/8, height, SCREEN_1, SCREEN_0);
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
		strncpy(displayLine1, displayLine2, sizeof(displayLine1));
		line1Importance = (line2Importance != 0) ? line2Importance - 1 : 0;

		/* And move line 3 to line 2. */
		strncpy(displayLine2, displayLine3, sizeof(displayLine2));
		line2Importance = line3Importance;
		displayLine3[0] = '\0';

		line3Importance = -1;
		displayTimer = Timer_GetTicks() + (line2Importance <= line1Importance ? 900 : 1);
		scrollInProgress = false;
		return;
	}

	if (buffer[0] != '\0') {
		/* If new line arrived, different from every line that is in the display buffers, and more important than existing messages,
		 * insert it at the right place.
		 */
		if (strcasecmp(buffer, displayLine1) != 0 && strcasecmp(buffer, displayLine2) != 0 && strcasecmp(buffer, displayLine3) != 0) {
			/* This was originally importance >= line2Importance.
			 * However, that means that newer messages of equal
			 * importance are inserted before older messages.
			 */
			if (importance > line2Importance) {
				/* Move line 2 to line 2 to make room for the new line. */
				strncpy(displayLine3, displayLine2, sizeof(displayLine3));
				line3Importance = line2Importance;
				/* Copy new line to line 2. */
				strncpy(displayLine2, buffer, sizeof(displayLine2));
				line2Importance = importance;

			} else if (importance >= line3Importance) {
				/* Copy new line to line 3. */
				strncpy(displayLine3, buffer, sizeof(displayLine3));
				line3Importance = importance;
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

void
GUI_DrawStatusBarText(int x, int y)
{
	MenuBar_DrawStatusBar(displayLine1, displayLine2, scrollInProgress, x, y, textOffset);
	GUI_DisplayText(NULL, 0);
}

/**
 * Draw a char on the screen.
 *
 * @param c The char to draw.
 * @param x The most left position where to draw the string.
 * @param y The most top position where to draw the string.
 */
void GUI_DrawChar_(unsigned char c, int x, int y)
{
	const int width = (g_curWidgetIndex == WINDOWID_RENDER_TEXTURE) ? g_widgetProperties[WINDOWID_RENDER_TEXTURE].width : SCREEN_WIDTH;
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
	int x;
	uint16 y;
	const char *s;

	if (g_fontCurrent == NULL) return;

	if (top  < 0) top  = 0;
	if (left > TRUE_DISPLAY_WIDTH) return;
	if (top  > TRUE_DISPLAY_HEIGHT) return;

	colours[0] = bgColour;
	colours[1] = fgColour;

	GUI_InitColors(colours, 0, 1);

	s = string;
	x = left;
	y = top;

	Video_HoldBitmapDrawing(true);

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

		Video_DrawChar(*s, g_colours, x, y);

		x += width;
		s++;
	}

	Video_HoldBitmapDrawing(false);
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

			case 0x0060:
				/* Shadow, but no gap. */
				colours[2] = 12;
				colours[3] = 0;
				g_fontCharOffset = -2;
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

		/* When structure damaged more than 50%, blink repair text. */
		colour = (!g_structureHighHealth && animationToggle) ? 6 : 15;
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

		GUI_Palette_2BA5_00A2(g_palette1, WINDTRAP_COLOUR, toggleColour);

		if (!GUI_Palette_2BA5_00A2(g_palette1, WINDTRAP_COLOUR, toggleColour)) {
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
void GUI_DrawSprite_(Screen screenID, uint8 *sprite, int16 posX, int16 posY, uint16 windowID, uint16 flags, ...)
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
	buf += g_widgetProperties[windowID].xBase;

	if ((flags & 0x4000) == 0) posX -= g_widgetProperties[windowID].xBase;

	width = g_widgetProperties[windowID].width;
	top = g_widgetProperties[windowID].yBase;

	if ((flags & 0x4000) != 0) posY += g_widgetProperties[windowID].yBase;

	bottom = g_widgetProperties[windowID].yBase + g_widgetProperties[windowID].height;

	loc10 = READ_LE_UINT16(sprite);
	sprite += 2;

	loc12 = *sprite++;

	if ((flags & 0x4) != 0) {
		loc12 *= loc32;
		loc12 >>= 8;
		if (loc12 == 0) return;
	}

	if ((flags & 0x8000) != 0) posY -= loc12 / 2;

	loc1A = READ_LE_UINT16(sprite);
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

	locbx = READ_LE_UINT16(sprite);
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
		if (s->o.type == STRUCTURE_SLAB_1x1 || s->o.type == STRUCTURE_SLAB_2x2 || s->o.type == STRUCTURE_WALL) continue;

		score += g_table_structureInfo[s->o.type].o.buildCredits / 100;
	}

	g_validateStrictIfZero++;

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

	g_validateStrictIfZero--;

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

	Prim_FillRect_i(SCREEN_WIDTH / 2 - halfWidth, top, SCREEN_WIDTH / 2 + halfWidth, top + 6, 116);
	GUI_DrawText_Wrapper(string, SCREEN_WIDTH / 2, top, 0xF, 0, 0x121);
}

#if 0
static uint16 GUI_HallOfFame_GetRank(uint16 score);
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

#if 0
extern void GUI_DrawBorder(uint16 left, uint16 top, uint16 width, uint16 height, uint16 colourSchemaIndex, bool fill);
#endif

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

	hint = stringID - STR_HINT_YOU_MUST_BUILD_A_WINDTRAP_TO_PROVIDE_POWER_TO_YOUR_BASE_WITHOUT_POWER_YOUR_STRUCTURES_WILL_DECAY;

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
extern void GUI_DrawProgressbar(uint16 current, uint16 max);
#endif

/**
 * Draw the interface (borders etc etc) and radar on the screen.
 * @param screenID The screen to draw the radar on.  Always 0.
 */
void GUI_DrawInterfaceAndRadar(void)
{
	const Screen screenID = SCREEN_0;
	PoolFindStruct find;
	Screen oldScreenID;

	oldScreenID = GFX_Screen_SetActive((screenID == SCREEN_0) ? SCREEN_1 : screenID);

	g_viewport_forceRedraw = true;

	MenuBar_Draw(g_playerHouseID);

	A5_UseTransform(SCREENDIV_SIDEBAR);
	GUI_Widget_ActionPanel_Draw(true);

	GUI_DrawScreen(g_screenActiveID);

#if 0
	find.houseID = HOUSE_INVALID;
	find.index   = 0xFFFF;
	find.type    = 0xFFFF;

	while (true) {
		Structure *s;

		s = Structure_Find(&find);
		if (s == NULL) break;
		if (s->o.type == STRUCTURE_SLAB_1x1 || s->o.type == STRUCTURE_SLAB_2x2 || s->o.type == STRUCTURE_WALL) continue;

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

	GFX_Screen_SetActive(oldScreenID);
}

/**
 * Draw the credits on the screen, and animate it when the value is changing.
 * @param houseID The house to display the credits from.
 * @param mode The mode of displaying. 0 = animate, 1 = force draw, 2 = reset.
 */
void
GUI_DrawCredits(int credits, uint16 mode, int x)
{
	static int64_t l_tickCreditsAnimation = 0;    /*!< Next tick when credits animation needs an update. */
	static uint16 creditsAnimation = 0;           /* How many credits are shown in current animation of credits. */
	static int16  creditsAnimationOffset = 0;     /* Offset of the credits for the animation of credits. */

	int16 creditsDiff;
	int32 creditsNew;
	int32 creditsOld;
	int16 offset;

	if (l_tickCreditsAnimation > Timer_GetTicks() && mode == 0) return;
	l_tickCreditsAnimation = Timer_GetTicks() + 1;

	/* House *h = House_Get_ByIndex(houseID); */

	if (mode == 2) {
		g_playerCredits = credits;
		creditsAnimation = credits;
	}

	if (mode == 0 && credits == creditsAnimation && creditsAnimationOffset == 0) return;

	creditsDiff = credits - creditsAnimation;
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
	Screen oldScreenID = GFX_Screen_SetActive(SCREEN_1);
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
		GUI_Screen_Copy(g_curWidgetXBase/8, g_curWidgetYBase, g_curWidgetXBase/8, g_curWidgetYBase - 40, g_curWidgetWidth/8, g_curWidgetHeight, g_screenActiveID, oldScreenID);
		GUI_Mouse_Show_InWidget();
	}

	GFX_Screen_SetActive(oldScreenID);

	Widget_SetCurrentWidget(oldValue_07AE_0000);
#else
	MenuBar_DrawCredits(creditsNew, creditsOld, offset - creditsAnimationOffset, x);
#endif
}

/**
 * Change the selection type.
 * @param selectionType The new selection type.
 */
void GUI_ChangeSelectionType(uint16 selectionType)
{
	Screen oldScreenID;

	if (selectionType == SELECTIONTYPE_UNIT && !Unit_AnySelected()) {
		selectionType = SELECTIONTYPE_STRUCTURE;
	}

	if (selectionType == SELECTIONTYPE_STRUCTURE && Unit_AnySelected()) {
		Unit_UnselectAll();
	}

	oldScreenID = GFX_Screen_SetActive(SCREEN_1);

	if (g_selectionType != selectionType) {
		uint16 oldSelectionType = g_selectionType;

		Timer_SetTimer(TIMER_GAME, false);

		g_selectionType = selectionType;
		g_selectionTypeNew = selectionType;
		/* g_var_37B8 = true; */

		switch (oldSelectionType) {
			case SELECTIONTYPE_PLACE:
				Map_SetSelection(g_structureActivePosition);
				/* Fall-through */

			/* ENHANCEMENT -- Originally, SELECTIONTYPE_TARGET was
			 * above SELECTIONTYPE_PLACE, so that if the selected unit
			 * moved a tile between activating the target command and
			 * clicking, the unit loses its selection (another unit
			 * might be selected).  This works badly with multiple
			 * selection.
			 */
			case SELECTIONTYPE_TARGET:
			case SELECTIONTYPE_STRUCTURE:
				GUI_DisplayText(NULL, -1);
				break;

			case SELECTIONTYPE_UNIT:
				if (Unit_AnySelected() && selectionType != SELECTIONTYPE_TARGET && selectionType != SELECTIONTYPE_UNIT) {
					int iter;
					for (Unit *u = Unit_FirstSelected(&iter); u != NULL; u = Unit_NextSelected(&iter)) {
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
			g_viewport_fadein = true;
		}

		Widget_SetCurrentWidget(g_table_selectionType[selectionType].defaultWidget);

		if (g_curWidgetIndex != 0) {
			GUI_Widget_DrawBorder(g_curWidgetIndex, 0, false);
		}

		if (selectionType != SELECTIONTYPE_MENTAT) {
			Widget *w = g_widgetLinkedListHead;

			while (w != NULL) {
				const int8 *s = g_table_selectionType[selectionType].visibleWidgets;

				w->state.selected = false;
				w->flags.invisible = true;

				for (; *s != -1; s++) {
					if (*s == w->index) {
						w->flags.invisible = false;
						break;
					}
				}

				GUI_Widget_Draw(w);
				w = GUI_Widget_GetNext(w);
			}

			GUI_Widget_DrawAll(g_widgetLinkedListHead);
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
				Timer_SetTimer(TIMER_GAME, true);
				break;

			case SELECTIONTYPE_PLACE:
				Unit_UnselectAll();

				Map_SetSelectionSize(g_table_structureInfo[g_structureActiveType].layout);

				Timer_SetTimer(TIMER_GAME, true);
				break;

			case SELECTIONTYPE_UNIT:
				Timer_SetTimer(TIMER_GAME, true);
				break;

			case SELECTIONTYPE_STRUCTURE:
				g_factoryWindowTotal = -1;
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
void GUI_Screen_Copy(int16 xSrc, int16 ySrc, int16 xDst, int16 yDst, int16 width, int16 height, Screen screenSrc, Screen screenDst)
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

#if 0
static uint32 GUI_FactoryWindow_CreateWidgets(void);
static uint32 GUI_FactoryWindow_LoadGraymapTbl(void);

/* Moved to structure.c. */
static uint16 GUI_FactoryWindow_CalculateStarportPrice(uint16 credits);
static int GUI_FactoryWindow_Sorter(const void *a, const void *b);
extern void GUI_FactoryWindow_InitItems(enum StructureType s);

static void GUI_FactoryWindow_Init(Structure *s);
extern FactoryResult GUI_DisplayFactoryWindow(Structure *s, uint16 upgradeCost);
#endif

const char *
GUI_String_Get_ByIndex(int16 stringID)
{
	const uint16 speedStrings[5] = {
		STR_SLOWEST, STR_SLOW, STR_NORMAL, STR_FAST, STR_FASTEST
	};

	int onoff = 0xFFFF;

	switch (stringID) {
		case -5: case -4: case -3: case -2: case -1: {
			const char *s = g_savegameDesc[abs((int16)stringID + 1)];
			if (*s == '\0') return NULL;
			return s;
		}

		case -10: onoff = g_enable_music; break;

		case -11:
			if (SOUNDEFFECTS_SYNTH_ONLY <= g_enable_sound_effects && g_enable_sound_effects <= SOUNDEFFECTS_SYNTH_AND_SAMPLES) {
				const char *str[] = {
					NULL, "Synth", "Digital", "Both"
				};

				return str[g_enable_sound_effects];
			}
			else {
				stringID = STR_OFF;
				break;
			}

		case -12: stringID = speedStrings[g_gameConfig.gameSpeed]; break;
		case -13: onoff = g_gameConfig.hints; break;
		case -14: onoff = g_enable_subtitles; break;

		case -50: return (g_gameConfig.leftClickOrders) ? "Left-click" : "Right-click";
		case -51: return (g_gameConfig.holdControlToZoom) ? "Scroll sidebar" : "Zoom viewport";
		case -52: return (g_gameConfig.scrollAlongScreenEdge) ? "Screen" : "Viewport";
		case -53: onoff = g_gameConfig.autoScroll; break;

		case -70:
			if (enhancement_draw_health_bars == HEALTH_BAR_ALL_UNITS)
				return "ALL";

			onoff = enhancement_draw_health_bars;
			break;

		case -71: onoff = enhancement_high_res_overlays; break;
		case -72: onoff = enhancement_smooth_unit_animation; break;
		case -73: onoff = enhancement_infantry_squad_death_animations; break;
		case -74: onoff = g_gameConfig.hardwareCursor; break;

		default: break;
	}

	if (onoff != 0xFFFF)
		stringID = (onoff != 0) ? STR_ON : STR_OFF;

	return String_Get_ByIndex(stringID);
}

#if 0
static void GUI_StrategicMap_AnimateArrows(void);
static void GUI_StrategicMap_AnimateSelected(uint16 selected, StrategicMapData *data);
static bool GUI_StrategicMap_IsRegionDone(uint16 region);
static void GUI_StrategicMap_SetRegionDone(uint16 region, bool set);
static int16 GUI_StrategicMap_ClickedRegion(void);
static bool GUI_StrategicMap_FastForwardToggleWithESC(void);
static void GUI_StrategicMap_DrawText(const char *string);
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
void GUI_ClearScreen(Screen screenID)
{
	Screen oldScreenID = GFX_Screen_SetActive(screenID);

	GFX_ClearScreen();

	GFX_Screen_SetActive(oldScreenID);
}

#if 0
extern void GUI_DrawText_Monospace(const char *string, uint16 left, uint16 top, uint8 fgColour, uint8 bgColour, uint16 charWidth);
extern void GUI_FactoryWindow_B495_0F30(void);
extern FactoryWindowItem *GUI_FactoryWindow_GetItem(int16 offset);
extern void GUI_FactoryWindow_DrawDetails(void);
extern void GUI_FactoryWindow_DrawCaption(const char *caption);
extern void GUI_FactoryWindow_UpdateDetails(void);
extern void GUI_FactoryWindow_UpdateSelection(bool selectionChanged);
#endif

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
void GUI_Screen_FadeIn(uint16 xSrc, uint16 ySrc, uint16 xDst, uint16 yDst, uint16 width, uint16 height, Screen screenSrc, Screen screenDst)
{
	uint16 offsetsY[100];
	uint16 offsetsX[40];
	int x, y;

	if (screenDst == SCREEN_0) {
		GUI_Mouse_Hide_InRegion(xDst << 3, yDst, (xDst + width) << 3, yDst + height);
	}

	height /= 2;

	for (x = 0; x < width;  x++) offsetsX[x] = x;
	for (y = 0; y < height; y++) offsetsY[y] = y;

	for (x = 0; x < width; x++) {
		uint16 index;
		uint16 temp;

		index = Tools_RandomLCG_Range(0, width - 1);

		temp = offsetsX[index];
		offsetsX[index] = offsetsX[x];
		offsetsX[x] = temp;
	}

	for (y = 0; y < height; y++) {
		uint16 index;
		uint16 temp;

		index = Tools_RandomLCG_Range(0, height - 1);

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

		/* XXX -- This delays the system so you can in fact see the animation.
		 * XXX -- This effect isn't reproduced in Dune Dynasty.
		 */
		/* if ((y % 4) == 0) Timer_Sleep(1); */
	}

	if (screenDst == SCREEN_0) {
		GUI_Mouse_Show_InRegion();
	}
}

#if 0
extern void GUI_FactoryWindow_PrepareScrollList(void);
extern void GUI_Screen_FadeIn2(int16 x, int16 y, int16 width, int16 height, Screen screenSrc, Screen screenDst, uint16 delay, bool skipNull);
extern void GUI_Mouse_Show(void);
extern void GUI_Mouse_Hide(void);
extern void GUI_Mouse_Hide_Safe(void);
extern void GUI_Mouse_Show_Safe(void);
extern void GUI_Mouse_Show_InRegion(void);
extern void GUI_Mouse_Hide_InRegion(uint16 left, uint16 top, uint16 right, uint16 bottom);
extern void GUI_Mouse_Show_InWidget(void);
extern void GUI_Mouse_Hide_InWidget(uint16 widgetIndex);
extern void GUI_DrawBlockedRectangle(int16 left, int16 top, int16 width, int16 height, uint8 colour);
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
void GUI_Palette_RemapScreen(uint16 left, uint16 top, uint16 width, uint16 height, Screen screenID, uint8 *remap)
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
extern uint16 GUI_HallOfFame_Tick(void);
#endif

static Widget *GUI_HallOfFame_CreateButtons(HallOfFameStruct *data)
{
	const char *resumeString;
	const char *clearString;
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
	memset(&wClear->flags, 0, sizeof(wClear->flags));
	wClear->flags.requiresClick = true;
	wClear->flags.clickAsHover = true;
	wClear->flags.loseSelect = true;
	wClear->flags.notused2 = true;
	wClear->flags.buttonFilterLeft = 4;
	wClear->flags.buttonFilterRight = 4;
	wClear->data      = data;

	/* "Resume Game" */
	wResume = GUI_Widget_Allocate(101, *resumeString, 178, 180, 0xFFFE, 0x146);
	wResume->width     = width;
	wResume->height    = 10;
	memset(&wResume->flags, 0, sizeof(wResume->flags));
	wResume->flags.requiresClick = true;
	wResume->flags.clickAsHover = true;
	wResume->flags.loseSelect = true;
	wResume->flags.notused2 = true;
	wResume->flags.buttonFilterLeft = 4;
	wResume->flags.buttonFilterRight = 4;
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
		data->rank = HallOfFame_GetRank(score);
		data->campaignID = g_campaignID;

		return i + 1;
	}

	return 0;
}

void
GUI_HallOfFame_Show(enum HouseType houseID, uint16 score)
{
	HallOfFameData *fame = &g_hall_of_fame_state;
	const uint16 old_widget = g_curWidgetIndex;

	uint16 width;
	uint16 editLine;
	Widget *w;
	uint8 fileID;
	HallOfFameStruct *data;

	if (score == 0xFFFF) {
#if 0
		if (!File_Exists_Personal("SAVEFAME.DAT")) {
			return;
		}
#endif
		s_ticksPlayed = 0;
	}

	data = (HallOfFameStruct *)GFX_Screen_Get_ByIndex(SCREEN_2);

	if (!File_Exists_Personal("SAVEFAME.DAT")) {
		memset(data, 0, 128);

#if 0
		uint16 written;
		GUI_HallOfFame_Encode(data);

		fileID = File_Open_Personal("SAVEFAME.DAT", FILE_MODE_WRITE);
		written = File_Write(fileID, data, 128);
		File_Close(fileID);

		if (written != 128) return;
#endif
	}
	else {
		File_ReadBlockFile_Personal("SAVEFAME.DAT", data, 128);
		GUI_HallOfFame_Decode(data);
	}

	if (score == 0xFFFF) {
		editLine = 0;
	} else {
		editLine = GUI_HallOfFame_InsertScore(data, score);
	}

	width = GUI_HallOfFame_DrawData(data, false);

	if (editLine != 0) {
		WidgetProperties backupProperties;
		char name[9];

		/* name = data[editLine - 1].name; */
		memset(name, 0, sizeof(name));
		data[editLine - 1].name[0] = '\0';

		memcpy(&backupProperties, &g_widgetProperties[19], sizeof(WidgetProperties));

		g_widgetProperties[19].xBase = 4*8;
		g_widgetProperties[19].yBase = (editLine - 1) * 11 + 90;
		g_widgetProperties[19].width = width;
		g_widgetProperties[19].height = 11;
		g_widgetProperties[19].fgColourBlink = 6;
		g_widgetProperties[19].fgColourNormal = 116;

		GUI_DrawText_Wrapper(NULL, 0, 0, 0, 0, 0x22);

		while (true) {
			char *nameEnd;
			Screen oldScreenID;

			oldScreenID = GFX_Screen_SetActive(SCREEN_0);
			Widget_SetAndPaintCurrentWidget(19);
			GFX_Screen_SetActive(oldScreenID);

			HallOfFame_DrawBackground(houseID, true);
			HallOfFame_DrawScoreTime(fame->score, fame->time);
			HallOfFame_DrawRank(fame);

			GUI_HallOfFame_DrawData(data, false);

			Input_Tick(true);
			int ret = GUI_EditBox(name, 8, 19, NULL, NULL, 0);
			GUI_EditBox_Draw(name);
			if (ret == SCANCODE_ENTER) {
				if (*name == '\0')
					continue;

				nameEnd = name + strlen(name) - 1;

				while (*nameEnd <= ' ' && nameEnd >= name)
					*nameEnd-- = '\0';

				/* name */
				memcpy(data[editLine - 1].name, name, 5);
				memcpy(data[editLine - 1].name_extended, name + 5, 3);
				data[editLine - 1].name[5] = '\0';

				break;
			}

			Video_Tick();
			sleepIdle();
		}

		memcpy(&g_widgetProperties[19], &backupProperties, sizeof(WidgetProperties));

		GUI_HallOfFame_Encode(data);
		fileID = File_Open_Personal("SAVEFAME.DAT", FILE_MODE_WRITE);
		File_Write(fileID, data, 128);
		File_Close(fileID);
		GUI_HallOfFame_Decode(data);
	}

	w = GUI_HallOfFame_CreateButtons(data);

	/* Disable the clear button if no scores. */
	if (data[0].score == 0) {
		GUI_Widget_MakeInvisible(GUI_Widget_Get_ByIndex(w, 100));
	}

	Input_History_Clear();

	GFX_Screen_SetActive(SCREEN_0);

	g_yesNoWindowDesc.stringID = STR_ARE_YOU_SURE_YOU_WANT_TO_CLEAR_THE_HIGH_SCORES;
	GUI_Window_Create(&g_yesNoWindowDesc);
	GUI_Widget_Get_ByIndex(g_widgetLinkedListTail, 30)->data = data;

	bool confirm_clear = false;
	bool redraw = true;
	while (true) {
		if (Input_Tick(true))
			redraw = true;

		if (confirm_clear) {
			const int ret = GUI_Widget_HOF_ClearList_Click(g_widgetLinkedListTail);

			if (ret == -1) {
				confirm_clear = false;
				redraw = true;
			}
			else if (ret == 1) {
				break;
			}

			Widget *widget = g_widgetLinkedListTail;
			while (widget != NULL) {
				if (widget->state.selected != widget->state.selectedLast) {
					redraw = true;
					break;
				}

				widget = GUI_Widget_GetNext(widget);
			}
		}
		else {
			const uint16 key = GUI_Widget_HandleEvents(w);

			if (key == (0x8000 | 100)) { /* Clear list */
				confirm_clear = true;
				redraw = true;
			}
			else if (key == (0x8000 | 101)) { /* Resume */
				break;
			}

			Widget *widget = w;
			while (widget != NULL) {
				if (widget->state.hover1 != widget->state.hover1Last) {
					redraw = true;
					break;
				}

				widget = GUI_Widget_GetNext(widget);
			}
		}

		if (!redraw) {
			Timer_Sleep(1);
			continue;
		}

		redraw = false;
		HallOfFame_DrawBackground(houseID, true);
		if (score == 0xFFFF) {
			GUI_DrawText_Wrapper(String_Get_ByIndex(STR_HALL_OF_FAME2), SCREEN_WIDTH / 2, 15, 15, 0, 0x122);

			if (g_campaign_selected == CAMPAIGNID_DUNE_II) {
				GUI_DrawText_Wrapper("Dune II", SCREEN_WIDTH / 2, 43, 15, 0, 0x122);
			}
			else {
				GUI_DrawText_Wrapper(g_campaign_list[g_campaign_selected].name, SCREEN_WIDTH / 2, 43, 15, 0, 0x122);
			}
		}
		else {
			HallOfFame_DrawScoreTime(fame->score, fame->time);
			HallOfFame_DrawRank(fame);
		}
		GUI_HallOfFame_DrawData(data, true);

		if (confirm_clear) {
			GUI_Widget_DrawWindow(&g_yesNoWindowDesc);
			GUI_Widget_DrawAll(g_widgetLinkedListTail);
		}

		GUI_Widget_DrawAll(w);

		Video_Tick();
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

	Screen oldScreenID;
	const char *scoreString;
	const char *battleString;
	uint16 width = 0;
	uint16 offsetY;
	uint16 scoreX;
	uint16 battleX;
	uint8 i;

	oldScreenID = GFX_Screen_SetActive(SCREEN_1);
	Prim_FillRect_i(8, 80, 311, 178, 116);
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
		if (data[i].rank >= MAX_RANKS) break;
		if (data[i].houseID >= HOUSE_MAX) break;

		if (g_table_languageInfo[g_gameConfig.language].noun_before_adj) {
			p1 = HallOfFame_GetRankString(data[i].rank);
			p2 = g_table_houseInfo[data[i].houseID].name;
		} else {
			p1 = g_table_houseInfo[data[i].houseID].name;
			p2 = HallOfFame_GetRankString(data[i].rank);
		}

		snprintf(buffer, sizeof(buffer), "%.5s%.3s, %s %s", data[i].name, data[i].name_extended, p1, p2);

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

#if 0
extern void GUI_DrawXorFilledRectangle(int16 left, int16 top, int16 right, int16 bottom, uint8 colour);
#endif

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
void GUI_DrawScreen(Screen screenID)
{
	static uint32 s_timerViewportMessage = 0;
	bool loc10;
	Screen oldScreenID;
	uint16 xpos;

	if (g_selectionType == SELECTIONTYPE_MENTAT) return;
	if (g_selectionType == SELECTIONTYPE_DEBUG) return;
	if (g_selectionType == SELECTIONTYPE_UNKNOWN6) return;
	if (g_selectionType == SELECTIONTYPE_INTRO) return;

	loc10 = false;

	oldScreenID = GFX_Screen_SetActive(screenID);

	if (screenID != SCREEN_0) g_viewport_forceRedraw = true;

	Explosion_Tick();
	Animation_Tick();
	Unit_Sort();

	Map_UpdateFogOfWar();

#if 0
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

			GUI_Screen_Copy(max(-xOffset << 1, 0), 40 + max(-yOffset << 4, 0), max(0, xOffset << 1), 40 + max(0, yOffset << 4), xOverlap << 1, yOverlap << 4, SCREEN_0, SCREEN_1);
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
#endif

	if (loc10) {
		Map_SetSelectionObjectPosition(0xFFFF);
	}

	g_selectionRectanglePosition = g_selectionPosition;

	if (g_viewportMessageCounter != 0 && s_timerViewportMessage < Timer_GetTicks()) {
		g_viewportMessageCounter--;
		s_timerViewportMessage = Timer_GetTicks() + 60;

		for (xpos = 0; xpos < 14; xpos++) {
			Map_Update(g_viewportPosition + xpos + 6 * 64, 0, true);
		}
	}

	A5_UseTransform(SCREENDIV_VIEWPORT);
	GUI_Widget_Viewport_Draw(g_viewport_forceRedraw, loc10, screenID != SCREEN_0);
	A5_UseTransform(SCREENDIV_MAIN);

	g_viewport_forceRedraw = false;

	GFX_Screen_SetActive(oldScreenID);

	Map_SetSelectionObjectPosition(g_selectionRectanglePosition);

	GUI_Mouse_Show_InWidget();
}

#if 0
extern void GUI_SetPaletteAnimated(uint8 *palette, int16 ticksOfAnimation);
#endif

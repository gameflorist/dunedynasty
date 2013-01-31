/** @file src/gfx.c Graphics routines. */

#include <assert.h>
#include <stdlib.h>
#include <string.h>

#include "gfx.h"

#include "enhancement.h"
#include "gui/widget.h"
#include "house.h"
#include "opendune.h"
#include "sprites.h"
#include "video/video.h"

int TRUE_DISPLAY_WIDTH = 640;
int TRUE_DISPLAY_HEIGHT = 480;
enum AspectRatioCorrection g_aspect_correction = ASPECT_RATIO_CORRECTION_AUTO;
float g_pixel_aspect_ratio = 1.1f;

static uint16 s_spriteSpacing  = 0;
static uint16 s_spriteHeight   = 0;
static uint16 s_spriteWidth    = 0;
static uint8  s_spriteMode     = 0;
static uint8  s_spriteInfoSize = 0;

static const uint16 s_screenBufferSize[5] = { 0xFA00, 0xFBF4, 0xFA00, 0xFD0D, 0xA044 };
static void *s_screenBuffer[5] = { NULL, NULL, NULL, NULL, NULL };

Screen g_screenActiveID = SCREEN_0;

ScreenDiv g_screenDiv[SCREENDIV_MAX] = {
	{ 1.0f, 1.0f,   0,   0, 320, 200 }, /* SCREENDIV_MAIN */
	{ 1.0f, 1.0f,   0,   0, 320, 200 }, /* SCREENDIV_MENU */
	{ 1.0f, 1.0f,   0,   0, 320,  40 }, /* SCREENDIV_MENUBAR */
	{ 1.0f, 1.0f, 240,  40,  80, 160 }, /* SCREENDIV_SIDEBAR */
	{ 1.0f, 1.0f,   0,  40, 240, 160 }, /* SCREENDIV_VIEWPORT */
};

void
GFX_InitDefaultViewportScales(bool adjust_viewport)
{
	ScreenDiv *menubar = &g_screenDiv[SCREENDIV_MENUBAR];
	ScreenDiv *sidebar = &g_screenDiv[SCREENDIV_SIDEBAR];
	ScreenDiv *viewport = &g_screenDiv[SCREENDIV_VIEWPORT];

	/* Default viewport scales. */
	if (TRUE_DISPLAY_WIDTH <= 320) {
		menubar->scalex = 1.0f;
		sidebar->scalex = 1.0f;

		if (adjust_viewport)
			viewport->scalex = 1.0f;
	}
	else if (TRUE_DISPLAY_WIDTH <= 640) {
		menubar->scalex = 1.0f;
		sidebar->scalex = 1.0f;

		if (adjust_viewport)
			viewport->scalex = 2.0f;
	}
	else {
		menubar->scalex = 2.0f;
		sidebar->scalex = 2.0f;

		if (adjust_viewport)
			viewport->scalex = 2.0f;
	}
}

float
GFX_AspectCorrection_GetRatio(void)
{
	/* 'Auto' corrects the menus so that the planets appear rounder.
	 * The in-game graphics are not corrected; they appear to be drawn
	 * assuming square pixels (e.g. carryall, starport landing pad).
	 *
	 * For small screens, avoid scaling but use the 20% larger buttons
	 * where possible.
	 */
	switch (g_aspect_correction) {
		default:
		case ASPECT_RATIO_CORRECTION_NONE:
			return 1.0f;

		case ASPECT_RATIO_CORRECTION_PARTIAL:
		case ASPECT_RATIO_CORRECTION_FULL:
			return g_pixel_aspect_ratio;

		case ASPECT_RATIO_CORRECTION_AUTO:
			return (TRUE_DISPLAY_HEIGHT < 200 * 4) ? 1.0f : g_pixel_aspect_ratio;
	}
}

/**
 * Get the codesegment of the active screen buffer.
 * @return The codesegment of the screen buffer.
 */
void *GFX_Screen_GetActive(void)
{
	return GFX_Screen_Get_ByIndex(g_screenActiveID);
}

/**
 * Returns the size of a screenbuffer.
 * @param screenID The screenID to get the size of.
 * @return Some size value.
 */
uint16 GFX_Screen_GetSize_ByIndex(Screen screenID)
{
	return s_screenBufferSize[screenID >> 1];
}

/**
 * Get the pointer to a screenbuffer.
 * @param screenID The screenbuffer to get.
 * @return A pointer to the screenbuffer.
 */
void *GFX_Screen_Get_ByIndex(Screen screenID)
{
	return s_screenBuffer[screenID >> 1];
}

/**
 * Change the current active screen to the new value.
 * @param screenID The new screen to get active.
 * @return Old screenID that was currently active.
 */
Screen GFX_Screen_SetActive(Screen screenID)
{
	Screen oldScreen = g_screenActiveID;
	g_screenActiveID = screenID;
	return oldScreen;
}

/**
 * Initialize the GFX system.
 */
void GFX_Init(void)
{
	uint8 *screenBuffers;
	uint32 totalSize = 0;
	int i;

	for (i = 0; i < 5; i++) {
		totalSize += GFX_Screen_GetSize_ByIndex(i * 2);
	}

	if (totalSize < (uint32)g_widgetProperties[WINDOWID_RENDER_TEXTURE].width * g_widgetProperties[WINDOWID_RENDER_TEXTURE].height)
		totalSize = g_widgetProperties[WINDOWID_RENDER_TEXTURE].width * g_widgetProperties[WINDOWID_RENDER_TEXTURE].height;

	screenBuffers = calloc(1, totalSize);

	for (i = 0; i < 5; i++) {
		s_screenBuffer[i] = screenBuffers;

		screenBuffers += GFX_Screen_GetSize_ByIndex(i * 2);
	}

	g_screenActiveID = SCREEN_0;
}

/**
 * Uninitialize the GFX system.
 */
void GFX_Uninit(void)
{
	int i;

	free(s_screenBuffer[0]);

	for (i = 0; i < 5; i++) {
		s_screenBuffer[i] = NULL;
	}
}

/**
 * Draw a sprite on the screen.
 * @param spriteID The sprite to draw.
 * @param x The x-coordinate to draw the sprite.
 * @param y The y-coordinate to draw the sprite.
 * @param houseID The house the sprite belongs (for recolouring).
 */
void GFX_DrawSprite_(uint16 spriteID, uint16 x, uint16 y, uint8 houseID)
{
	int i, j;
	uint16 spacing;
	uint16 height;
	uint16 width;
	uint8 *iconRTBL;
	uint8 *iconRPAL;
	uint8 *wptr;
	uint8 *rptr;
	uint8 palette[16];

	assert(houseID < HOUSE_MAX);

	iconRTBL = g_iconRTBL + spriteID;
	iconRPAL = g_iconRPAL + ((*iconRTBL) << 4);

	for (i = 0; i < 16; i++) {
		uint8 colour = *iconRPAL++;

		if (enhancement_fix_ix_colour_remapping) {
			if (colour >= 0x90 && colour <= 0x96) colour += houseID << 4;
		} else {
			if (colour >= 0x90 && colour <= 0xA0) colour += houseID << 4;
		}
		palette[i] = colour;
	}

	if (s_spriteMode == 4) return;

	wptr = GFX_Screen_GetActive();

	if (g_curWidgetIndex == WINDOWID_RENDER_TEXTURE) {
		wptr += g_widgetProperties[WINDOWID_RENDER_TEXTURE].width * y + x;
		s_spriteSpacing = g_widgetProperties[WINDOWID_RENDER_TEXTURE].width - 2*s_spriteWidth;
	}
	else {
		wptr += SCREEN_WIDTH * y + x;
	}

	rptr = g_spriteInfo + ((spriteID * s_spriteInfoSize) << 4);

	spacing = s_spriteSpacing;
	height  = s_spriteHeight;
	width   = s_spriteWidth;

	for (j = 0; j < height; j++) {
		for (i = 0; i < width; i++) {
			uint8 left  = (*rptr) >> 4;
			uint8 right = (*rptr) & 0xF;
			rptr++;

			if (palette[left] != 0) *wptr = palette[left];
			wptr++;
			if (palette[right] != 0) *wptr = palette[right];
			wptr++;
		}

		wptr += spacing;
	}
}

/**
 * Initialize sprite information.
 *
 * @param widthSize Value between 0 and 2, indicating the width of the sprite.
 * @param heightSize Value between 0 and 2, indicating the width of the sprite.
 */
void GFX_Init_SpriteInfo(uint16 widthSize, uint16 heightSize)
{
	if (widthSize == heightSize && widthSize < 3) {
		s_spriteMode = widthSize & 2;
		s_spriteInfoSize = (2 << widthSize);

		s_spriteWidth   = widthSize << 2;
		s_spriteHeight  = widthSize << 3;
		s_spriteSpacing = SCREEN_WIDTH - s_spriteHeight;
	} else {
		s_spriteMode = 4;
		s_spriteInfoSize = 2;

		s_spriteWidth   = 4;
		s_spriteHeight  = 8;
		s_spriteSpacing = 312;

		widthSize = 1;
		heightSize = 1;
	}
}

#if 0
extern void GFX_PutPixel(uint16 x, uint16 y, uint8 colour);
#endif

/**
 * Copy information from one screenbuffer to the other.
 * @param xSrc The X-coordinate on the source.
 * @param ySrc The Y-coordinate on the source.
 * @param xDst The X-coordinate on the destination.
 * @param yDst The Y-coordinate on the destination.
 * @param width The width.
 * @param height The height.
 * @param screenSrc The ID of the source screen.
 * @param screenDst The ID of the destination screen.
 * @param skipNull Wether to skip pixel colour 0.
 */
void GFX_Screen_Copy2(int16 xSrc, int16 ySrc, int16 xDst, int16 yDst, int16 width, int16 height, Screen screenSrc, Screen screenDst, bool skipNull)
{
	uint8 *src;
	uint8 *dst;

	if (xSrc >= SCREEN_WIDTH) return;
	if (xSrc < 0) {
		xDst += xSrc;
		width += xSrc;
		xSrc = 0;
	}

	if (ySrc >= SCREEN_HEIGHT) return;
	if (ySrc < 0) {
		yDst += ySrc;
		height += ySrc;
		ySrc = 0;
	}

	if (xDst >= SCREEN_WIDTH) return;
	if (xDst < 0) {
		xSrc += xDst;
		width += xDst;
		xDst = 0;
	}

	if (yDst >= SCREEN_HEIGHT) return;
	if (yDst < 0) {
		ySrc += yDst;
		height += yDst;
		yDst = 0;
	}

	if (SCREEN_WIDTH - xSrc - width < 0) width = SCREEN_WIDTH - xSrc;
	if (SCREEN_HEIGHT - ySrc - height < 0) height = SCREEN_HEIGHT - ySrc;
	if (SCREEN_WIDTH - xDst - width < 0) width = SCREEN_WIDTH - xDst;
	if (SCREEN_HEIGHT - yDst - height < 0) height = SCREEN_HEIGHT - yDst;

	if (xSrc < 0 || xSrc >= SCREEN_WIDTH) return;
	if (xDst < 0 || xDst >= SCREEN_WIDTH) return;
	if (ySrc < 0 || ySrc >= SCREEN_HEIGHT) return;
	if (yDst < 0 || yDst >= SCREEN_HEIGHT) return;
	if (width < 0 || width >= SCREEN_WIDTH) return;
	if (height < 0 || height >= SCREEN_HEIGHT) return;

	src = GFX_Screen_Get_ByIndex(screenSrc);
	dst = GFX_Screen_Get_ByIndex(screenDst);

	src += xSrc + ySrc * SCREEN_WIDTH;
	dst += xDst + yDst * SCREEN_WIDTH;

	while (height-- != 0) {
		if (skipNull) {
			uint16 i;
			for (i = 0; i < width; i++) {
				if (src[i] != 0) dst[i] = src[i];
			}
		} else {
			memmove(dst, src, width);
		}
		dst += SCREEN_WIDTH;
		src += SCREEN_WIDTH;
	}
}

/**
 * Copy information from one screenbuffer to the other.
 * @param xSrc The X-coordinate on the source.
 * @param ySrc The Y-coordinate on the source.
 * @param xDst The X-coordinate on the destination.
 * @param yDst The Y-coordinate on the destination.
 * @param width The width.
 * @param height The height.
 * @param screenSrc The ID of the source screen.
 * @param screenDst The ID of the destination screen.
 */
void GFX_Screen_Copy(int16 xSrc, int16 ySrc, int16 xDst, int16 yDst, int16 width, int16 height, Screen screenSrc, Screen screenDst)
{
	uint8 *src;
	uint8 *dst;

	if (xSrc >= SCREEN_WIDTH) return;
	if (xSrc < 0) xSrc = 0;

	if (ySrc >= SCREEN_HEIGHT) return;
	if (ySrc < 0) ySrc = 0;

	if (xDst >= SCREEN_WIDTH) return;
	if (xDst < 0) xDst = 0;

	if ((yDst + height) > SCREEN_HEIGHT) {
		height = SCREEN_HEIGHT - 1 - yDst;
	}
	if (height < 0) return;

	if (yDst >= SCREEN_HEIGHT) return;
	if (yDst < 0) yDst = 0;

	src = GFX_Screen_Get_ByIndex(screenSrc);
	dst = GFX_Screen_Get_ByIndex(screenDst);

	src += xSrc + ySrc * SCREEN_WIDTH;
	dst += xDst + yDst * SCREEN_WIDTH;

	if (width < 1 || width > SCREEN_WIDTH) return;

	while (height-- != 0) {
		memmove(dst, src, width);
		dst += SCREEN_WIDTH;
		src += SCREEN_WIDTH;
	}
}

/**
 * Clears the screen.
 */
void GFX_ClearScreen(void)
{
	memset(GFX_Screen_GetActive(), 0, SCREEN_WIDTH * SCREEN_HEIGHT);
}

/**
 * Clears the given memory block.
 * @param index The memory block.
 */
void GFX_ClearBlock(Screen index)
{
	memset(GFX_Screen_Get_ByIndex(index), 0, GFX_Screen_GetSize_ByIndex(index));
}

/**
 * Set a new palette for the screen.
 * @param palette The palette in RGB order.
 */
void GFX_SetPalette(uint8 *palette)
{
	Video_SetPalette(palette, 0, 256);

	memcpy(g_paletteActive, palette, 256 * 3);
}

#if 0
extern uint8 GFX_GetPixel(uint16 x, uint16 y);
extern uint16 GFX_GetSize(int16 width, int16 height);
extern void GFX_CopyFromBuffer(int16 left, int16 top, uint16 width, uint16 height, uint8 *buffer);
extern void GFX_CopyToBuffer(int16 left, int16 top, uint16 width, uint16 height, uint8 *buffer);
#endif

/*--------------------------------------------------------------*/

/* Simulate the screen shakes present in the game.  That is, shift the
 * screen 4 pixels up for (2 * num_ticks) frames.
 */
static int s_screen_shake_ticks = 0;

void
GFX_ScreenShake_Start(int num_ticks)
{
	s_screen_shake_ticks = 2 * num_ticks;
}

bool
GFX_ScreenShake_Tick(void)
{
	if (s_screen_shake_ticks == 0)
		return false;

	/* Don't actually need to return true so often, by
	 * s_screen_shake_ticks has a maximum of 2 anyway.
	 */
	s_screen_shake_ticks--;
	return true;
}

int
GFX_ScreenShake_Offset(void)
{
	if (s_screen_shake_ticks == 0) {
		return 0;
	}
	else {
		return -4 * TRUE_DISPLAY_HEIGHT / SCREEN_HEIGHT;
	}
}

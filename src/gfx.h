/* $Id$ */

/** @file src/gfx.h Graphics definitions. */

#ifndef GFX_H
#define GFX_H

#include "types.h"

enum {
	SCREEN_WIDTH  = 320, /*!< Width of Dune 2 screen in pixels. */
	SCREEN_HEIGHT = 200  /*!< Height of Dune 2 screen in pixels. */
};

extern int TRUE_DISPLAY_WIDTH;
extern int TRUE_DISPLAY_HEIGHT;

extern uint16 g_screenActiveID;

extern void GFX_Init(void);
extern void GFX_Uninit(void);
extern uint16 GFX_Screen_SetActive(uint16 screenID);
extern void *GFX_Screen_GetActive(void);
extern uint16 GFX_Screen_GetSize_ByIndex(uint16 screenID);
extern void *GFX_Screen_Get_ByIndex(uint16 screenID);

extern void GFX_DrawSprite(uint16 spriteID, uint16 x, uint16 y, uint8 houseID);
extern void GFX_DrawSprite_(uint16 spriteID, uint16 x, uint16 y, uint8 houseID);
extern void GFX_Init_SpriteInfo(uint16 widthSize, uint16 heightSize);
extern void GFX_Screen_Copy2(int16 xSrc, int16 ySrc, int16 xDst, int16 yDst, int16 width, int16 height, uint16 memBlockSrc, uint16 memBlockDst, bool skipNull);
extern void GFX_Screen_Copy(int16 xSrc, int16 ySrc, int16 xDst, int16 yDst, int16 width, int16 height, uint16 memBlockSrc, uint16 memBlockDst);
extern void GFX_ClearScreen(void);
extern void GFX_SetPalette(uint8 *palette);
extern void GFX_CopyFromBuffer(int16 left, int16 top, uint16 width, uint16 height, uint8 *buffer);
extern void GFX_CopyToBuffer(int16 left, int16 top, uint16 width, uint16 height, uint8 *buffer);

#endif /* GFX_H */

#ifndef VIDEO_VIDEO_H
#define VIDEO_VIDEO_H

#include "../shape.h"

enum {
	RADIO_BUTTON_BACKGROUND_COLOUR = 20,
	CONQUEST_COLOUR = 146,
	STRATEGIC_MAP_ARROW_EDGE_COLOUR = 148,
	WINDTRAP_COLOUR = 223,
	STRATEGIC_MAP_ARROW_COLOUR = 251,
};

enum CPSID {
	CPS_MENUBAR_LEFT,
	CPS_MENUBAR_MIDDLE,
	CPS_MENUBAR_RIGHT,
	CPS_STATUSBAR_LEFT,
	CPS_STATUSBAR_MIDDLE,
	CPS_STATUSBAR_RIGHT,
	CPS_SIDEBAR_TOP,
	CPS_SIDEBAR_MIDDLE,
	CPS_SIDEBAR_BOTTOM,
	CPS_CONQUEST_EN,
	CPS_CONQUEST_FR,
	CPS_CONQUEST_DE,

	CPS_SPECIAL_MAX
};

struct FadeInAux;

#include "prim.h"

extern void Video_InitMentatSprites(bool use_benepal);

extern void Video_SetPalette(const uint8 *palette, int from, int length);
extern void Video_SetClippingArea(int x, int y, int w, int h);
extern void Video_SetCursor(int cursor);
extern void Video_ShowCursor(void);
extern void Video_HideCursor(void);
extern void Video_HideHWCursor(void);
extern void Video_WarpCursor(int x, int y);
extern void Video_ShadeScreen(int alpha);
extern void Video_HoldBitmapDrawing(bool hold);

extern void Video_DrawFadeIn(const struct FadeInAux *aux);
extern bool Video_TickFadeIn(struct FadeInAux *aux);
extern struct FadeInAux *Video_InitFadeInCPS(const char *filename, int x, int y, int w, int h, bool fade_in);
extern struct FadeInAux *Video_InitFadeInShape(enum ShapeID shapeID, enum HouseType houseID, int x, int y);

extern void Video_DrawMinimap(int left, int top, int map_scale, int mode);

#include "video_a5.h"

#define Video_Init()            true
#define Video_Uninit()
#define Video_Tick              VideoA5_Tick

#define Video_DrawCPS                VideoA5_DrawCPS
#define Video_DrawCPSRegion          VideoA5_DrawCPSRegion
#define Video_DrawCPSSpecial         VideoA5_DrawCPSSpecial
#define Video_DrawCPSSpecialScale    VideoA5_DrawCPSSpecialScale
#define Video_DrawIcon          VideoA5_DrawIcon
#define Video_DrawIconAlpha     VideoA5_DrawIconAlpha
#define Video_DrawChar          VideoA5_DrawChar
#define Video_DrawWSA           VideoA5_DrawWSA
#define Video_DrawWSAStatic     VideoA5_DrawWSAStatic

#define GUI_Mouse_Show()
#define GUI_Mouse_Hide()
#define GUI_Mouse_Show_Safe()
#define GUI_Mouse_Hide_Safe()
#define GUI_Mouse_Show_InRegion()
#define GUI_Mouse_Hide_InRegion(l,t,r,b)
#define GUI_Mouse_Show_InWidget()   \
	do {} while (false)
#define GUI_Mouse_Hide_InWidget(w)

#if 0
#include "video_sdl.h"

#define Video_Init                  VideoSDL_Init
#define Video_Uninit                VideoSDL_Uninit
#define Video_Tick                  VideoSDL_Tick
#define Video_SetPalette            VideoSDL_SetPalette
#define Video_Mouse_SetPosition     VideoSDL_Mouse_SetPosition
#define Video_Mouse_SetRegion       VideoSDL_Mouse_SetRegion
#endif

#endif

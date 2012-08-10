#ifndef VIDEO_VIDEO_H
#define VIDEO_VIDEO_H

#include "video_a5.h"

#define Video_Init()            true
#define Video_Uninit()
#define Video_Tick              VideoA5_Tick
#define Video_SetPalette        VideoA5_SetPalette
#define Video_SetClippingArea   VideoA5_SetClippingArea
#define Video_SetCursor         VideoA5_SetCursor

#define Video_DrawIcon          VideoA5_DrawIcon
#define Video_DrawShape         VideoA5_DrawShape
#define Video_DrawShapeGrey     VideoA5_DrawShapeGrey
#define Video_DrawShapeTint     VideoA5_DrawShapeTint
#define Video_DrawChar          VideoA5_DrawChar

#define GFX_PutPixel            VideoA5_PutPixel
#define GUI_DrawLine            VideoA5_DrawLine
#define GUI_DrawWiredRectangle  VideoA5_DrawRectangle
#define GUI_DrawFilledRectangle VideoA5_DrawFilledRectangle

#define GUI_Mouse_Show()
#define GUI_Mouse_Hide()
#define GUI_Mouse_Show_Safe()
#define GUI_Mouse_Hide_Safe()
#define GUI_Mouse_Show_InRegion()
#define GUI_Mouse_Hide_InRegion(l,t,r,b)
#define GUI_Mouse_Show_InWidget()   \
	do {} while (false)
#define GUI_Mouse_Hide_InWidget(w)
#define Video_Mouse_SetPosition(x,y)
#define Video_Mouse_SetRegion(l,r,t,b)

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

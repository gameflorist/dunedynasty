#ifndef VIDEO_VIDEO_H
#define VIDEO_VIDEO_H

#include "video_a5.h"

#define Video_Init()            true
#define Video_Uninit()
#define Video_Tick              VideoA5_Tick
#define Video_SetPalette        VideoA5_SetPalette

#define Video_DrawIcon          VideoA5_DrawIcon

#define Video_Mouse_SetPosition(x,y)    \
	do { VARIABLE_NOT_USED(x); VARIABLE_NOT_USED(y); } while (false)
#define Video_Mouse_SetRegion(l,r,t,b)  \
	do { VARIABLE_NOT_USED(l); VARIABLE_NOT_USED(r); VARIABLE_NOT_USED(t); VARIABLE_NOT_USED(b); } while (false)

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

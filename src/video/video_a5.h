#ifndef VIDEO_VIDEOA5_H
#define VIDEO_VIDEOA5_H

#include "types.h"

extern bool VideoA5_Init(void);
extern void VideoA5_Uninit(void);
extern void VideoA5_Tick(void);
extern void VideoA5_SetPalette(const uint8 *palette, int from, int length);

extern void VideoA5_InitSprites(void);

#endif

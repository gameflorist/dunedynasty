#ifndef VIDEO_VIDEOSDL_H
#define VIDEO_VIDEOSDL_H

extern bool VideoSDL_Init(void);
extern void VideoSDL_Uninit(void);
extern void VideoSDL_Tick(void);
extern void VideoSDL_SetPalette(void *palette, int from, int length);
extern void VideoSDL_Mouse_SetPosition(uint16 x, uint16 y);
extern void VideoSDL_Mouse_SetRegion(uint16 minX, uint16 maxX, uint16 minY, uint16 maxY);

#endif

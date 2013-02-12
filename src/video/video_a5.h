#ifndef VIDEO_VIDEOA5_H
#define VIDEO_VIDEOA5_H

#include "video.h"
#include "../file.h"

enum GraphicsDriver {
	GRAPHICS_DRIVER_OPENGL,
	GRAPHICS_DRIVER_DIRECT3D,
};

extern enum GraphicsDriver g_graphics_driver;

extern bool VideoA5_Init(void);
extern void VideoA5_Uninit(void);
extern void VideoA5_ToggleFullscreen(void);
extern void VideoA5_ToggleFPS(void);
extern void VideoA5_CaptureScreenshot(void);
extern void VideoA5_Tick(void);

extern void VideoA5_InitSprites(void);
extern void VideoA5_DisplayFound(void);
extern void VideoA5_DrawCPS(enum SearchDirectory dir, const char *filename);
extern void VideoA5_DrawCPSRegion(enum SearchDirectory dir, const char *filename, int sx, int sy, int dx, int dy, int w, int h);
extern void VideoA5_DrawCPSSpecial(enum CPSID cpsID, enum HouseType houseID, int x, int y);
extern void VideoA5_DrawCPSSpecialScale(enum CPSID cpsID, enum HouseType houseID, int x, int y, float scale);
extern void VideoA5_DrawIcon(uint16 iconID, enum HouseType houseID, int x, int y);
extern void VideoA5_DrawIconAlpha(uint16 iconID, int x, int y, unsigned char alpha);
extern void VideoA5_DrawRectCross(int x1, int y1, int w, int h, unsigned char c);
extern void VideoA5_DrawShape(enum ShapeID shapeID, enum HouseType houseID, int x, int y, int flags);
extern void VideoA5_DrawShapeRotate(enum ShapeID shapeID, enum HouseType houseID, int x, int y, int orient256, int flags);
extern void VideoA5_DrawShapeScale(enum ShapeID shapeID, int x, int y, int w, int h, int flags);
extern void VideoA5_DrawShapeGrey(enum ShapeID shapeID, int x, int y, int flags);
extern void VideoA5_DrawShapeGreyScale(enum ShapeID shapeID, int x, int y, int w, int h, int flags);
extern void VideoA5_DrawShapeTint(enum ShapeID shapeID, int x, int y, unsigned char c, int flags);
extern void VideoA5_DrawChar(unsigned char c, const uint8 *pal, int x, int y);
extern bool VideoA5_DrawWSA(void *wsa, int frame, int sx, int sy, int dx, int dy, int w, int h);
extern void VideoA5_DrawWSAStatic(int frame, int x, int y);

#endif

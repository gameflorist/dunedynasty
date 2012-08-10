#ifndef VIDEO_VIDEOA5_H
#define VIDEO_VIDEOA5_H

#include "../shape.h"

extern bool VideoA5_Init(void);
extern void VideoA5_Uninit(void);
extern void VideoA5_Tick(void);
extern void VideoA5_SetPalette(const uint8 *palette, int from, int length);

extern void VideoA5_PutPixel(int x, int y, uint8 c);
extern void VideoA5_DrawLine(int x1, int y1, int x2, int y2, uint8 c);
extern void VideoA5_DrawRectangle(int x1, int y1, int x2, int y2, uint8 c);
extern void VideoA5_DrawFilledRectangle(int x1, int y1, int x2, int y2, uint8 c);

extern void VideoA5_InitSprites(void);
extern void VideoA5_DrawIcon(uint16 iconID, enum HouseType houseID, int x, int y);
extern void VideoA5_DrawShape(enum ShapeID shapeID, enum HouseType houseID, int x, int y, int flags);
extern void VideoA5_DrawShapeGrey(enum ShapeID shapeID, int x, int y, int flags);
extern void VideoA5_DrawShapeTint(enum ShapeID shapeID, int x, int y, unsigned char c, int flags);

#endif

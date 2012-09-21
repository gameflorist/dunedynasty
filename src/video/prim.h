#ifndef VIDEO_PRIM_H
#define VIDEO_PRIM_H

#include "types.h"

extern void Prim_Line(float x1, float y1, float x2, float y2, uint8 c, float thickness);
extern void Prim_Hline(int x1, int y, int x2, uint8 c);
extern void Prim_Vline(int x, int y1, int y2, uint8 c);

extern void Prim_Rect(float x1, float y1, float x2, float y2, uint8 c, float thickness);
extern void Prim_Rect_i(int x1, int y1, int x2, int y2, uint8 c);

extern void Prim_FillRect(float x1, float y1, float x2, float y2, uint8 c);
extern void Prim_FillRect_i(int x1, int y1, int x2, int y2, uint8 c);
extern void Prim_FillRect_RGBA(int x1, int y1, int x2, int y2, unsigned char r, unsigned char g, unsigned char b, unsigned char alpha);

#endif

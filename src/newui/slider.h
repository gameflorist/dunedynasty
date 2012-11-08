#ifndef NEWUI_SLIDER_H
#define NEWUI_SLIDER_H

#include "../gui/widget.h"

typedef struct SliderData {
	int min;
	int max;
	int curr;
	int tics;
} SliderData;

extern int Slider_Click(Widget *w);
extern Widget *Slider_Allocate(uint16 index, uint16 parentID, uint16 offsetX, uint16 offsetY, int16 width, int16 height);
extern void Slider_Free(Widget *w);

#endif

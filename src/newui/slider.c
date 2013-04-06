/* slider.c */

#include <assert.h>
#include <math.h>
#include <stdlib.h>
#include <string.h>
#include "../os/math.h"

#include "slider.h"

#include "../input/mouse.h"
#include "../video/video.h"

static void
Slider_Draw(Widget *w)
{
	const WidgetProperties *wi = &g_widgetProperties[w->parentID];
	const SliderData *data = w->data;
	const int x = wi->xBase + w->offsetX;
	const int y = wi->yBase + w->offsetY;
	const int pos = x + clamp(0, w->width * (data->curr - data->min) / (data->max - data->min), w->width - 1);

#if 0
	/* Meter. */
	const int width = w->width * (data->curr - data->min) / (data->max - data->min);
	Prim_FillRect(x - 1, y - 1, x + w->width, y + w->height + 1, 12);
	Prim_FillRect(x, y, x + w->width - 1, y + w->height, w->bgColourNormal);
	Prim_FillRect(x, y, pos, y + w->height, w->fgColourNormal);
#endif

	/* Slider. */
	Prim_Hline(x, y + 5, x + w->width - 1, w->bgColourNormal);
	Prim_Hline(x, y + 6, x + w->width - 1, w->fgColourNormal);

	/* Ticks. */
	for (int t = data->min; t <= data->max; t += data->tics) {
		const int ticx = x + clamp(0, w->width * (t - data->min) / (data->max - data->min), w->width - 1);

		Prim_Vline(ticx, y + 4, y + 5, w->bgColourNormal);
	}

	/* Handle. */
	Prim_DrawBorder(pos - 3, y, 6.0f, 12.0f, 1, false, true, 3);
}

int
Slider_Click(Widget *w)
{
	SliderData *data = w->data;
	const WidgetProperties *wi = &g_widgetProperties[w->parentID];
	const int x = wi->xBase + w->offsetX;
	const int range = data->max - data->min;
	const int new_value = clamp(data->min, data->min + round((double)range * (g_mouseX - x + 1) / w->width), data->max);
	const int delta = new_value - data->curr;

	data->curr = new_value;
	return delta;
}

Widget *
Slider_Allocate(uint16 index, uint16 parentID, uint16 offsetX, uint16 offsetY, int16 width, int16 height)
{
	Widget *w = calloc(1, sizeof(*w));

	w->index    = index;
	w->parentID = parentID;
	w->offsetX  = offsetX;
	w->offsetY  = offsetY;
	w->width    = width;
	w->height   = height;

	w->fgColourSelected = 232; /* s_colourBorderSchema[3][2]. */
	w->bgColourSelected = 235; /* s_colourBorderSchema[3][1]. */
	w->fgColourNormal = 232;
	w->bgColourNormal = 235;

	memset(&w->flags, 0, sizeof(w->flags));
	w->flags.buttonFilterLeft = 7;
	w->flags.loseSelect = true;

	memset(&w->state, 0, sizeof(w->state));
	w->state.hover2Last = true;

	w->drawModeNormal   = DRAW_MODE_CUSTOM_PROC;
	w->drawModeSelected = DRAW_MODE_CUSTOM_PROC;
	w->drawModeDown     = DRAW_MODE_CUSTOM_PROC;
	w->drawParameterNormal.proc   = &Slider_Draw;
	w->drawParameterSelected.proc = &Slider_Draw;
	w->drawParameterDown.proc     = &Slider_Draw;
	w->clickProc                  = NULL;

	w->data = calloc(1, sizeof(SliderData));

	return w;
}

void
Slider_Free(Widget *w)
{
	free(w->data);
	free(w);
}

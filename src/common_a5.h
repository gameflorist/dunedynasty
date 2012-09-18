#ifndef COMMON_A5_H
#define COMMON_A5_H

#include <stdbool.h>
#include "gfx.h"

extern struct ALLEGRO_EVENT_QUEUE *g_a5_input_queue;

extern void A5_InitTransform(bool screen_size_changed);
extern bool A5_InitOptions(void);
extern bool A5_Init(void);
extern void A5_Uninit(void);
extern void A5_UseTransform(enum ScreenDivID div);
extern enum ScreenDivID A5_SaveTransform(void);

#endif

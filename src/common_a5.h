#ifndef COMMON_A5_H
#define COMMON_A5_H

#include <stdbool.h>

extern struct ALLEGRO_EVENT_QUEUE *g_a5_input_queue;

extern bool A5_Init(void);
extern void A5_Uninit(void);

#endif

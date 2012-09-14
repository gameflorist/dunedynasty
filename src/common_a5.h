#ifndef COMMON_A5_H
#define COMMON_A5_H

#include <stdbool.h>

extern struct ALLEGRO_EVENT_QUEUE *g_a5_input_queue;

extern void A5_InitTransform(void);
extern bool A5_InitOptions(void);
extern bool A5_Init(void);
extern void A5_Uninit(void);
extern void A5_UseIdentityTransform(void);
extern void A5_UseMenuTransform(void);
extern bool A5_SaveTransform(void);
extern void A5_RestoreTransform(bool prev_transform);

#endif

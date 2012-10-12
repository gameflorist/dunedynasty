#ifndef INPUT_INPUTA5_H
#define INPUT_INPUTA5_H

#include <stdbool.h>

union ALLEGRO_EVENT;

extern bool InputA5_Init(void);
extern void InputA5_Uninit(void);
extern bool InputA5_ProcessEvent(union ALLEGRO_EVENT *event, bool apply_mouse_transform);
extern bool InputA5_Tick(bool apply_mouse_transform);

#endif

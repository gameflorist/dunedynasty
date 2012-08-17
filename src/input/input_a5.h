#ifndef INPUT_INPUTA5_H
#define INPUT_INPUTA5_H

#include <stdbool.h>

extern bool InputA5_Init(void);
extern void InputA5_Uninit(void);
extern void InputA5_Tick(bool apply_mouse_transform);

#endif

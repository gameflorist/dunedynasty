#ifndef INPUT_INPUT_H
#define INPUT_INPUT_H

#include <stdbool.h>
#include "scancode.h"

enum InputMouseMode {
	INPUT_MOUSE_MODE_NORMAL = 0,
	INPUT_MOUSE_MODE_RECORD = 1,
	INPUT_MOUSE_MODE_PLAY   = 2
};

extern void Input_Init(void);
extern void Input_History_Clear(void);
extern void Input_EventHandler(enum Scancode key);
extern bool Input_Test(enum Scancode key);
extern bool Input_IsInputAvailable(void);
extern enum Scancode Input_PeekNextKey(void);
extern enum Scancode Input_GetNextKey(void);

#include "input_a5.h"

#define Input_Tick  InputA5_Tick

#endif

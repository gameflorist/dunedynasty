/* input_a5.c */

#include <assert.h>
#include <allegro5/allegro.h>
#include <allegro5/allegro_audio.h>

#include "input_a5.h"

#include "../audio/audio.h"
#include "../common_a5.h"
#include "../config.h"
#include "../input/input.h"
#include "../input/mouse.h"
#include "../opendune.h"
#include "../video/video_a5.h"
#include "scancode.h"

bool
InputA5_Init(void)
{
	if (al_install_keyboard() != true)
		return false;

	if (al_install_mouse() != true)
		return false;

	al_register_event_source(g_a5_input_queue, al_get_keyboard_event_source());
	al_register_event_source(g_a5_input_queue, al_get_mouse_event_source());

	return true;
}

void
InputA5_Uninit(void)
{
}

static int
InputA5_KeycodeToScancode(int kc)
{
	const int map[26 + 10 + 10 + 12] = {
		SCANCODE_A, SCANCODE_B, SCANCODE_C, SCANCODE_D, SCANCODE_E,
		SCANCODE_F, SCANCODE_G, SCANCODE_H, SCANCODE_I, SCANCODE_J,
		SCANCODE_K, SCANCODE_L, SCANCODE_M, SCANCODE_N, SCANCODE_O,
		SCANCODE_P, SCANCODE_Q, SCANCODE_R, SCANCODE_S, SCANCODE_T,
		SCANCODE_U, SCANCODE_V, SCANCODE_W, SCANCODE_X, SCANCODE_Y,
		SCANCODE_Z,

		SCANCODE_0, SCANCODE_1, SCANCODE_2, SCANCODE_3, SCANCODE_4,
		SCANCODE_5, SCANCODE_6, SCANCODE_7, SCANCODE_8, SCANCODE_9,

		SCANCODE_KEYPAD_0, SCANCODE_KEYPAD_1, SCANCODE_KEYPAD_2, SCANCODE_KEYPAD_3, SCANCODE_KEYPAD_4,
		SCANCODE_KEYPAD_5, SCANCODE_KEYPAD_6, SCANCODE_KEYPAD_7, SCANCODE_KEYPAD_8, SCANCODE_KEYPAD_9,

		SCANCODE_F1, SCANCODE_F2, SCANCODE_F3, SCANCODE_F4, SCANCODE_F5,
		SCANCODE_F6, SCANCODE_F7, SCANCODE_F8, SCANCODE_F9, SCANCODE_F10,
		SCANCODE_F11, SCANCODE_F12
	};

	const int map_misc[] = {
		SCANCODE_ESCAPE, SCANCODE_TILDE, SCANCODE_MINUS, SCANCODE_EQUALS, SCANCODE_BACKSPACE,
		SCANCODE_TAB, SCANCODE_OPENBRACE, SCANCODE_CLOSEBRACE, -1, SCANCODE_SEMICOLON,
		SCANCODE_QUOTE, SCANCODE_BACKSLASH, SCANCODE_BACKSLASH, SCANCODE_COMMA, SCANCODE_FULLSTOP,
		SCANCODE_SLASH, SCANCODE_SPACE,

		SCANCODE_INSERT, SCANCODE_DELETE, SCANCODE_HOME, SCANCODE_END, SCANCODE_PGUP,
		SCANCODE_PGDN, SCANCODE_LEFT, SCANCODE_RIGHT, SCANCODE_UP, SCANCODE_DOWN,

		SCANCODE_KEYPAD_SLASH, SCANCODE_KEYPAD_ASTERISK, SCANCODE_KEYPAD_MINUS, SCANCODE_KEYPAD_PLUS, SCANCODE_KEYPAD_DELETE,
		SCANCODE_KEYPAD_ENTER,

		SCANCODE_PRINTSCREEN
	};

	const int map_mods[] = {
		SCANCODE_LSHIFT, SCANCODE_LSHIFT,
		SCANCODE_LCTRL, SCANCODE_RCTRL,
		SCANCODE_LALT, SCANCODE_RALT,
		SCANCODE_LWIN, SCANCODE_RWIN, SCANCODE_MENU,
		SCANCODE_SCROLLLOCK, SCANCODE_NUMLOCK, SCANCODE_CAPSLOCK
	};

	if (ALLEGRO_KEY_A <= kc && kc <= ALLEGRO_KEY_F12)
		return map[kc - ALLEGRO_KEY_A];

	if (ALLEGRO_KEY_ESCAPE <= kc && kc <= ALLEGRO_KEY_PRINTSCREEN)
		return map_misc[kc - ALLEGRO_KEY_ESCAPE];

	if (ALLEGRO_KEY_LSHIFT <= kc && kc <= ALLEGRO_KEY_CAPSLOCK)
		return map_mods[kc - ALLEGRO_KEY_LSHIFT];

	return -1;
}

bool
InputA5_ProcessEvent(ALLEGRO_EVENT *event, bool apply_mouse_transform)
{
	enum Scancode mouse_event = 0;

	switch (event->type) {
		case ALLEGRO_EVENT_DISPLAY_CLOSE:
			PrepareEnd();
			exit(0);
			break;

		case ALLEGRO_EVENT_DISPLAY_EXPOSE:
			return true;

#ifdef ALLEGRO_WINDOWS
		case ALLEGRO_EVENT_DISPLAY_FOUND:
			VideoA5_DisplayFound();
			return true;
#endif

		case ALLEGRO_EVENT_MOUSE_BUTTON_UP:
			mouse_event |= SCANCODE_RELEASE;
			/* Fall through. */
		case ALLEGRO_EVENT_MOUSE_BUTTON_DOWN:
			mouse_event |= MOUSE_LMB - (event->mouse.button - 1);
			/* Fall through. */
		case ALLEGRO_EVENT_MOUSE_AXES:
			g_mouseDX += event->mouse.dx;
			g_mouseDY += event->mouse.dy;
			/* Fall through. */
		case ALLEGRO_EVENT_MOUSE_WARPED:
			/* In panning mode, we only warp the mouse to the middle
			 * of the screen to avoid hitting the edge of the screen.
			 * The mouse cursor is still supposed to be at g_mouseX/Y.
			 */
			if (g_mousePanning) {
				Mouse_EventHandler(false, g_mouseX, g_mouseY, 0, mouse_event);
			}
			else {
				Mouse_EventHandler(apply_mouse_transform, event->mouse.x, event->mouse.y, event->mouse.dz, mouse_event);
			}
			return (!g_gameConfig.hardwareCursor && !g_mouseHidden);

		case ALLEGRO_EVENT_KEY_CHAR:
			if ((event->keyboard.keycode == ALLEGRO_KEY_F11) ||
			    (event->keyboard.keycode == ALLEGRO_KEY_ENTER && (event->keyboard.modifiers & (ALLEGRO_KEYMOD_ALT | ALLEGRO_KEYMOD_ALTGR)))) {
				VideoA5_ToggleFullscreen();
				return true;
			}
			else if (event->keyboard.keycode == ALLEGRO_KEY_F10) {
				VideoA5_ToggleFPS();
				return true;
			}
			else if (event->keyboard.keycode == ALLEGRO_KEY_F12) {
				VideoA5_CaptureScreenshot();
				return true;
			}
			else if (event->keyboard.keycode == ALLEGRO_KEY_ENTER) { /* Enter without alt. */
				Input_EventHandler(SCANCODE_ENTER);
				return true;
			}
			break;

		case ALLEGRO_EVENT_KEY_DOWN:
		case ALLEGRO_EVENT_KEY_UP:
			{
				int sc = InputA5_KeycodeToScancode(event->keyboard.keycode);

				if (sc < 0)
					break;

				if (event->type == ALLEGRO_EVENT_KEY_UP)
					sc |= SCANCODE_RELEASE;

				Input_EventHandler(sc);
			}
			break;

		case ALLEGRO_EVENT_AUDIO_STREAM_FRAGMENT:
			AudioA5_PollMusic();
			break;
	}

	return false;
}

bool
InputA5_Tick(bool apply_mouse_transform)
{
	bool redraw = false;

	while (!al_is_event_queue_empty(g_a5_input_queue)) {
		ALLEGRO_EVENT event;

		al_get_next_event(g_a5_input_queue, &event);

		if (InputA5_ProcessEvent(&event, apply_mouse_transform))
			redraw = true;
	}

	return redraw;
}

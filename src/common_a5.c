/* common_a5.c */

#include <allegro5/allegro.h>
#include <assert.h>

#include "common_a5.h"

#include "audio/audio_a5.h"
#include "config.h"
#include "input/input_a5.h"
#include "input/mouse.h"
#include "timer/timer_a5.h"
#include "video/video_a5.h"

ALLEGRO_EVENT_QUEUE *g_a5_input_queue;

static ALLEGRO_TRANSFORM s_transform[SCREENDIV_MAX];
static enum ScreenDivID curr_transform;

/*--------------------------------------------------------------*/

void
A5_InitTransform(void)
{
	const double preferred_aspect = (double)SCREEN_WIDTH / SCREEN_HEIGHT;
	const double w = TRUE_DISPLAY_WIDTH;
	const double h = TRUE_DISPLAY_HEIGHT;
	const double aspect = w/h;

	float scale = (double)TRUE_DISPLAY_WIDTH / SCREEN_WIDTH;
	float offx = 0.0f;
	float offy = 0.0f;

	if (aspect < preferred_aspect - 0.001) {
		const double newh = w / preferred_aspect;
		offy = (h - newh) / 2;
		scale = newh / SCREEN_HEIGHT;
	}
	else if (aspect > preferred_aspect + 0.001) {
		const double neww = h * preferred_aspect;
		offx = (w - neww) / 2;
		scale = neww / SCREEN_WIDTH;
	}

	g_mouse_transform_scale = scale;
	g_mouse_transform_offx = offx;
	g_mouse_transform_offy = offy;

	al_identity_transform(&s_transform[SCREENDIV_MAIN]);
	al_build_transform(&s_transform[SCREENDIV_MENU], offx, offy, scale, scale, 0.0f);

	A5_UseTransform(curr_transform);
}

bool
A5_InitOptions(void)
{
	if (al_init() != true)
		return false;

	GameOptions_Load();

	return true;
}

bool
A5_Init(void)
{
	g_a5_input_queue = al_create_event_queue();
	if (g_a5_input_queue == NULL)
		return false;

	if (InputA5_Init() != true)
		return false;

	if (VideoA5_Init() != true)
		return false;

	if (TimerA5_Init() != true)
		return false;

	AudioA5_Init();
	A5_InitTransform();

	return true;
}

void
A5_Uninit(void)
{
	AudioA5_Uninit();
	TimerA5_Uninit();
	VideoA5_Uninit();
	InputA5_Uninit();

	al_destroy_event_queue(g_a5_input_queue);
	g_a5_input_queue = NULL;
}

void
A5_UseTransform(enum ScreenDivID div)
{
	al_use_transform(&s_transform[div]);
	curr_transform = div;
}

enum ScreenDivID
A5_SaveTransform(void)
{
	return curr_transform;
}

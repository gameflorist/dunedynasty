/* common_a5.c */

#include <assert.h>
#include <allegro5/allegro.h>
#include <math.h>

#include "common_a5.h"

#include "audio/audio_a5.h"
#include "config.h"
#include "input/input_a5.h"
#include "input/mouse.h"
#include "timer/timer_a5.h"
#include "video/video_a5.h"

ALLEGRO_EVENT_QUEUE *g_a5_input_queue;

static ALLEGRO_BITMAP *s_transform[SCREENDIV_MAX];
static enum ScreenDivID curr_transform;

/*--------------------------------------------------------------*/

static void
A5_InitScreenDiv(ALLEGRO_BITMAP *parent, enum ScreenDivID divID,
		float x1, float y1, float x2, float y2)
{
	const float w = x2 - x1;
	const float h = y2 - y1;

	ScreenDiv *div = &g_screenDiv[divID];
	ALLEGRO_TRANSFORM trans;

	div->x = x1;
	div->y = y1;
	div->width = ceil(w / div->scale);
	div->height = ceil(h / div->scale);

	al_destroy_bitmap(s_transform[divID]);
	s_transform[divID] = al_create_sub_bitmap(parent, x1, y1 + GFX_ScreenShake_Offset(), w, h);
	al_set_target_bitmap(s_transform[divID]);

	if (divID == SCREENDIV_MAIN) {
		al_identity_transform(&trans);
		al_use_transform(&trans);
	}
	else {
		al_build_transform(&trans, 0, 0, div->scale, div->scale, 0.0f);
		al_use_transform(&trans);
	}
}

void
A5_InitTransform(bool screen_size_changed)
{
	ALLEGRO_TRANSFORM trans;

	/* Identity. */
	if (screen_size_changed) {
		s_transform[SCREENDIV_MAIN] = al_get_target_bitmap();
		assert(!al_is_sub_bitmap(s_transform[SCREENDIV_MAIN]));

		al_identity_transform(&trans);
		al_use_transform(&trans);
	}

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

	ALLEGRO_BITMAP *target = s_transform[SCREENDIV_MAIN];
	assert(target != NULL);

	/* Menu. */
	{
		ScreenDiv *div = &g_screenDiv[SCREENDIV_MENU];
		div->scale = scale;

		ALLEGRO_BITMAP *sub = al_create_sub_bitmap(target, 0, 0, TRUE_DISPLAY_WIDTH, TRUE_DISPLAY_HEIGHT);
		al_set_target_bitmap(sub);

		al_destroy_bitmap(s_transform[SCREENDIV_MENU]);
		s_transform[SCREENDIV_MENU] = sub;
		al_build_transform(&trans, offx, offy, div->scale, div->scale, 0.0f);
		al_use_transform(&trans);
	}

	ScreenDiv *menubar = &g_screenDiv[SCREENDIV_MENUBAR];
	A5_InitScreenDiv(target, SCREENDIV_MENUBAR,
			0.0f, 0.0f, TRUE_DISPLAY_WIDTH, menubar->scale * 40.0f);

	ScreenDiv *sidebar = &g_screenDiv[SCREENDIV_SIDEBAR];
	A5_InitScreenDiv(target, SCREENDIV_SIDEBAR,
			TRUE_DISPLAY_WIDTH - sidebar->scale * 80.0f, menubar->scale * menubar->height,
			TRUE_DISPLAY_WIDTH, TRUE_DISPLAY_HEIGHT);

	A5_InitScreenDiv(target, SCREENDIV_VIEWPORT,
			0.0f, menubar->scale * menubar->height, sidebar->x, TRUE_DISPLAY_HEIGHT);

	A5_UseTransform(curr_transform);
}

bool
A5_InitOptions(void)
{
	if (al_init() != true)
		return false;

#ifdef ALLEGRO_WINDOWS
	al_set_app_name("Dune Dynasty");
#else
	al_set_app_name("dunedynasty");
#endif

	GFX_InitDefaultViewportScales(true);
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
	A5_InitTransform(true);

	return true;
}

void
A5_Uninit(void)
{
	GameOptions_Save();

	al_set_target_bitmap(s_transform[SCREENDIV_MAIN]);

	s_transform[SCREENDIV_MAIN] = NULL;

	for (enum ScreenDivID div = SCREENDIV_MAIN + 1; div < SCREENDIV_MAX; div++) {
		al_destroy_bitmap(s_transform[div]);
		s_transform[div] = NULL;
	}

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
	al_set_target_bitmap(s_transform[div]);
	curr_transform = div;
}

enum ScreenDivID
A5_SaveTransform(void)
{
	return curr_transform;
}

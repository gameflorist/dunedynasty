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
	ScreenDiv *div = &g_screenDiv[divID];
	ALLEGRO_TRANSFORM trans;

	if (g_aspect_correction == ASPECT_RATIO_CORRECTION_FULL) {
		div->scaley = div->scalex * GFX_AspectCorrection_GetRatio();
	}
	else {
		div->scaley = div->scalex;
	}

	if (divID == SCREENDIV_MENUBAR) {
		y2 = div->scaley * y2;
	}

	const float w = x2 - x1;
	const float h = y2 - y1;

	div->x = x1;
	div->y = y1;
	div->width = ceil(w / div->scalex);
	div->height = ceil(h / div->scaley);

	al_destroy_bitmap(s_transform[divID]);
	s_transform[divID] = al_create_sub_bitmap(parent, x1, y1 + GFX_ScreenShake_Offset(), w, h);
	al_set_target_bitmap(s_transform[divID]);

	if (divID == SCREENDIV_MAIN) {
		al_identity_transform(&trans);
		al_use_transform(&trans);
	}
	else {
		al_build_transform(&trans, 0, 0, div->scalex, div->scaley, 0.0f);
		al_use_transform(&trans);
	}
}

void
A5_InitTransform(bool screen_size_changed)
{
	ALLEGRO_TRANSFORM trans;

	const float pixel_aspect_ratio = GFX_AspectCorrection_GetRatio();

	/* Identity. */
	if (screen_size_changed) {
		s_transform[SCREENDIV_MAIN] = al_get_target_bitmap();
		assert(!al_is_sub_bitmap(s_transform[SCREENDIV_MAIN]));

		al_identity_transform(&trans);
		al_use_transform(&trans);
	}

	const double preferred_aspect = (double)SCREEN_WIDTH / SCREEN_HEIGHT;
	const double aspect = pixel_aspect_ratio * TRUE_DISPLAY_WIDTH / TRUE_DISPLAY_HEIGHT;

	float scale = (double)TRUE_DISPLAY_WIDTH / SCREEN_WIDTH;
	float offx = 0.0f;
	float offy = 0.0f;

	/* Tall screens. */
	if (aspect < preferred_aspect - 0.001) {
		const double newh = pixel_aspect_ratio * scale * SCREEN_HEIGHT;
		offy = (TRUE_DISPLAY_HEIGHT - newh) / 2;
	}
	else if (aspect > preferred_aspect + 0.001) {
		scale = (double)TRUE_DISPLAY_HEIGHT / (pixel_aspect_ratio * SCREEN_HEIGHT);
		const double neww = scale * SCREEN_WIDTH;
		offx = (TRUE_DISPLAY_WIDTH - neww) / 2;
	}

	ALLEGRO_BITMAP *target = s_transform[SCREENDIV_MAIN];
	assert(target != NULL);

	/* Menu. */
	{
		ScreenDiv *div = &g_screenDiv[SCREENDIV_MENU];
		div->x = offx;
		div->y = offy;
		div->scalex = scale;
		div->scaley = scale * pixel_aspect_ratio;

		ALLEGRO_BITMAP *sub = al_create_sub_bitmap(target, 0, 0, TRUE_DISPLAY_WIDTH, TRUE_DISPLAY_HEIGHT);
		al_set_target_bitmap(sub);

		al_destroy_bitmap(s_transform[SCREENDIV_MENU]);
		s_transform[SCREENDIV_MENU] = sub;
		al_build_transform(&trans, offx, offy, div->scalex, div->scaley, 0.0f);
		al_use_transform(&trans);
	}

	ScreenDiv *menubar = &g_screenDiv[SCREENDIV_MENUBAR];
	A5_InitScreenDiv(target, SCREENDIV_MENUBAR,
			0.0f, 0.0f, TRUE_DISPLAY_WIDTH, 40.0f);

	ScreenDiv *sidebar = &g_screenDiv[SCREENDIV_SIDEBAR];
	A5_InitScreenDiv(target, SCREENDIV_SIDEBAR,
			TRUE_DISPLAY_WIDTH - sidebar->scalex * 80.0f, menubar->scaley * menubar->height,
			TRUE_DISPLAY_WIDTH, TRUE_DISPLAY_HEIGHT);

	A5_InitScreenDiv(target, SCREENDIV_VIEWPORT,
			0.0f, menubar->scaley * menubar->height, sidebar->x, TRUE_DISPLAY_HEIGHT);

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

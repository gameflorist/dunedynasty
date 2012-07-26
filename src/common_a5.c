/* common_a5.c */

#include <allegro5/allegro.h>
#include <assert.h>

#include "common_a5.h"

#include "input/input_a5.h"
#include "timer/timer_a5.h"
#include "video/video_a5.h"

ALLEGRO_EVENT_QUEUE *g_a5_input_queue;

bool
A5_Init(void)
{
	if (al_init() != true)
		return false;

	g_a5_input_queue = al_create_event_queue();
	if (g_a5_input_queue == NULL)
		return false;

	if (InputA5_Init() != true)
		return false;

	if (VideoA5_Init() != true)
		return false;

	if (TimerA5_Init() != true)
		return false;

	return true;
}

void
A5_Uninit(void)
{
	TimerA5_Uninit();
	VideoA5_Uninit();
	InputA5_Uninit();

	al_destroy_event_queue(g_a5_input_queue);
	g_a5_input_queue = NULL;
}

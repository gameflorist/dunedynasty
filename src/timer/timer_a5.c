/* timer_a5.c */

#include <allegro5/allegro.h>
#include <assert.h>

#include "timer_a5.h"

#include "../audio/audio.h"
#include "../common_a5.h"
#include "../config.h"
#include "../enhancement.h"

enum GameSpeed {
	GAMESPEED_SLOWEST   = 0,
	GAMESPEED_SLOW      = 1,
	GAMESPEED_NORMAL    = 2,
	GAMESPEED_FAST      = 3,
	GAMESPEED_FASTEST   = 4,

	GAMESPEED_MAX
};

static const double s_game_speed[GAMESPEED_MAX] = {
	1.0/30.0, 1.0/45.0, 1.0/60.0, 1.0/90.0, 1.0/120.0
};

static ALLEGRO_TIMER *s_timer[2];
ALLEGRO_EVENT_QUEUE *s_timer_queue;

bool
TimerA5_Init(void)
{
	s_timer[TIMER_GUI] = al_create_timer(s_game_speed[GAMESPEED_NORMAL]);
	s_timer[TIMER_GAME] = al_create_timer(s_game_speed[GAMESPEED_NORMAL]);
	if (s_timer[TIMER_GUI] == NULL || s_timer[TIMER_GAME] == NULL)
		return false;

	s_timer_queue = al_create_event_queue();
	if (s_timer_queue == NULL)
		return false;

	al_register_event_source(g_a5_input_queue, al_get_timer_event_source(s_timer[TIMER_GUI]));
	return true;
}

void
TimerA5_Uninit(void)
{
	al_destroy_timer(s_timer[TIMER_GUI]);
	al_destroy_timer(s_timer[TIMER_GAME]);
}

/*--------------------------------------------------------------*/

bool
Timer_SetTimer(enum TimerType timer, bool set)
{
	assert(timer <= TIMER_GAME);

	if (set) {
		if (timer == TIMER_GAME) {
			if (enhancement_true_game_speed_adjustment) {
				al_set_timer_speed(s_timer[timer], s_game_speed[g_gameConfig.gameSpeed]);
			}
			else {
				al_set_timer_speed(s_timer[timer], s_game_speed[GAMESPEED_NORMAL]);
			}
		}

		al_start_timer(s_timer[timer]);
		return true;
	}
	else {
		al_stop_timer(s_timer[timer]);
		return false;
	}
}

int64_t
Timer_GetTimer(enum TimerType timer)
{
	assert(timer <= TIMER_GAME);

	return al_get_timer_count(s_timer[timer]);
}

void
Timer_RegisterSource(void)
{
	ALLEGRO_EVENT_SOURCE *source = al_get_timer_event_source(s_timer[TIMER_GUI]);

	al_register_event_source(s_timer_queue, source);
	al_flush_event_queue(s_timer_queue);
}

void
Timer_UnregisterSource(void)
{
	ALLEGRO_EVENT_SOURCE *source = al_get_timer_event_source(s_timer[TIMER_GUI]);

	al_unregister_event_source(s_timer_queue, source);
}

void
Timer_WaitForEvent(void)
{
	ALLEGRO_EVENT ev;
	al_wait_for_event(s_timer_queue, &ev);
}

void
Timer_Sleep(int tics)
{
	Timer_RegisterSource();

	for (int i = 0; i < tics; i++) {
		Timer_WaitForEvent();
		Audio_PollMusic();
	}

	Timer_UnregisterSource();
}

bool
Timer_QueueIsEmpty(void)
{
	return al_event_queue_is_empty(s_timer_queue);
}

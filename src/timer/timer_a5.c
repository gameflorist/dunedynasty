/* timer_a5.c */

#include <allegro5/allegro.h>
#include <assert.h>

#include "timer_a5.h"

static ALLEGRO_TIMER *s_timer[2];
static ALLEGRO_EVENT_QUEUE *s_timer_queue;

bool
TimerA5_Init(void)
{
	s_timer[TIMER_GUI] = al_create_timer(1.0 / 60.0);
	s_timer[TIMER_GAME] = al_create_timer(1.0 / 60.0);
	if (s_timer[TIMER_GUI] == NULL || s_timer[TIMER_GAME] == NULL)
		return false;

	s_timer_queue = al_create_event_queue();
	if (s_timer_queue == NULL)
		return false;

	return true;
}

void
TimerA5_Uninit(void)
{
	al_destroy_timer(s_timer[TIMER_GUI]);
	al_destroy_timer(s_timer[TIMER_GAME]);
}

bool
TimerA5_SetTimer(enum TimerType timer, bool set)
{
	assert(timer <= TIMER_GAME);

	if (set) {
		al_start_timer(s_timer[timer]);
		return true;
	}
	else {
		al_stop_timer(s_timer[timer]);
		return false;
	}
}

void
TimerA5_Sleep(int tics)
{
	ALLEGRO_EVENT_SOURCE *source = al_get_timer_event_source(s_timer[TIMER_GUI]);

	al_register_event_source(s_timer_queue, source);
	al_flush_event_queue(s_timer_queue);

	for (int i = 0; i < tics; i++) {
		al_wait_for_event(s_timer_queue, NULL);
		al_drop_next_event(s_timer_queue);
	}

	al_unregister_event_source(s_timer_queue, source);
}

int64_t
TimerA5_GetTicks(enum TimerType timer)
{
	assert(timer <= TIMER_GAME);

	return al_get_timer_count(s_timer[timer]);
}

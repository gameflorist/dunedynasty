/* common_a5.c */

#include <allegro5/allegro.h>
#include <assert.h>

#include "common_a5.h"

#include "timer/timer_a5.h"
#include "video/video_a5.h"

bool
A5_Init(void)
{
	if (al_init() != true)
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
}

#include <stdio.h>
#include <allegro5/allegro.h>
#include <allegro5/allegro_audio.h>
#include "audlib_a5.h"

int main(int argc, const char *argv[])
{
    ALLEGRO_DISPLAY *dpy;
    ALLEGRO_FILE *fp;
    AUDSTREAM *aud;
    ALLEGRO_AUDIO_STREAM *as;
    ALLEGRO_EVENT_QUEUE *queue;
    ALLEGRO_EVENT ev;
    int rc;

    al_init();
    al_install_keyboard();
    al_install_audio();
    al_reserve_samples(0);  /* setup default mixer */

    if (argc < 2)
	return 1;

    fp = al_fopen(argv[1], "rb");
    if (!fp)
	return 1;

    aud = load_aud_stream(fp);
    if (!aud)
	return 1;

    as = get_aud_stream(aud);
    al_attach_audio_stream_to_mixer(as, al_get_default_mixer());

    queue = al_create_event_queue();
    al_register_event_source(queue, al_get_audio_stream_event_source(as));

    dpy = al_create_display(320, 200);
    if (!dpy)
	return 1;
    al_register_event_source(queue, al_get_keyboard_event_source());

    for (;;) {
	al_wait_for_event(queue, &ev);
	if (ev.type == ALLEGRO_EVENT_AUDIO_STREAM_FRAGMENT) {
	    rc = poll_aud_stream(aud);
	    if (rc > 1) {
		printf("loop\n");
		restart_aud_stream(aud);
	    }
	}
	if (ev.type == ALLEGRO_EVENT_KEY_CHAR) {
	    if (ev.keyboard.unichar == 'r') {
		restart_aud_stream(aud);
	    }
	    if (ev.keyboard.unichar == 'q') {
		break;
	    }
	}
    }

    unload_aud_stream(aud);

    return 0;
}

/* vim: set sts=4 sw=4 et: */

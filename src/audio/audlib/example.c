/* This file is part of the AUDlib package.  See README for details. */
#include <stdio.h>
#include <ctype.h>
#include <allegro.h>
#include "audlib.h"

int main(int argc, char **argv)
{
    char *fn[20];
    int i, num;

    if (argc < 2) {
	printf("usage: %s audfile.aud [audfile2.aud ...] (max:20)\n", 
	       get_filename(argv[0]));
	printf("\teg. %s music/*.aud\n", get_filename(argv[0]));
	return 0;
    }

    /* Install Allegro. */
    allegro_init();
    install_keyboard();

    /* You need digital sound installed. */
    if (install_sound(DIGI_AUTODETECT, MIDI_NONE, NULL) == -1) {
	printf("Error: install_sound() failed!\n");
	return 1;
    }

    for (i = 1; i < argc && i - 1 < 20; i++)
	fn[i - 1] = argv[i];
    num = i - 1;
    while (i < 20)
	fn[i++] = NULL;

    for (i = 0; i < num; i++)
	printf(" %c  %s\n", 'A' + i, fn[i]);

    printf("Keys:   A - T           Select file\n");
    printf("        Up/Down         Adjust volume\n");
    printf("        Enter           Restart\n");
    printf("        P/R             Pause/Resume\n");
    printf("        Space (hold)    Don't call poll_aud()\n");
    printf("        Escape          Quit\n\n");

    while (!key[KEY_ESC]) {
	/* Poll the sound as often as possible.
	 * KEY_SPACE: Allow user to hear what it sounds like if you don't.
	 */
	if (!key[KEY_SPACE])
	    poll_aud_stream();

	if (keypressed()) {
	    int k = toupper(readkey() & 0xff);

	    if (k >= 'A' && k < 'A' + num) {
		/* Load the file, with looping.
		 * If a file was previously loaded, it will automagically 
		 * be unloaded first.
		 */
		if (load_aud_stream(fn[k - 'A'], TRUE) == FALSE)
		    printf("Error: %s\n", aud_stream_error);
	    }

	    /* Example of restart. */
	    if (k == '\r')
		restart_aud_stream();

	    /* Example of pause / resume. */
	    if (k == 'P')
		pause_aud_stream();
	    else if (k == 'R')
		resume_aud_stream();

	    /* (very dodgy here) */

	    if (key[KEY_UP]) {
		/* set_aud_stream_volume() will clip 0 - 255 for you. */
		set_aud_stream_volume(aud_stream_volume + 2);
		printf("Volume: %03d\n", aud_stream_volume);
	    }

	    if (key[KEY_DOWN]) {
		set_aud_stream_volume(aud_stream_volume - 2);
		printf("Volume: %03d\n", aud_stream_volume);
	    }
	}
    }

    /* You need to call this. */
    unload_aud_stream();
    return 0;
}

END_OF_MAIN();

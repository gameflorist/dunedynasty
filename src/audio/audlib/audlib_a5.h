/* This file is part of the AUDlib package, see README for details. */
#ifndef __included_audlib_a5_h
#define __included_audlib_a5_h

#include <allegro5/allegro.h>
#include <allegro5/allegro_audio.h>

#ifdef __cplusplus
extern "C" {
#endif


typedef struct AUDSTREAM AUDSTREAM;

AUDSTREAM *load_aud_stream(ALLEGRO_FILE *f);
void unload_aud_stream(AUDSTREAM *aud);
ALLEGRO_AUDIO_STREAM *get_aud_stream(AUDSTREAM *aud);
int  poll_aud_stream(AUDSTREAM *aud);
void restart_aud_stream(AUDSTREAM *aud);


#ifdef __cplusplus
    }
#endif

#endif

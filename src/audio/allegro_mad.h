#ifndef __included_allegro_mad_h
#define __included_allegro_mad_h

#include <allegro5/allegro.h>
#include <allegro5/allegro_audio.h>

typedef struct MP3 MP3;

MP3 *load_mp3(const char *filename);
void unload_mp3(MP3 *mp3);
ALLEGRO_AUDIO_STREAM *get_mp3_stream(MP3 *mp3);
int poll_mp3(MP3 *mp3);

#endif

/* vim: set sts=4 sw=4 et: */

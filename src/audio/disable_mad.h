#ifndef AUDIO_DISABLE_MAD_H
#define AUDIO_DISABLE_MAD_H

typedef void MP3;

MP3 *
load_mp3(const char *filename)
{
	(void)filename;
	return NULL;
}

void
unload_mp3(MP3 *mp3)
{
	(void)mp3;
}

ALLEGRO_AUDIO_STREAM *
get_mp3_stream(MP3 *mp3)
{
	(void)mp3;
	return NULL;
}

int
poll_mp3(MP3 *mp3)
{
	(void)mp3;
	return 0;
}

#endif

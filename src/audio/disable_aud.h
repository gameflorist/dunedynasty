#ifndef AUDIO_DISABLE_AUD_H
#define AUDIO_DISABLE_AUD_H

typedef void AUDSTREAM;

AUDSTREAM *
load_aud_stream(ALLEGRO_FILE *f)
{
	(void)f;
	return NULL;
}

void
unload_aud_stream(AUDSTREAM *aud)
{
	(void)aud;
}

ALLEGRO_AUDIO_STREAM *
get_aud_stream(AUDSTREAM *aud)
{
	(void)aud;
	return NULL;
}

int
poll_aud_stream(AUDSTREAM *aud)
{
	(void)aud;
	return 0;
}

void
restart_aud_stream(AUDSTREAM *aud)
{
	(void)aud;
}

#endif

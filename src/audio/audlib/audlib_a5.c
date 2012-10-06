/*  AUDlib, a .AUD sample format extension for Allegro.
 *  Copyright (C) 1998 Peter Wang.
 *  See README for more information.
 */

#include <stdio.h>
#include <string.h>
#include <allegro5/allegro.h>
#include <allegro5/allegro_audio.h>
#include "audlib_a5.h"

#define FRAG_COUNT	4
#define BUFFER_SIZE	2048	/* must be a multiple of 2048 */

#define AUD_HEADER_LEN	12


struct AUDSTREAM {
    ALLEGRO_FILE *fp;
    int64_t file_endpos;
    int stereo;
    ALLEGRO_AUDIO_STREAM *stream;

    /* the current chunk's data */
    unsigned char *data;
    int data_size;
    int data_index;
    int low_nibble;

    /* decompression variables */
    int step_index[2];
    int valpred[2];

    int last_val;
};



/*
 *  Internal use only
 */

/* IMA-ADPCM compression / decompression tables */
static const int index_table[16] = {
    -1, -1, -1, -1, 2, 4, 6, 8,
    -1, -1, -1, -1, 2, 4, 6, 8
};

static const int step_table[89] = {
    7, 8, 9, 10, 11, 12, 13, 14, 16,
    17, 19, 21, 23, 25, 28, 31, 34, 37,
    41, 45, 50, 55, 60, 66, 73, 80, 88,
    97, 107, 118, 130, 143, 157, 173, 190, 209,
    230, 253, 279, 307, 337, 371, 408, 449, 494,
    544, 598, 658, 724, 796, 876, 963, 1060, 1166,
    1282, 1411, 1552, 1707, 1878, 2066, 2272, 2499, 2749,
    3024, 3327, 3660, 4026, 4428, 4871, 5358, 5894, 6484,
    7132, 7845, 8630, 9493, 10442, 11487, 12635, 13899, 15289,
    16818, 18500, 20350, 22385, 24623, 27086, 29794, 32767
};



/*
 *  Functions
 */

static AUDSTREAM *load_aud_stream_inner(ALLEGRO_FILE *fp, AUDSTREAM *aud)
{
    unsigned short freq;	/* frequency */
    unsigned long size;		/* size of file (without header) */
    unsigned long outsize;	/* size of output data */
    unsigned char flags;	/* bit 0=stereo, 1=16bit */
    unsigned char type;		/* 1=Westwood compressed, 99=IMA ADPCM */
    int chanconf;

    freq = al_fread16le(fp);
    size = al_fread32le(fp);
    outsize = al_fread32le(fp);
    flags = al_fgetc(fp);
    type = al_fgetc(fp);
    (void)outsize;

    if (type == 1) {
	/* unsupported compression type */
	return NULL;
    }
    else if (type != 99) {
	/* not valid AUD file */
	return NULL;
    }

    aud->fp = fp;
    aud->file_endpos = AUD_HEADER_LEN + size;
    aud->stereo = (flags & 1);

    if (aud->stereo)
	chanconf = ALLEGRO_CHANNEL_CONF_2;
    else
	chanconf = ALLEGRO_CHANNEL_CONF_1;

    aud->stream = al_create_audio_stream(FRAG_COUNT, BUFFER_SIZE, freq,
	ALLEGRO_AUDIO_DEPTH_INT16, chanconf);
    if (!aud->stream)
	return NULL;

    aud->step_index[0] = aud->valpred[0] = 0;
    aud->step_index[1] = aud->valpred[1] = 0;

    return aud;
}

AUDSTREAM *load_aud_stream(ALLEGRO_FILE *f)
{
    AUDSTREAM *aud = calloc(1, sizeof(AUDSTREAM));

    if (!load_aud_stream_inner(f, aud)) {
	unload_aud_stream(aud);
	return NULL;
    }

    return aud;
}

void unload_aud_stream(AUDSTREAM *aud)
{
    if (!aud)
	return;

    free(aud->data);
    al_destroy_audio_stream(aud->stream);
    al_fclose(aud->fp);
    free(aud);
}

ALLEGRO_AUDIO_STREAM *get_aud_stream(AUDSTREAM *aud)
{
    return aud->stream;
}

static bool aud_eof(AUDSTREAM *aud)
{
    int64_t pos = al_ftell(aud->fp);
    return (pos < 0 || pos >= aud->file_endpos);
}

static inline int get_next_chunk(AUDSTREAM *aud)
{
    unsigned short size;	/* size of compressed data */
    unsigned short outsize;	/* size of output data */
    unsigned long id;		/* always 0x0000DEAF */

    if (aud_eof(aud))
	return 0;

    size = al_fread16le(aud->fp);
    outsize = al_fread16le(aud->fp);
    id = al_fread32le(aud->fp);
    (void)outsize;
    (void)id;

    if (aud->data_size < size) {
	aud->data_size = size;
	aud->data = realloc(aud->data, size);
    }

    al_fread(aud->fp, aud->data, aud->data_size);
    aud->data_index = 0;
    aud->low_nibble = true;
    return aud->data_size;
}

static inline int get_next_delta(AUDSTREAM *aud)
{
    if (aud->low_nibble) {
	aud->low_nibble = false;
	return aud->data[aud->data_index] & 0xf;
    }
    else {
	aud->low_nibble = true;
	return (aud->data[aud->data_index++] >> 4) & 0xf;
    }
}

int poll_aud_stream(AUDSTREAM *aud)
{
    unsigned short *ostart;
    unsigned short *o;
    int step[2], delta, vpdiff;
    int count, i, ch = 0;

    ostart = al_get_audio_stream_fragment(aud->stream);
    if (ostart) {
	o = ostart;
	i = BUFFER_SIZE * (aud->stereo ? 2 : 1);

	/* Reached end of aud. */
	if (aud_eof(aud)) {
	    while (i--) {
		*o++ = aud->last_val;
	    }

	    al_set_audio_stream_fragment(aud->stream, ostart);
	    return 2;	/* finished */
	}

	/* Read data from file. */
	while (i) {
	    count = get_next_chunk(aud) << 1;
	    if (count == 0) {
		while (--i) {
		    *o++ = aud->last_val;
		}
	    }
	    else {
		while (count-- > 0) {
		    delta = get_next_delta(aud);

		    /* update step value */
		    step[ch] = step_table[aud->step_index[ch]];

		    /* compute difference and new predicted value */
		    vpdiff = step[ch] >> 3;
		    if (delta & 4)
			vpdiff += step[ch];
		    if (delta & 2)
			vpdiff += step[ch] >> 1;
		    if (delta & 1)
			vpdiff += step[ch] >> 2;

		    /* calculate new sample value */
		    if (delta & 0x8)
			aud->valpred[ch] -= vpdiff;
		    else
			aud->valpred[ch] += vpdiff;

		    /* clamp */
		    if (aud->valpred[ch] > 32767)
			aud->valpred[ch] = 32767;
		    else if (aud->valpred[ch] < -32768)
			aud->valpred[ch] = -32768;

		    /* find new index value */
		    aud->step_index[ch] += index_table[delta];
		    if (aud->step_index[ch] < 0)
			aud->step_index[ch] = 0;
		    else if (aud->step_index[ch] > 88)
			aud->step_index[ch] = 88;

		    /* output */
		    *o++ = aud->valpred[ch];
		    i--;

		    /* separate channel decoding */
		    if (aud->stereo)
			ch = !ch;
		}

		aud->last_val = aud->valpred[ch];
	    }
	}

	al_set_audio_stream_fragment(aud->stream, ostart);
	return 1;
    }

    return 0;
}

void restart_aud_stream(AUDSTREAM *aud)
{
    al_fseek(aud->fp, AUD_HEADER_LEN, ALLEGRO_SEEK_SET);

    /* Reset predictions and data position. */
    aud->step_index[0] = aud->valpred[0] = 0;
    aud->step_index[1] = aud->valpred[1] = 0;
}

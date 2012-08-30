/*  AUDlib, a .AUD sample format extension for Allegro.
 *  Copyright (C) 1998 Peter Wang.
 *  See README for more information.
 */

#include <stdio.h>
#include <string.h>
#include <allegro.h>
#include "audlib.h"

#define BUFFER_SIZE	8192	/* must be a multiple of 2048 */


/*
 *  Internal use only
 */

/* IMA-ADPCM compression / decompression tables */
static int index_table[16] = {
    -1, -1, -1, -1, 2, 4, 6, 8,
    -1, -1, -1, -1, 2, 4, 6, 8
};

static int step_table[89] = {
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

/* the current .aud */
static char *fn;
static PACKFILE *fp;
static AUDIOSTREAM *stream;
static int looping;

/* ``file inside a file'' */
static int file_offset, file_length, file_pos; 

/* the current chunk's data */
static unsigned char *data = NULL;
static int data_size = 0;
static int data_index, low_nibble;

/* decompression variables */
static int step_index[2], valpred[2];

/* error strings */
static char *err_no_file       = "Error opening file";
static char *err_ww_compressed = "Unsupported compression type";
static char *err_not_aud       = "Not valid AUD file";
static char *err_bad_stream    = "Error creating audio stream";

/* FIXME: I hate this.  Prevents the `cutoff' at the end of the buffer. */
static int ugly_hack = 0;


/*
 *  Exported globals
 */

int aud_stream_loaded = FALSE;
int aud_stream_paused = FALSE;
int aud_stream_freq;
int aud_stream_stereo;
int aud_stream_volume = 255;
const char *aud_stream_error = NULL;


/*
 *  Functions
 */

void unload_aud_stream(void)
{
    ugly_hack = 0;

    if (aud_stream_loaded) {
	free(fn);
	free(data);
	data = NULL;
	data_size = 0;
	stop_audio_stream(stream);
	aud_stream_loaded = FALSE;
	pack_fclose(fp);
    }
}

int load_aud_stream(const char *filename, int loop)
{
    return load_aud_stream_offset(filename, loop, 0);
}

int load_aud_stream_offset(const char *filename, int loop, int offset)
{
    unsigned short freq;	/* frequency */
    unsigned long size;		/* size of file (without header) */
    unsigned long outsize;	/* size of output data */
    unsigned char flags;	/* bit 0=stereo, 1=16bit */
    unsigned char type;		/* 1=Westwood compressed, 99=IMA ADPCM */

    unload_aud_stream();

    fp = pack_fopen(filename, "r");
    if (!fp) {
	aud_stream_error = err_no_file;
	return FALSE;
    }

    if (offset)		
    	pack_fseek(fp, offset);

    aud_stream_error = NULL;

    freq = pack_igetw(fp);
    size = pack_igetl(fp);
    outsize = pack_igetl(fp);
    flags = pack_getc(fp);
    type = pack_getc(fp);

    if (type == 1) {
	aud_stream_error = err_ww_compressed;
	pack_fclose(fp);
	return FALSE;
    }
    else if (type != 99) {
	aud_stream_error = err_not_aud;
	pack_fclose(fp);
	return FALSE;
    }

    aud_stream_freq = freq;
    aud_stream_stereo = (flags & 1) ? TRUE : FALSE;

    stream = play_audio_stream(BUFFER_SIZE, 16, aud_stream_stereo,
			       aud_stream_freq, aud_stream_volume, 128);
    if (!stream) {
	aud_stream_error = err_bad_stream;
	pack_fclose(fp);
	return FALSE;
    }

    step_index[0] = valpred[0] = 0;
    step_index[1] = valpred[1] = 0;

    fn = strdup(filename);
    file_offset = offset + 12;	/* skip header */
    file_length = size;
    file_pos = 0;
    looping = loop;

    aud_stream_loaded = TRUE;
    aud_stream_paused = FALSE;
    return TRUE;
}

void set_aud_stream_volume(int vol)
{
    aud_stream_volume = MID(0, vol, 255);
    if (aud_stream_loaded) 
	voice_set_volume(stream->voice, vol);
}

static inline int get_next_chunk()
{
    unsigned short size;	/* size of compressed data */
    unsigned short outsize;	/* size of output data */
    unsigned long id;		/* always 0x0000DEAF */

    if (!fp || (file_pos >= file_length))
	return 0;

    size = pack_igetw(fp);
    outsize = pack_igetw(fp);
    id = pack_igetl(fp);
    file_pos += 8;

#if 0
    if (id != 0x0000deaf)
	exit(1);
#endif

    if (data_size < size) {
	data_size = size;
	if (data)
	    data = realloc(data, size);
	else
	    data = malloc(size);
    }

    pack_fread(data, data_size, fp);
    data_index = 0;
    low_nibble = TRUE;

    file_pos += data_size; 

    return data_size;
}

static inline int get_next_delta()
{
    if (low_nibble == TRUE) {
	low_nibble = FALSE;
	return data[data_index] & 0xf;
    }
    else {
	low_nibble = TRUE;
	return (data[data_index++] >> 4) & 0xf;
    }
}

void restart_aud_stream(void)
{
    if (aud_stream_loaded) {
	/* Packfile routines do not support seeking backwards. */
 	pack_fclose(fp);
 	fp = pack_fopen(fn,"r");
 	pack_fseek(fp, file_offset);

	/* Reset predictions and data position. */
 	step_index[0] = valpred[0] = 0;
 	step_index[1] = valpred[1] = 0;
 	file_pos = 0;
    }
}

int poll_aud_stream(void)
{
    unsigned short *o;
    int step[2], delta, vpdiff;
    int count, i, ch = 0;
    static int ugly_hack_last_value = 0;

    if (!aud_stream_loaded)
	return -1;

    o = get_audio_stream_buffer(stream);
    if (o) {
	i = BUFFER_SIZE << (aud_stream_stereo ? 1 : 0);

	/* Paused, so play silence. */
	if (aud_stream_paused) {
	    while (i--)
		*o++ = ugly_hack_last_value;

	    free_audio_stream_buffer(stream);
	    return 1;
	}

	/* Reached end of aud. */
 	if (file_pos >= file_length) {
    	    if (!ugly_hack) {
		ugly_hack = 1;

		while (i--)
		    *o++ = ugly_hack_last_value;

		free_audio_stream_buffer(stream);
		return 1;
	    }
	    else {
		ugly_hack = 0;

		if (!looping) {
		    unload_aud_stream();
		    return -1;
		}

		restart_aud_stream();
	    }
	}

	/* Read data from file. */
	while (i) {
	    count = get_next_chunk() << 1;
	    if (count == 0)
		while (--i)
		    *o++ = ugly_hack_last_value;
	    else {
		while (count-- > 0) {
		    delta = get_next_delta();

		    /* update step value */
		    step[ch] = step_table[step_index[ch]];

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
			valpred[ch] -= vpdiff;
		    else
			valpred[ch] += vpdiff;

		    /* clamp */
		    if (valpred[ch] > 32767)
			valpred[ch] = 32767;
		    else if (valpred[ch] < -32768)
			valpred[ch] = -32768;

		    /* find new index value */
		    step_index[ch] += index_table[delta];
		    if (step_index[ch] < 0)
			step_index[ch] = 0;
		    else if (step_index[ch] > 88)
			step_index[ch] = 88;

		    /* output */
		    *o++ = valpred[ch] ^ 0x8000;
		    i--;

		    /* separate channel decoding */
		    if (aud_stream_stereo)
			ch = !ch;
		}

		ugly_hack_last_value = valpred[ch] ^ 0x8000;
	    }
	}

	free_audio_stream_buffer(stream);
	return 1;
    }
    else
	return 0;
}

void pause_aud_stream(void)
{
    aud_stream_paused = TRUE;
}

void resume_aud_stream(void)
{
    aud_stream_paused = FALSE;
}

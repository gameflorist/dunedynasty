/* Allegro libmad glue
 * Peter Wang <tjaden@users.sourceforge.net>
 */

#include <mad.h>
#include <allegro5/allegro.h>
#include <allegro5/allegro_audio.h>
#include "allegro_mad.h"

#define NUMFRAGS            3
#define FRAGLEN             2048
#define INPUT_BUFFER_SIZE   32768

typedef float spl_t;

struct MP3 {
    struct mad_stream mstream;
    struct mad_frame mframe;
    struct mad_synth msynth;
    ALLEGRO_FILE *fp;
    int64_t file_size;
    bool astereo;
    ALLEGRO_AUDIO_STREAM *astream;
    unsigned char inp[INPUT_BUFFER_SIZE + MAD_BUFFER_GUARD];
    unsigned char *guard_ptr;
    spl_t spill[FRAGLEN * 2];   /* stereo */
    ssize_t spill_len;
    bool unrecoverable_error;
    int done;
};

/* Hopefully optimised by the compiler. */
#define mad_to_spl(x)   ((spl_t) mad_f_todouble(x))

static bool do_read_more(MP3 *mp3)
{
    size_t prev_remaining;
    unsigned char *read_start;
    size_t read_size;

    if (mp3->mstream.next_frame != NULL) {
        prev_remaining = mp3->mstream.bufend - mp3->mstream.next_frame;
        memmove(mp3->inp, mp3->mstream.next_frame, prev_remaining);
        read_start = mp3->inp + prev_remaining;
        read_size = INPUT_BUFFER_SIZE - prev_remaining;
    }
    else {
        read_size = INPUT_BUFFER_SIZE;
        read_start = mp3->inp;
        prev_remaining = 0;
    }

    mp3->guard_ptr = NULL;

    read_size = al_fread(mp3->fp, read_start, read_size);
    if (read_size <= 0) {
        if (al_ferror(mp3->fp)) {
            mp3->unrecoverable_error = true;
        }
        return false;
    }

    /* To decode the last frame of a file, it must be followed by
     * MAD_BUFFER_GUARD zero bytes.  al_feof() and feof() both indicate
     * if we have read past the end of the file, whereas we need to know
     * if we have reached the end of the file.
     */
    if (al_feof(mp3->fp) || al_ftell(mp3->fp) == mp3->file_size) {
        mp3->guard_ptr = read_start + read_size;
        memset(mp3->guard_ptr, 0, MAD_BUFFER_GUARD);
        read_size += MAD_BUFFER_GUARD;
    }

    mad_stream_buffer(&mp3->mstream, mp3->inp, read_size + prev_remaining);
    mp3->mstream.error = 0;
    return true;
}

static bool maybe_read_more(MP3 *mp3)
{
    if (mp3->unrecoverable_error) {
        return false;
    }
    if (mp3->mstream.buffer == NULL ||
        mp3->mstream.error == MAD_ERROR_BUFLEN)
    {
        return do_read_more(mp3);
    }
    return true;
}

static bool decode_frame(MP3 *mp3)
{
    if (mad_frame_decode(&mp3->mframe, &mp3->mstream) == 0) {
        return true;
    }
    if (MAD_RECOVERABLE(mp3->mstream.error)) {
        return false;
    }
    if (mp3->mstream.error == MAD_ERROR_BUFLEN) {
        return false;
    }
    mp3->unrecoverable_error = true;
    return false;
}

MP3 *load_mp3(const char *filename)
{
    MP3 *mp3;
    int srate;
    ALLEGRO_CHANNEL_CONF chanconf;

    mp3 = calloc(1, sizeof(MP3));
    if (!mp3) {
        return NULL;
    }

    mp3->fp = al_fopen(filename, "rb");
    if (!mp3->fp) {
        free(mp3);
        return NULL;
    }

    /* al_fsize() may return -1 if the file size cannot be determined.
     * In that case the last frame in the file might not be decoded.
     */
    mp3->file_size = al_fsize(mp3->fp);

    mad_stream_init(&mp3->mstream);
    mad_frame_init(&mp3->mframe);
    mad_synth_init(&mp3->msynth);

    while (maybe_read_more(mp3)) {
        if (decode_frame(mp3))
            break;
    }

    srate = mp3->mframe.header.samplerate;
    if (srate == 0) {
        unload_mp3(mp3);
        return NULL;
    }

    if (mp3->mframe.header.mode == MAD_MODE_SINGLE_CHANNEL) {
        mp3->astereo = false;
        chanconf = ALLEGRO_CHANNEL_CONF_1;
    } else {
        mp3->astereo = true;
        chanconf = ALLEGRO_CHANNEL_CONF_2;
    }

    mp3->astream = al_create_audio_stream(NUMFRAGS, FRAGLEN,
        srate, ALLEGRO_AUDIO_DEPTH_FLOAT32, chanconf);
    if (!mp3->astream) {
        unload_mp3(mp3);
        return NULL;
    }

    return mp3;
}

void unload_mp3(MP3 *mp3)
{
    if (!mp3) {
        return;
    }
    if (mp3->astream) {
        al_destroy_audio_stream(mp3->astream);
    }
    if (mp3->fp) {
        al_fclose(mp3->fp);
    }
    mad_synth_finish(&mp3->msynth);
    mad_frame_finish(&mp3->mframe);
    mad_stream_finish(&mp3->mstream);
    free(mp3);
}

ALLEGRO_AUDIO_STREAM *get_mp3_stream(MP3 *mp3)
{
    return mp3->astream;
}

static spl_t *take_spill(MP3 *mp3, spl_t *out, spl_t *out_end)
{
    const ssize_t max = (out_end - out);
    ssize_t n;
    ssize_t remain;

    if (max < mp3->spill_len) {
        n = max;
        remain = 0;
    } else {
        n = mp3->spill_len;
        remain = mp3->spill_len - n;
    }

    memcpy(out, mp3->spill, n * sizeof(spl_t));
    memmove(mp3->spill, mp3->spill + n, remain * sizeof(spl_t));

    mp3->spill_len -= n;
    return out + n;
}

static inline void push_spill(MP3 *mp3, spl_t spl)
{
    assert(mp3->spill_len < FRAGLEN*2);
    mp3->spill[mp3->spill_len++] = spl;
}

static spl_t *push_synth_pcm(MP3 *mp3, spl_t *out, spl_t *out_end)
{
    const bool mstereo = (MAD_NCHANNELS(&mp3->mframe.header) > 1);
    int i;

    if (mstereo) {
        for (i = 0; i < mp3->msynth.pcm.length && out < out_end; i++) {
            *out++ = mad_to_spl(mp3->msynth.pcm.samples[0][i]);
            *out++ = mad_to_spl(mp3->msynth.pcm.samples[1][i]);
        }
        for (; i < mp3->msynth.pcm.length; i++) {
            push_spill(mp3, mad_to_spl(mp3->msynth.pcm.samples[0][i]));
            push_spill(mp3, mad_to_spl(mp3->msynth.pcm.samples[1][i]));
        }
    } else {
        for (i = 0; i < mp3->msynth.pcm.length && out < out_end; i++) {
            spl_t c = mad_to_spl(mp3->msynth.pcm.samples[0][i]);
            *out++ = c;
            *out++ = c;
        }
        for (; i < mp3->msynth.pcm.length; i++) {
            spl_t c = mad_to_spl(mp3->msynth.pcm.samples[0][i]);
            push_spill(mp3, c);
            push_spill(mp3, c);
        }
    }

    return out;
}

static spl_t *refill(MP3 *mp3, spl_t *out, spl_t *out_end)
{
    while (out < out_end) {
        if (!maybe_read_more(mp3))
            break;
        if (decode_frame(mp3)) {
            mad_synth_frame(&mp3->msynth, &mp3->mframe);
            out = push_synth_pcm(mp3, out, out_end);
        }
    }

    return out;
}

static void fill_silence(spl_t *out, spl_t *out_end)
{
    for (; out < out_end; out++) {
        *out = 0.0f;
    }
}

int poll_mp3(MP3 *mp3)
{
    spl_t *out_start;
    spl_t *out_end;
    spl_t *out;

    out_start = al_get_audio_stream_fragment(mp3->astream);
    if (!out_start) {
        return 0;
    }
    out_end = out_start + FRAGLEN * (mp3->astereo ? 2 : 1);

    out = take_spill(mp3, out_start, out_end);
    if (out < out_end) {
        out = refill(mp3, out, out_end);
    }
    if (out == out_start) {
        if (mp3->done <= NUMFRAGS)
            mp3->done++;
    }
    if (out < out_end) {
        fill_silence(out, out_end);
    }
    al_set_audio_stream_fragment(mp3->astream, out_start);

    if (mp3->done > NUMFRAGS) {
        return 2; /* finished */
    }

    return 1;
}

#ifdef TEST
int main(int argc, const char *argv[])
{
    MP3 *mp3;
    ALLEGRO_EVENT_QUEUE *queue;
    ALLEGRO_DISPLAY *dpy;

    if (!al_init()) {
        return 1;
    }
    if (!al_install_audio()) {
        return 1;
    }
    if (!al_reserve_samples(1)) {
        return 1;
    }
    dpy = al_create_display(320, 200);
    if (!dpy) {
        return 1;
    }

    mp3 = load_mp3(argv[1]);
    if (!mp3) {
        return 1;
    }

    al_attach_audio_stream_to_mixer(get_mp3_stream(mp3),
        al_get_default_mixer());

    queue = al_create_event_queue();
    al_register_event_source(queue,
        al_get_audio_stream_event_source(get_mp3_stream(mp3)));
    al_register_event_source(queue, al_get_display_event_source(dpy));

    for (;;) {
        ALLEGRO_EVENT ev;
        al_wait_for_event(queue, &ev);
        if (ev.type == ALLEGRO_EVENT_AUDIO_STREAM_FRAGMENT) {
            if (poll_mp3(mp3) > 1)
                break;
        }
        if (ev.type == ALLEGRO_EVENT_DISPLAY_CLOSE) {
            break;
        }
    }

    unload_mp3(mp3);

    return 0;
}
#endif /* TEST */

/* vim: set sts=4 sw=4 et: */

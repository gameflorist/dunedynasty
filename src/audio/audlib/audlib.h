/* This file is part of the AUDlib package, see README for details. */
#ifndef __AUDlib_H
#define __AUDlib_H

#ifdef __cplusplus
extern "C" {
#endif

    
#define AUDLIB_VERSION      0
#define AUDLIB_SUBVERSION   90

    
extern int aud_stream_loaded;
extern int aud_stream_freq;
extern int aud_stream_stereo;
extern int aud_stream_volume;
extern const char *aud_stream_error;

    
int  load_aud_stream(const char *filename, int loop);
int  load_aud_stream_offset(const char *filename, int loop, int offset);
void unload_aud_stream(void);
void set_aud_stream_volume(int vol);
int  poll_aud_stream(void);
void pause_aud_stream(void);
void resume_aud_stream(void);
void restart_aud_stream(void);


#ifdef __cplusplus
}
#endif

#endif

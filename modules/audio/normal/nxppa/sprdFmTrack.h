#ifndef __SPRD_FM_TRACK_H__
#define __SPRD_FM_TRACK_H__
#include <errno.h>
#include <pthread.h>
#include <stdio.h>
#include <utils/Log.h>
#include <tinyalsa/asoundlib.h>
#include <sys/mman.h>
#include <semaphore.h>
#include <fcntl.h>
#include <stdlib.h>
#include <linux/types.h>
#include <sys/ioctl.h>


typedef void*  fmtrackhandle;

extern fmtrackhandle  FmTrack_Open(int sampleRate, int MiniBufferSize,int channels);
extern int FmTrack_Start(fmtrackhandle FmTrack_handle);
extern int FmTrack_Write(fmtrackhandle FmTrack_handle, const void* buffer, int userSize, int blocking);
extern int FmTrack_Close(fmtrackhandle FmTrack_handle);

#endif


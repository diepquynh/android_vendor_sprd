#define LOG_TAG "sprdfmtrack"
#include <string.h>
#include <stdlib.h>
#include <stdio.h>
#include <utils/Log.h>

#include <media/AudioSystem.h>
#include <media/AudioTrack.h>
#include <system/audio.h>
#include <system/audio_policy.h>

#include "sprdFmTrack.h"

namespace android {

typedef struct t_fmTrack
{
    char flagStart;
    sp<AudioTrack> pAudioTrack;
    pthread_mutex_t TrackLock;
}T_FM_TRACK;

extern "C"
{
    fmtrackhandle  FmTrack_Open(int sampleRate, int MiniBufferSize, int channels);
    int FmTrack_Start(fmtrackhandle FmTrack_handle);
    int FmTrack_Write(fmtrackhandle FmTrack_handle, const void* buffer, int userSize, int blocking);
    int FmTrack_Close(fmtrackhandle FmTrack_handle);
}

fmtrackhandle  FmTrack_Open(int sampleRate, int MiniBufferSize, int channels)
{

    int status;
    audio_devices_t           audDevice;
    audio_stream_type_t       streamType;
    audio_output_flags_t    flags;
    ssize_t status2;
    T_FM_TRACK *ptFmTrackInfo=NULL;
    ptFmTrackInfo = (T_FM_TRACK *)malloc(sizeof(T_FM_TRACK ));
    if(!ptFmTrackInfo) {
        return NULL;
    }
    else {
        memset(ptFmTrackInfo, 0, sizeof(T_FM_TRACK ));
    }
    flags       = AUDIO_OUTPUT_FLAG_NONE;
    audDevice   = AUDIO_DEVICE_OUT_SPEAKER;
    streamType  = AUDIO_STREAM_MUSIC;
    ALOGE("FmTrack_Open in");
    pthread_mutex_init(&ptFmTrackInfo->TrackLock, NULL);

    if(channels == 2 ) {
        channels = AUDIO_CHANNEL_OUT_STEREO;
    }
    else if (channels == 1) {
        channels = AUDIO_CHANNEL_OUT_MONO;
    }
    ptFmTrackInfo->pAudioTrack = new AudioTrack(streamType, sampleRate, AUDIO_FORMAT_PCM_16_BIT, channels,
            (size_t)MiniBufferSize, flags, NULL, NULL,0,AUDIO_SESSION_ALLOCATE,android::AudioTrack::TRANSFER_DEFAULT,NULL,-1,-1,NULL);
    if(ptFmTrackInfo->pAudioTrack == NULL)
    {
        ALOGE("warning:set error: status = %d", status);
        free(ptFmTrackInfo);
        goto ERROR;
    }
    ALOGE("FmTrack_Open out");
    return (fmtrackhandle)ptFmTrackInfo;
ERROR:
    ALOGE("FmTrack_Open out error");
    return NULL;
}

int FmTrack_Start(fmtrackhandle FmTrack_handle)
{
    int status = 0;
    T_FM_TRACK *ptFmTrackInfo = NULL;
    ALOGE("FmTrack_Start in");
    ptFmTrackInfo = (T_FM_TRACK*)FmTrack_handle;
    pthread_mutex_lock(&ptFmTrackInfo->TrackLock);
    if(ptFmTrackInfo->flagStart == 1) {
        return 0;
    }
    status = ptFmTrackInfo->pAudioTrack->start();
    if(0 != status)
    {
        ALOGE("warning:start error: status = %d", status);
        return status;
    }
    ptFmTrackInfo->flagStart = 1;
    pthread_mutex_unlock(&ptFmTrackInfo->TrackLock);
    ALOGE("FmTrack_Start out");
    return 0;
}

int FmTrack_Write(fmtrackhandle FmTrack_handle, const void* buffer, int userSize, int blocking)
{
    int status;
    int toWrite;
    T_FM_TRACK *ptFmTrackInfo = NULL;
    ptFmTrackInfo = (T_FM_TRACK *)FmTrack_handle;
    pthread_mutex_lock(&ptFmTrackInfo->TrackLock);
    ALOGE("FmTrack_Write in");
    status = ptFmTrackInfo->pAudioTrack->write(buffer, userSize, (bool)blocking );
    ALOGE("FmTrack_Write out");
    if(status < 0)
    {
        ALOGE("Obtain buffer status_t =%d", status);
        pthread_mutex_unlock(&ptFmTrackInfo->TrackLock);
        return status;
    }
    pthread_mutex_unlock(&ptFmTrackInfo->TrackLock);
    return 0;
}

int FmTrack_Close(fmtrackhandle FmTrack_handle)
{
    T_FM_TRACK *ptFmTrackInfo = NULL;
    ptFmTrackInfo = (T_FM_TRACK *)FmTrack_handle;
    ALOGE("FmTrack_Close in");
    pthread_mutex_lock(&ptFmTrackInfo->TrackLock);
    if ( ptFmTrackInfo->pAudioTrack != NULL)
    {
        ALOGE("FmTrack_Close in 2");
        ptFmTrackInfo->pAudioTrack->stop();
        ALOGE("FmTrack_Close in 3");
        ptFmTrackInfo->pAudioTrack.clear();
        ptFmTrackInfo->pAudioTrack = NULL;
        ALOGE("FmTrack_Close in 4");
    }
    ALOGE("FmTrack_Close out");
    free(ptFmTrackInfo);
    return 0;
}
}

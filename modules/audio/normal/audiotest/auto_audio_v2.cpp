//
// Spreadtrum Auto Tester
//
#include <sys/select.h>
#include <fcntl.h>
#include <pthread.h>
#include <stdlib.h>
#include <pthread.h>
#include <stdint.h>

#include <fcntl.h>
#include <media/AudioRecord.h>
#include <media/AudioSystem.h>
#include <media/AudioTrack.h>
#include <media/mediarecorder.h>
#include <system/audio.h>
#include <system/audio_policy.h>


//#define LOG_NDEBUG 0
#define LOG_TAG "SPRD_AUDIOTRACK"

#include <math.h>
#include <utils/Log.h>
#include <cutils/properties.h>
#include "media/ToneGenerator.h"
#include <media/AudioSystem.h>
#include <media/IAudioFlinger.h>
#include <media/IAudioPolicyService.h>

#include <system/audio.h>
#include "auto_audio.h"
#include <cutils/str_parms.h>
//namespace sci_aud {
namespace android {
using namespace android;

sp<AudioRecord> sRcrder ;
sp<AudioTrack> sPlayer;
int pipe_file=-1;
//static  AudioTrack    * sPlayer     = NULL;
//static  AudioRecord   * sRcrder     = NULL;
static  int             sPlaySync   = 1;
static  size_t          sPlayBufLen = -1;
static  int             sRcrdBufLen = -1;
static void     playerCallback(int event, void* user, void *info);
static status_t setDeviceStateIsConnection( audio_devices_t device );

int audPlayerOpen( int outdev, int sampleRate, int stereo, int sync )
{
    if( sPlayer != NULL ) {
        ALOGW("already opend!");
        return 0;
    }

    AudioSystem::clearAudioConfigCache();

    status_t                  status;
    audio_devices_t           audDevice;
    audio_stream_type_t       streamType;
    audio_policy_force_use_t  forceUsage;
    audio_policy_forced_cfg_t forceConfig;
    audio_output_flags_t    flags;
    if( AUD_OUTDEV_SPEAKER == outdev ) {
        audDevice  = AUDIO_DEVICE_OUT_SPEAKER;
        //streamType = AUDIO_STREAM_SYSTEM;
        streamType = AUDIO_STREAM_MUSIC;
        forceUsage = AUDIO_POLICY_FORCE_FOR_MEDIA;
        forceConfig= AUDIO_POLICY_FORCE_SPEAKER;
        status = AudioSystem::setPhoneState(AUDIO_MODE_NORMAL);
    } else if( AUD_OUTDEV_EARPIECE == outdev ) {
        audDevice  = AUDIO_DEVICE_OUT_EARPIECE;
        streamType = AUDIO_STREAM_MUSIC;
        //streamType = AUDIO_STREAM_VOICE_CALL;
        forceUsage = AUDIO_POLICY_FORCE_FOR_COMMUNICATION;
        forceConfig= AUDIO_POLICY_FORCE_NONE;
        status = AudioSystem::setPhoneState(AUDIO_MODE_IN_COMMUNICATION);//
    } else {
        audDevice  = AUDIO_DEVICE_OUT_WIRED_HEADPHONE;
        streamType = AUDIO_STREAM_SYSTEM;
        forceUsage = AUDIO_POLICY_FORCE_FOR_MEDIA;
        forceConfig= AUDIO_POLICY_FORCE_NONE;
    }
    flags = AUDIO_OUTPUT_FLAG_NONE;

    status = setDeviceStateIsConnection(audDevice);
    ALOGI("setDeviceStateIsConnection: status = %d\n", status);

    status = AudioSystem::setForceUse(forceUsage, forceConfig);
    ALOGI("setForceUse: status = %d\n", status);

    sPlayer = new AudioTrack();
    if( sPlayer == NULL ) {
        ALOGE("new fail!\n");
        return -2;
    }

    sPlaySync = sync;


    status = AudioTrack::getMinFrameCount(&sPlayBufLen, streamType, sampleRate);
    if( status != NO_ERROR ) {
        ALOGE("getMinFrameCount error: status = %d\n", status);
        return -1; //
    }
    ALOGI("default min frame count = %d\n", sPlayBufLen);
    if( sPlayBufLen < AUD_PLAY_FRAME_BUF ) {
        sPlayBufLen = AUD_PLAY_FRAME_BUF;
    }

    AudioTrack::callback_t cbt = !sync ? playerCallback : NULL;
    void*                 usr = !sync ? sPlayer.get() : NULL;

    int chls = stereo ? AUDIO_CHANNEL_OUT_STEREO : AUDIO_CHANNEL_OUT_MONO;
    status = sPlayer->set(streamType, sampleRate, AUD_PLAY_PCM, chls,
            sPlayBufLen, flags, cbt, usr);
    if( status != NO_ERROR ){
        ALOGE("set error: status = %d\n", status);
        //delete sPlayer;
        sPlayer = NULL;
        return -3;
    }

    FUN_EXIT;
    return 0;
}

static uchar * s_data     = NULL;
static uint    s_data_len = 0;
static uint    s_data_pos = 0;

int audPlayerPlay( const uchar * data, int size )
{
    FUN_ENTER;
    #if 0
    if(NULL==sPlayer){
        return -1;
    }
    #endif
    if( sPlaySync ) {
        ALOGE("audPlayerPlay sPlaySync is true");
        sPlayer->start();

        while( size > 0 ) {
            AudioTrack::Buffer buffer;
            buffer.frameCount = (sPlayer->format() == AUDIO_FORMAT_PCM_16_BIT
) ? (size >> 1) : size;

            status_t status = sPlayer->obtainBuffer(&buffer, 1);
            if (status == NO_ERROR) {
                ALOGI("obtained buffer size = %d\n", buffer.size);
                memcpy(buffer.i8, data, buffer.size);
                data += buffer.size;
                size -= buffer.size;
                sPlayer->releaseBuffer(&buffer);
            } else if (status != TIMED_OUT && status != WOULD_BLOCK) {
                ALOGW("cannot write to AudioTrack: size = %d\n", size);
                return -1;
            }
        }
    } else {
        ALOGI("audPlayerPlay sPlaySync is false");
        while( !sPlayer->stopped() ) {
            sPlayer->stop();
            usleep(100 * 1000);
        }

        if( NULL != s_data ) {
            delete s_data;
        }
        s_data = new uchar[size];
        if( NULL == s_data ) {
            return -2;
        }

        memcpy(s_data, data, size);
        s_data_len = size;
        s_data_pos = 0;
        sPlayer->start();
    }

    FUN_EXIT;
    return 0;
}

void playerCallback( int event, void* user, void *info )
{
    if( AudioTrack::EVENT_MORE_DATA == event && NULL != s_data ) {
        AudioTrack::Buffer * buf = (AudioTrack::Buffer *)info;
        uint room = s_data_len - s_data_pos;
        if( room >= buf->size ) {
            memcpy(buf->i8, s_data + s_data_pos, buf->size);
            s_data_pos += buf->size;
        } else {
            memcpy(buf->i8, s_data + s_data_pos, room);
            s_data_pos = 0;
            buf->size  = room;
        }
        if( s_data_pos >= s_data_len ) {
            s_data_pos = 0;
        }
    }
}

int audPlayerStop( void )
{
    FUN_ENTER;
    if( sPlayer != NULL ) {
        sPlayer->stop();
    }
    FUN_EXIT;
    return 0;
}


int audPlayerClose( void )
{
    FUN_ENTER;
    if( sPlayer != NULL ) {
        while( !sPlayer->stopped() ) {
            sPlayer->stop();
            usleep(100 * 1000);
        }
        //delete sPlayer;
        sPlayer->flush();
        sPlayer.clear();
        sPlayer = NULL;
    }
    if( NULL != s_data ) {
        delete s_data;
        s_data = NULL;
    }
    sPlayBufLen = -1;
    FUN_EXIT;
    return 0;
}

status_t setDeviceStateIsConnection( audio_devices_t device )
{
    static audio_devices_t arrOut[] = {
        //AUDIO_DEVICE_OUT_EARPIECE,
        //AUDIO_DEVICE_OUT_SPEAKER,
        AUDIO_DEVICE_OUT_WIRED_HEADSET,
        AUDIO_DEVICE_OUT_WIRED_HEADPHONE,
        AUDIO_DEVICE_OUT_FM_HEADSET,
        AUDIO_DEVICE_OUT_FM_SPEAKER,
    };

    for( uint i = 0; i < sizeof(arrOut) / sizeof(arrOut[0]); ++i ) {
        if( arrOut[i] != device ) {
            AudioSystem::setDeviceConnectionState( arrOut[i],
            AUDIO_POLICY_DEVICE_STATE_UNAVAILABLE, "", "" );
        }
    }

    return AudioSystem::setDeviceConnectionState( device,
    AUDIO_POLICY_DEVICE_STATE_AVAILABLE, "", "");
}

int audRcrderRecord( uchar * data, int size )
{
    FUN_ENTER;
    int retunnum = 0;
    retunnum = sRcrder->read(data,size);
    ALOGE("retunnum = %d",retunnum);
    if(retunnum > 0)
        return 0;
    else
        return -1;
    FUN_EXIT;
    return 0;
}

int audRcrderOpen( int indev, int sampleRate )
{
    FUN_ENTER;
    if( sRcrder !=  NULL ) {
        ALOGW("already opened!\n");
        return 0;
    }

    if( sRcrdBufLen < AUD_RCRD_FRAME_BUF ) {
        sRcrdBufLen = AUD_RCRD_FRAME_BUF;
    }

    String16 opPackageName = String16();
    sRcrder = new AudioRecord(opPackageName);
    if( sRcrder == NULL ) {
        ALOGW("new fail!\n");
        return -2;
    }

    pipe_file=-1;

    status_t status;
    audio_source_t input;

    if (indev == AUD_INDEV_BACK_MIC){
        input = AUDIO_SOURCE_CAMCORDER;
        status = AudioSystem::setDeviceConnectionState(
            AUDIO_DEVICE_IN_BACK_MIC,
            AUDIO_POLICY_DEVICE_STATE_AVAILABLE, "", "");
            ALOGE("wangzuo:indev == AUD_INDEV_BACK_MIC");
    } else if (indev == AUD_INDEV_HEADSET_MIC){
        input = AUDIO_SOURCE_MIC;
        status = AudioSystem::setDeviceConnectionState(
            AUDIO_DEVICE_IN_WIRED_HEADSET,
            AUDIO_POLICY_DEVICE_STATE_AVAILABLE, "", "");
        ALOGE("wangzuo:indev == AUD_INDEV_HEADSET_MIC");
    } else if (indev == AUD_INDEV_BUILTIN_MIC ){
        input = AUDIO_SOURCE_MIC;
        ALOGE("wangzuo:indev == AUD_INDEV_BUILTIN_MIC");
    } else {
        ALOGE("wangzuo receive command error");
        return -2;
    }

    status = sRcrder->set(input, sampleRate, AUD_RCRD_PCM, AUD_RCRD_CHL,
            sRcrdBufLen);
    if( status != NO_ERROR ) {
        ALOGE("set error: status = %d\n", status);
        sRcrder.clear();
        return -3;
    }

    //if( sRcrder->stopped() ) {
        sRcrder->start();
    //}

    {
        /* clear data */
        uchar tmp[AUD_RCRD_FRAME_BUF];
        audRcrderRecord(tmp, AUD_RCRD_FRAME_BUF);
        audRcrderRecord(tmp, AUD_RCRD_FRAME_BUF);
        audRcrderRecord(tmp, AUD_RCRD_FRAME_BUF);
        audRcrderRecord(tmp, AUD_RCRD_FRAME_BUF);
    }

    FUN_EXIT;
    return 0;
}

int audRcrderRead(int size){
    FUN_ENTER;
    int fd=-1;
    int ret=-1;

    if(size<=0){
        size=AUD_RCRD_FRAME_BUF;
    }

    if( sRcrder ==  NULL ) {
        ALOGW("not opened!\n");
        return 0;
    }

    if(pipe_file<0){
        ALOGD("audRcrderOpen start open");
        pipe_file=open(MMI_DEFAULT_PCM_FILE, O_WRONLY);
        ALOGD("audRcrderOpen open end");
        if(pipe_file< 0) {
            ALOGE("audRcrderOpen open:%s failed",AUD_RCRD_FRAME_BUF);
            return -1;
        }
    }

    {
        uchar *tmp=NULL;
        uchar *data_tmp=NULL;
        int to_write=size;
        tmp = (uchar *)malloc(size);
        if(tmp==NULL){
            ALOGE("audRcrderRead malloc failed");
            return -1;
        }

        data_tmp=tmp;

        ret=audRcrderRecord(tmp, size);
        if(ret<0){
            ALOGE("audRcrderRead audRcrderRecord failed");
            free(tmp);
            return -1;
        }

        while(to_write) {
            ret = write(pipe_file,data_tmp,to_write);
            if(ret <= 0) {
                usleep(10000);
                continue;
            }
            if(ret < to_write) {
                usleep(10000);
            }
            to_write -= ret;
            data_tmp += ret;
            ALOGD("audRcrderRead num_write:0x%x err:%s", ret,strerror(errno));
        }

        free(tmp);
    }

    FUN_EXIT;
    return ret;
}


int audRcrderStop( void )
{
    FUN_ENTER;
    if( sRcrder != NULL ) {
        sRcrder->stop();
    }
    FUN_EXIT;
    return 0;
}

int audRcrderClose( void )
{
    FUN_ENTER;
    if( sRcrder != NULL ) {
        sRcrder->stop();
        //delete sRcrder;
        //sRcrder = NULL;
        sRcrder.clear();
    }

    if(pipe_file>0){
        close(pipe_file);
        pipe_file=-1;
    }
    FUN_EXIT;
    return 0;
}

unsigned char headsetPlugState( void )
{
    FUN_ENTER;
    int fd = open(HEADSET_STATE, O_RDONLY);

    if( fd < 0 ) {
        ALOGE("open '%s' error: %s\n", HEADSET_STATE, strerror(errno));
        return fd;
    }

    char buf[64];
    buf[0] = 0;
    read(fd, buf, sizeof buf);
    ALOGW("headset state = %s\n", buf);
    unsigned char plugState = (unsigned char)atoi(buf);
    close(fd);

    FUN_EXIT;
    return plugState;
}

//} // namespace

static const uchar s_pcmdata_mono[] = {
    0x92,0x02,0xcb,0x0b,0xd0,0x14,0x1d,0x1d,0xfc,0x24,0x17,0x2c,0x4a,0x32,0x69,0x37,
    0x92,0x3b,0x4e,0x3e,0x22,0x40,0x56,0x40,0x92,0x3f,0x12,0x3d,0x88,0x39,0x10,0x35,
    0xf0,0x2e,0x51,0x28,0xce,0x20,0x7f,0x18,0xd5,0x0f,0xda,0x06,0xdf,0xfd,0xa4,0xf4,
    0xa2,0xeb,0x39,0xe3,0x57,0xdb,0x3d,0xd4,0x1f,0xce,0xe2,0xc8,0xb1,0xc4,0xc0,0xc1,
    0xec,0xbf,0xc1,0xbf,0xa4,0xc0,0xf2,0xc2,0x18,0xc6,0xc2,0xca,0xc8,0xd0,0x36,0xd7,
    0xbb,0xde,0xe6,0xe6,0xa5,0xef,0xa6,0xf8,
};

static const uchar s_pcmdata_left[] = {
    0x92,0x02,0x00,0x00,0xCB,0x0B,0x00,0x00,0xD0,0x14,0x00,0x00,0x1D,0x1D,0x00,0x00,
    0xFC,0x24,0x00,0x00,0x17,0x2C,0x00,0x00,0x4A,0x32,0x00,0x00,0x69,0x37,0x00,0x00,
    0x92,0x3B,0x00,0x00,0x4E,0x3E,0x00,0x00,0x22,0x40,0x00,0x00,0x56,0x40,0x00,0x00,
    0x92,0x3F,0x00,0x00,0x12,0x3D,0x00,0x00,0x88,0x39,0x00,0x00,0x10,0x35,0x00,0x00,
    0xF0,0x2E,0x00,0x00,0x51,0x28,0x00,0x00,0xCE,0x20,0x00,0x00,0x7F,0x18,0x00,0x00,
    0xD5,0x0F,0x00,0x00,0xDA,0x06,0x00,0x00,0xDF,0xFD,0x00,0x00,0xA4,0xF4,0x00,0x00,
    0xA2,0xEB,0x00,0x00,0x39,0xE3,0x00,0x00,0x57,0xDB,0x00,0x00,0x3D,0xD4,0x00,0x00,
    0x1F,0xCE,0x00,0x00,0xE2,0xC8,0x00,0x00,0xB1,0xC4,0x00,0x00,0xC0,0xC1,0x00,0x00,
    0xEC,0xBF,0x00,0x00,0xC1,0xBF,0x00,0x00,0xA4,0xC0,0x00,0x00,0xF2,0xC2,0x00,0x00,
    0x18,0xC6,0x00,0x00,0xC2,0xCA,0x00,0x00,0xC8,0xD0,0x00,0x00,0x36,0xD7,0x00,0x00,
    0xBB,0xDE,0x00,0x00,0xE6,0xE6,0x00,0x00,0xA5,0xEF,0x00,0x00,0xA6,0xF8,0x00,0x00,
};

static const uchar s_pcmdata_right[] = {
    0x00,0x00,0x92,0x02,0x00,0x00,0xCB,0x0B,0x00,0x00,0xD0,0x14,0x00,0x00,0x1D,0x1D,
    0x00,0x00,0xFC,0x24,0x00,0x00,0x17,0x2C,0x00,0x00,0x4A,0x32,0x00,0x00,0x69,0x37,
    0x00,0x00,0x92,0x3B,0x00,0x00,0x4E,0x3E,0x00,0x00,0x22,0x40,0x00,0x00,0x56,0x40,
    0x00,0x00,0x92,0x3F,0x00,0x00,0x12,0x3D,0x00,0x00,0x88,0x39,0x00,0x00,0x10,0x35,
    0x00,0x00,0xF0,0x2E,0x00,0x00,0x51,0x28,0x00,0x00,0xCE,0x20,0x00,0x00,0x7F,0x18,
    0x00,0x00,0xD5,0x0F,0x00,0x00,0xDA,0x06,0x00,0x00,0xDF,0xFD,0x00,0x00,0xA4,0xF4,
    0x00,0x00,0xA2,0xEB,0x00,0x00,0x39,0xE3,0x00,0x00,0x57,0xDB,0x00,0x00,0x3D,0xD4,
    0x00,0x00,0x1F,0xCE,0x00,0x00,0xE2,0xC8,0x00,0x00,0xB1,0xC4,0x00,0x00,0xC0,0xC1,
    0x00,0x00,0xEC,0xBF,0x00,0x00,0xC1,0xBF,0x00,0x00,0xA4,0xC0,0x00,0x00,0xF2,0xC2,
    0x00,0x00,0x18,0xC6,0x00,0x00,0xC2,0xCA,0x00,0x00,0xC8,0xD0,0x00,0x00,0x36,0xD7,
    0x00,0x00,0xBB,0xDE,0x00,0x00,0xE6,0xE6,0x00,0x00,0xA5,0xEF,0x00,0x00,0xA6,0xF8,
};

int sprd_audiotrack_test_start(int channel,int devices, char *buf, int size){
    const uchar *pcm;
    int ret=-1;
    int     pcm_len;

    audPlayerClose();

    if((NULL!=buf) && (size>0)){
        if(audPlayerOpen(devices, PLAY_SAMPLE_RATE, channel, 0) < 0 ||
            audPlayerPlay((const uchar *)buf, size) < 0 ){
            ret = -1;
        }
    }else{
        if( 0 == channel ) {
            pcm     = s_pcmdata_mono;
            pcm_len = sizeof s_pcmdata_mono;
        } else if( 1 == channel ) {
            pcm     = s_pcmdata_left;
            pcm_len = sizeof s_pcmdata_left;
        } else {
            pcm     = s_pcmdata_right;
            pcm_len = sizeof s_pcmdata_right;
        }
        if( audPlayerOpen(devices, PLAY_SAMPLE_RATE, channel, 0) < 0 ||
            audPlayerPlay(pcm, pcm_len) < 0 ) {
            ret = -1;
        }
    }
    return ret;
}

int sprd_audiotrack_test_stop(void){
    return audPlayerClose();
}


#define RECORD_SAMPLE_RATE  16000
#define RECORD_MAX_SIZE    1600

int sprd_audiorecord_test_start(int devices){
    const uchar *pcm;
    int     pcm_len;
    int ret=-1;
    uchar rcrdBuf[RECORD_MAX_SIZE];
    audRcrderClose();
    audRcrderOpen(devices, RECORD_SAMPLE_RATE);

    if(audRcrderRecord((uchar *)rcrdBuf, RECORD_MAX_SIZE) < 0 ) {
        ret = -1;
    }
    return ret;
}

int sprd_audiorecord_test_read(int size){
     return audRcrderRead(size);
}

int sprd_audiorecord_test_stop(void){
    return audRcrderClose();
}

int sprd_fm_test_open(void){
    AudioSystem::setDeviceConnectionState(AUDIO_DEVICE_OUT_FM_SPEAKER,
        AUDIO_POLICY_DEVICE_STATE_UNAVAILABLE, "", "");
    AudioSystem::setDeviceConnectionState(AUDIO_DEVICE_OUT_FM_HEADSET,
        AUDIO_POLICY_DEVICE_STATE_UNAVAILABLE, "", "");
    return 0;
}

int sprd_fm_test_play(void){
    status_t   status;
    String8 fm_volume("FM_Volume=11");
    String8 fm_mute("FM_Volume=0");

    AudioSystem::setParameters(audio_io_handle_t(0),fm_mute);
    AudioSystem::setForceUse(AUDIO_POLICY_FORCE_FOR_FM,AUDIO_POLICY_FORCE_NONE);

    status = AudioSystem::setDeviceConnectionState(AUDIO_DEVICE_OUT_FM_HEADSET,
         AUDIO_POLICY_DEVICE_STATE_AVAILABLE, "", "");
    // AudioSystem::setForceUse(AUDIO_POLICY_FORCE_FOR_MEDIA,AUDIO_POLICY_FORCE_NONE);

    //AudioSystem::setParameters(audio_io_handle_t(0),fm_volume);

    if ( NO_ERROR != status ) {
        ALOGE("out to fm headset error!\n");
        return -1;
    }
    AudioSystem::setParameters(audio_io_handle_t(0),fm_volume);
    return 0;
}

int sprd_fm_test_stop(void){
    AudioSystem::setDeviceConnectionState(AUDIO_DEVICE_OUT_FM_HEADSET,
        AUDIO_POLICY_DEVICE_STATE_UNAVAILABLE, "", "");
        AudioSystem::setForceUse(AUDIO_POLICY_FORCE_FOR_MEDIA,
        AUDIO_POLICY_FORCE_NONE);
    return 0;
}
}

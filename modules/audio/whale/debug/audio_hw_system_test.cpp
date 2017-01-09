//
// Spreadtrum Auto Tester
//
#include <sys/select.h>
#include <fcntl.h>
#include <pthread.h>
#include <stdlib.h>
#include <pthread.h>
#include <stdint.h>
#include "audio_debug.h"
#include <fcntl.h>
#include <media/AudioRecord.h>
#include <media/AudioSystem.h>
#include <media/AudioTrack.h>
#include <media/mediarecorder.h>
#include <system/audio.h>
#include <system/audio_policy.h>


//#define LOG_NDEBUG 0
#define LOG_TAG "audio_hw_system_test"

#include <math.h>
#include <utils/Log.h>
#include <cutils/properties.h>
#include "media/ToneGenerator.h"
#include <media/AudioSystem.h>
#include <media/IAudioFlinger.h>
#include <media/IAudioPolicyService.h>

#include <system/audio.h>
#include "audio_hw_dev_test.h"
#include <cutils/str_parms.h>
#include "audio_hw.h"
//namespace sci_aud {
namespace android {
using namespace android;

sp<AudioRecord> sRcrder ;
sp<AudioTrack> sPlayer;

#define LOOP_TEST 1
#define IN_TEST 2
#define OUT_TEST 3
int test_case;

int pipe_file=-1;
static uchar * s_data     = NULL;
static uint    s_data_len = 0;
static uint    s_data_pos = 0;

static  int             sPlaySync   = 1;
static  size_t          sPlayBufLen = -1;
static  int             sRcrdBufLen = -1;
static void     playerCallback(int event, void* user, void *info);
static status_t setDeviceStateIsConnection( audio_devices_t device );

int audPlayerOpen( int outdev, int sampleRate, int stereo, int sync )
{
    if(LOOP_TEST==test_case){
        return 0;
    }

    if( sPlayer != NULL ) {
        LOG_W("already opend!");
        return 0;
    }

    AudioSystem::clearAudioConfigCache();

    status_t                  status;
    audio_stream_type_t       streamType;
    audio_policy_force_use_t  forceUsage;
    audio_policy_forced_cfg_t forceConfig;
    audio_output_flags_t    flags;

    switch(outdev){
        case AUDIO_DEVICE_OUT_SPEAKER:
            streamType = AUDIO_STREAM_MUSIC;
            forceUsage = AUDIO_POLICY_FORCE_FOR_MEDIA;
            forceConfig= AUDIO_POLICY_FORCE_SPEAKER;
            //status = AudioSystem::setPhoneState(AUDIO_MODE_NORMAL);
            break;

        case AUD_OUTDEV_EARPIECE:
            streamType = AUDIO_STREAM_VOICE_CALL;
            //streamType = AUDIO_STREAM_VOICE_CALL;
            forceUsage = AUDIO_POLICY_FORCE_FOR_COMMUNICATION;
            forceConfig= AUDIO_POLICY_FORCE_NONE;
            //status = AudioSystem::setPhoneState(AUDIO_MODE_IN_COMMUNICATION);//
            break;

        default :
            outdev=AUDIO_DEVICE_OUT_WIRED_HEADPHONE;
            streamType = AUDIO_STREAM_SYSTEM;
            forceUsage = AUDIO_POLICY_FORCE_FOR_MEDIA;
            forceConfig= AUDIO_POLICY_FORCE_NONE;
            break;
    }

    flags = AUDIO_OUTPUT_FLAG_NONE;

    status = setDeviceStateIsConnection(outdev);
    LOG_I("setDeviceStateIsConnection: status = %d dev:%d\n", status,outdev);

    status = AudioSystem::setForceUse(forceUsage, forceConfig);
    LOG_I("setForceUse: status = %d\n", status);

    sPlayer = new AudioTrack();
    if( sPlayer == NULL ) {
        LOG_E("new fail!\n");
        return -2;
    }

    sPlaySync = sync;


    status = AudioTrack::getMinFrameCount(&sPlayBufLen, streamType, sampleRate);
    if( status != NO_ERROR ) {
        LOG_E("getMinFrameCount error: status = %d\n", status);
        return -1; //
    }
    LOG_I("default min frame count = %d\n", sPlayBufLen);
    if( sPlayBufLen < AUD_PLAY_FRAME_BUF ) {
        sPlayBufLen = AUD_PLAY_FRAME_BUF;
    }

    AudioTrack::callback_t cbt = !sync ? playerCallback : NULL;
    void*                 usr = !sync ? sPlayer.get() : NULL;

    int chls = stereo ? AUDIO_CHANNEL_OUT_STEREO : AUDIO_CHANNEL_OUT_MONO;
    status = sPlayer->set(streamType, sampleRate, AUD_PLAY_PCM, chls,
            sPlayBufLen, flags, cbt, usr);
    if( status != NO_ERROR ){
        LOG_E("set error: status = %d\n", status);
        //delete sPlayer;
        sPlayer = NULL;
        return -3;
    }

    FUN_EXIT;
    return 0;
}

int audPlayerPlay( const uchar * data, int size )
{
    FUN_ENTER;
    #if 0
    if(NULL==sPlayer){
        return -1;
    }
    #endif
    if( sPlaySync ) {
        LOG_E("audPlayerPlay sPlaySync is true");
        sPlayer->start();

        while( size > 0 ) {
            AudioTrack::Buffer buffer;
            buffer.frameCount = (sPlayer->format() == AUDIO_FORMAT_PCM_16_BIT
) ? (size >> 1) : size;

            status_t status = sPlayer->obtainBuffer(&buffer, 1);
            if (status == NO_ERROR) {
                LOG_I("obtained buffer size = %d\n", buffer.size);
                memcpy(buffer.i8, data, buffer.size);
                data += buffer.size;
                size -= buffer.size;
                sPlayer->releaseBuffer(&buffer);
            } else if (status != TIMED_OUT && status != WOULD_BLOCK) {
                LOG_W("cannot write to AudioTrack: size = %d\n", size);
                return -1;
            }
        }
    } else {
        LOG_I("audPlayerPlay sPlaySync is false");
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

        if(LOOP_TEST==test_case){
            buf->size=audRcrderRecord((uchar *)buf->i8,buf->size);
        }else{
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
    if(LOOP_TEST==test_case){
        return 0;
    }

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
#ifdef ANDROID_VERSIO_5_X
            AudioSystem::setDeviceConnectionState( arrOut[i],
            AUDIO_POLICY_DEVICE_STATE_UNAVAILABLE, "" );
#else
            AudioSystem::setDeviceConnectionState( arrOut[i],
            AUDIO_POLICY_DEVICE_STATE_UNAVAILABLE, "", "" );
#endif /* ANDROID_VERSIO_5_X */
            ALOGE("AUD_INDEV_BACK_MIC");
        }
    }

#ifdef ANDROID_VERSIO_5_X
    return AudioSystem::setDeviceConnectionState( device,
        AUDIO_POLICY_DEVICE_STATE_AVAILABLE, "" );
#else
    return AudioSystem::setDeviceConnectionState( device,
        AUDIO_POLICY_DEVICE_STATE_UNAVAILABLE, "", "");
#endif /* ANDROID_VERSIO_5_X */
}

int audRcrderRecord( uchar * data, int size )
{
    int retunnum = 0;
    retunnum = sRcrder->read(data,size);
    LOG_E("audRcrderRecord = %d",retunnum);
    if(retunnum > 0)
        return retunnum;
    else
        return -1;
    return 0;
}

int audRcrderOpen( int indev, int sampleRate )
{
    FUN_ENTER;
    if(LOOP_TEST==test_case){
        return 0;
    }

    if( sRcrder !=  NULL ) {
        LOG_W("already opened!\n");
        return 0;
    }

    if( sRcrdBufLen < AUD_RCRD_FRAME_BUF ) {
        sRcrdBufLen = AUD_RCRD_FRAME_BUF;
    }

#ifdef ANDROID_VERSIO_5_X
    sRcrder = new AudioRecord();
#else
    String16 opPackageName = String16();
    sRcrder = new AudioRecord(opPackageName);
#endif /* ANDROID_VERSIO_5_X */
    if( sRcrder == NULL ) {
        LOG_W("new fail!\n");
        return -2;
    }

    pipe_file=-1;

    status_t status;
    audio_source_t input;

    if (indev == AUD_INDEV_BACK_MIC){
        input = AUDIO_SOURCE_CAMCORDER;
#ifdef ANDROID_VERSIO_5_X
        status = AudioSystem::setDeviceConnectionState(
            AUDIO_DEVICE_IN_BACK_MIC,
            AUDIO_POLICY_DEVICE_STATE_AVAILABLE, "");
#else
        status = AudioSystem::setDeviceConnectionState(
            AUDIO_DEVICE_IN_BACK_MIC,
            AUDIO_POLICY_DEVICE_STATE_AVAILABLE, "", "");
            ALOGE("AUD_INDEV_BACK_MIC");
#endif /* ANDROID_VERSIO_5_X */
            LOG_E("AUD_INDEV_BACK_MIC");
    } else if (indev == AUD_INDEV_HEADSET_MIC){
        input = AUDIO_SOURCE_MIC;
#ifdef ANDROID_VERSIO_5_X
        status = AudioSystem::setDeviceConnectionState(
            AUDIO_DEVICE_IN_WIRED_HEADSET,
            AUDIO_POLICY_DEVICE_STATE_AVAILABLE, "");
#else
        status = AudioSystem::setDeviceConnectionState(
            AUDIO_DEVICE_IN_WIRED_HEADSET,
            AUDIO_POLICY_DEVICE_STATE_AVAILABLE, "", "");
            ALOGE("AUD_INDEV_BACK_MIC");
#endif /* ANDROID_VERSIO_5_X */
    } else if (indev == AUD_INDEV_BUILTIN_MIC ){
        input = AUDIO_SOURCE_MIC;
        LOG_E("AUD_INDEV_BUILTIN_MIC");
    } else {
        LOG_E("receive command error");
        return -2;
    }

    status = sRcrder->set(input, sampleRate, AUD_RCRD_PCM, AUD_RCRD_CHL,
            sRcrdBufLen);
    if( status != NO_ERROR ) {
        LOG_E("set error: status = %d\n", status);
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
        LOG_W("not opened!\n");
        return 0;
    }

    if(pipe_file<0){
        LOG_D("audRcrderOpen start open");
        pipe_file=open(MMI_DEFAULT_PCM_FILE, O_WRONLY);
        LOG_D("audRcrderOpen open end");
        if(pipe_file< 0) {
            LOG_E("audRcrderOpen open:%d failed",AUD_RCRD_FRAME_BUF);
            return -1;
        }
    }

    {
        uchar *tmp=NULL;
        int to_write=size;
        tmp = (uchar *)malloc(size);
        if(tmp==NULL){
            LOG_E("audRcrderRead malloc failed");
            return -1;
        }

        ret=audRcrderRecord(tmp, size);
        if(ret<0){
            LOG_E("audRcrderRead audRcrderRecord failed");
            return -1;
        }

        while(to_write) {
            ret = write(pipe_file,tmp,to_write);
            if(ret <= 0) {
                usleep(10000);
                continue;
            }
            if(ret < to_write) {
                usleep(10000);
            }
            to_write -= ret;
            tmp += ret;
            LOG_D("audRcrderRead num_write:0x%x err:%s", ret,strerror(errno));
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
    if(LOOP_TEST==test_case){
        return 0;
    }
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

//} // namespace

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


int sprd_setStreamVolume(audio_stream_type_t stream,int level){
    int ret=0;
    float volume=0.000000;
    ALOGI("setStreamVolume[%d]:%d",stream,level);

    if(ret==0){
        ret = AudioSystem::setStreamVolumeIndex(stream,level,audio_io_handle_t(0));
    }

    if(NO_ERROR==ret){
       ret=0;
    }else{
       ret=-1;
    }

    return ret;
}
}

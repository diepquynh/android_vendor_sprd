// 
// Spreadtrum Auto Tester
//
// anli   2012-11-10
//
#include "type.h"
#include "audio.h"
#include "perm.h"

#include <fcntl.h>
#include <media/AudioRecord.h>
#include <media/AudioSystem.h>
#include <media/AudioTrack.h>
#include <media/mediarecorder.h>
#include <system/audio.h>
#include <system/audio_policy.h>
//#include <utils/String8.h>


//------------------------------------------------------------------------------
// enable or disable local debug
#define DBG_ENABLE_DBGMSG
#define DBG_ENABLE_WRNMSG
#define DBG_ENABLE_ERRMSG
#define DBG_ENABLE_INFMSG
#define DBG_ENABLE_FUNINF
#include "debug.h"
//------------------------------------------------------------------------------

//++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
//--namespace sci_aud {

using namespace android;
using namespace at_perm;
//++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++

//------------------------------------------------------------------------------
#define AUD_PLAY_PCM        AUDIO_FORMAT_PCM_16_BIT
#define AUD_PLAY_CHL        AUDIO_CHANNEL_OUT_STEREO //MONO
#define AUD_PLAY_FRAME_BUF  2 * 1024

#define AUD_RCRD_PCM        AUDIO_FORMAT_PCM_16_BIT
#define AUD_RCRD_CHL        AUDIO_CHANNEL_IN_MONO
#define AUD_RCRD_FRAME_BUF  5 * 1024
//

//------------------------------------------------------------------------------
//------------------------------------------------------------------------------
sp<AudioRecord> sRcrder ;
sp<AudioTrack> sPlayer;
//static  AudioTrack    * sPlayer     = NULL;
//static  AudioRecord   * sRcrder     = NULL;
static  int             sPlaySync   = 1;
static  size_t             sPlayBufLen = -1;
static  int             sRcrdBufLen = -1;
//------------------------------------------------------------------------------
static void     playerCallback(int event, void* user, void *info);
static status_t setDeviceStateIsConnection( audio_devices_t device );
//------------------------------------------------------------------------------
//------------------------------------------------------------------------------

int audPlayerOpen( int outdev, int sampleRate, int stereo, int sync )
{
    FUN_ENTER;
    if( sPlayer != NULL ) {
        WRNMSG("already opend!");
        return 0;
    }
    permInstallService(NULL);

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
        status = AudioSystem::setPhoneState(AUDIO_MODE_NORMAL);//AUDIO_MODE_NORMAL
    } else if( AUD_OUTDEV_EARPIECE == outdev ) {
		audDevice  = AUDIO_DEVICE_OUT_EARPIECE;
        streamType = AUDIO_STREAM_MUSIC;
        //streamType = AUDIO_STREAM_VOICE_CALL;
        forceUsage = AUDIO_POLICY_FORCE_FOR_COMMUNICATION;
        forceConfig= AUDIO_POLICY_FORCE_NONE;
        status = AudioSystem::setPhoneState(AUDIO_MODE_IN_COMMUNICATION);//AUDIO_MODE_NORMAL
        //DBGMSG("setPhoneState: status = %d\n", status);
    } else {
		audDevice  = AUDIO_DEVICE_OUT_WIRED_HEADPHONE;
        streamType = AUDIO_STREAM_SYSTEM;
        forceUsage = AUDIO_POLICY_FORCE_FOR_MEDIA;
        forceConfig= AUDIO_POLICY_FORCE_NONE;
    }
	 flags = AUDIO_OUTPUT_FLAG_NONE;

	status = setDeviceStateIsConnection(audDevice);
	DBGMSG("setDeviceStateIsConnection: status = %d\n", status);

	status = AudioSystem::setForceUse(forceUsage, forceConfig);
    DBGMSG("setForceUse: status = %d\n", status);

    sPlayer = new AudioTrack();
    if( sPlayer == NULL ) {
        ERRMSG("new fail!\n");
        return -2;
    }

    sPlaySync = sync;


	status = AudioTrack::getMinFrameCount(&sPlayBufLen, streamType, sampleRate);
	if( status != NO_ERROR ) {
        ERRMSG("getMinFrameCount error: status = %d\n", status);
        return -1; //
    }
    DBGMSG("default min frame count = %d\n", sPlayBufLen);
    if( sPlayBufLen < AUD_PLAY_FRAME_BUF ) {
        sPlayBufLen = AUD_PLAY_FRAME_BUF;
    }

    AudioTrack::callback_t cbt = !sync ? playerCallback : NULL;
    void*                 usr = !sync ? sPlayer.get() : NULL;
	
	int chls = stereo ? AUDIO_CHANNEL_OUT_STEREO : AUDIO_CHANNEL_OUT_MONO;
	status = sPlayer->set(streamType, sampleRate, AUD_PLAY_PCM, chls,
            sPlayBufLen, flags, cbt, usr);
    if( status != NO_ERROR )    {
        ERRMSG("set error: status = %d\n", status);
        //delete sPlayer;
        sPlayer = NULL;
        return -3;
    }
    
    FUN_EXIT;
    return 0;
}

//------------------------------------------------------------------------------
//------------------------------------------------------------------------------
static uchar * s_data     = NULL;
static uint    s_data_len = 0;
static uint    s_data_pos = 0;
//------------------------------------------------------------------------------
int audPlayerPlay( const uchar * data, int size )
{
    FUN_ENTER;
    AT_ASSERT( sPlayer != NULL );
    //AT_ASSERT( 0 == (size & 1) );
    if( sPlaySync ) {
        ALOGE("audPlayerPlay sPlaySync is true");
        sPlayer->start();
        
        while( size > 0 ) { 
            AudioTrack::Buffer buffer;
            buffer.frameCount = (sPlayer->format() == AUDIO_FORMAT_PCM_16_BIT) ? (size >> 1) : size;
            
            status_t status = sPlayer->obtainBuffer(&buffer, 1);
            if (status == NO_ERROR) {
                DBGMSG("obtained buffer size = %d\n", buffer.size);
                
                memcpy(buffer.i8, data, buffer.size);
                data += buffer.size;
                size -= buffer.size;
                
                sPlayer->releaseBuffer(&buffer);
            } else if (status != TIMED_OUT && status != WOULD_BLOCK) {
                ERRMSG("cannot write to AudioTrack: size = %d\n", size);
                return -1;
            }
        }
    } else {
        ALOGE("audPlayerPlay sPlaySync is false");
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

//------------------------------------------------------------------------------
void playerCallback( int event, void* user, void *info )
{
    //DBGMSG("event = %d\n", event);
    if( AudioTrack::EVENT_MORE_DATA == event && NULL != s_data ) {
        AudioTrack::Buffer * buf = (AudioTrack::Buffer *)info;
        
        //DBGMSG("buf.size = %d\n", buf->size);
        
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

//------------------------------------------------------------------------------
int audPlayerStop( void )
{
    FUN_ENTER;
    if( sPlayer != NULL ) {
        sPlayer->stop();
    }
    FUN_EXIT;
    return 0;
}

//------------------------------------------------------------------------------
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

//------------------------------------------------------------------------------
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
				AUDIO_POLICY_DEVICE_STATE_UNAVAILABLE, "" );
		}
	}

	return AudioSystem::setDeviceConnectionState( device,
            AUDIO_POLICY_DEVICE_STATE_AVAILABLE, "" );
}

//------------------------------------------------------------------------------
//------------------------------------------------------------------------------
int audRcrderOpen( int indev, int sampleRate )
{
    FUN_ENTER;
    if( sRcrder !=  NULL ) {
        WRNMSG("already opened!\n");
        return 0;
    }
    
    permInstallService("android.permission.RECORD_AUDIO");
    //if( AudioRecord::getMinFrameCount(&sRcrdBufLen, sampleRate,
    //        AUD_RCRD_PCM, AUD_RCRD_CHL) != NO_ERROR ) {
    //    ERRMSG("getMinFrameCount error!\n");
    //    return -1;
    //}
    //DBGMSG("default min frame count = %d\n", sRcrdBufLen);
    if( sRcrdBufLen < AUD_RCRD_FRAME_BUF ) {
        sRcrdBufLen = AUD_RCRD_FRAME_BUF;
    }
    
    sRcrder = new AudioRecord();
    if( sRcrder == NULL ) {
        ERRMSG("new fail!\n");
        return -2;
    }

    status_t status;
    audio_source_t input;

    if (indev == AUD_INDEV_BACK_MIC){
        input = AUDIO_SOURCE_CAMCORDER;
        status = AudioSystem::setDeviceConnectionState(AUDIO_DEVICE_IN_BACK_MIC,
            AUDIO_POLICY_DEVICE_STATE_AVAILABLE, "");
            ALOGE("wangzuo:indev == AUD_INDEV_BACK_MIC");
    } else if (indev == AUD_INDEV_HEADSET_MIC){
        input = AUDIO_SOURCE_MIC;
        status = AudioSystem::setDeviceConnectionState(AUDIO_DEVICE_IN_WIRED_HEADSET,
            AUDIO_POLICY_DEVICE_STATE_AVAILABLE, "");
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
        ERRMSG("set error: status = %d\n", status);
        sRcrder.clear();
        return -3;
    }

	//if( sRcrder->stopped() ) {
        sRcrder->start();
    //}

	{
		uchar tmp[AUD_RCRD_FRAME_BUF];
		audRcrderRecord(tmp, AUD_RCRD_FRAME_BUF);
		audRcrderRecord(tmp, AUD_RCRD_FRAME_BUF);
		audRcrderRecord(tmp, AUD_RCRD_FRAME_BUF);
		audRcrderRecord(tmp, AUD_RCRD_FRAME_BUF);
	}

    FUN_EXIT;
    return 0;
}

//------------------------------------------------------------------------------
int audRcrderRecord( uchar * data, int size )
{
    FUN_ENTER;
    #if 0
    if( sRcrder->stopped() ) {
        sRcrder->start();
    }
    #endif
    int retunnum = 0;
    retunnum = sRcrder->read(data,size);
    #if 0
    while( size > 0 ) {
        ALOGE("wangzuo,audRcrderRecord0");
        AudioRecord::Buffer buffer;
        buffer.frameCount = (sRcrder->format() == AUDIO_FORMAT_PCM_16_BIT) ? (size >> 1) : size;
        
        status_t status = sRcrder->obtainBuffer(&buffer, 1);

        ALOGE("wangzuo,audRcrderRecord1 status %d",status);
        if (status == NO_ERROR) {
            DBGMSG("obtained buffer size = %d\n", buffer.size);
            
            memcpy(data, buffer.i8, buffer.size);
            data += buffer.size;
            size -= buffer.size;
            
            sRcrder->releaseBuffer(&buffer);
        } else if (status != TIMED_OUT && status != WOULD_BLOCK) {
            ERRMSG("cannot read from AudioRecord: size = %d\n", size);
            sRcrder->stop();
            return -1;
        }
    }
    #endif
    //sRcrder->stop();
    ALOGE("retunnum = %d",retunnum);
    if(retunnum > 0)
        return 0;
    else
        return -1;
    FUN_EXIT;
    return 0;
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

//------------------------------------------------------------------------------
int audRcrderClose( void )
{
    FUN_ENTER;
    if( sRcrder != NULL ) {
        sRcrder->stop();
        //delete sRcrder;
        //sRcrder = NULL;
        sRcrder.clear();
    }
    FUN_EXIT;
    return 0;
}

//++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
//--} // namespace
//++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++

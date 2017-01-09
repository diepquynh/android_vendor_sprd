#ifndef SPRD_AUDIO_TEST_H_
#define SPRD_AUDIO_TEST_H_
#include <media/AudioSystem.h>
namespace android{
typedef unsigned char  byte;
typedef unsigned char  uchar;

#ifndef uint
typedef unsigned int   uint;
#endif // uint
#define PLAY_SAMPLE_RATE  48000
#define RECORD_SAMPLE_RATE 48000
typedef unsigned short ushort;
#define AUD_PLAY_PCM        AUDIO_FORMAT_PCM_16_BIT
#define AUD_PLAY_CHL        AUDIO_CHANNEL_OUT_STEREO //MONO
#define AUD_PLAY_FRAME_BUF  2 * 1024

#define AUD_RCRD_PCM        AUDIO_FORMAT_PCM_16_BIT
#define AUD_RCRD_CHL        AUDIO_CHANNEL_IN_MONO
#define AUD_RCRD_FRAME_BUF  5 * 1024

#define FUN_ENTER     do{ALOGD("%s %d enter",__func__,__LINE__);}while(0)
#define FUN_EXIT     do{ALOGD("%s %d exit",__func__,__LINE__);}while(0)
#define HEADSET_STATE "/sys/class/switch/h2w/state"
#define MMI_DEFAULT_PCM_FILE  "/data/local/media/aploopback.pcm"
enum aud_outdev_e {
    AUD_OUTDEV_SPEAKER = 0,
    AUD_OUTDEV_EARPIECE,
    AUD_OUTDEV_HEADSET,
};


enum aud_indev_e {
    AUD_INDEV_BUILTIN_MIC = 0,
    AUD_INDEV_HEADSET_MIC,
    AUD_INDEV_BACK_MIC,
};
extern "C"{
int audPlayerOpen( int outdev, int sampleRate, int stereo, int sync );
int audPlayerPlay( const uchar * data, int size );
int audPlayerStop( void );
int audPlayerClose( void );

int audRcrderOpen( int indev, int sampleRate );
int audRcrderRecord( uchar * data, int size );
int audRcrderStop( void );
int audRcrderClose( void );
int sprd_audiorecord_test_read(int size);
int sprd_audiorecord_test_start(int devices);
int sprd_audiorecord_test_stop(void);
int sprd_audiotrack_test_start(int channel,int devices, char *buf, int size);
int sprd_audiotrack_test_stop(void);
int sprd_audioloop_test_start(int channel,int devices_in, int devices_out);
int sprd_audioloop_test_stop(void);
int sprd_setStreamVolume(audio_stream_type_t stream,int level);
}
}
#endif /*SPRD_AUDIO_TRACK_H_*/

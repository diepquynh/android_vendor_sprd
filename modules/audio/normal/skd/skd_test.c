#include <sys/select.h>
#include <fcntl.h>
#include <pthread.h>
#include <stdlib.h>
#include <pthread.h>
#include <stdint.h>

#include "ring_buffer.h"

#include <errno.h>
#include <pthread.h>
#include <stdint.h>
#include <sys/time.h>
#include <sys/stat.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/mman.h>
#include <semaphore.h>


#include <cutils/log.h>
#include <cutils/str_parms.h>
#include <cutils/properties.h>

#include <hardware/hardware.h>
#include <system/audio.h>
#include <hardware/audio.h>

#include <expat.h>

#include <tinyalsa/asoundlib.h>

#define LOG_TAG "skd_test"

int audio_skd_test(void * dev,struct str_parms *parms,int type);

enum aud_skd_test_m {
    TEST_AUDIO_SKD_IDLE =0,
    TEST_AUDIO_SKD_OUT_DEVICES,
    TEST_AUDIO_SKD_IN_DEVICES,
    TEST_AUDIO_SKD_IN_OUT_LOOP,
    TEST_AUDIO_SKD_OUT_IN_LOOP,
};
#if 0
struct skd_test_t {
    pthread_t thread;
    int state;
    struct ring_buffer ring_buf;
    int mode;
    void *dev;
    int delay; //ms
    int fd;
    pthread_mutex_t lock; 
};
#endif
struct loop_ctl_t
{
    pthread_t thread;
    sem_t   sem;
    bool is_exit;
    pthread_mutex_t lock;
    void *dev;
    struct pcm *pcm;
    struct ring_buffer *ring_buf;
    struct tiny_stream_out *stream;
    int state;
    void * skd_loop;
};

struct skd_loop_t {
    struct loop_ctl_t in;
    struct loop_ctl_t out;
    int state;
    int loop_delay;
};

#define DEFAULT_OUT_SAMPLING_RATE 44100
/* constraint imposed by VBC: all period sizes must be multiples of 160 */
#define VBC_BASE_FRAME_COUNT 160
/* number of base blocks in a short period (low latency) */
#define SHORT_PERIOD_MULTIPLIER 8 /* 29 ms */
/* number of frames per short period (low latency) */
#define SHORT_PERIOD_SIZE (VBC_BASE_FRAME_COUNT * SHORT_PERIOD_MULTIPLIER)
#define CAPTURE_PERIOD_COUNT 3
#define PLAYBACK_SHORT_PERIOD_COUNT   3      /*4*/

static  struct pcm_config _default_config = {
    .channels = 2,
    .rate = DEFAULT_OUT_SAMPLING_RATE,
    .period_size = SHORT_PERIOD_SIZE,
    .period_count = PLAYBACK_SHORT_PERIOD_COUNT,
    .format = PCM_FORMAT_S16_LE,
    .start_threshold = SHORT_PERIOD_SIZE,
    .avail_min = SHORT_PERIOD_SIZE,
};


struct skd_loop_t skd_loop;

#if 0

static  struct pcm_config default_record_config = {
    .channels = 1,
    .rate = 16000,
    .period_size = 640,
    .period_count = 8,
    .format = PCM_FORMAT_S16_LE,
};

struct pcm_config playback_config = {
    .channels = 2,
    .rate = DEFAULT_OUT_SAMPLING_RATE,
    .period_size = SHORT_PERIOD_SIZE,
    .period_count = PLAYBACK_SHORT_PERIOD_COUNT,
    .format = PCM_FORMAT_S16_LE,
};

struct pcm_config record_config = {
    .channels = 1,
    .rate = 16000,
    .period_size = 320,
    .period_count = 8,
    .format = PCM_FORMAT_S16_LE,
};

static const unsigned char s_pcmdata_mono[] = {
    0x92,0x02,0xcb,0x0b,0xd0,0x14,0x1d,0x1d,0xfc,0x24,0x17,0x2c,0x4a,0x32,0x69,0x37,
    0x92,0x3b,0x4e,0x3e,0x22,0x40,0x56,0x40,0x92,0x3f,0x12,0x3d,0x88,0x39,0x10,0x35,
    0xf0,0x2e,0x51,0x28,0xce,0x20,0x7f,0x18,0xd5,0x0f,0xda,0x06,0xdf,0xfd,0xa4,0xf4,
    0xa2,0xeb,0x39,0xe3,0x57,0xdb,0x3d,0xd4,0x1f,0xce,0xe2,0xc8,0xb1,0xc4,0xc0,0xc1,
    0xec,0xbf,0xc1,0xbf,0xa4,0xc0,0xf2,0xc2,0x18,0xc6,0xc2,0xca,0xc8,0xd0,0x36,0xd7,
    0xbb,0xde,0xe6,0xe6,0xa5,0xef,0xa6,0xf8,
};
#endif


#define TEST_AUDIO_LOOP_MIN_DATA_MS 500
//#define DSPLOOP_FRAME_SIZE 488
#define DSPLOOP_FRAME_SIZE 512

static int _calculation_ring_buffer_size(int ms,struct pcm_config *config){
    int size=ms*config->rate*config->channels*2/1000;
    int i=31;
    while(i){
        if((size & (1<<i))!=0)
            break;
        else
            i--;
    }

    if(i<=0)
        return 0;
    else
        return 1<<(i+1);
}

static unsigned int producer_proc(struct ring_buffer *ring_buf,unsigned char * buf,unsigned int size)
{
    int to_write=size;
    int ret=0;
    unsigned char *tmp=NULL;
    tmp=buf;
    while(to_write) {
        ret = ring_buffer_put(ring_buf, (void *)tmp, to_write);
        if(ret <= 0) {
            usleep(10000);
            continue;
        }
        if(ret < to_write) {
            usleep(10000);
        }
        to_write -= ret;
        tmp += ret;
    }
    return size;
}

static void *skd_loop_rx_thread(void *args){
    struct loop_ctl_t *in=(struct loop_ctl_t *)args;
    struct tiny_audio_device *adev=(struct tiny_audio_device *)in->dev;
    char *buffer=NULL;
    int num_read = 0;
    int size=0;
    int bytes_read=0;

    pthread_mutex_lock(&(in->lock));

    in->pcm =pcm_open(get_snd_card_number(CARD_SPRDPHONE), 0, PCM_IN , &_default_config);
    if (!pcm_is_ready(in->pcm)) {
        ALOGE("start_input_stream:cannot open pcm : %s",
              pcm_get_error(in->pcm));
        goto ERR;
    }

    size = pcm_frames_to_bytes(in->pcm, pcm_get_buffer_size(in->pcm));
    size =size/DSPLOOP_FRAME_SIZE;
    size++;
    size*=DSPLOOP_FRAME_SIZE;

    buffer = (char *)malloc(size);
    if (!buffer) {
        ALOGE("Unable to allocate %d bytes\n", size);
        goto ERR;
    }

    sem_wait(&in->sem);
    while(in->is_exit==false){
        num_read=pcm_read(in->pcm, buffer, size);
        ALOGD("skd_loop_rx_thread read:0x%x req:0x%x",num_read,size);
        if (!num_read){
            bytes_read += size;
            producer_proc(in->ring_buf, (unsigned char *)buffer, (unsigned int)size);
            ALOGI("skd_loop_rx_thread capture 0x%x total:0x%x",size,bytes_read);
        }else{
            ALOGW("skd_loop_rx_thread: no data read num_read:%d",num_read);
            usleep(100*1000);
        }
    }
    pthread_mutex_unlock(&(in->lock));
    ALOGI("skd_loop_rx_thread exit success");
    return NULL;

ERR:
    if(buffer){
        free(buffer);
        buffer=NULL;
    }

    if(in->pcm){
        pcm_close(in->pcm);
        in->pcm=NULL;
    }
    pthread_mutex_unlock(&(in->lock));

    ALOGE("skd_loop_rx_thread exit err");
    return NULL;
}

static void *skd_loop_tx_thread(void *args){
    struct loop_ctl_t *out=(struct loop_ctl_t *)args;
    struct tiny_audio_device *adev=(struct tiny_audio_device *)out->dev;
    char *buffer=NULL;
    int num_read = 0;
    int size=0;
    int ret=-1;
    int write_count=0;
    int no_data_count=0;

    pthread_mutex_lock(&(out->lock));

    out->pcm  =
        pcm_open(get_snd_card_number(CARD_SPRDPHONE), 0, PCM_OUT , &_default_config);

    if (!pcm_is_ready(out->pcm)) {
        ALOGE("audio_skd_loop_start:cannot open pcm : %s",
              pcm_get_error(out->pcm));
        goto ERR;
    }

    size = pcm_frames_to_bytes(out->pcm, pcm_get_buffer_size(out->pcm));
    size =size/DSPLOOP_FRAME_SIZE;
    size++;
    size*=DSPLOOP_FRAME_SIZE;

    buffer = (char *)malloc(size);
    if (!buffer) {
        ALOGE("skd_loop_tx_thread Unable to allocate %d bytes\n", size);
        goto ERR;
    }

    sem_wait(&out->sem);
    out->state=true;
    while(1){
        num_read=ring_buffer_get(out->ring_buf, (void *)buffer, size);
        ALOGI("skd_loop_tx_thread read:0x%x req:0x%x",num_read,size);

        if(num_read > 0){
            no_data_count=0;
            ret = pcm_write(out->pcm, buffer,num_read);
            if (ret) {
                int ret = 0;
                ALOGE("skd_loop_tx_thread Error playing sample:%s\n",pcm_get_error(out->pcm));
                ret = pcm_close(out->pcm);
                out->pcm = pcm_open(get_snd_card_number(CARD_SPRDPHONE), 0, PCM_OUT , &_default_config);
                if (!out->pcm  || !pcm_is_ready(out->pcm)) {
                    ALOGE("skd_loop_tx_thread Unable to open PCM device %u (%s)\n",
                          0, pcm_get_error(out->pcm));
                    goto ERR;
                }
            }else{
                write_count+=num_read;
                ALOGI("skd_loop_tx_thread write:0x%x total:0x%x",num_read,write_count);
            }
        }else{
            usleep(100*1000);
            no_data_count++;
            ALOGI("skd_loop_tx_thread no data read");
            if(out->is_exit==true){
                break;
            }else{
                if(no_data_count>=10){
                    break;
                }
            }
        }
    }

    if(buffer){
        free(buffer);
        buffer=NULL;
    }
    out->state=false;
    pthread_mutex_unlock(&(out->lock));
    ALOGI("skd_loop_tx_thread exit success");
    return NULL;

ERR:
    if(buffer){
        free(buffer);
        buffer=NULL;
    }

    if(out->pcm){
        pcm_close(out->pcm);
        out->pcm=NULL;
    }

    out->state=false;

    pthread_mutex_unlock(&(out->lock));
    ALOGE("skd_loop_tx_thread exit err");
    return NULL;
}

static int audio_skd_loop_standby(struct tiny_audio_device *adev,struct skd_loop_t *loop)
{
    struct loop_ctl_t *ctl=NULL;
    void *res;
//    struct skd_loop_t *loop=&adev->loop_ctl;
    int wait_count=30;

    ALOGI("audio_skd_loop_stop enter");
    if(TEST_AUDIO_SKD_IDLE==loop->state){
        ALOGW("audio_skd_loop_stop failed, loop is not work");
        return -1;
    }

    loop->in.is_exit=true;
    loop->out.is_exit=true;

    pthread_join(loop->in.thread,  &res);

    ALOGI("audio_skd_loop_stop %d",__LINE__);
    pthread_join(loop->out.thread, &res);

    ALOGI("audio_skd_loop_stop %d",__LINE__);
    ctl=&loop->in;
    pthread_mutex_lock(&(ctl->lock));
    ctl->is_exit=true;
    ctl->dev=NULL;
    if(ctl->pcm!=NULL){
        pcm_close(ctl->pcm);
        ctl->pcm=NULL;
        ALOGI("audio_skd_loop_stop %d",__LINE__);
    }
    pthread_mutex_unlock(&(ctl->lock));
    ctl=&loop->out;
    pthread_mutex_lock(&(ctl->lock));
    ctl->is_exit=true;
    ctl->dev=NULL;
    if(ctl->pcm!=NULL){
        pcm_close(ctl->pcm);
        ctl->pcm=NULL;
        ALOGI("audio_skd_loop_stop %d",__LINE__);
    }
    pthread_mutex_unlock(&(ctl->lock));

    if(loop->out.ring_buf!=NULL){
        ring_buffer_free(loop->out.ring_buf);
    }

    loop->state=TEST_AUDIO_SKD_IDLE;

    pthread_mutex_destroy(&(loop->out.lock));
    pthread_mutex_destroy(&(loop->in.lock));


    ALOGI("audio_skd_loop_standby exit");

    return 0;
}


static int audio_skd_loop_start(struct tiny_audio_device *adev,struct skd_loop_t *loop,int type){
    struct ring_buffer *ring_buf=NULL;
    struct pcm_config * in_config=NULL;
    int size=0;
    int min_buffer_size=0;
    int skd_zero_size=0;
    int delay_size=0;
    if(TEST_AUDIO_SKD_IDLE!=loop->state){
        ALOGW("audio_skd_loop_start failed, loop is working now");
        return -1;
    }

//    in_config=&(adev->dev_ctl->pcm_handle.record[AUD_PCM_DSP_LOOP]);
    in_config = &_default_config;

    size=_calculation_ring_buffer_size(loop->loop_delay*16,in_config);
    min_buffer_size=(TEST_AUDIO_LOOP_MIN_DATA_MS *in_config->rate*in_config->channels*2/1000);
    if(size<min_buffer_size){
        size=_calculation_ring_buffer_size(TEST_AUDIO_LOOP_MIN_DATA_MS,in_config);
    }

    /* skd process frame buffer size is 122 word(244bytes),
       the delay buffer size must be a multiple of skd frame buffer */
    delay_size=loop->loop_delay*in_config->rate*in_config->channels*2/1000;
    skd_zero_size=delay_size/DSPLOOP_FRAME_SIZE;
    skd_zero_size+=1;
    skd_zero_size*=DSPLOOP_FRAME_SIZE;

    ring_buf=ring_buffer_init(size,skd_zero_size);
    if(NULL==ring_buf){
        goto error;
    }

    if (pthread_mutex_init(&(loop->in.lock), NULL) != 0) {
        ALOGE("Failed pthread_mutex_init loop->in.lock,errno:%u,%s",
              errno, strerror(errno));
        goto error;
    }

    if (pthread_mutex_init(&(loop->out.lock), NULL) != 0) {
        ALOGE("Failed pthread_mutex_init loop->out.lock,errno:%u,%s",
              errno, strerror(errno));
        goto error;
    }

    loop->in.ring_buf=ring_buf;
    loop->out.ring_buf=ring_buf;

    loop->in.state=true;
    loop->out.state=true;

    loop->in.dev=adev;
    loop->out.dev=adev;

    loop->in.is_exit=false;
    loop->out.is_exit=false;

    loop->in.skd_loop=loop;
    loop->out.skd_loop=loop;

    sem_init(&loop->in.sem, 0, 1);
    sem_init(&loop->out.sem, 0, 1);

    if(TEST_AUDIO_SKD_IN_OUT_LOOP==type){
        if(pthread_create(&loop->out.thread, NULL, skd_loop_tx_thread, (void *)&(loop->out))) {
            ALOGE("skd_loop_tx_thread creating tx thread failed !!!!");
            goto error;
        }

        if(pthread_create(&loop->in.thread, NULL, skd_loop_rx_thread, (void *)&(loop->in))){
            ALOGE("skd_loop_rx_thread creating rx thread failed !!!!");
            goto error;
        }
    }

    loop->state=type;

    ALOGI("audio_skd_loop_start sucess");
    return 0;

error:

    if(NULL!=loop->out.pcm){
        pcm_close(loop->out.pcm);
        loop->in.pcm=NULL;
    }

    if(NULL!=loop->in.pcm){
        pcm_close(loop->in.pcm);
        loop->in.pcm=NULL;
    }

    if(NULL!=ring_buf){
        ring_buffer_free(ring_buf);
    }

    pthread_mutex_destroy(&(loop->out.lock));
    pthread_mutex_destroy(&(loop->in.lock));

    loop->state=TEST_AUDIO_SKD_IDLE;

    ALOGI("audio_skd_loop_start failed");
    return -1;
}

int audio_skd_test(void * dev,struct str_parms *parms,int type){
    int ret=0;
    struct tiny_audio_device * adev=(struct tiny_audio_device *)dev;

    pthread_mutex_lock(&adev->lock);

    if(type){
        ret=audio_skd_loop_start(adev,&skd_loop,type);
    }else{
        ret=audio_skd_loop_standby(adev,&skd_loop);
    }
    pthread_mutex_unlock(&adev->lock);

    return ret;
}

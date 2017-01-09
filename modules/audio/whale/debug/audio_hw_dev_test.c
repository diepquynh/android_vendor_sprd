#include <sys/select.h>
#include <fcntl.h>
#include <pthread.h>
#include <stdlib.h>
#include <pthread.h>
#include <stdint.h>
#include "audio_hw.h"
#include "audio_control.h"
#include "ring_buffer.h"
#define LOG_TAG "audio_hw_dev_test"

#define TEST_AUDIO_INVALID_DATA_MS 500
#define TEST_LOOPBACK_BUFFER_DATA_MS  400
#define MMI_DEFAULT_PCM_FILE  "/data/local/media/aploopback.pcm"
#define MMI_LOOP_PCM_FILE  "/data/local/media/loop.pcm"

#define TEST_AUDIO_LOOP_MIN_DATA_MS 500
extern force_all_standby(struct tiny_audio_device *adev);

int calculation_ring_buffer_size(int ms,struct pcm_config *config){
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

unsigned int producer_proc(struct ring_buffer *ring_buf,unsigned char * buf,unsigned int size)
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

static void *in_test_thread(void *args){
    struct in_test_t *in_test=(struct in_test_t *)args;
    struct dev_test_t *dev_test = in_test->dev_test;
    struct tiny_audio_device *adev=
        (struct tiny_audio_device   *)in_test->dev_test->adev;

    struct pcm *pcm;
    char *buffer;
#ifdef REC_PROCESS
    char *proc_buf=NULL;
    int proc_buf_size=0;
#endif /* REC_PROCESS */
    unsigned int size;
    unsigned int bytes_read = 0;
    int num_read = 0;
    int read_count = 0;
    int process_init = 0;
    int invalid_data_size=0;

    pcm=NULL;
    buffer=NULL;
    bytes_read=0;

    LOG_I("capture_thread wait");
    sem_wait(&in_test->sem);
    LOG_I("capture_thread running");

    switch_vbc_iis_route(adev->control,UC_MM_RECORD,true);

    pcm = pcm_open(in_test->dev_test->adev->dev_ctl->cards.s_tinycard, PORT_MM, PCM_IN, &in_test->config.config);
    if (!pcm || !pcm_is_ready(pcm)) {
        LOG_W("Unable to open PCM device (%s)\n",pcm_get_error(pcm));
        goto ERR;
    }

    size = pcm_frames_to_bytes(pcm, pcm_get_buffer_size(pcm));
    buffer = (char *)malloc(size);
    if (!buffer) {
        LOG_W("Unable to allocate %d bytes\n", size);
        goto ERR;
    }

    if(in_test->isloop != true){
        invalid_data_size=(TEST_AUDIO_INVALID_DATA_MS *in_test->config.config.rate*in_test->config.config.channels*2/1000);
    }
#ifdef REC_PROCESS
    process_init = init_rec_process(GetAudio_InMode_number_from_device(in_test->dev_test->adev->in_devices),
        in_test->config.config.rate);
    proc_buf_size = size;
    proc_buf = malloc(proc_buf_size);
    if (!proc_buf) {
        LOG_W("Unable to allocate %d bytes\n", size);
        goto ERR;
    }
#endif /* REC_PROCESS */
    memset(buffer,0,size);
#ifdef REC_PROCESS
    memset(proc_buf,0,proc_buf_size);
#endif /* REC_PROCESS */
    LOG_I("begin capture!!!!!!!!!!!!!!!!");

    do{
        num_read=pcm_read(pcm, buffer, size);
        LOG_I("capture read:0x%x req:0x%x",num_read,size);
        read_count++;

        if (!num_read){
            bytes_read += size;
#ifdef REC_PROCESS
            aud_rec_do_process(buffer, size,proc_buf,proc_buf_size);
#endif /* REC_PROCESS */
            if(in_test->isloop == true){
                producer_proc(in_test->ring_buf, (unsigned char *)buffer, (unsigned int)size);
            }else{
                if(bytes_read >invalid_data_size){

                    if(in_test->config.fd > 0){
                        if (write(in_test->config.fd,buffer,size) != size){
                            LOG_W("audiotest_capture_thread write err\n");
                        }
                    }
                }
            }
            LOG_I("capture %d bytes bytes_read:0x%x",size,bytes_read);
        }else{
            LOG_W("aploop_capture_thread: no data read num_read:%d",num_read);
            usleep(20000);
        }
    }while(in_test->state);

    close_in_control(in_test->dev_test->adev->dev_ctl);
ERR:
    if(buffer){
        free(buffer);
        buffer=NULL;
    }

#ifdef REC_PROCESS
    if(proc_buf){
        free(proc_buf);
        proc_buf=NULL;
    }
#endif /* REC_PROCESS */

    if(pcm){
        pcm_close(pcm);
        pcm=NULL;
    }
    if(in_test->config.fd >0){
        close(in_test->config.fd);
        in_test->config.fd=-1;
    }
    if(process_init)
        AUDPROC_DeInitDp();

    LOG_E("audiotest_capture_thread exit");
    return NULL;
}

static void *out_test_thread(void *args)
{
    struct out_test_t *out_test=(struct in_test_t *)args;
    struct tiny_audio_device *adev=
        (struct tiny_audio_device   *)out_test->dev_test->adev;
    struct pcm *pcm;
    char *buffer=NULL;
    char *malloc_buffer=NULL;
    unsigned int size;
    int playback_fd;
    int num_read;
    int ret = 0;

    pcm = NULL;
    buffer = NULL;
    num_read = 0;
    size = 0;

    LOG_I("playback_thread wait");
    sem_wait(&out_test->sem);
    LOG_I("playback_thread running");

    switch_vbc_iis_route(adev->control,UC_MUSIC,true);

    pcm = pcm_open(out_test->dev_test->adev->dev_ctl->cards.s_tinycard, PORT_MM, PCM_OUT, &out_test->config.config);
    if (!pcm || !pcm_is_ready(pcm)) {
        LOG_W("Unable to open PCM device %u (%s)\n",
              0, pcm_get_error(pcm));
        goto ERR;
    }

    size = pcm_frames_to_bytes(pcm, pcm_get_buffer_size(pcm));
    malloc_buffer = malloc(size);
    if (!malloc_buffer) {
        LOG_W("Unable to allocate %d bytes\n", size);
        goto ERR;
    }

    do {
        if(out_test->isloop == true){
            buffer=malloc_buffer;
            num_read=ring_buffer_get(out_test->ring_buf, (void *)buffer, size);
        }else{
            if(out_test->config.fd>0){
                buffer=malloc_buffer;
                LOG_I("out_test_thread begin read:%d",out_test->config.fd);
                num_read=read(out_test->config.fd,buffer,size);
                if(num_read<=0){
                    LOG_I("out_test_thread begin lseek:%d",out_test->config.fd);
                    lseek(out_test->config.fd,0,SEEK_SET);
                    num_read = read(out_test->config.fd, buffer, size);
                }
                LOG_I("out_test_thread return :%d",num_read);
            }else{
                buffer = s_pcmdata_mono;
                num_read = sizeof(s_pcmdata_mono);
            }
        }

        if(num_read > 0){
            LOG_I("out_test_thread begin write :%d",num_read);
            ret = pcm_write(pcm, buffer,num_read);
            LOG_I("out_test_thread end write :%d",num_read);
        }

        if (ret) {
            int ret = 0;
            LOG_W("Error playing sample:%s\n",pcm_get_error(pcm));
            ret = pcm_close(pcm);
            pcm = pcm_open(out_test->dev_test->adev->dev_ctl->cards.s_tinycard, PORT_MM, PCM_OUT, &out_test->config.config);
            if (!pcm || !pcm_is_ready(pcm)) {
                LOG_W("Unable to open PCM device %u (%s)\n",
                      0, pcm_get_error(pcm));
                goto ERR;
            }
        }
        //LOG_D("write 0x:%x bytest",num_read);
    }while(out_test->state);

    close_out_control(out_test->dev_test->adev->dev_ctl);

ERR:

    if(malloc_buffer)  {
        free(malloc_buffer);
        malloc_buffer = NULL;
    }

    if(pcm) {
        pcm_close(pcm);
        pcm = NULL;
    }

    if(out_test->config.fd > 0) {
        close(out_test->config.fd);
        out_test->config.fd = -1;
    }
    LOG_E("audiotest_playback_thread exit");
    return NULL;
}

static int in_devices_test_config(struct audiotest_config *config,struct str_parms *parms)
{
    char value[50];
    int ret = -1;
    int val_int = 0;
    int fd = -1;

    LOG_D("%s %d",__func__,__LINE__);
    memcpy(&config->config, &default_normal_config, sizeof(struct pcm_config));
    ret = str_parms_get_str(parms,"samplerate", value, sizeof(value));
    if(ret >= 0){
        LOG_D("%s %d",__func__,__LINE__);
        val_int = atoi(value);
    }
    config->config.rate = val_int;

    ret = str_parms_get_str(parms,"channels", value, sizeof(value));
    if(ret >= 0){
        val_int = atoi(value);
        LOG_D("%s %d %d",__func__,__LINE__,val_int);
    }
    if((val_int !=1) && (val_int != 2)) {
        LOG_D("%s %d %d",__func__,__LINE__,val_int);
        goto error;
    }
    config->config.channels = val_int;
    ret = str_parms_get_str(parms,"filepath", value, sizeof(value));
    if(ret >= 0){
        LOG_D("%s %d:%s",__func__,__LINE__,value);
        fd = open(value, O_WRONLY);
        if(fd > 0) {
            config->fd = fd;
        }
        else
            config->fd = -1;
    }
    else {
        LOG_D("%s %d",__func__,__LINE__);
        fd = open(MMI_DEFAULT_PCM_FILE, O_WRONLY);
        if(fd > 0) {
            config->fd = fd;
        }
        else{
            config->fd = -1;
        }
    }
    LOG_D("%s %d",__func__,__LINE__);
    return config->fd;
error:
    return -1;
}

static int loop_devices_test_config(struct audiotest_config *config,struct str_parms *parms)
{
    char value[50];
    int ret = -1;
    int val_int = 0;
    int fd = -1;

    LOG_D("%s %d",__func__,__LINE__);
    memcpy(&config->config, &default_normal_config, sizeof(struct pcm_config));
    ret = str_parms_get_str(parms,"samplerate", value, sizeof(value));
    if(ret >= 0){
        LOG_D("%s %d",__func__,__LINE__);
        val_int = atoi(value);
        config->config.rate = val_int;
    }
    ret = str_parms_get_str(parms,"channels", value, sizeof(value));
    if(ret >= 0){
        val_int = atoi(value);
        LOG_I("%s %d %d",__func__,__LINE__,val_int);
        if((val_int !=1) && (val_int != 2)) {
            LOG_E("%s %d %d",__func__,__LINE__,val_int);
            goto error;
        }
        config->config.rate = val_int;
    }
    config->fd = -1;
    LOG_I("%s %d",__func__,__LINE__);
    return 0;
error:
    return -1;
}

static int out_devices_test_config(struct audiotest_config *config,struct str_parms *parms)
{
    char value[50];
    int ret = -1;
    int val_int = 0;
    int fd = -1;

    LOG_D("%s %d",__func__,__LINE__);
    memcpy(&config->config, &default_normal_config, sizeof(struct pcm_config));
    ret = str_parms_get_str(parms,"samplerate", value, sizeof(value));
    if(ret >= 0){
        LOG_D("%s %d",__func__,__LINE__);
        val_int = atoi(value);
    }
    config->config.rate = val_int;

    ret = str_parms_get_str(parms,"channels", value, sizeof(value));
    if(ret >= 0){
        val_int = atoi(value);
        LOG_D("%s %d %d",__func__,__LINE__,val_int);
    }
    if((val_int !=1) && (val_int != 2)) {
        LOG_D("%s %d %d",__func__,__LINE__,val_int);
        goto error;
    }
    config->config.channels = val_int;
    ret = str_parms_get_str(parms,"filepath", value, sizeof(value));
    if(ret >= 0){
        LOG_D("%s %d:%s",__func__,__LINE__,value);
        fd = open(value, O_RDONLY);
        if(fd > 0) {
            config->fd = fd;
        }
        else{
            config->fd = -1;
            config->config.rate=48000;
            LOG_E("open:%s failed",value);
        }
    }else{
        config->fd = -1;
    }
    return 0;
error:
    return -1;
}

int audio_loop_devices_test_start(struct tiny_audio_device *adev,struct str_parms *parms){
    int ret=0;

    struct dev_test_t *dev_test = (struct dev_test_t *) &adev->dev_test;
    struct in_test_t *in_test = (struct in_test_t *) &dev_test->in_test;
    struct out_test_t *out_test = (struct out_test_t *) &dev_test->out_test;
    struct ring_buffer *ring_buf=NULL;
    int min_buffer_size=0;
    int size=0;
    in_test->dev_test=dev_test;
    out_test->dev_test=dev_test;
    dev_test->adev=adev;

    if(TEST_AUDIO_IDLE != dev_test->test_mode){
        LOG_E("audio_loop_devices_test mode:%d",dev_test->test_mode);
        return -1;
    }else{
        dev_test->test_mode=TEST_AUDIO_IN_OUT_LOOP;
    }

    in_test->isloop=true;
    ret = loop_devices_test_config((struct audiotest_config *)&(in_test->config),parms);
    if(ret < 0 ) {
        LOG_D("%s %d",__func__,__LINE__);
        goto error;
    }

    if(adev->loop_delay>=20*1000){
        LOG_E("audio_loop_devices_test_start loop delay is too long,use default");
        adev->loop_delay=2000;
    }

    size=calculation_ring_buffer_size(adev->loop_delay*16,&(in_test->config));
    min_buffer_size=(TEST_AUDIO_LOOP_MIN_DATA_MS *in_test->config.config.rate*in_test->config.config.channels*2/1000);
    if(size<min_buffer_size){
        size=calculation_ring_buffer_size(TEST_AUDIO_LOOP_MIN_DATA_MS,&(in_test->config));
    }

    ring_buf=ring_buffer_init(size,adev->loop_delay*in_test->config.config.rate*in_test->config.config.channels*2/1000);
    if(NULL==ring_buf){
        goto error;
    }

    in_test->state=true;
    in_test->ring_buf=ring_buf;

    select_devices_new(adev->dev_ctl,adev->in_devices,1);
    select_devices_new(adev->dev_ctl,adev->out_devices,0);

    out_test->isloop=true;
    ret = loop_devices_test_config((struct audiotest_config *)&(out_test->config),parms);
    if(ret < 0 ) {
        goto error;
    }

    out_test->state=true;
    out_test->ring_buf=ring_buf;
    sem_init(&in_test->sem, 0, 1);
    sem_init(&out_test->sem, 0, 1);

    if(pthread_create(&in_test->thread, NULL, in_test_thread, (void *)in_test)){
        LOG_E("audiotest_capture_thread creating failed !!!!");
        goto error;
    }
    if(pthread_create(&out_test->thread, NULL, out_test_thread, (void *)out_test)) {
        LOG_E("audio_out_devices_test creating failed !!!!");
        goto error;
    }

    return 0;
error:
    dev_test->test_mode=TEST_AUDIO_IDLE;
    return -1;
}

static int audio_loop_devices_test_stop(struct tiny_audio_device *adev,struct str_parms *parms){
    struct dev_test_t *dev_test = (struct dev_test_t *) &adev->dev_test;
    struct in_test_t *in_test = (struct in_test_t *) &dev_test->in_test;
    struct out_test_t *out_test = (struct out_test_t *) &dev_test->out_test;
    int ret = -1;
    if(TEST_AUDIO_IN_OUT_LOOP != dev_test->test_mode){
        return -1;
    }
    in_test->state=0;
    out_test->state=0;
    ret = pthread_join(in_test->thread, NULL);
    ret = pthread_join(out_test->thread, NULL);
    LOG_E("audio_loop_devices_test_stop thread destory ret is %d", ret);
    sem_destroy(&in_test->sem);
    sem_destroy(&out_test->sem);
    dev_test->test_mode=TEST_AUDIO_IDLE;

    ring_buffer_free(out_test->ring_buf);
    in_test->ring_buf=NULL;
    out_test->ring_buf=NULL;
    return 0;
}

static int audio_in_devices_test_start(struct tiny_audio_device *adev,struct str_parms *parms){
    struct dev_test_t *dev_test = (struct dev_test_t *) &adev->dev_test;
    struct in_test_t *in_test = (struct in_test_t *) &dev_test->in_test;
    int ret = -1;

    if(TEST_AUDIO_IDLE != dev_test->test_mode){
        LOG_E("audio_in_devices_test mode:%d",dev_test->test_mode);
        return -1;
    }else{
        dev_test->test_mode=TEST_AUDIO_IN_DEVICES;
    }

    in_test->dev_test=dev_test;
    dev_test->adev=adev;
    in_test->isloop=false;
    ret = in_devices_test_config((struct audiotest_config *)&(in_test->config),parms);
    if(ret < 0 ) {
        LOG_D("%s %d",__func__,__LINE__);
        goto error;
    }
    in_test->state=true;
    sem_init(&in_test->sem, 0, 1);
    if(pthread_create(&in_test->thread, NULL, in_test_thread, (void *)in_test)){
        LOG_E("audiotest_capture_thread creating failed !!!!");
        goto error;
    }
    return 0;
error:
    dev_test->test_mode=TEST_AUDIO_IDLE;
    return -1;
}

int audio_in_devices_test_stop(struct tiny_audio_device *adev,struct str_parms *parms){
    struct dev_test_t *dev_test = (struct dev_test_t *) &adev->dev_test;
    struct in_test_t *in_test = (struct in_test_t *) &dev_test->in_test;
    adev->dev_test.adev=adev;
    int ret = -1;
    if(TEST_AUDIO_IN_DEVICES != dev_test->test_mode){
        return -1;
    }
    in_test->state=0;
    ret = pthread_join(in_test->thread, NULL);
    LOG_E("audio_in_devices_test_stop thread destory ret is %d", ret);
    sem_destroy(&in_test->sem);

    dev_test->test_mode=TEST_AUDIO_IDLE;
    return 0;
}

static int audio_out_devices_test_start(struct tiny_audio_device *adev,struct str_parms *parms){
    struct dev_test_t *dev_test = (struct dev_test_t *) &adev->dev_test;
    struct out_test_t *out_test = (struct out_test_t *) &dev_test->out_test;
    adev->dev_test.adev=adev;
    int ret = -1;

    if(TEST_AUDIO_IDLE != dev_test->test_mode){
        LOG_E("audio_out_devices_test mode:%d",dev_test->test_mode);
        return -1;
    }else{
        dev_test->test_mode=TEST_AUDIO_OUT_DEVICES;
    }

    out_test->dev_test=dev_test;
    dev_test->adev=adev;
    out_test->isloop=false;
    ret = out_devices_test_config((struct audiotest_config *)&(out_test->config),parms);
    if(ret < 0 ) {
        goto error;
    }
    out_test->state=true;
    sem_init(&out_test->sem, 0, 1);
    if(pthread_create(&out_test->thread, NULL, out_test_thread, (void *)out_test)) {
        LOG_E("audio_out_devices_test creating failed !!!!");
        goto error;
    }
    return 0;
error:
    dev_test->test_mode=TEST_AUDIO_IDLE;
    return -1;
}

static int audio_out_devices_test_stop(struct tiny_audio_device *adev,struct str_parms *parms){
    struct dev_test_t *dev_test = (struct dev_test_t *) &adev->dev_test;
    struct out_test_t *out_test = (struct out_test_t *) &dev_test->out_test;
    int ret = -1;
    if(TEST_AUDIO_OUT_DEVICES != dev_test->test_mode){
        return -1;
    }

    out_test->state=0;
    ret = pthread_join(out_test->thread, NULL);
    LOG_E("audio_out_devices_test_stop thread destory ret is %d", ret);
    sem_destroy(&out_test->sem);

    dev_test->test_mode=TEST_AUDIO_IDLE;
    return 0;
}

int audio_out_devices_test(void * adev,struct str_parms *parms,bool is_start, char * val){
    int ret=-1;
    LOG_D("%s %d",__func__,__LINE__);
    if(true==is_start){
        ret=audio_out_devices_test_start(adev,parms);
    }else{
        ret=audio_out_devices_test_stop(adev,parms);
    }
    return ret;
error:
    return -1;
}

int audio_in_devices_test(void * adev,struct str_parms *parms,bool is_start, char * val){
    int ret=-1;
    LOG_D("%s %d",__func__,__LINE__);
    if(true==is_start){
        ret=audio_in_devices_test_start(adev,parms);
    }else{
        ret=audio_in_devices_test_stop(adev,parms);
    }
    return ret;
error:
    return -1;
}

int sprd_audioloop_test(void * dev,struct str_parms *parms,bool is_start, char * val){
    int ret=-1;
    struct tiny_audio_device *adev=(struct tiny_audio_device *)dev;
    LOG_D("%s %d",__func__,__LINE__);
    if(true==is_start){
        force_all_standby(adev);
        set_usecase(adev->dev_ctl, UC_LOOP, true);
        ret=audio_loop_devices_test_start(adev,parms);
    }else{
        ret=audio_loop_devices_test_stop(adev,parms);
        set_usecase(adev->dev_ctl, UC_LOOP, false);
    }
    return ret;
error:
    return -1;
}
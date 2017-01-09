/*
* Copyright (C) 2010 The Android Open Source Project
* Copyright (C) 2012-2015, The Linux Foundation. All rights reserved.
*
* Not a Contribution, Apache license notifications and license are retained
* for attribution purposes only.
*
* Licensed under the Apache License, Version 2.0 (the "License");
* you may not use this file except in compliance with the License.
* You may obtain a copy of the License at
*
*      http://www.apache.org/licenses/LICENSE-2.0
*
* Unless required by applicable law or agreed to in writing, software
* distributed under the License is distributed on an "AS IS" BASIS,
* WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
* See the License for the specific language governing permissions and
* limitations under the License.
*/

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
    struct stream_test_t *in_test=(struct stream_test_t *)args;
    struct tiny_stream_in * in_stream=(struct tiny_stream_out *)in_test->stream;
    struct tiny_audio_device *adev=(struct tiny_audio_device *)in_stream->dev;
    char *buffer=NULL;
    unsigned int size;
    int ret = 0;

    LOG_I("in_test_thread wait");
    sem_wait(&in_test->sem);
    LOG_I("in_test_thread running");

    if(NULL==in_stream){
        LOG_E("in_test_thread stream error");
        goto ERR;
    }

    size =in_stream->config->period_count * in_stream->config->period_size *2 *in_stream->config->channels;
    buffer = (char *)malloc(size);
    if (!buffer) {
        LOG_W("in_test_thread Unable to allocate %d bytes\n", size);
        goto ERR;
    }

    memset(buffer,0,size);

    while((!in_test->is_exit) && is_usecase(adev->dev_ctl, UC_AUDIO_TEST)){
        ret=in_stream->stream.read(in_stream, buffer, size);
        if (ret<0){
            LOG_W("in_test_thread: no data read ret:%d",ret);
            usleep(20000);
        }else{
            if(in_test->fd){
                write(in_test->fd,buffer,size);
            }else if(in_test->ring_buf){
                producer_proc(in_test->ring_buf, (unsigned char *)buffer, (unsigned int)size);
            }
        }
    }

ERR:

    if(buffer){
        free(buffer);
        buffer=NULL;
    }

    if(in_test->fd >0){
        close(in_test->fd);
        in_test->fd=-1;
    }

    in_test->ring_buf=NULL;

    if(NULL!=in_stream){
        do_input_standby(in_stream);
    }
    LOG_E("in_test_thread exit");

    set_usecase(adev->dev_ctl, UC_AUDIO_TEST, false);

    return NULL;
}


static void *out_test_thread(void *args)
{
    struct stream_test_t *out_test=(struct stream_test_t *)args;
    struct tiny_stream_out *out_stream=(struct tiny_stream_out *)out_test->stream;
    struct tiny_audio_device *adev=(struct tiny_audio_device *)out_stream->dev;

    char *buffer=NULL;
    char *malloc_buffer=NULL;
    unsigned int size;
    int num_read;
    int ret = 0;

    buffer = NULL;
    num_read = 0;
    size = 0;

    LOG_I("playback_thread wait");
    sem_wait(&out_test->sem);
    LOG_I("playback_thread running");

    if(NULL==out_stream){
        LOG_E("in_test_thread stream error");
        goto ERR;
    }

    size = out_stream->config->period_count * out_stream->config->period_size *2 *out_stream->config->channels;
    malloc_buffer = malloc(size);
    if (!malloc_buffer) {
        LOG_W("Unable to allocate %d bytes\n", size);
        goto ERR;
    }

    while((!out_test->is_exit) && is_usecase(adev->dev_ctl, UC_AUDIO_TEST)){
        if(NULL!=out_test->ring_buf){
            buffer=malloc_buffer;
            num_read=ring_buffer_get(out_test->ring_buf, (void *)buffer, size);
        }else if(out_test->fd>0){
            buffer=malloc_buffer;
            LOG_D("out_test_thread begin read:%d",out_test->fd);
            num_read=read(out_test->fd,buffer,size);
            if(num_read<=0){
                LOG_D("out_test_thread begin lseek:%d",out_test->fd);
                lseek(out_test->fd,0,SEEK_SET);
                num_read = read(out_test->fd, buffer, size);
            }
            LOG_D("out_test_thread return :%d",num_read);
        }else{
            buffer = s_pcmdata_mono;
            num_read = sizeof(s_pcmdata_mono);
        }

        if(num_read > 0){
            if (true==out_stream->standby){
                ret = start_output_stream(out_stream);
                 if (ret != 0) {
                        goto ERR;
                }
                out_stream->standby = false;
            }
            ret = normal_out_write(out_stream, buffer, num_read);
        }
    };

ERR:

    if(malloc_buffer)  {
        free(malloc_buffer);
        malloc_buffer = NULL;
    }

    if(NULL!=out_stream){
        do_output_standby(out_stream);
    }

    out_test->stream=NULL;

    if(out_test->fd > 0) {
        close(out_test->fd);
        out_test->fd = -1;
    }

    out_test->ring_buf=NULL;

    set_usecase(adev->dev_ctl, UC_AUDIO_TEST, false);

    LOG_E("audiotest_playback_thread exit");
    return NULL;
}


static void * open_out_devices_test(struct tiny_audio_device *adev,int devices){
    struct tiny_stream_out * out_stream=NULL;
    int ret=-1;
    out_stream = get_output_stream(adev,AUDIO_HW_APP_PRIMARY);
    pthread_mutex_lock(&out_stream->lock);
    if(NULL!=out_stream){
        out_stream->devices=devices;
    }
    if(NULL!=out_stream->standby_fun){
        out_stream->standby_fun(out_stream->dev,out_stream, out_stream->audio_app_type);
    }
    pthread_mutex_unlock(&out_stream->lock);
    return out_stream;
}

static void * open_in_devices_test(struct tiny_audio_device *adev,int devices,int sample_rate,int channel){
    struct tiny_stream_in * in_stream=NULL;
    struct audio_config config;
    int ret;
    if(channel==2){
        config.channel_mask=AUDIO_CHANNEL_IN_STEREO;
    }else{
        config.channel_mask=AUDIO_CHANNEL_IN_MONO;
    }
    config.sample_rate=sample_rate;
    config.format=AUDIO_FORMAT_PCM_16_BIT;

    if(is_usecase(adev->dev_ctl, UC_MM_RECORD)){
        LOG_W("open_in_devices_test: UC_MM_RECORD");
        return NULL;
    }

    pthread_mutex_lock(&adev->lock);
    if(list_empty(&adev->active_input_list)){
        pthread_mutex_unlock(&adev->lock);
        ret=adev_open_input_stream(adev,0,0,&config,&in_stream,AUDIO_INPUT_FLAG_NONE,NULL,AUDIO_SOURCE_MIC);
        if(0!=ret){
            LOG_E("open_in_devices_test failed");
            return NULL;
        }
    }else{
        pthread_mutex_unlock(&adev->lock);
        return NULL;
    }

    pthread_mutex_lock(&in_stream->lock);
    in_stream->devices=devices;
    pthread_mutex_unlock(&in_stream->lock);
    return in_stream;
}

static int parse_audio_devtest_config(struct str_parms *parms,struct  dev_test_config *config,int mode){
    int ret=0;
    char value[256]={0};

    if((TEST_AUDIO_OUT_DEVICES == mode) || (TEST_AUDIO_IN_OUT_LOOP == mode)){
        ret = str_parms_get_str(parms,"test_stream_route", value, sizeof(value));
        if(ret >= 0){
            config->out_devices=strtoul(value,NULL,0);
        }else{
            config->out_devices=0;
        }
    }

    if((TEST_AUDIO_IN_DEVICES == mode) || (TEST_AUDIO_IN_OUT_LOOP == mode)){
        ret = str_parms_get_str(parms,"test_in_stream_route", value, sizeof(value));
        if(ret >= 0){
            config->in_devices=strtoul(value,NULL,16);
        }else{
            config->in_devices=0;
        }
    }

    if(TEST_AUDIO_IN_OUT_LOOP == mode){
        ret = str_parms_get_str(parms,"delay", value, sizeof(value));
        if(ret >= 0){
            config->delay=strtoul(value,NULL,10);
            LOG_I("parse_audio_devtest_config delay:%d",config->delay);
        }else{
            config->delay=0;
        }
    }

    ret = str_parms_get_str(parms,"sample_rate", value, sizeof(value));
    if(ret >= 0){
        config->sample_rate=strtoul(value,NULL,10);
    }else{
        config->sample_rate=48000;
    }

    ret = str_parms_get_str(parms,"channel", value, sizeof(value));
    if(ret >= 0){
        config->channel=strtoul(value,NULL,10);
    }else{
        config->channel=2;
    }

    ret = str_parms_get_str(parms,"filepath", value, sizeof(value));
    if(ret >= 0){
        if(TEST_AUDIO_IN_DEVICES == mode){
            config->fd= open(value, O_WRONLY);
        }else{
            config->fd= open(value, O_RDONLY);
        }
        if(config->fd < 0) {
            LOG_W("open:%s failed",value);
        }
    }else{
        config->fd  = -1;
    }

    return 0;
}

int audio_loop_devices_test_start(struct tiny_audio_device *adev,struct str_parms *parms){
    int ret=0;

    struct dev_test_t *dev_test = (struct dev_test_t *) &adev->dev_test;
    struct stream_test_t *in_test = (struct stream_test_t *) &dev_test->in_test;
    struct stream_test_t *out_test = (struct stream_test_t *) &dev_test->out_test;
    int min_buffer_size=0;
    int size=0;
    struct pcm_config *config=NULL;
    struct tiny_stream_out * out_stream=NULL;
    struct tiny_stream_out * in_stream=NULL;
    struct  dev_test_config dev_config;

    if(is_usecase(adev->dev_ctl, UC_AUDIO_TEST)){
        LOG_W("audio_loop_devices_test_start aready in test mode");
        return -1;
    }

    ret=parse_audio_devtest_config(parms,&dev_config,TEST_AUDIO_IN_OUT_LOOP);
    if(ret<0){
        goto error;
    }

    out_stream = open_out_devices_test(adev,dev_config.out_devices);
    if(out_stream==NULL){
        LOG_E("audio_loop_devices_test_start failed:%d",__LINE__);
        goto error;
    }

    config=out_stream->config;

    in_stream = open_in_devices_test(adev,dev_config.in_devices,
        config->rate,config->channels);
    if(in_stream==NULL){
        LOG_E("audio_loop_devices_test_start failed:%d",__LINE__);
        goto error;
    }

    set_usecase(adev->dev_ctl, UC_AUDIO_TEST, true);

    size=calculation_ring_buffer_size(0,config);
    min_buffer_size=(TEST_AUDIO_LOOP_MIN_DATA_MS *config->rate*config->channels*2/1000);
    if(size<min_buffer_size){
        size=calculation_ring_buffer_size(TEST_AUDIO_LOOP_MIN_DATA_MS,&(config));
    }

    if(dev_test->ring_buf!=NULL){
        ring_buffer_free(dev_test->ring_buf);
        dev_test->ring_buf=NULL;
    }

    dev_test->ring_buf=ring_buffer_init(size,dev_config.delay*config->rate*config->channels*2/1000);
    if(NULL==dev_test->ring_buf){
        goto error;
    }

    in_test->is_exit=false;
    in_test->ring_buf=dev_test->ring_buf;
    in_test->stream=in_stream;

    out_test->is_exit=false;
    out_test->ring_buf=dev_test->ring_buf;
    out_test->stream=out_stream;

    sem_init(&in_test->sem, 0, 1);
    sem_init(&out_test->sem, 0, 1);

    LOG_I("LOOP out:%p %p in:%p %p",out_test->stream,out_stream->pcm,in_test->stream,in_stream->pcm)
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

    if(NULL!=dev_test->ring_buf){
        ring_buffer_free(dev_test->ring_buf);
        dev_test->ring_buf=NULL;
    }

    set_usecase(adev->dev_ctl, UC_AUDIO_TEST, false);
    return -1;
}

static int audio_loop_devices_test_stop(struct tiny_audio_device *adev,struct str_parms *parms){
    struct dev_test_t *dev_test = (struct dev_test_t *) &adev->dev_test;
    struct stream_test_t *in_test = (struct stream_test_t *) &dev_test->in_test;
    struct stream_test_t *out_test = (struct stream_test_t *) &dev_test->out_test;
    int ret = -1;

    if(false==is_usecase(adev->dev_ctl, UC_AUDIO_TEST)){
        LOG_W("audio_loop_devices_test_stop not in test mode");
        return 0;
    }
    in_test->is_exit=true;
    out_test->is_exit=true;
    LOG_I("audio_loop_devices_test_stop thread destory %d",__LINE__);
    ret = pthread_join(in_test->thread, NULL);
    LOG_I("audio_loop_devices_test_stop thread destory %d",__LINE__);
    ret = pthread_join(out_test->thread, NULL);
    LOG_I("audio_loop_devices_test_stop thread destory %d",__LINE__);
    sem_destroy(&in_test->sem);
    sem_destroy(&out_test->sem);

    if(NULL!=in_test->stream){
        adev->hw_device.close_input_stream(adev, in_test->stream);
        in_test->stream=NULL;
    }

    ring_buffer_free(dev_test->ring_buf);
    dev_test->ring_buf=NULL;
    set_usecase(adev->dev_ctl, UC_AUDIO_TEST, false);
    return 0;
}

static int audio_in_devices_test_start(struct tiny_audio_device *adev,struct str_parms *parms){
    struct dev_test_t *dev_test = (struct dev_test_t *) &adev->dev_test;
    struct stream_test_t *in_test = (struct stream_test_t *) &dev_test->in_test;
    struct tiny_stream_in * in_stream=NULL;
    struct dev_test_config config;
    int ret = -1;

    if(is_usecase(adev->dev_ctl, UC_AUDIO_TEST)){
        LOG_W("audio_loop_devices_test_start aready in test mode");
        return -1;
    }

    ret=parse_audio_devtest_config(parms,&config,TEST_AUDIO_IN_DEVICES);
    if(ret<0){
        goto error;
    }
    in_test->fd=config.fd;

    in_stream = open_in_devices_test(adev,config.in_devices,config.sample_rate,config.channel);
    if(NULL==in_stream){
        LOG_E("audio_in_devices_test_start failed:%d",__LINE__);
        goto error;
    }

    set_usecase(adev->dev_ctl, UC_AUDIO_TEST, true);

    in_test->stream=in_stream;
    in_test->is_exit=false;
    in_test->ring_buf=NULL;
    sem_init(&in_test->sem, 0, 1);

    if(pthread_create(&in_test->thread, NULL, in_test_thread, (void *)in_test)){
        LOG_E("in_test_thread creating failed !!!!");
        goto error;
    }
    return 0;
error:

    set_usecase(adev->dev_ctl, UC_AUDIO_TEST, false);
    if(in_test->fd){
        close(in_test->fd);
        in_test->fd=-1;
    }

    return -1;
}

int audio_in_devices_test_stop(struct tiny_audio_device *adev,struct str_parms *parms){
    struct dev_test_t *dev_test = (struct dev_test_t *) &adev->dev_test;
    struct stream_test_t *in_test = (struct stream_test_t *) &dev_test->in_test;
    int ret = -1;

    if(false==is_usecase(adev->dev_ctl, UC_AUDIO_TEST)){
        LOG_W("audio_in_devices_test_stop failed");
        return -1;
    }

    in_test->is_exit=true;
    ret = pthread_join(in_test->thread, NULL);
    LOG_E("audio_in_devices_test_stop thread destory ret is %d", ret);
    sem_destroy(&in_test->sem);
    if(NULL!=in_test->stream){
        adev->hw_device.close_input_stream(adev, in_test->stream);
        in_test->stream=NULL;
    }
    set_usecase(adev->dev_ctl, UC_AUDIO_TEST, false);
    return 0;
}

static int audio_out_devices_test_start(struct tiny_audio_device *adev,struct str_parms *parms){
    struct dev_test_t *dev_test = (struct dev_test_t *) &adev->dev_test;
    struct stream_test_t *out_test = (struct stream_test_t *) &dev_test->out_test;
    struct tiny_stream_out * out_stream=NULL;
    int ret=-1;
    struct  dev_test_config config;

    if(is_usecase(adev->dev_ctl, UC_AUDIO_TEST)){
        LOG_W("audio_loop_devices_test_start aready in test mode");
        return -1;
    }

    ret=parse_audio_devtest_config(parms,&config,TEST_AUDIO_OUT_DEVICES);
    if(ret<0){
        goto error;
    }

    out_test->fd  = config.fd;

    out_stream = open_out_devices_test(adev,config.out_devices);
    if(NULL==out_stream){
        LOG_E("audio_out_devices_test_start failed:%d",__LINE__);
        goto error;
    }

    set_usecase(adev->dev_ctl, UC_AUDIO_TEST, true);

    out_test->stream=out_stream;
    out_test->is_exit=false;
    out_test->ring_buf=NULL;
    sem_init(&out_test->sem, 0, 1);

    if(pthread_create(&out_test->thread, NULL, out_test_thread, (void *)out_test)) {
        LOG_E("audio_out_devices_test creating failed !!!!");
        goto error;
    }
    return 0;
error:
    set_usecase(adev->dev_ctl, UC_AUDIO_TEST, false);

    if(out_test->fd){
        close(out_test->fd);
        out_test->fd=-1;
    }

    return -1;
}

static int audio_out_devices_test_stop(struct tiny_audio_device *adev,struct str_parms *parms){
    struct dev_test_t *dev_test = (struct dev_test_t *) &adev->dev_test;
    struct stream_test_t *out_test = (struct stream_test_t *) &dev_test->out_test;
    int ret = -1;

    if(false==is_usecase(adev->dev_ctl, UC_AUDIO_TEST)){
        LOG_W("audio_out_devices_test_stop failed");
        return -1;
    }

    out_test->is_exit=true;
    ret = pthread_join(out_test->thread, NULL);
    LOG_E("audio_out_devices_test_stop thread destory ret is %d", ret);
    sem_destroy(&out_test->sem);

    set_usecase(adev->dev_ctl, UC_AUDIO_TEST, false);
    return 0;
}

int audio_out_devices_test(void * dev,struct str_parms *parms,bool is_start, char * val){
    int ret=-1;
    struct tiny_audio_device *adev=(struct tiny_audio_device *)dev;
    bool support_24bits=adev->dev_ctl->config.support_24bits;
    LOG_D("%s %d",__func__,__LINE__);
    if(true==is_start){
        adev->dev_ctl->config.support_24bits=false;
        ret=audio_out_devices_test_start(adev,parms);
        if(ret!=0){
            adev->dev_ctl->config.support_24bits=support_24bits;
        }
    }else{
        ret=audio_out_devices_test_stop(adev,parms);
        parse_audio_config(adev->dev_ctl);
    }
    return ret;
error:
    return -1;
}

int audio_in_devices_test(void * dev,struct str_parms *parms,bool is_start, char * val){
    int ret=-1;
    struct tiny_audio_device *adev=(struct tiny_audio_device *)dev;
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
    bool support_24bits=adev->dev_ctl->config.support_24bits;
    LOG_D("%s %d",__func__,__LINE__);
    if(true==is_start){
        adev->dev_ctl->config.support_24bits=false;
        ret=audio_loop_devices_test_start(adev,parms);
        if(ret!=0){
            adev->dev_ctl->config.support_24bits=support_24bits;
        }
    }else{
        ret=audio_loop_devices_test_stop(adev,parms);
        parse_audio_config(adev->dev_ctl);
    }
    return ret;
error:
    return -1;
}

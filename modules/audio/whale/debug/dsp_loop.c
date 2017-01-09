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

#include <sys/wait.h>
#include <sys/mman.h>
#include <sys/time.h>
#include <sys/select.h>
#include <fcntl.h>
#include <signal.h>
#include <semaphore.h>
#include "audio_debug.h"
#include "audio_hw.h"
#include "audio_control.h"
#define LOG_TAG "audio_hw_dsp_loop"

#define TEST_AUDIO_LOOP_MIN_DATA_MS 500
#define DSPLOOP_FRAME_SIZE 488
extern unsigned int producer_proc(struct ring_buffer *ring_buf,unsigned char * buf,unsigned int size);

static void *get_dsploop_ap_param(AUDIO_PARAM_T  *audio_param,audio_devices_t in_devices,audio_devices_t out_devices){
    int id=-1;
    struct audio_param_res  res;
    struct audio_ap_param *ap_param=NULL;
    int offset=0;

    res.net_mode=AUDIO_NET_LOOP;
    res.usecase=UC_LOOP;
    res.in_devices=in_devices;
    res.out_devices=out_devices;

    id=get_loopback_param(out_devices,in_devices);
    if(id<0)
        return NULL;

    ap_param=(struct audio_ap_param *)(audio_param->param[SND_AUDIO_PARAM_RECORD_PROCESS_PROFILE].data);

    return  &(ap_param->loop[id-PROFILE_MODE_LOOP_Handset_MainMic]);
}

static void *dsp_loop_rx_thread(void *args){
    struct loop_ctl_t *in=(struct loop_ctl_t *)args;
    struct pcm *pcm=in->pcm;
    struct tiny_audio_device *adev=(struct tiny_audio_device *)in->dev;
    char *buffer=NULL;
    int num_read = 0;
    int size=0;
    int bytes_read=0;

    pthread_mutex_lock(&(in->lock));
    if(NULL==in->pcm){
        LOG_E("dsp_loop_rx_thread pcm is null");
        goto ERR;
    }

    size = pcm_frames_to_bytes(pcm, pcm_get_buffer_size(pcm));
    size =size/DSPLOOP_FRAME_SIZE;
    size++;
    size*=DSPLOOP_FRAME_SIZE;

    buffer = (char *)malloc(size);
    if (!buffer) {
        LOG_E("Unable to allocate %d bytes\n", size);
        goto ERR;
    }

    sem_wait(&in->sem);
    while(in->is_exit==false){
        num_read=pcm_mmap_read(pcm, buffer, size);
        LOG_D("dsp_loop_rx_thread read:0x%x req:0x%x",num_read,size);
        if (!num_read){
            bytes_read += size;
            if(adev->dev_ctl->config.dump.record.dsploop.hex_fd>0){
                write(adev->dev_ctl->config.dump.record.dsploop.hex_fd,buffer,size);
            }

            producer_proc(in->ring_buf, (unsigned char *)buffer, (unsigned int)size);
            LOG_I("dsp_loop_rx_thread capture 0x%x total:0x%x",size,bytes_read);
        }else{
            LOG_W("dsp_loop_rx_thread: no data read num_read:%d",num_read);
            usleep(100*1000);
        }
    }
    in->state=false;
    pthread_mutex_unlock(&(in->lock));
    LOG_I("dsp_loop_rx_thread exit success");
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
    in->state=false;
    pthread_mutex_unlock(&(in->lock));

    LOG_E("dsp_loop_rx_thread exit err");
    return NULL;
}

static void *dsp_loop_tx_thread(void *args){
    struct loop_ctl_t *out=(struct loop_ctl_t *)args;
    struct pcm *pcm=out->pcm;
    struct tiny_audio_device *adev=(struct tiny_audio_device *)out->dev;
    char *buffer=NULL;
    int num_read = 0;
    int size=0;
    int ret=-1;
    int write_count=0;
    int no_data_count=0;

    pthread_mutex_lock(&(out->lock));
    out->state=true;
    if(NULL==out->pcm){
        LOG_E("dsp_loop_tx_thread pcm is null");
        goto ERR;
    }

    size = pcm_frames_to_bytes(pcm, pcm_get_buffer_size(pcm));
    size =size/DSPLOOP_FRAME_SIZE;
    size++;
    size*=DSPLOOP_FRAME_SIZE;

    buffer = (char *)malloc(size);
    if (!buffer) {
        LOG_E("dsp_loop_tx_thread Unable to allocate %d bytes\n", size);
        goto ERR;
    }

    sem_wait(&out->sem);
    while(out->is_exit==false){
        num_read=ring_buffer_get(out->ring_buf, (void *)buffer, size);
        LOG_I("dsp_loop_tx_thread read:0x%x req:0x%x",num_read,size);

        if(num_read > 0){
            no_data_count=0;
            ret = pcm_mmap_write(pcm, buffer,num_read);
            if (ret) {
                int ret = 0;
                LOG_E("dsp_loop_tx_thread Error playing sample:%s\n",pcm_get_error(pcm));
                ret = pcm_close(pcm);
                out->pcm = pcm_open(adev->dev_ctl->cards.s_tinycard, adev->dev_ctl->pcm_handle.playback_devices[AUD_PCM_DSP_LOOP],
                        PCM_OUT | PCM_MMAP | PCM_NOIRQ |PCM_MONOTONIC , &adev->dev_ctl->pcm_handle.play[AUD_PCM_DSP_LOOP]);
                if (!pcm || !pcm_is_ready(out->pcm)) {
                    LOG_E("dsp_loop_tx_thread Unable to open PCM device %u (%s)\n",
                          0, pcm_get_error(out->pcm));
                    goto ERR;
                }
                pcm=out->pcm;
            }else{
                write_count+=num_read;
                LOG_I("dsp_loop_tx_thread write:0x%x total:0x%x",num_read,write_count);
                if(adev->dev_ctl->config.dump.playback.dsploop.hex_fd>0){
                    write(adev->dev_ctl->config.dump.playback.dsploop.hex_fd,buffer,num_read);
                }
            }
        }else{
            usleep(100*1000);
            no_data_count++;
            LOG_I("dsp_loop_tx_thread no data read");
        }
    }

    if(buffer){
        free(buffer);
        buffer=NULL;
    }

    out->state=false;
    pthread_mutex_unlock(&(out->lock));
    LOG_I("dsp_loop_tx_thread exit success");
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
    LOG_E("dsp_loop_tx_thread exit err");
    return NULL;
}

static int audio_dsp_loop_standby(struct tiny_audio_device *adev,void * loop_stream,AUDIO_HW_APP_T type)
{
    struct tiny_stream_out *dsp_loop=(struct tiny_stream_in *)loop_stream;
    struct loop_ctl_t *ctl=NULL;
    struct dsp_loop_t *loop=&adev->loop_ctl;
    int wait_count=30;

    LOG_I("audio_dsp_loop_stop enter");
    set_mdg_mute(adev->dev_ctl,UC_CALL,true);

    if(false==loop->state){
        LOG_W("audio_dsp_loop_stop failed, loop is not work");
        return -1;
    }

    loop->in.is_exit=true;
    loop->out.is_exit=true;

    /* first:close record pcm devices */
    pthread_join(loop->in.thread,  NULL);
    usleep(100*1000);

    LOG_I("audio_dsp_loop_stop %d",__LINE__);
    loop->out.is_exit=true;
    pthread_join(loop->out.thread, NULL);

    while(wait_count--){
        if(false==loop->out.state){
            break;
        }
        usleep(300*1000);
    }

    LOG_I("audio_dsp_loop_stop %d",__LINE__);
    ctl=&loop->in;
    pthread_mutex_lock(&(ctl->lock));
    ctl->is_exit=true;
    ctl->dev=NULL;
    if(ctl->pcm!=NULL){
        pcm_close(ctl->pcm);
        ctl->pcm=NULL;
        LOG_I("audio_dsp_loop_stop %d",__LINE__);
    }
    pthread_mutex_unlock(&(ctl->lock));
    ctl=&loop->out;
    pthread_mutex_lock(&(ctl->lock));
    ctl->is_exit=true;
    ctl->dev=NULL;
    if(ctl->pcm!=NULL){
        pcm_close(ctl->pcm);
        ctl->pcm=NULL;
        LOG_I("audio_dsp_loop_stop %d",__LINE__);
    }
    pthread_mutex_unlock(&(ctl->lock));

    if(loop->out.ring_buf!=NULL){
        ring_buffer_free(loop->out.ring_buf);
    }

    switch_vbc_iis_route(adev->dev_ctl,UC_LOOP,false);
    loop->state=false;

    dsp_loop->standby=true;
    adev->stream_status &=~(1<<dsp_loop->audio_app_type);

    pthread_mutex_destroy(&(loop->out.lock));
    pthread_mutex_destroy(&(loop->in.lock));

    set_usecase(adev->dev_ctl,UC_LOOP,false);
    set_audioparam(adev->dev_ctl,PARAM_USECASE_CHANGE,NULL,false);
    adev->is_dsp_loop = false;
    LOG_I("audio_dsp_loop_standby exit");

    return 0;
}

int audio_dsp_loop_open(struct tiny_audio_device *adev){
    struct tiny_stream_out *dsp_loop=NULL;
    dsp_loop=(struct tiny_stream_out *)calloc(1,
                                sizeof(struct tiny_stream_out));
    if(NULL==dsp_loop){
        LOG_E("audio_dsp_loop_open calloc falied");
        goto error;
    }

    LOG_I("audio_dsp_loop_open out:%p",dsp_loop);

    if (pthread_mutex_init(&(dsp_loop->lock), NULL) != 0) {
        LOG_E("Failed pthread_mutex_init dsp_loop->lock,errno:%u,%s",
              errno, strerror(errno));
        goto error;
    }

    dsp_loop->dev=adev;
    dsp_loop->standby=false;
    dsp_loop->audio_app_type=AUDIO_HW_APP_DSP_LOOP;
    dsp_loop->standby_fun = audio_dsp_loop_standby;
    audio_add_output(adev,dsp_loop);

    LOG_I("audio_dsp_loop_open success");
    return 0;

error:


    if(NULL!=dsp_loop){
        free(dsp_loop);
    }

    return -1;
}

static int audio_dsp_loop_start(struct tiny_audio_device *adev,struct dsp_loop_t *loop){
    struct ring_buffer *ring_buf=NULL;
    struct pcm_config * in_config=NULL;
    struct tiny_stream_out *dsp_loop=NULL;
    int size=0;
    int min_buffer_size=0;
    int dsp_zero_size=0;
    int delay_size=0;
    int ret=0;
    struct audio_param_res param_res;
    struct dsp_loop_param *loop_ap_param=NULL;

    if(true==loop->state){
        LOG_W("audio_dsp_loop_start failed, loop is working now");
        return -1;
    }

    dsp_loop= get_output_stream(adev,AUDIO_HW_APP_DSP_LOOP);
    if(NULL==dsp_loop){
        LOG_E("audio_dsp_loop_start failed");
        return -1;
    }

    force_all_standby(adev);
   // switch_vbc_iis_route(adev->dev_ctl,UC_LOOP,true);
    ret=set_usecase(adev->dev_ctl,UC_LOOP,true);
    if(ret<0){
        goto error;
    }
    adev->is_dsp_loop = true;
    select_devices_new(adev->dev_ctl,AUDIO_HW_APP_DSP_LOOP,adev->dev_ctl->in_devices,true,false,true);
    select_devices_new(adev->dev_ctl,AUDIO_HW_APP_DSP_LOOP,adev->dev_ctl->out_devices,false,false,true);

    dsp_loop->standby=false;

    in_config=&(adev->dev_ctl->pcm_handle.record[AUD_PCM_DSP_LOOP]);

    loop_ap_param=(struct  dsp_loop_param *)get_dsploop_ap_param(adev->dev_ctl->audio_param,
        adev->dev_ctl->in_devices, adev->dev_ctl->out_devices);
    if(NULL==loop_ap_param){
        LOG_E("audio_dsp_loop_start get_dsploop_ap_param failed: in_devices:0x%x out_devices:%d",
            adev->dev_ctl->in_devices,adev->dev_ctl->out_devices);
        goto error;
    }else{
        LOG_I("audio_dsp_loop_start delay:%dms type:%d",loop_ap_param->delay,loop_ap_param->type);
    }
    size=calculation_ring_buffer_size(loop_ap_param->delay*16,in_config);
    min_buffer_size=(TEST_AUDIO_LOOP_MIN_DATA_MS *in_config->rate*in_config->channels*2/1000);
    if(size<min_buffer_size){
        size=calculation_ring_buffer_size(TEST_AUDIO_LOOP_MIN_DATA_MS,in_config);
    }

    /* dsp process frame buffer size is 122 word(244bytes),
       the delay buffer size must be a multiple of dsp frame buffer */
    delay_size=loop_ap_param->delay*in_config->rate*in_config->channels*2/1000;
    dsp_zero_size=delay_size/DSPLOOP_FRAME_SIZE;
    dsp_zero_size+=1;
    dsp_zero_size*=DSPLOOP_FRAME_SIZE;

    ring_buf=ring_buffer_init(size,dsp_zero_size);
    if(NULL==ring_buf){
        goto error;
    }

    if (pthread_mutex_init(&(loop->in.lock), NULL) != 0) {
        LOG_E("Failed pthread_mutex_init loop->in.lock,errno:%u,%s",
              errno, strerror(errno));
        goto error;
    }

    if (pthread_mutex_init(&(loop->out.lock), NULL) != 0) {
        LOG_E("Failed pthread_mutex_init loop->out.lock,errno:%u,%s",
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

    sem_init(&loop->in.sem, 0, 1);
    sem_init(&loop->out.sem, 0, 1);

    LOG_I("audio_dsp_loop_start out devices:%d rate:%d card:%d",
        adev->dev_ctl->pcm_handle.playback_devices[AUD_PCM_DSP_LOOP],
        adev->dev_ctl->pcm_handle.play[AUD_PCM_DSP_LOOP].rate,
        adev->dev_ctl->cards.s_tinycard);

    /*
        dsp loop test needs to open the record pcm devices first and
        then open the playback pcm devices.
    */
    loop->out.pcm  =
        pcm_open(adev->dev_ctl->cards.s_tinycard, adev->dev_ctl->pcm_handle.playback_devices[AUD_PCM_DSP_LOOP],
        PCM_OUT | PCM_MMAP | PCM_NOIRQ |PCM_MONOTONIC , &adev->dev_ctl->pcm_handle.play[AUD_PCM_DSP_LOOP]);

    if (!pcm_is_ready(loop->out.pcm)) {
        LOG_E("audio_dsp_loop_start:cannot open pcm : %s",
              pcm_get_error(loop->out.pcm));
        goto error;
    }

    LOG_I("audio_dsp_loop_start in devices:%d rate:%d",
        adev->dev_ctl->pcm_handle.record_devices[AUD_PCM_DSP_LOOP],
        adev->dev_ctl->pcm_handle.record[AUD_PCM_DSP_LOOP].rate);

    loop->in.pcm  =
        pcm_open(adev->dev_ctl->cards.s_tinycard, adev->dev_ctl->pcm_handle.record_devices[AUD_PCM_DSP_LOOP],
        PCM_IN |PCM_MMAP | PCM_NOIRQ,  &adev->dev_ctl->pcm_handle.record[AUD_PCM_DSP_LOOP]);

    if (!pcm_is_ready(loop->in.pcm)){
        LOG_E("start_input_stream:cannot open pcm : %s",
              pcm_get_error(loop->in.pcm));
        goto error;
    }

    memcpy(&param_res,&adev->dev_ctl->param_res,sizeof(struct audio_param_res));
    param_res.net_mode=AUDIO_NET_LOOP;

    set_audioparam(adev->dev_ctl,PARAM_USECASE_DEVICES_CHANGE,NULL,false);

    apply_dsploop_control(&(adev->dev_ctl->route.dsploop_ctl),loop_ap_param->type);
    set_mdg_mute(adev->dev_ctl,UC_CALL,false);
    loop->state=true;

    adev->stream_status |=(1<<dsp_loop->audio_app_type);

    if(pthread_create(&loop->out.thread, NULL, dsp_loop_tx_thread, (void *)&(loop->out))) {
        LOG_E("audio_out_devices_test creating tx thread failed !!!!");
        goto error;
    }

    if(pthread_create(&loop->in.thread, NULL, dsp_loop_rx_thread, (void *)&(loop->in))){
        LOG_E("audio_out_devices_test creating rx thread failed !!!!");
        goto error;
    }

    LOG_I("audio_dsp_loop_start sucess");
    return 0;

error:

    adev->stream_status &=~(1<<AUDIO_HW_APP_DSP_LOOP);

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

    dsp_loop->standby=true;

    set_usecase(adev->dev_ctl,UC_LOOP,false);
    set_audioparam(adev->dev_ctl,PARAM_USECASE_CHANGE,NULL,false);
    loop->state=false;

    LOG_I("audio_dsp_loop_start failed");
    return -1;
}

int audio_dsp_loop(void * dev,struct str_parms *parms,bool is_start){
    int ret=0;
    struct tiny_audio_device * adev=(struct tiny_audio_device *)dev;
    struct tiny_stream_out * out = NULL;

    if(true==is_start){

        if(is_usecase(adev->dev_ctl, UC_LOOP)){
            return 0;
        }

        ret=audio_dsp_loop_start(adev,&adev->loop_ctl);
    }else{

        if(false==is_usecase(adev->dev_ctl, UC_LOOP)){
            return 0;
        }

        out = get_output_stream(adev,AUDIO_HW_APP_DSP_LOOP);
        if(out) {
            LOG_I("%s %d out:%p type:%d",__func__,__LINE__,out,out->audio_app_type);
            do_output_standby(out);
        }
    }

    return ret;
}

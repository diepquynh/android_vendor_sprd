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

#define LOG_NDEBUG 0
#include "aud_proc.h"

#include "audio_hw.h"
#include "audio_control.h"
#include "audio_param/audio_param.h"

#include <hardware/hardware.h>
#include <system/audio.h>
#include <hardware/audio.h>
#include <hardware_legacy/power.h>
#include "audio_offload.h"
#include "tinycompress/tinycompress.h"
#include "audio_debug.h"
#include "tinyalsa_util.h"
#ifdef LOCAL_SOCKET_SERVER
#include "audiotester/local_socket.h"
#endif

#include "audio_control.h"
#define LOG_TAG "audio_hw_primary"

extern int ext_control_init(struct tiny_audio_device *adev);
//int enable_device_gain(struct audio_control *ctl, int device, int usecase, int in_device);
int aud_rec_do_process(void *buffer, size_t bytes, void *tmp_buffer,
                              size_t tmp_buffer_bytes);
void force_all_standby(void *adev);
void start_audio_tunning_server(struct tiny_audio_device *adev);

//#include "at_commands_generic.c"

extern int modem_monitor_open(void *arg);

extern int ext_contrtol_process(struct tiny_audio_device *adev,const char *cmd_string);

static int out_deinit_resampler(struct tiny_stream_out *out);
static int out_init_resampler(struct tiny_stream_out *out);
int send_cmd_to_dsp_thread(struct dsp_control_t *agdsp_ctl,int cmd,void * parameter);

static int _pcm_mixer(int16_t *buffer, uint32_t samples);
static ssize_t read_pcm_data(void *stream, void* buffer, size_t bytes);

extern int audio_dsp_loop_open(struct tiny_audio_device *adev);
static int right2left(int16_t *buffer, uint32_t samples){
    int i = samples/2;
    while(i--){
        buffer[2*i+1]=buffer[2*i];
    }
    return 0;
}

static int left2right(int16_t *buffer, uint32_t samples)
{
    int i = samples/2;
    while(i--){
        buffer[2*i]=buffer[2*i+1];
    }
    return 0;
}

static inline size_t audio_stream_in_frame_size_l(const struct audio_stream_in *s)
{
    size_t chan_samp_sz;
    struct tiny_stream_in *in = (struct tiny_stream_in *) s;
    audio_format_t format = s->common.get_format(&s->common);

    if (audio_is_linear_pcm(format)) {
        chan_samp_sz = audio_bytes_per_sample(format);
        return in->config->channels* chan_samp_sz;
    }

    return sizeof(int8_t);
}

static ssize_t _pcm_read(void *stream, void* buffer,
        size_t bytes)
{
    int ret =0;
    struct audio_stream_in *stream_tmp=(struct audio_stream_in *)stream;
    struct tiny_stream_in *in = (struct tiny_stream_in *)stream;
    struct tiny_audio_device *adev = in->dev;
    size_t frames_rq = bytes / audio_stream_in_frame_size_l((const struct audio_stream *)(&stream_tmp->common));

    ret = pcm_mmap_read(in->pcm, buffer,bytes);
    if(adev->dev_ctl->config.dump.record.vbc.hex_fd>0){
        write(adev->dev_ctl->config.dump.record.vbc.hex_fd,buffer,
                               bytes);
    }

    if(0==ret){
        if(in->config->channels==2){
                right2left(buffer, bytes/2);
        }
    }

    return ret;
}


static record_nr_handle init_nr_process(struct audio_stream_in *stream)
{
    struct tiny_stream_in *in = (struct tiny_stream_in *)stream;
    struct tiny_audio_device *adev = in->dev;

    struct audio_record_proc_param *param_data = NULL;
    NR_CONTROL_PARAM_T *nr_control=NULL;

    LOG_I("init_nr_process");

    param_data=(struct audio_record_proc_param *)get_ap_record_param(adev->dev_ctl->audio_param,adev->dev_ctl->in_devices);

    if(NULL==param_data){
      LOG_I("audio record process param is null");
      return false;
    }

    nr_control = &param_data->nr_control;

    LOG_D("init_nr_process nr_switch:%x nr_dgain:%x ns_factor:%x",
        nr_control->nr_switch,nr_control->nr_dgain,nr_control->ns_factor);
    return AudioRecordNr_Init(nr_control, _pcm_read, (void *)in,
                                           in->config->channels);
}

static int pcm_mixer(int16_t *buffer, uint32_t samples)
{
    int i = 0;
    int16_t *tmp_buf = buffer;
    for (i = 0; i < (samples / 2); i++) {
        tmp_buf[i] = (buffer[2 * i + 1] + buffer[2 * i]) / 2;
    }
    return 0;
}



void force_all_standby(void *dev)
{
    struct tiny_stream_out *out;
    struct listnode *item;
    struct listnode *item2;

    struct tiny_audio_device *adev = (struct tiny_audio_device *)dev;
    LOG_I("force_all_standby stream_status:0x%x",adev->stream_status)

    pthread_mutex_lock(&adev->lock);
    if(!list_empty(&adev->active_out_list)){
        list_for_each_safe(item, item2,&adev->active_out_list){
            out = node_to_item(item, struct tiny_stream_out, node);
            if(NULL!=out->standby_fun){
                pthread_mutex_lock(&out->lock);
                out->standby_fun(adev,out,out->audio_app_type);
                pthread_mutex_unlock(&out->lock);
            }
        }
    }
    pthread_mutex_unlock(&adev->lock);

    LOG_I("force_all_standby:exit");
}


int audio_agdsp_reset(void * dev,struct str_parms *parms,bool is_start){
    int ret=0;
    struct tiny_audio_device * adev=(struct tiny_audio_device *)dev;

    if(true==is_start){
        force_all_standby(adev);
        pthread_mutex_lock(&adev->lock);
        adev->call_mode = AUDIO_MODE_NORMAL;
        agdsp_boot();
        pthread_mutex_unlock(&adev->lock);
    }
    return ret;
}

int audio_add_input(struct tiny_audio_device *adev,struct tiny_stream_in *in){
    LOG_I("audio_add_input type:%d in:%p ",in->audio_app_type,in);

    struct listnode *item;
    struct listnode *item2;

    struct tiny_stream_in *in_tmp;
    pthread_mutex_lock(&adev->lock);
    adev->stream_status |=(1<<in->audio_app_type);

    list_for_each_safe(item,item2, (&adev->active_input_list)) {
        in_tmp = node_to_item(item,struct tiny_stream_in, node);
        if (in_tmp == in) {
            LOG_W("audio_add_input:%p type:%d aready in list",in,in->audio_app_type);
            pthread_mutex_unlock(&adev->lock);
            return 0;
        }
    }
    list_add_tail(&adev->active_input_list, &in->node);
    pthread_mutex_unlock(&adev->lock);
    return 0;
}

int audio_del_input(struct tiny_audio_device *adev,struct tiny_stream_in *in)
{
    struct listnode *item;
    struct listnode *item2;
    struct tiny_stream_in *in_tmp;

    LOG_I("audio_del_input type:%d out:%p",in->audio_app_type,in);
    pthread_mutex_lock(&adev->lock);
    adev->stream_status |=(1<<in->audio_app_type);
    if(!list_empty(&adev->active_input_list)){
        list_for_each_safe(item, item2,&adev->active_input_list) {
            in_tmp = node_to_item(item, struct tiny_stream_in, node);
            if (in_tmp == in) {
                LOG_I("audio_del_input:%p type:%d",in,in->audio_app_type);
                list_remove(item);
                break;
            }
        }
    }
    pthread_mutex_unlock(&adev->lock);
    return 0;
}

int audio_add_output(struct tiny_audio_device *adev,struct tiny_stream_out *out)
{
    struct listnode *item;
    struct listnode *item2;

    struct tiny_stream_out *out_tmp;

    LOG_I("audio_add_output type:%d out:%p %p",out->audio_app_type,out,out->node);

    pthread_mutex_lock(&adev->lock);
    adev->stream_status |=(1<<out->audio_app_type);
    list_for_each_safe(item, item2,(&adev->active_out_list)){
        out_tmp = node_to_item(item, struct tiny_stream_out, node);
        if (out_tmp == out) {
            LOG_W("audio_add_output:%p type:%d aready in list",out,out->audio_app_type);
            pthread_mutex_unlock(&adev->lock);
            return 0;
        }
    }

    list_add_tail(&adev->active_out_list, &out->node);
    pthread_mutex_unlock(&adev->lock);
    return 0;
}

int audio_del_output(struct tiny_audio_device *adev,struct tiny_stream_out *out)
{
    struct listnode *item;
    struct listnode *item2;
    struct tiny_stream_out * out_tmp = NULL;
    LOG_I("audio_del_output type:%d out:%p",out->audio_app_type,out);

    pthread_mutex_lock(&adev->lock);
    if(!list_empty(&adev->active_out_list)){
        LOG_I("audio_del_output out:%p %p %p %p",item,item2,&adev->active_out_list,adev->active_out_list.next);
            list_for_each_safe(item, item2,&adev->active_out_list){
            out_tmp = node_to_item(item, struct tiny_stream_out, node);
            if(out == out_tmp){
                LOG_I("audio_del_output-:%p type:%d",out,out->audio_app_type);
                list_remove(item);
                break;
             }
        }
    }
    pthread_mutex_unlock(&adev->lock);

    return 0;
    LOG_I("audio_del_output:exit");
}

static void do_normal_output_standby(struct tiny_audio_device *adev,void * out,AUDIO_HW_APP_T type)
{
    int usecase = UC_UNKNOWN;
    struct tiny_stream_out *normal_out=(struct tiny_stream_out *)out;
    LOG_I("do_normal_output_standby enter:%p standby:%d",normal_out->pcm,normal_out->standby);
    usecase = stream_type_to_usecase(normal_out->audio_app_type);

    if(false==normal_out->standby){
        set_mdg_mute(adev->dev_ctl,usecase,true);
    }

    if (NULL!=normal_out->pcm) {
        LOG_I("do_normal_output_standby pcm_close:%p",normal_out->pcm);
        pcm_close(normal_out->pcm);
        LOG_I("do_normal_output_standby pcm_close:%p success",normal_out->pcm);
        normal_out->pcm = NULL;
    }

    out_deinit_resampler(normal_out);
    normal_out->standby=true;

    set_usecase(adev->dev_ctl, usecase, false);
    set_audioparam(adev->dev_ctl,PARAM_USECASE_CHANGE,NULL,false);
    LOG_I("do_normal_output_standby :%p %d exit",out,normal_out->audio_app_type);
}

ssize_t normal_out_write(struct tiny_stream_out *out, const void *buffer,
                                size_t bytes)
{
    void *buf;
    size_t frame_size = 0;
    size_t in_frames = 0;
    size_t out_frames = 0;
    bool low_power;
    int kernel_frames;
    static long time1 = 0, time2 = 0, deltatime = 0;
    int ret=-1;
    struct tiny_audio_device *adev = out->dev;

    struct pcm_config *config=NULL;
    frame_size = audio_stream_frame_size(&out->stream.common);
    in_frames = bytes / frame_size;
    out_frames = RESAMPLER_BUFFER_SIZE / frame_size;
    low_power = adev->low_power && !is_usecase_unlock(adev->dev_ctl, UC_MM_RECORD | UC_FM_RECORD | UC_VOICE_RECORD |
        UC_VOIP_RECORD | UC_BT_RECORD);

    if(out->pcm){
        if(out->config->rate !=out->requested_rate){
            audio_format_t format = out->stream.common.get_format(&out->stream.common);
            if((out->config->format ==  PCM_FORMAT_S16_LE)
                && (format ==  AUDIO_FORMAT_PCM_8_24_BIT)){
                memcpy_to_i16_from_q8_23(out->buffer, buffer,  bytes/audio_bytes_per_sample(AUDIO_FORMAT_PCM_8_24_BIT));
                out->resampler->resample_from_input(out->resampler,
                    (int16_t *)out->buffer,&in_frames,
                    (int16_t *) buffer,&out_frames);
                frame_size  =4;
                buf = buffer;
            }
            else {
                out->resampler->resample_from_input(out->resampler,
                        (int16_t *)buffer,&in_frames,
                        (int16_t *) out->buffer,&out_frames);
                buf = out->buffer;
            }
        }else{
            out_frames = in_frames;
            buf = (void *)buffer;
        }

        if((out->config->channels==1) &&(frame_size == 4)){
            frame_size=2;
            LOG_D("normal_out_write chanage channle");
            pcm_mixer(buf, out_frames*frame_size);
        }
#if 1
        if(AUDIO_HW_APP_DEEP_BUFFER == out->audio_app_type){
            if(low_power != out->low_power){
                if(low_power){
                    out->write_threshold = out->config->period_size * out->config->period_count;
                    out->config->avail_min = ( out->write_threshold *3 ) /4;
                    LOG_I("low_power avail_min:%d, write_threshold:%d", out->config->avail_min, out->write_threshold);
                } else{
                    out->write_threshold = (out->config->period_size * out->config->period_count)/2;
                    out->config->avail_min = out->write_threshold;
                    LOG_I("avail_min:%d, write_threshold:%d", out->config->avail_min, out->write_threshold);
                }
                pcm_set_avail_min(out->pcm, out->config->avail_min);
                out->low_power = low_power;
            }

            do{
                struct timespec time_stamp;

                if (pcm_get_htimestamp(out->pcm, (unsigned int *)&kernel_frames, &time_stamp) < 0)
                    break;

                kernel_frames = pcm_get_buffer_size(out->pcm) - kernel_frames;
                if(kernel_frames > out->write_threshold){
                    unsigned long time = (unsigned long)(((int64_t)(kernel_frames - out->write_threshold) * 1000000) /DEFAULT_OUT_SAMPLING_RATE);
                    if (time < MIN_WRITE_SLEEP_US){
                        time = MIN_WRITE_SLEEP_US;
                    }
                    time1 = getCurrentTimeUs();
                    usleep(time);
                    time2 = getCurrentTimeUs();
                }
            }while(kernel_frames > out->write_threshold);
        }
#endif

        ret = pcm_mmap_write(out->pcm, (void *)buf,
                         out_frames * frame_size);
        if(0==ret){
            LOG_D("normal_out_write out frames  is %d bytes:%d", out_frames,bytes);
            if(AUDIO_HW_APP_DEEP_BUFFER == out->audio_app_type){
                if(adev->dev_ctl->config.dump.playback.deep_buffer.hex_fd>0){
                    write(adev->dev_ctl->config.dump.playback.deep_buffer.hex_fd,buf,out_frames * frame_size);
                }
            }
            if(AUDIO_HW_APP_PRIMARY == out->audio_app_type){
                if(adev->dev_ctl->config.dump.playback.normal.hex_fd>0){
                    write(adev->dev_ctl->config.dump.playback.normal.hex_fd,buf,out_frames * frame_size);
                }
            }
        }else{
            LOG_E("normal_out_write ret:0x%x pcm:0x%p", ret,out->pcm);
            if (ret < 0) {
                if (out->pcm) {
                    LOG_W("normal_out_write warning:%d, (%s)", ret,
                          pcm_get_error(out->pcm));
                }
            }
        }
    }
    return (ret == 0) ? bytes:ret;
}

int stream_type_to_usecase(int audio_app_type)
{
    int usecase = UC_UNKNOWN;
     switch(audio_app_type){
        case AUDIO_HW_APP_VAUDIO:
            usecase = UC_NORMAL_PLAYBACK;
            break;
        case AUDIO_HW_APP_VOIP:
            usecase = UC_VOIP;
            break;
        case AUDIO_HW_APP_VOIP_BT:
            usecase = UC_BT_VOIP;
            break;
        case AUDIO_HW_APP_PRIMARY:
            usecase = UC_NORMAL_PLAYBACK;
            break;
        case AUDIO_HW_APP_DIRECT:
            usecase = UC_NORMAL_PLAYBACK;
            break;
        case AUDIO_HW_APP_DEEP_BUFFER:
            usecase = UC_DEEP_BUFFER_PLAYBACK;
            break;
        case AUDIO_HW_APP_OFFLOAD:
            usecase = UC_OFFLOAD_PLAYBACK;
            break;
        case AUDIO_HW_APP_NORMAL_RECORD:
            usecase = UC_MM_RECORD;
            break;
         case AUDIO_HW_APP_CALL:
            usecase = UC_CALL;
            break;
         case AUDIO_HW_APP_FM:
            usecase = UC_FM;
            break;
         case AUDIO_HW_APP_VOIP_RECORD:
            usecase = UC_VOIP_RECORD;
            break;
        case AUDIO_HW_APP_FM_RECORD:
            usecase = UC_FM_RECORD;
            break;
        case AUDIO_HW_APP_CALL_RECORD:
            usecase = UC_VOICE_RECORD;
            break;
        case AUDIO_HW_APP_BT_RECORD:
            usecase = UC_BT_RECORD;
            break;
        default:
            LOG_E("start_output_stream stream type:0x%x", audio_app_type);
            break;
    }
    return usecase;
}

int start_output_stream(struct tiny_stream_out *out)
{
    struct tiny_audio_device *adev = out->dev;
    int pcm_devices=-1;
    int ret = 0;
    int usecase=UC_UNKNOWN;
    struct audio_control *control = NULL;

    control = adev->dev_ctl;
    out->low_power = 1;

    if(NULL==out->buffer){
        out->buffer = malloc(RESAMPLER_BUFFER_SIZE);
        if (!out->buffer) {
            LOG_E("start_output_stream: alloc fail, size: %d",
                  RESAMPLER_BUFFER_SIZE);
            goto error;
        }
    }
    memset(out->buffer, 0, RESAMPLER_BUFFER_SIZE);

    if(NULL!=out->pcm){
        LOG_E("start_output_stream pcm err:%x",out->pcm);
        pcm_close(out->pcm);
        out->pcm=NULL;
    }

    ret = dev_ctl_get_out_pcm_config(control, out->audio_app_type, &pcm_devices, out->config);
    if(ret != 0) {
        LOG_E("start_output_stream, out->config is NULL");
        goto error;
    }

    usecase = stream_type_to_usecase(out->audio_app_type);
    LOG_I("start_output_stream usecase:%x",usecase);
    ret=set_usecase(adev->dev_ctl, usecase, true);
    if(ret < 0) {
        goto error;
    }

    LOG_I("start_output_stream pcm_open start config:%p, open card:%d",out->config,adev->dev_ctl->cards.s_tinycard);
    out->pcm =
        pcm_open(adev->dev_ctl->cards.s_tinycard, pcm_devices, PCM_OUT | PCM_MMAP | PCM_NOIRQ |PCM_MONOTONIC, out->config);
    LOG_I("start_output_stream pcm_open end format %d", out->config->format);

    if (!pcm_is_ready(out->pcm)) {
        LOG_E("%s:cannot open pcm : %s", __func__,
          pcm_get_error(out->pcm));
        goto error;
    }

    if((out->config->rate !=out->requested_rate)&&(NULL==out->resampler)){
        LOG_I("start_output_stream create_resampler");
        if(0!=out_init_resampler(out)){
            goto error;
        }
        out->resampler->reset(out->resampler);
    }

    LOG_I("start_output_stream pcm_open devices:%d pcm:%p %d %d",pcm_devices,out->pcm
        ,out->config->rate,out->requested_rate);

    set_mdg_mute(adev->dev_ctl,usecase,false);

    out->standby_fun=do_normal_output_standby;
    select_devices_new(adev->dev_ctl,out->audio_app_type, out->devices,false,false,true);

    if((out->audio_app_type == AUDIO_HW_APP_VOIP)||(out->audio_app_type == AUDIO_HW_APP_VOIP_BT)){
        set_dsp_volume(adev->dev_ctl,adev->voice_volume);
    }

    set_audioparam(adev->dev_ctl,PARAM_USECASE_DEVICES_CHANGE,NULL,false);
    return 0;

error:
    set_usecase(adev->dev_ctl, usecase, false);
    set_audioparam(adev->dev_ctl,PARAM_USECASE_CHANGE,NULL,false);
    adev->stream_status &=~(1<<out->audio_app_type);

    if(out->pcm) {
        pcm_close(out->pcm);
        out->pcm = NULL;
    }
    return -1;
}

static int check_input_parameters(uint32_t sample_rate, int format,
                                  int channel_count)
{
    if (format != AUDIO_FORMAT_PCM_16_BIT)
    { return -EINVAL; }

    if ((channel_count < 1) || (channel_count > 2))
    { return -EINVAL; }

    switch (sample_rate) {
    case 8000:
    case 11025:
    case 16000:
    case 22050:
    case 24000:
    case 32000:
    case 44100:
    case 48000:
        break;
    default:
        return -EINVAL;
    }

    return 0;
}

static size_t get_input_buffer_size(const struct tiny_audio_device *adev,
                                    uint32_t sample_rate, int format,
                                    int channel_count)
{
    size_t size;
    size_t device_rate;
    struct audio_control *control = adev->dev_ctl;
    if (check_input_parameters(sample_rate, format, channel_count) != 0) {
        return 0;
    }

    /*  take resampling into account and return the closest majoring
        multiple of 16 frames, as audioflinger expects audio buffers to
        be a multiple of 16 frames */
    size =
        (control->pcm_handle.play[AUD_PCM_MM_NORMAL].period_size *
         sample_rate) / control->pcm_handle.play[AUD_PCM_MM_NORMAL].rate;
    size = ((size + 15) / 16) * 16;
    return size * channel_count * sizeof(short);
}

static uint32_t out_get_sample_rate(const struct audio_stream *stream)
{
    struct tiny_stream_out *out = (struct tiny_stream_out *)stream;

    if(out->audio_app_type == AUDIO_HW_APP_OFFLOAD) {
        return out->offload_samplerate;
    }else{
        return 48000;
    }
}

static int out_set_sample_rate(struct audio_stream *stream, uint32_t rate)
{
    return 0;
}

static size_t out_get_buffer_size(const struct audio_stream *stream)
{
    struct tiny_stream_out *out = (struct tiny_stream_out *)stream;
    if(out->audio_app_type == AUDIO_HW_APP_OFFLOAD)
    { return out->compress_config.fragment_size; }

    /*  take resampling into account and return the closest majoring
        multiple of 16 frames, as audioflinger expects audio buffers to
        be a multiple of 16 frames */
    size_t size =
        (out->config->period_size * DEFAULT_OUT_SAMPLING_RATE) /
        out->config->rate;
    size = ((size + 15) / 16) * 16;
    LOG_V("%s size=%d, frame_size=%d", __func__, size,
          audio_stream_frame_size(stream));
    return size * audio_stream_frame_size(stream);
}

static audio_channel_mask_t out_get_channels(const struct audio_stream *stream)
{
    struct tiny_stream_out *out = (struct tiny_stream_out *)stream;

    if(out->audio_app_type == AUDIO_HW_APP_OFFLOAD) {
        return out->offload_channel_mask;
    }

    return (audio_channel_mask_t) AUDIO_CHANNEL_OUT_STEREO;
}

static audio_format_t out_get_format(const struct audio_stream *stream)
{
    struct tiny_stream_out *out = (struct tiny_stream_out *)stream;
    struct tiny_audio_device *adev = out->dev;
    struct audio_control *actl=adev->dev_ctl;

    if(out->audio_app_type == AUDIO_HW_APP_OFFLOAD) {
        return out->offload_format;
    }

#ifdef AUDIO_24BIT_PLAYBACK_SUPPORT
    if (actl->config.support_24bits) {
        if(out->audio_app_type == AUDIO_HW_APP_DEEP_BUFFER) {
            LOG_D("out_get_format AUDIO_FORMAT_PCM_16_BIT\n");
            return AUDIO_FORMAT_PCM_8_24_BIT;
        }
        else
            return AUDIO_FORMAT_PCM_16_BIT;
    }
#endif

    return AUDIO_FORMAT_PCM_16_BIT;
}

static int out_set_format(struct audio_stream *stream, audio_format_t format)
{
    return 0;
}

struct tiny_stream_out * get_output_stream(struct tiny_audio_device *adev,AUDIO_HW_APP_T audio_app_type){
    struct tiny_stream_out *out = NULL;
    struct tiny_stream_out * out_ret = NULL;
    struct listnode *item;
    struct listnode *item2;
    LOG_I("get_output_stream type:%p",audio_app_type);
    pthread_mutex_lock(&adev->lock);
    LOG_I("%s %d",__func__,__LINE__);
    if(!list_empty(&adev->active_out_list)){
        list_for_each_safe(item, item2,&adev->active_out_list){
            out = node_to_item(item, struct tiny_stream_out, node);
            if(out->audio_app_type==audio_app_type){
                out_ret = out;
                break;
             }
        }
    }
    pthread_mutex_unlock(&adev->lock);
    return out_ret;
}

struct tiny_stream_out * get_output_stream_unlock(struct tiny_audio_device *adev,AUDIO_HW_APP_T audio_app_type){
    struct tiny_stream_out *out = NULL;
    struct tiny_stream_out * out_ret = NULL;
    struct listnode *item;
    struct listnode *item2;
    LOG_D("get_output_stream_unlock type:%p",audio_app_type);
    LOG_D("%s %d",__func__,__LINE__);
    if(!list_empty(&adev->active_out_list)){
        list_for_each_safe(item, item2,&adev->active_out_list){
            out = node_to_item(item, struct tiny_stream_out, node);
            if(out->audio_app_type==audio_app_type){
                out_ret = out;
                break;
             }
        }
    }
    return out_ret;
}


struct tiny_stream_out * force_out_standby_unlock(struct tiny_audio_device *adev,AUDIO_HW_APP_T audio_app_type){
    struct tiny_stream_out *out = NULL;
    LOG_I("force_out_standby_unlockaudio_app_type:0x%x",audio_app_type);

    out=get_output_stream_unlock(adev,audio_app_type);

    if((NULL!=out) && (NULL!=out->standby_fun)){
        pthread_mutex_lock(&out->lock);
        out->standby_fun(adev,out,out->audio_app_type);
        pthread_mutex_unlock(&out->lock);
    }
    LOG_I("force_out_standby_unlock:exit");
    return out;
}

struct tiny_stream_out * force_out_standby(struct tiny_audio_device *adev,AUDIO_HW_APP_T audio_app_type){
    struct tiny_stream_out *out = NULL;
    LOG_I("force_out_standby audio_app_type:0x%x",audio_app_type);

    out=get_output_stream(adev,audio_app_type);

    if((NULL!=out) && (NULL!=out->standby_fun)){
        pthread_mutex_lock(&out->lock);
        out->standby_fun(adev,out,out->audio_app_type);
        pthread_mutex_unlock(&out->lock);
    }
    LOG_I("force_out_standby:exit");
    return out;
}

int do_output_standby(struct tiny_stream_out *out)
{
    struct tiny_audio_device *adev = out->dev;
    LOG_I("do_output_standby %p audio_app_type:%d",out,out->audio_app_type);
    if(NULL!=out->standby_fun){
        pthread_mutex_lock(&out->lock);
        if(out->standby == false){
            out->standby_fun(adev,out,out->audio_app_type);
        }
        pthread_mutex_unlock(&out->lock);
    }
    return 0;
}

static int out_standby(struct audio_stream *stream)
{
    struct tiny_stream_out *out = (struct tiny_stream_out *)stream;
    int status=-1;
    LOG_I("out_standby %p",stream);
    status = do_output_standby(out);
    return status;
}

static int out_dump(const struct audio_stream *stream, int fd)
{
    return 0;
}

static bool is_primary_output(struct audio_stream *stream){
    struct tiny_stream_out *out = (struct tiny_stream_out *)stream;

    if((AUDIO_HW_APP_PRIMARY ==out->audio_app_type)
        ||(AUDIO_HW_APP_VOIP ==out->audio_app_type)){
        return true;
    }else{
        return false;
    }
}

static int out_devices_check_unlock(struct audio_stream *stream,audio_devices_t device){
    struct tiny_stream_out *out = (struct tiny_stream_out *)stream;
    struct tiny_audio_device *adev = out->dev;
    struct audio_control *actl=adev->dev_ctl;
    struct listnode *item;
    struct listnode *item2;

    if(!list_empty(&adev->active_out_list)){
        list_for_each_safe(item, item2,&adev->active_out_list){
            out = node_to_item(item, struct tiny_stream_out, node);
            if(out!=stream){
                out->devices=device;
            }
        }
    }
    return 0;
}

static int out_devices_check(struct audio_stream *stream,audio_devices_t device){
    struct tiny_stream_out *out = (struct tiny_stream_out *)stream;
    struct tiny_audio_device *adev = out->dev;
    pthread_mutex_lock(&adev->lock);
    out_devices_check_unlock(stream,device);
    pthread_mutex_unlock(&adev->lock);
    return 0;
}

static int out_set_parameters(struct audio_stream *stream, const char *kvpairs)
{
    struct tiny_stream_out *out = (struct tiny_stream_out *)stream;
    struct tiny_audio_device *adev = out->dev;
    struct audio_control *actl=adev->dev_ctl;
    struct str_parms *parms;
    char *str;
    char value[32];
    int ret, val = 0;
    static int cur_mode = 0;
    audio_devices_t pre_devices=AUDIO_DEVICE_NONE;
    parms = str_parms_create_str(kvpairs);

    LOG_I("out_set_parameters type:%d:%s",out->audio_app_type,kvpairs);
    ret =
        str_parms_get_str(parms, AUDIO_PARAMETER_STREAM_ROUTING, value,
                          sizeof(value));
    if (ret >= 0) {
        val = atoi(value);
        if(CHECK_OUT_DEVICE_IS_INVALID(val)) {
            LOG_E(" %s out_device is invalid, out_device = 0x%x", __FUNCTION__, val);
            ret = -EINVAL;
            goto error;
        }
        pthread_mutex_lock(&out->lock);
        if (val != 0 && out->devices != (unsigned int)val) {
            pre_devices=out->devices;
            out->devices = val;
        }
        pthread_mutex_unlock(&out->lock);
        LOG_D("out_set_parameters dev_ctl:%p out:%p",adev->dev_ctl,out);

        pthread_mutex_lock(&adev->lock);
        out_devices_check_unlock(out,val);

        if(is_call_active_unlock(adev)){
            if((pre_devices&AUDIO_DEVICE_OUT_ALL_SCO)&&(0==(val&AUDIO_DEVICE_OUT_ALL_SCO))){
                force_in_standby_unlock(adev,AUDIO_HW_APP_BT_RECORD);
            }else if((val&AUDIO_DEVICE_OUT_ALL_SCO)&&(0==(pre_devices&AUDIO_DEVICE_OUT_ALL_SCO))){
                force_in_standby_unlock(adev,AUDIO_HW_APP_NORMAL_RECORD);
            }
        }
        pthread_mutex_unlock(&adev->lock);
        select_devices_new(adev->dev_ctl,out->audio_app_type,val,false,true,false);
        pthread_mutex_lock(&adev->lock);
        if((AUDIO_MODE_IN_CALL == adev->call_mode)&& (false==adev->call_start) &&(is_primary_output(out))){
            adev->call_start=true;
            pthread_mutex_unlock(&adev->lock);
            send_cmd_to_dsp_thread(adev->dev_ctl->agdsp_ctl,AUDIO_CTL_START_VOICE,NULL);
        }else{
            pthread_mutex_unlock(&adev->lock);
        }
        ret = 0;
    }

exit:
    if (out->audio_app_type == AUDIO_HW_APP_OFFLOAD) {
        audio_get_compress_metadata(out, parms);
    }
error:
    str_parms_destroy(parms);
    return ret;
}

static char *out_get_parameters(const struct audio_stream *stream,
                                const char *keys)
{
    return strdup("");
}

static uint32_t out_get_latency(const struct audio_stream_out *stream)
{
    struct tiny_stream_out *out = (struct tiny_stream_out *)stream;
#if 1
    if(AUDIO_HW_APP_DEEP_BUFFER == out->audio_app_type){
        uint32_t latency_time ;
        latency_time = (out->config->period_size * out->config->period_count * 1000) /
               out->config->rate;
        if(latency_time > 100)
            return latency_time/2;
        return latency_time;
    }
#endif
    return (out->config->period_size * out->config->period_count * 1000) /
            out->config->rate;
}

 static int out_set_volume(struct audio_stream_out *stream, float left,
                           float right)
{
    struct tiny_stream_out *out = (struct tiny_stream_out *)stream;
    struct tiny_audio_device *adev = out->dev;
    int ret=-ENOSYS;

    if (out->audio_app_type == AUDIO_HW_APP_OFFLOAD) {
        pthread_mutex_lock(&out->lock);
        ret= set_offload_volume(adev->dev_ctl,left,right);
        pthread_mutex_unlock(&out->lock);
    }

    pthread_mutex_lock(&adev->lock);
    adev->dev_ctl->music_volume=left;
    pthread_mutex_unlock(&adev->lock);
    return ret;
}
 static int is_stream_active_unlock(struct tiny_audio_device *adev, AUDIO_HW_APP_T stream_type)
 {
     struct tiny_stream_out *out = NULL;

     out=get_output_stream_unlock(adev,stream_type);
     if((NULL!=out) && (false ==out->standby)){
         return true;
     }

     return false;
 }

 int is_call_active_unlock(struct tiny_audio_device *adev)
 {
     voice_status_t call_status;
     int is_call_active;
     call_status =adev->call_status;
     is_call_active = (call_status != VOICE_INVALID_STATUS) && (call_status != VOICE_STOP_STATUS);

     return is_call_active;
 }

static bool out_bypass_data(struct tiny_stream_out *out,bool voip_start,voice_status_t call_status)
{
    struct tiny_audio_device *adev = out->dev;
    bool by_pass=false;

    if(adev->is_agdsp_asserted) {
            by_pass = true;
    }

     if(adev->is_audio_test_on) {
            by_pass = true;
    }

    if(adev->is_dsp_loop) {
            by_pass = true;
    }

    return by_pass;
}

static int out_get_presentation_position(const struct audio_stream_out *stream,
        uint64_t *frames, struct timespec *timestamp)
{
    struct tiny_stream_out *out = (struct tiny_stream_out *)stream;
    struct pcm *pcm = NULL;
    int ret = -1;
    unsigned long dsp_frames;
    pthread_mutex_lock(&out->lock);
    pcm = out->pcm;

    if(out->audio_app_type == AUDIO_HW_APP_OFFLOAD){
        if (out->compress != NULL) {
            compress_get_tstamp(out->compress, &dsp_frames,
                            &out->offload_samplerate);
            *frames = dsp_frames;
            ret = 0;
        }
    } else{
        if (pcm) {
            size_t avail;
            if (pcm_get_htimestamp(pcm, &avail, timestamp) == 0) {
                //LOG_E("out_get_presentation_position out frames %llu timestamp.tv_sec %llu timestamp.tv_nsec %llu",*frames,timestamp->tv_sec,timestamp->tv_nsec);
                size_t kernel_buffer_size = out->config->period_size *
                                            out->config->period_count;
                int64_t signed_frames = out->written - kernel_buffer_size + avail;
                if (signed_frames >= 0) {
                    *frames = signed_frames;
                    ret = 0;
                }
            }
        }
    }
    pthread_mutex_unlock(&out->lock);
    //LOG_E("out_get_presentation_position ret %d",ret);
    return ret;
}

static int out_app_type_check(struct tiny_audio_device *adev, int audio_app_type,bool voip_start,audio_devices_t devices,voice_status_t call_status)
{
    int ret_type = audio_app_type;

    if(voip_start) {
        if((VOICE_START_STATUS==call_status)&&
            ((audio_app_type == AUDIO_HW_APP_PRIMARY) || (audio_app_type == AUDIO_HW_APP_VOIP))){
            ret_type = AUDIO_HW_APP_PRIMARY;
        }else{
            if((audio_app_type == AUDIO_HW_APP_VOIP)||(audio_app_type == AUDIO_HW_APP_PRIMARY)||(audio_app_type == AUDIO_HW_APP_VOIP_BT)){
                if(devices&AUDIO_DEVICE_OUT_ALL_SCO){
                    ret_type = AUDIO_HW_APP_VOIP_BT;
                }else{
                    ret_type = AUDIO_HW_APP_VOIP;
                }
            }
        }
    }
    else {
        if((audio_app_type == AUDIO_HW_APP_VOIP)
            ||(audio_app_type == AUDIO_HW_APP_PRIMARY)
            ||(audio_app_type == AUDIO_HW_APP_VOIP_BT)) {
            ret_type = AUDIO_HW_APP_PRIMARY;
        }
    }
    return ret_type;
}

static ssize_t out_write(struct audio_stream_out *stream, const void *buffer,
                         size_t bytes)
{
    int ret = 0;
    int cur_app_type;
    int usecase = UC_UNKNOWN;
    struct tiny_stream_out *out = (struct tiny_stream_out *)stream;
    struct tiny_audio_device *adev = out->dev;
    int frame_size=0;
    bool voip_start=false;
    voice_status_t call_status;
    audio_mode_t call_mode;

    pthread_mutex_lock(&adev->lock);
    pthread_mutex_lock(&out->lock);
    call_status =adev->call_status;
    voip_start=adev->voip_start;
    call_mode=adev->call_mode;
    if(is_call_active_unlock(adev)
        || is_stream_active_unlock(adev, AUDIO_HW_APP_OFFLOAD)) {
        voip_start=false;
    }

    LOG_D("out_write out:%p call_status:%d usecase:0x%x audio_app_type:%x voip:%d",
          out, call_status, adev->dev_ctl->usecase,
          out->audio_app_type,voip_start);

    frame_size = audio_stream_frame_size((const struct audio_stream *)(&out->stream.common));
    out->written += bytes /frame_size;

    cur_app_type = out_app_type_check(adev,out->audio_app_type,voip_start,out->devices,call_status);

    if(cur_app_type != out->audio_app_type) {
        LOG_W("%s %d out_app_type_check %d %d",__func__,__LINE__,out->audio_app_type,cur_app_type);
        if(out->standby_fun) {
            out->standby_fun(adev,out,out->audio_app_type);
        }
         out->audio_app_type = cur_app_type;
    }

    if (out_bypass_data(out,voip_start,call_status)){
        LOG_I("out_write type:%x bypass call_status:%d",out->audio_app_type,call_status);
        pthread_mutex_unlock(&out->lock);
        pthread_mutex_unlock(&adev->lock);
        usleep((int64_t) bytes * 1000000 / audio_stream_frame_size(&stream->common) / out_get_sample_rate(&stream->common));
        return bytes;
    }

    if (true==out->standby){
        if(out->audio_app_type == AUDIO_HW_APP_OFFLOAD){
            ret = audio_start_compress_output(out);
        }
        else {
            ret = start_output_stream(out);
        }
         if (ret != 0) {
            pthread_mutex_unlock(&adev->lock);
            goto exit;
        }
        out->standby = false;
    }

    pthread_mutex_unlock(&adev->lock);

    if(out->audio_app_type == AUDIO_HW_APP_OFFLOAD) {
        ret = out_write_compress(out, buffer, bytes);
        if(adev->dev_ctl->config.dump.playback.offload.hex_fd>0){
            write(adev->dev_ctl->config.dump.playback.offload.hex_fd,buffer,bytes);
        }
    }
    else {
        ret = normal_out_write(out, buffer, bytes);
    }

exit:
    if((ret < 0) && (NULL!=out->standby_fun)){
        LOG_W("out_write failed %p app type:%d",out,out->audio_app_type);
        out->standby_fun(out->dev,out, out->audio_app_type);
    }
    pthread_mutex_unlock(&out->lock);
    if(ret < 0 ) {
        LOG_E("out_write error");
        usleep(20*1000);
    }

    if(out->audio_app_type != AUDIO_HW_APP_OFFLOAD){
        return bytes;
    }else{
        return ret;
    }
}
static int out_get_render_position(const struct audio_stream_out *stream,
                                   uint32_t *dsp_frames)
{
    struct tiny_stream_out *out = (struct tiny_stream_out *)stream;

    if (dsp_frames == NULL)
    { return -EINVAL; }
    *dsp_frames = 0;
    if (out->audio_app_type == AUDIO_HW_APP_OFFLOAD) {
        pthread_mutex_lock(&out->lock);
        if (out->compress != NULL) {
            compress_get_tstamp(out->compress, (unsigned long *)dsp_frames,
                                &out->offload_samplerate);
        }
        pthread_mutex_unlock(&out->lock);
        return 0;
    }
    LOG_W("out_get_render_position failed %p app type:%d",out,out->audio_app_type);
    return -EINVAL;
}

static int out_add_audio_effect(const struct audio_stream *stream,
                                effect_handle_t effect)
{
    return 0;
}

static int out_remove_audio_effect(const struct audio_stream *stream,
                                   effect_handle_t effect)
{
    return 0;
}

/** audio_stream_in implementation **/

static int get_next_buffer(struct resampler_buffer_provider *buffer_provider,
                           struct resampler_buffer *buffer);

static void release_buffer(struct resampler_buffer_provider *buffer_provider,
                           struct resampler_buffer *buffer);

static int out_deinit_resampler(struct tiny_stream_out *out)
{
    if (out->resampler) {
        release_resampler(out->resampler);
        out->resampler = NULL;
    }

    if (out->buffer) {
        free(out->buffer);
        out->buffer = NULL;
    }
    return 0;
}

static int out_init_resampler(struct tiny_stream_out *out)
{
    int ret = 0;
    int size = 0;
    #if 0
    size =
        out->config->period_size *
        audio_stream_frame_size(&out->stream.common);
    #else
    size = RESAMPLER_BUFFER_SIZE;
    #endif

    if(NULL!=out->buffer){
        free(out->buffer);
    }

    out->buffer = malloc(size);
    if (!out->buffer) {
        LOG_E("in_init_resampler: alloc fail, size: %d", size);
        ret = -ENOMEM;
        goto err;
    }
    ret = create_resampler(out->requested_rate,
                           out->config->rate,
                           2,
                           RESAMPLER_QUALITY_DEFAULT,
                           NULL, &out->resampler);
    if (ret != 0) {
        ret = -EINVAL;
        goto err;
    }

    return ret;

err:
    out_deinit_resampler(out);
    return ret;
}

static int in_deinit_resampler(struct tiny_stream_in *in)
{
    LOG_I("in_deinit_resampler:%p",in->resampler);

    if (in->resampler) {
        release_resampler(in->resampler);
        in->resampler = NULL;
    }
    if (in->buffer) {
        free(in->buffer);
        in->buffer = NULL;
    }
    return 0;
}

static int in_init_resampler(struct tiny_stream_in *in)
{
    int ret = 0;
    int size = 0;
    in->buf_provider.get_next_buffer = get_next_buffer;
    in->buf_provider.release_buffer = release_buffer;
    LOG_I("in_init_resampler");
    size = 4*
        in->config->period_size *
        audio_stream_in_frame_size_l(&in->stream.common);

    if(NULL==in->buffer){
        in->buffer = malloc(size);
    }

    if (!in->buffer) {
        LOG_E("in_init_resampler: alloc fail, size: %d", size);
        ret = -ENOMEM;
        goto err;
    } else {
        memset(in->buffer, 0, size);
    }

    if(in->resampler!=NULL){
        LOG_W("in_init_resampler resampler is:%p",in->resampler);
    }
    LOG_I("in_init_resampler %d %d %d",
        in->config->rate,in->requested_rate,in->config->channels);
    ret = create_resampler(in->config->rate,
                           in->requested_rate,
                           in->config->channels,
                           RESAMPLER_QUALITY_DEFAULT,
                           &in->buf_provider, &in->resampler);
    if (ret != 0) {
        ret = -EINVAL;
        goto err;
    }

    return ret;

err:
    in_deinit_resampler(in);
    return ret;
}

/*  if bt sco playback stream is not started and bt sco capture stream is started, we will
    start duplicate_thread to write zero data to bt_sco_card.
    we will stop duplicate_thread to write zero data to bt_sco_card if bt sco playback stream
    is started or bt sco capture stream is stoped.
*/

static void do_normal_inputput_standby(struct tiny_audio_device *adev,void * in,AUDIO_HW_APP_T type)
{
    int usecase = UC_UNKNOWN;
    struct tiny_stream_in *normal_in=(struct tiny_stream_in *)in;
    LOG_I("do_normal_inputput_standby:%p",normal_in->pcm);
    if (NULL!=normal_in->pcm) {
        LOG_I("do_normal_inputput_standby pcm_close:%p",normal_in->pcm);
        pcm_close(normal_in->pcm);
        normal_in->pcm = NULL;
    }

    if (NULL!=normal_in->channel_buffer) {
        free(normal_in->channel_buffer);
        normal_in->channel_buffer = NULL;
    }

    if (NULL!=normal_in->proc_buf) {
        free(normal_in->proc_buf);
        normal_in->proc_buf = NULL;
    }
    normal_in->proc_buf_size=0;

    in_deinit_resampler(in);

    if (normal_in->nr_buffer) {
        free(normal_in->nr_buffer);
        normal_in->nr_buffer = NULL;
    }

    if (normal_in->active_rec_proc) {
        AUDPROC_DeInitDp();
        normal_in->active_rec_proc = false;

        if(normal_in->rec_nr_handle) {
            AudioRecordNr_Deinit(normal_in->rec_nr_handle);
            normal_in->rec_nr_handle = NULL;
        }
    }

    normal_in->standby=true;
    usecase = stream_type_to_usecase(normal_in->audio_app_type);
    set_usecase(adev->dev_ctl,usecase,false);
    set_audioparam(adev->dev_ctl,PARAM_USECASE_CHANGE,NULL,false);
    LOG_I("do_normal_inputput_standby exit");
}

/* must be called with hw device and input stream mutexes locked */
static int start_input_stream(struct tiny_stream_in *in)
{
    struct tiny_audio_device *adev = in->dev;
    int pcm_devices=-1;
    int ret = 0;
    int usecase = UC_UNKNOWN;
    struct audio_control *control = NULL;
    AUDIO_HW_APP_T audio_app_type=AUDIO_HW_APP_NORMAL_RECORD;

    control = adev->dev_ctl;

    LOG_I("start_input_stream usecase:%x  type:%d",control->usecase,in->audio_app_type);

    usecase = stream_type_to_usecase(in->audio_app_type);
    ret = set_usecase(control,  usecase, true);
    if(ret < 0) {
        return -2;
    }

    ret = dev_ctl_get_in_pcm_config(control, in->audio_app_type, &pcm_devices, in->config);
    if(ret!= 0){
        goto error;
    }

    if((true==adev->dev_ctl->config.record.record_process_enable) && (2==in->config->channels)){
        in->active_rec_proc = init_rec_process(in, in->config->rate);
    }else{
        in->active_rec_proc=false;
    }

    LOG_I("start_input_stream rate:%d channel:%d",in->config->rate,in->config->channels);
    if((in->config->rate == 48000) && (true==adev->dev_ctl->config.record.record_nr_enable)
        && (true==adev->dev_ctl->config.record.record_process_enable) && (true== in->active_rec_proc)){


    if(NULL==in->nr_buffer){
        in->nr_buffer = malloc(in->config->period_size *
        audio_stream_frame_size(&in->stream.common)*4);
    }

    if (!in->nr_buffer) {
        LOG_E("in_init_resampler: alloc fail");
        ret = -ENOMEM;
        goto error;
    } else {
        memset(in->nr_buffer, 0, in->config->period_size *
        audio_stream_frame_size(&in->stream.common)*4);
    }


        in->rec_nr_handle = init_nr_process(in);
    }else{
        in->rec_nr_handle =NULL;
    }

    LOG_I("start_input_stream pcm_open start");
    in->pcm =
        pcm_open(adev->dev_ctl->cards.s_tinycard, pcm_devices, PCM_IN | PCM_MMAP | PCM_NOIRQ , in->config);
    LOG_I("start_input_stream pcm_open end");

    if (!pcm_is_ready(in->pcm)) {
        LOG_E("start_input_stream:cannot open pcm : %s",
              pcm_get_error(in->pcm));
        goto error;
    }

    if(in->resampler!=NULL){
        in_deinit_resampler(in);
        in->resampler=NULL;
    }

    if(in->config->rate !=in->requested_rate){
        LOG_I("start_input_stream create_resampler");
        ret = in_init_resampler(in);
        if (ret) {
            LOG_E(": in_init_resampler failed");
            goto error;
        }
        in->resampler->reset(in->resampler);
        in->frames_in = 0;
    }

    LOG_I("start_input_stream pcm_open devices:%d pcm:%p %d requested_rate:%d requested channels:%d in_devices:%x",pcm_devices,in->pcm
        ,in->config->rate,in->requested_rate,in->requested_channels,adev->dev_ctl->in_devices);

    in->standby_fun = do_normal_inputput_standby;

    select_devices_new(adev->dev_ctl,in->audio_app_type,in->devices,true,false,true);
    set_audioparam(adev->dev_ctl,PARAM_USECASE_DEVICES_CHANGE,NULL,false);

    return 0;

error:

    if (in->proc_buf) {
        free(in->proc_buf);
        in->proc_buf = NULL;
    }
    in->proc_buf_size=0;

    if(in->pcm) {
        LOG_E("%s: pcm open error: %s", __func__,
              pcm_get_error(in->pcm));

        pcm_close(in->pcm);
        in->pcm = NULL;
    }

    in_deinit_resampler(in);

    if (in->active_rec_proc) {
        AUDPROC_DeInitDp();
        in->active_rec_proc = false;
    }

    if(in->rec_nr_handle){
        AudioRecordNr_Deinit(in->rec_nr_handle);
        in->rec_nr_handle = NULL;
    }
    set_usecase(control,  usecase, false);
    set_audioparam(adev->dev_ctl,PARAM_USECASE_CHANGE,NULL,false);
    return -1;
}

static uint32_t in_get_sample_rate(const struct audio_stream *stream)
{
    struct tiny_stream_in *in = (struct tiny_stream_in *)stream;
    return in->requested_rate;
}

static int in_set_sample_rate(struct audio_stream *stream, uint32_t rate)
{
	LOG_I("in_set_sample_rate %d",rate);
    return 0;
}

static size_t in_get_buffer_size(const struct audio_stream *stream)
{
    size_t size;
    size_t device_rate;

    struct tiny_stream_in *in = (struct tiny_stream_in *)stream;

    if (check_input_parameters
        (in->requested_rate, AUDIO_FORMAT_PCM_16_BIT,
         in->config->channels) != 0) {
        return 0;
    }

    /*  take resampling into account and return the closest majoring
        multiple of 16 frames, as audioflinger expects audio buffers to
        be a multiple of 16 frames */
    if(in->requested_rate != in->config->rate){
        if(in->audio_app_type == AUDIO_HW_APP_NORMAL_RECORD || in->audio_app_type == AUDIO_HW_APP_FM_RECORD){
            in->config->period_size = (((((in->config->period_size * in->config->period_count * 1000)/in->config->rate)
                                                                * in->requested_rate)/(1000 * in->config->period_count))/160)*160;
        }else{
            in->config->period_size = (((in->config->period_size * in->config->period_count * 1000)/in->config->rate)
                                                            * in->requested_rate)/(1000 * in->config->period_count);
        }
    }
    size = in->config->period_size;
    size = ((size + 15) / 16) * 16;

    return size * in->requested_channels * sizeof(short);
}

static uint32_t in_get_channels(const struct audio_stream *stream)
{
    struct tiny_stream_in *in = (struct tiny_stream_in *)stream;

    if (1==in->requested_channels) {
//   if (1==in->config->channels) {
        return AUDIO_CHANNEL_IN_MONO;
    } else {
        return AUDIO_CHANNEL_IN_STEREO;
    }
}

static audio_format_t in_get_format(const struct audio_stream *stream)
{
    return AUDIO_FORMAT_PCM_16_BIT;
}

static int in_set_format(struct audio_stream *stream, audio_format_t format)
{
    return 0;
}

/* must be called with hw device and input stream mutexes locked */
int do_input_standby(struct tiny_stream_in *in)
{
    struct tiny_audio_device *adev = in->dev;

    if((NULL!=in->standby_fun) && (in->standby == false)) {
        pthread_mutex_lock(&in->lock);
        in->standby_fun(adev,in,in->audio_app_type);
        pthread_mutex_unlock(&in->lock);
    }
    return 0;
}

void force_in_standby_unlock(struct tiny_audio_device *adev,AUDIO_HW_APP_T audio_app_type)
{
    struct tiny_stream_in *in=NULL;
    struct tiny_stream_in *in_ret=NULL;

    struct listnode *item;
    struct listnode *tmp;
    struct listnode *list=&adev->active_input_list;
    bool empty=false;
    int stream_status=0;

    LOG_I("force_in_standby_unlock audio_app_type:0x%x in:%p node:%p list:%p",
        audio_app_type,in,&(in->node),list);

    list_for_each_safe(item, tmp, list){
        LOG_I("force_in_standby item:%p next:%p",item,item->next);
        in = node_to_item(item, struct tiny_stream_in, node);
        if(in==NULL){
            break;
        }
        if(in->audio_app_type==audio_app_type){
            in_ret=in;
            break;
         }
    }

    if((NULL!=in_ret) && (NULL!=in_ret->standby_fun)){
        pthread_mutex_lock(&in_ret->lock);
        in_ret->standby_fun(adev,in_ret,in_ret->audio_app_type);
        pthread_mutex_unlock(&in_ret->lock);
    }
}

void force_in_standby(struct tiny_audio_device *adev,AUDIO_HW_APP_T audio_app_type)
{
    struct tiny_stream_in *in=NULL;
    struct tiny_stream_in *in_ret=NULL;

    struct listnode *item;
    struct listnode *tmp;
    struct listnode *list=&adev->active_input_list;
    bool empty=false;
    int stream_status=0;

    LOG_I("force_in_standby audio_app_type:0x%x in:%p node:%p list:%p",
        audio_app_type,in,&(in->node),list);

    pthread_mutex_lock(&adev->lock);
    list_for_each_safe(item, tmp, list){
        LOG_I("force_in_standby item:%p next:%p",item,item->next);
        in = node_to_item(item, struct tiny_stream_in, node);
        if(in->audio_app_type==audio_app_type){
            in_ret=in;
            break;
         }
    }

    if((NULL!=in_ret) && (NULL!=in_ret->standby_fun)){
        pthread_mutex_lock(&in_ret->lock);
        in_ret->standby_fun(adev,in_ret,in_ret->audio_app_type);
        pthread_mutex_unlock(&in_ret->lock);
    }

    pthread_mutex_unlock(&adev->lock);
    LOG_I("force_in_standby:exit");
}


static int in_standby(struct audio_stream *stream)
{
    int status=0;
    struct tiny_stream_in *in = (struct tiny_stream_in *)stream;
    LOG_I("in_standby:%p",stream);
    if(NULL!=stream){
        status = do_input_standby(in);
    }
    return status;
}

static int in_dump(const struct audio_stream *stream, int fd)
{
    return 0;
}

static int in_set_parameters(struct audio_stream *stream, const char *kvpairs)
{
    struct tiny_stream_in *in = (struct tiny_stream_in *)stream;
    struct tiny_audio_device *adev = in->dev;
    struct audio_control *actl=adev->dev_ctl;

    struct str_parms *parms;
    char *str;
    char value[32];
    int ret, val = 0;

    parms = str_parms_create_str(kvpairs);

    ret =
        str_parms_get_str(parms, AUDIO_PARAMETER_STREAM_INPUT_SOURCE, value,
                          sizeof(value));
    if (ret >= 0) {
        val = atoi(value);
        //TODO
    }

    ret =
        str_parms_get_str(parms, AUDIO_PARAMETER_STREAM_ROUTING, value,
                          sizeof(value));
    if (ret >= 0) {
        val = atoi(value);
        if(in->audio_app_type == AUDIO_HW_APP_FM_RECORD){
            LOG_I("FM is recording");
        }else{
            pthread_mutex_lock(&in->lock);
            if (val != 0) {
                in->devices = val;
            }
            pthread_mutex_unlock(&in->lock);
            select_devices_new(adev->dev_ctl,in->audio_app_type,val,true,true,true);
        }
        ret = 0;
    }

    str_parms_destroy(parms);
    return ret;
}

static char *in_get_parameters(const struct audio_stream *stream,
                               const char *keys)
{
    return strdup("");
}

static int in_set_gain(struct audio_stream_in *stream, float gain)
{
    return 0;
}

static int get_next_buffer(struct resampler_buffer_provider *buffer_provider,
                           struct resampler_buffer *buffer)
{
    struct tiny_stream_in *in;
    struct tiny_audio_device *adev = NULL;
    int ret=-1;

    int period_size=0;
    if (buffer_provider == NULL || buffer == NULL)
    { return -EINVAL; }

    in = container_of(buffer_provider, struct tiny_stream_in, buf_provider);
    adev =(struct tiny_audio_device *) in->dev;

    period_size=in->config->period_size *
                                           audio_stream_in_frame_size_l((const struct audio_stream *)(&in->stream.common));

    if ((in->pcm == NULL) && (in->mux_pcm == NULL)) {
        buffer->raw = NULL;
        buffer->frame_count = 0;
        in->read_status = -ENODEV;
        return -ENODEV;
    }

    if (in->frames_in == 0){
        if (in->pcm) {
            if((in->config->rate == 48000) && (in->rec_nr_handle) && (in->active_rec_proc)){
                ret = AudioRecordNr_Proc(in->rec_nr_handle,in->nr_buffer, period_size);
                if(ret <0){
                    LOG_E("AudioRecordNr_Proc  error");
                }

                memcpy(in->buffer,in->nr_buffer,period_size);
                if(adev->dev_ctl->config.dump.record.nr.hex_fd>0){
                    write(adev->dev_ctl->config.dump.record.nr.hex_fd,(void *)in->nr_buffer,period_size);
                }
                in->read_status =ret;
            }else{
                in->read_status = _pcm_read(in, in->buffer, period_size);

                if(in->active_rec_proc){
                    aud_rec_do_process(in->buffer, period_size, in->proc_buf, in->proc_buf_size);
                    if(adev->dev_ctl->config.dump.record.process.hex_fd>0){
                        write(adev->dev_ctl->config.dump.record.process.hex_fd,in->buffer,period_size);
                    }
                }
            }
        }

        if (in->read_status != 0) {
            if (in->pcm) {
                LOG_E
                ("get_next_buffer() pcm_read sattus=%d, error: %s",
                 in->read_status, pcm_get_error(in->pcm));
            }
            buffer->raw = NULL;
            buffer->frame_count = 0;
            return in->read_status;
        }
        in->frames_in = in->config->period_size;
    }
    buffer->frame_count = (buffer->frame_count > in->frames_in) ?
                          in->frames_in : buffer->frame_count;
    buffer->i16 = in->buffer + (in->config->period_size - in->frames_in) *
                  in->config->channels;

    return in->read_status;
}

static void release_buffer(struct resampler_buffer_provider *buffer_provider,
                           struct resampler_buffer *buffer)
{
    struct tiny_stream_in *in;

    if (buffer_provider == NULL || buffer == NULL)
    { return; }

    in = container_of(buffer_provider, struct tiny_stream_in, buf_provider);

    in->frames_in -= buffer->frame_count;
}

/*  read_frames() reads frames from kernel driver, down samples to capture rate
    if necessary and output the number of frames requested to the buffer specified */
static ssize_t read_frames(struct tiny_stream_in *in, void *buffer,
                           ssize_t frames)
{
    struct tiny_audio_device *adev = in->dev;
    ssize_t frames_wr = 0;
    while (frames_wr < frames) {
        size_t frames_rd = frames - frames_wr;
        if (in->resampler != NULL) {
            LOG_D("resample_from_provider");
            in->resampler->resample_from_provider(in->resampler,
                (int16_t *) ((char *) buffer +frames_wr *audio_stream_in_frame_size_l(
                (const struct audio_stream *)(&in->stream.common))),&frames_rd);
        } else {
            struct resampler_buffer buf = {
                {raw:NULL,},
                frame_count:
                frames_rd,
            };
            get_next_buffer(&in->buf_provider, &buf);
            if (buf.raw != NULL) {
                memcpy((char *)buffer +
                       frames_wr *
                       audio_stream_in_frame_size_l((const struct
                                                audio_stream
                                                *)(&in->stream.
                                                   common)),
                       buf.raw,
                       buf.frame_count *
                       audio_stream_in_frame_size_l((const struct
                                                audio_stream
                                                *)(&in->stream.
                                                   common)));
                frames_rd = buf.frame_count;
            }
            release_buffer(&in->buf_provider, &buf);
        }
        if (in->read_status != 0) {
            return in->read_status;
        }

        frames_wr += frames_rd;
    }
    return frames_wr;
}

static bool in_bypass_data(struct tiny_stream_in *in, voice_status_t voice_status)
{
    struct tiny_audio_device *adev = in->dev;

    if((AUDIO_HW_APP_CALL_RECORD==in->audio_app_type) && (voice_status!=VOICE_START_STATUS)){
        return true;
    }

    if ((is_usecase(adev->dev_ctl,UC_AGDSP_ASSERT)) ||
          (((VOICE_PRE_START_STATUS ==voice_status)
        || (VOICE_PRE_STOP_STATUS ==voice_status)) && (AUDIO_HW_APP_VOIP_RECORD== in->audio_app_type))){
        return true;
    } else {
        return false;
    }
}

static ssize_t read_pcm_data(void *stream, void* buffer,
        size_t bytes)
{
    int ret =0;
    struct audio_stream_in *stream_tmp=(struct audio_stream_in *)stream;
    struct tiny_stream_in *in = (struct tiny_stream_in *)stream;
    struct tiny_audio_device *adev = in->dev;
    size_t frames_rq = bytes / audio_stream_in_frame_size_l((const struct audio_stream *)(&stream_tmp->common));

    if (in->resampler != NULL) {
        LOG_D("in_read %d start frames_rq:%d,in->config->channels %d",bytes,frames_rq,in->config->channels);
        ret = read_frames(in, buffer, frames_rq);
        if (ret != frames_rq) {
            LOG_E("ERR:in_read0");
            ret = -1;
        } else {

        /*
            if(in->config->channels==1){
                _pcm_mixer(buffer, frames_rq);
            }
        */
            ret = 0;
        }
        LOG_D("in_read end:%d",ret);
    } else {
            if((in->config->rate == 48000) && (in->rec_nr_handle) && (in->active_rec_proc)){
                ret = AudioRecordNr_Proc(in->rec_nr_handle,buffer, bytes);
                if(ret <0){
                    LOG_E("AudioRecordNr_Proc  error");
                }
                if(adev->dev_ctl->config.dump.record.nr.hex_fd>0){
                    write(adev->dev_ctl->config.dump.record.nr.hex_fd,(void *)in->nr_buffer,bytes);
                }
            }
            else{
                ret = _pcm_read(in, buffer, bytes);
                if(in->active_rec_proc){
                    aud_rec_do_process(buffer, bytes, in->proc_buf, in->proc_buf_size);
                    if(adev->dev_ctl->config.dump.record.process.hex_fd>0){
                        write(adev->dev_ctl->config.dump.record.process.hex_fd,buffer,bytes);
                    }
                }
            }
    }

    if(adev->dev_ctl->config.dump.record.mixer_vbc.hex_fd>0){
        write(adev->dev_ctl->config.dump.record.mixer_vbc.hex_fd,buffer,
                               bytes);
    }
    return ret;
}


int in_app_type_check(struct tiny_audio_device *adev, int audio_app_type,bool voip_start,
    bool fm_record,voice_status_t voice_status,audio_devices_t devices){
    int ret_type = audio_app_type;

    if(audio_app_type==AUDIO_HW_APP_CALL_RECORD){
        return AUDIO_HW_APP_CALL_RECORD;
    }

    if(true==voip_start){
        ret_type = AUDIO_HW_APP_VOIP_RECORD;
    }else{
        if(true == fm_record){
            ret_type = AUDIO_HW_APP_FM_RECORD;
        }else if(is_call_active_unlock(adev)){
            if(false==is_bt_voice(adev)){
                ret_type=AUDIO_HW_APP_NORMAL_RECORD;
            }else{
                ret_type = AUDIO_HW_APP_BT_RECORD;
            }
        }else{
            if(AUDIO_DEVICE_IN_ALL_SCO&((~AUDIO_DEVICE_BIT_IN)&devices)){
                ret_type = AUDIO_HW_APP_BT_RECORD;
            }else {
                ret_type = AUDIO_HW_APP_NORMAL_RECORD;
            }
        }
    }

    if(ret_type != audio_app_type) {
        LOG_W("in_app_type_check ret type: %d,adev->fm_record %d,adev->voip_start %d voice_status:%d devices:0x%x",
            ret_type,adev->fm_record,adev->voip_start,voice_status,devices);
    }
    return ret_type;
}

static int32_t stereo2mono(int16_t *out, int16_t * in, uint32_t in_samples) {
    int i = 0;
    int out_samples =  in_samples >> 1;
    for(i = 0 ; i< out_samples ; i++) {
        out[i] =(in[2*i+1] + in[2*i]) /2;
    }
    return out_samples;
}

static int32_t  mono2stereo(int16_t *out, int16_t * in, uint32_t in_samples) {
    int i = 0;
    int out_samples = in_samples<<1;
    for(i = 0 ; i< in_samples; i++) {
        out[2*i] =in[i];
        out[2*i+1] = in[i];
    }
    return out_samples ;
}

static ssize_t in_read(struct audio_stream_in *stream, void *buffer,
                       size_t bytes)
{
    int ret = 0;
    struct tiny_stream_in *in = (struct tiny_stream_in *)stream;
    struct tiny_audio_device *adev = in->dev;
    int cur_app_type = 0;
    bool voip_start=false;
    bool fm_record=false;
    voice_status_t voice_status;
    int frames_rq =bytes /audio_stream_frame_size((const struct audio_stream *)(&stream->common));

    LOG_D("in_read type:%d call_mode%d usecase:%d bytes:%d",in->audio_app_type,adev->call_mode,adev->dev_ctl->usecase,bytes);

    pthread_mutex_lock(&adev->lock);
    pthread_mutex_lock(&in->lock);
    voip_start=adev->voip_start;
    fm_record=adev->fm_record;
    voice_status=adev->call_status;

    if(is_call_active_unlock(adev)
        || is_stream_active_unlock(adev, AUDIO_HW_APP_OFFLOAD)) {
        voip_start=false;
    }

    cur_app_type = in_app_type_check(adev,in->audio_app_type,voip_start,fm_record,voice_status,in->devices);
    if(in->audio_app_type != cur_app_type){
        if(in->standby_fun){
            in->standby_fun(adev,in,in->audio_app_type);
        }
        in->audio_app_type = cur_app_type;
        LOG_W("in_read audio_app_type:%d",in->audio_app_type);
    }

    if (in_bypass_data(in,voice_status)) {
        pthread_mutex_unlock(&in->lock);
        pthread_mutex_unlock(&adev->lock);
        LOG_I("in_read type:%x bypass voice_status:%d",in->audio_app_type,voice_status);
        memset(buffer,0,bytes);
        usleep((int64_t) bytes * 1000000 / audio_stream_frame_size(&stream->common) / in_get_sample_rate(&stream->common));
        return bytes;
    }

    if (in->standby) {
        ret = start_input_stream(in);
        if (ret < 0) {
            LOG_E("start_input_stream error ret=%d", ret);
            pthread_mutex_unlock(&adev->lock);
            goto error;
        }
        in->standby = false;
    }

    pthread_mutex_unlock(&adev->lock);


    if(NULL == in->pcm){
        LOG_E("in_read pcm err");
        ret=-1;
        goto error;
    }else{
        if(in->requested_channels != in->config->channels){
            if(in->channel_buffer==NULL){
                in->channel_buffer=malloc(bytes*2);
            }

            if((in->requested_channels == 1) && (in->config->channels == 2)) {
                ret = read_pcm_data(in, in->channel_buffer, bytes * 2);
                if(ret < 0) {
                    goto error;
                }
                stereo2mono(buffer,in->channel_buffer,bytes);
            }else if((in->requested_channels == 2) && (in->config->channels == 1)) {
                ret = read_pcm_data(in, in->channel_buffer, bytes /2);
                if(ret < 0) {
                    goto error;
                }
                mono2stereo(buffer,in->channel_buffer,bytes/4);
            }
        }else {
            ret = read_pcm_data(in, buffer, bytes);
        }

        if(ret < 0) {
            goto error;
        }
    }

    if(adev->dev_ctl->config.dump.record.normal.hex_fd > 0) {
        write(adev->dev_ctl->config.dump.record.normal.hex_fd, buffer, bytes);
    }
    pthread_mutex_unlock(&in->lock);

    if (bytes > 0) {
        in->frames_read += bytes / audio_stream_in_frame_size(stream);
    }
    return bytes;

error:

    if (ret < 0){
        LOG_I("in_read do_normal_inputput_standby:%p", in->pcm);
        if (NULL != in->pcm) {
            LOG_E("in_read  do_normal_inputput_standby pcm_close:%p, error: %s", in->pcm, pcm_get_error(in->pcm));
        }
        if(in->standby==false)
            in->standby_fun(adev,in,in->audio_app_type);
        memset(buffer, 0,bytes);
        pthread_mutex_unlock(&in->lock);
        usleep((int64_t) bytes * 1000000 / audio_stream_frame_size(&stream->common) / in_get_sample_rate(&stream->common));
        return  bytes;
    }else{
        LOG_D("in_read:%d", bytes);
        if(adev->dev_ctl->config.dump.record.normal.hex_fd > 0) {
            write(adev->dev_ctl->config.dump.record.normal.hex_fd, buffer, bytes);
        }
    }
    pthread_mutex_unlock(&in->lock);
    return  bytes;
}

static uint32_t in_get_input_frames_lost(struct audio_stream_in *stream)
{
    return 0;
}

#ifdef AUDIO_HAL_ANDROID_N_API
static int in_get_capture_position(const struct audio_stream_in *stream,
                                   int64_t *frames, int64_t *time)
{
    if (stream == NULL || frames == NULL || time == NULL) {
        return -EINVAL;
    }
    struct tiny_stream_in *in = (struct tiny_stream_in *)stream;
    int ret = -ENOSYS;

    pthread_mutex_lock(&in->lock);
    if (in->pcm) {
        struct timespec timestamp;
        unsigned int avail;
        if (pcm_get_htimestamp(in->pcm, &avail, &timestamp) == 0) {
            *frames = in->frames_read + avail;
            *time = timestamp.tv_sec * 1000000000LL + timestamp.tv_nsec;
            ret = 0;
        }
    }
    pthread_mutex_unlock(&in->lock);
    return ret;
}
#endif

static int adev_open_output_stream(struct audio_hw_device *dev,
                                   audio_io_handle_t handle,
                                   audio_devices_t devices,
                                   audio_output_flags_t flags,
                                   struct audio_config *config,
                                   struct audio_stream_out **stream_out)
{
    struct tiny_audio_device *ladev = (struct tiny_audio_device *)dev;
    struct tiny_stream_out *out=NULL;
    struct audio_control *control = NULL;
    int ret;
    control = ladev->dev_ctl;

    LOG_I("%s, devices = %d flags:0x%x rate:%d", __func__, devices,flags,config->sample_rate);

    if(0 ==  (flags & AUDIO_OUTPUT_FLAG_COMPRESS_OFFLOAD)){
        if(config->sample_rate!=control->pcm_handle.record[AUD_PCM_MM_NORMAL].rate){
            LOG_I("adev_open_input_stream sample_rate:%d is not support",config->sample_rate);
            //config->sample_rate = control->pcm_handle.record[AUD_PCM_MM_NORMAL].rate;
            //return -1;
        }
    }

    //config->sample_rate=control->pcm_handle.record[AUD_PCM_MM_NORMAL].rate;

    out =
        (struct tiny_stream_out *)calloc(1, sizeof(struct tiny_stream_out));
    if (!out) {
        LOG_E("adev_open_output_stream calloc fail, size:%d",
              sizeof(struct tiny_stream_out));
        return -ENOMEM;
    }
    LOG_I("adev_open_output_stream out:%p",out);

    memset(out, 0, sizeof(struct tiny_stream_out));
    out->config = (struct pcm_config *) malloc(sizeof(struct pcm_config));
    if(!out->config){
        ALOGE(" adev_open_output_stream malloc failed");
        goto err_open;
    }
    pthread_mutex_init(&out->lock, NULL);

    if (flags & AUDIO_OUTPUT_FLAG_COMPRESS_OFFLOAD) {
        /* check offload supported information */
        if (config->offload_info.version != 1 ||
            config->offload_info.size != AUDIO_INFO_INITIALIZER.size) {
            LOG_E("%s: offload information is not supported ", __func__);
            ret = -EINVAL;
            goto err_open;
        }
        if (!audio_is_offload_support_format(config->offload_info.format)) {
            LOG_E("%s: offload audio format(%d) is not supported ",
                  __func__, config->offload_info.format);
            ret = -EINVAL;
            goto err_open;
        }
        /*  codec type and parameters requested */
        out->compress_config.codec = (struct snd_codec *)calloc(1,
                                     sizeof(struct snd_codec));
        out->audio_app_type = AUDIO_HW_APP_OFFLOAD;
        out->offload_format = config->format;
        out->offload_samplerate = config->sample_rate;
        out->offload_channel_mask = config->channel_mask;

        out->stream.set_callback = out_offload_set_callback;
        out->stream.pause = out_offload_pause;
        out->stream.resume = out_offload_resume;
        out->stream.drain = out_offload_drain;
        out->stream.flush = out_offload_flush;

        out->compress_config.codec->id =
            audio_get_offload_codec_id(config->offload_info.format);
        out->compress_config.fragment_size = ladev->dev_ctl->pcm_handle.compress.fragment_size;
        out->compress_config.fragments = ladev->dev_ctl->pcm_handle.compress.fragments;
        out->compress_config.codec->sample_rate =
            compress_get_alsa_rate(config->offload_info.sample_rate);
        out->compress_config.codec->bit_rate =
            config->offload_info.bit_rate;
        out->compress_config.codec->ch_in =
            popcount(config->channel_mask);
        out->compress_config.codec->ch_out = out->compress_config.codec->ch_in;

        memcpy(out->config, &(control->pcm_handle.play[AUD_PCM_OFF_LOAD]), sizeof(struct pcm_config));

        LOG_I("compress_config fragment_size:0x%x fragments:0x%x",
            out->compress_config.fragment_size,out->compress_config.fragments);

        if (flags & AUDIO_OUTPUT_FLAG_NON_BLOCKING) {
            out->is_offload_nonblocking = 1;
        }
        out->is_offload_need_set_metadata = 1;
        audio_offload_create_thread(out);
    } else {
        out->buffer = malloc(RESAMPLER_BUFFER_SIZE);	/* todo: allow for reallocing */
        if (NULL == out->buffer) {
            LOG_E("adev_open_output_stream out->buffer alloc fail, size:%d",
                  RESAMPLER_BUFFER_SIZE);
            goto err_open;
        } else {
            memset(out->buffer, 0, RESAMPLER_BUFFER_SIZE);
        }

        if(flags & AUDIO_OUTPUT_FLAG_DEEP_BUFFER){
            out->audio_app_type = AUDIO_HW_APP_DEEP_BUFFER;
            memcpy(out->config, &(control->pcm_handle.play[AUD_PCM_DEEP_BUFFER]), sizeof(struct pcm_config));
        }else{
            out->audio_app_type = AUDIO_HW_APP_PRIMARY;
            memcpy(out->config, &(control->pcm_handle.play[AUD_PCM_MM_NORMAL]), sizeof(struct pcm_config));
        }
    }
    out->stream.common.get_sample_rate = out_get_sample_rate;
    out->stream.common.set_sample_rate = out_set_sample_rate;
    out->stream.common.get_buffer_size = out_get_buffer_size;
    out->stream.common.get_channels = out_get_channels;
    out->stream.common.get_format = out_get_format;
    out->stream.common.set_format = out_set_format;
    out->stream.common.standby = out_standby;
    out->stream.common.dump = out_dump;
    out->stream.common.set_parameters = out_set_parameters;
    out->stream.common.get_parameters = out_get_parameters;
    out->stream.common.add_audio_effect = out_add_audio_effect;
    out->stream.common.remove_audio_effect = out_remove_audio_effect;
    out->stream.get_latency = out_get_latency;
    out->stream.set_volume = out_set_volume;
    out->stream.write = out_write;
    out->stream.get_render_position = out_get_render_position;
    out->stream.get_presentation_position = out_get_presentation_position;

    LOG_I("normal pcm:%d %d %d", out->config->rate, out->config->period_size,
              out->config->period_size);
    out->dev = ladev;
    out->standby = true;
    out->devices = devices;
    out->flags = flags;
    out->is_voip = false;

    config->format = out->stream.common.get_format(&out->stream.common);
    config->channel_mask =
        out->stream.common.get_channels(&out->stream.common);
    config->sample_rate =
        out->stream.common.get_sample_rate(&out->stream.common);

    out->requested_rate = config->sample_rate;

    audio_add_output(ladev,out);

    LOG_I("adev_open_output_stream Successful audio_app_type:%d out:%p",out->audio_app_type,out);
    *stream_out = &out->stream;
    return 0;

err_open:
    LOG_E("Error adev_open_output_stream");
    if(out->config){
        free(out->config);
        out->config = NULL;
    }
    free(out);
    *stream_out = NULL;
    return ret;
    return 0;
}

static void adev_close_output_stream(struct audio_hw_device *dev,
                                     struct audio_stream_out *stream)
{
    struct tiny_stream_out *out = (struct tiny_stream_out *)stream;
    LOG_I("adev_close_output_stream");
    out_standby(&stream->common);
    if (out->buffer) {
        free(out->buffer);
    }
    if (out->resampler) {
        release_resampler(out->resampler);
        out->resampler = NULL;
    }

    if (out->audio_app_type == AUDIO_HW_APP_OFFLOAD) {
        audio_offload_destroy_thread(out);
        if (out->compress_config.codec != NULL)
        { free(out->compress_config.codec); }
    }

    if(out->config){
        free(out->config);
        out->config = NULL;
    }

    audio_del_output(dev,out);
    free(stream);
}

static int adev_set_parameters(struct audio_hw_device *dev, const char *kvpairs)
{
   return ext_contrtol_process((struct tiny_audio_device *)dev,kvpairs);
}

static char *adev_get_parameters(const struct audio_hw_device *dev,
                                 const char *keys)
{
    return strdup("");
}

static int adev_init_check(const struct audio_hw_device *dev)
{
    LOG_W("adev_init_check");
    return 0;
}

int adev_set_voice_volume(struct audio_hw_device *dev, float volume)
{
    struct tiny_audio_device *adev = (struct tiny_audio_device *)dev;
    int voice_volume = 0;
    pthread_mutex_lock(&adev->lock);
    voice_volume = (int)(volume * 6);
    if(voice_volume >= 5){
        voice_volume = 5;
    }
    adev->voice_volume=voice_volume;
    LOG_I("adev_set_voice_volume volume:%f level:%d", volume,voice_volume);

    /*Send at command to dsp side */
    if((is_call_active_unlock(adev)) || (true==adev->voip_start))
        set_dsp_volume(adev->dev_ctl,voice_volume);

    pthread_mutex_unlock(&adev->lock);
    return 0;
}

static int adev_set_master_volume(struct audio_hw_device *dev, float volume)
{
    return -ENOSYS;
}

int set_voice_status(struct audio_hw_device *dev, voice_status_t status){
    struct tiny_audio_device *adev = (struct tiny_audio_device *)dev;
   // pthread_mutex_lock(&adev->lock);
    adev->call_status = status;
    LOG_I("set_voice_status:%d",status);
   // pthread_mutex_unlock(&adev->lock);
    return 0;
}

static int adev_set_mode(struct audio_hw_device *dev, audio_mode_t mode)
{
    struct tiny_audio_device *adev = (struct tiny_audio_device *)dev;

    LOG_E("adev_set_mode:%d adev mode:%d",mode,adev->call_mode);
    if(true==is_usecase(adev->dev_ctl,UC_LOOP)){
        LOG_E("adev_set_mode looping now, can not start voice call");
        return 0;
    }
    pthread_mutex_lock(&adev->lock);
    if (adev->call_mode != mode){
        adev->call_mode = mode;
        if (((mode == AUDIO_MODE_NORMAL) || (mode == AUDIO_MODE_IN_COMMUNICATION))&&(true== adev->call_start)){
            adev->call_start=false;
            pthread_mutex_unlock(&adev->lock);
            send_cmd_to_dsp_thread(adev->dev_ctl->agdsp_ctl,AUDIO_CTL_STOP_VOICE,NULL);
            return 0;
        }
    }
    pthread_mutex_unlock(&adev->lock);
    return 0;
}


int ext_setVoiceMode(void * dev,struct str_parms *parms,int mode, char * val){
    struct tiny_audio_device *adev = (struct tiny_audio_device *)dev;
    adev_set_mode(dev,mode);
    if(AUDIO_MODE_IN_CALL == mode){
        pthread_mutex_lock(&adev->lock);
        adev->call_start=true;
        pthread_mutex_unlock(&adev->lock);
        send_cmd_to_dsp_thread(adev->dev_ctl->agdsp_ctl,AUDIO_CTL_START_VOICE,NULL);
    }
    return 0;
}

static int adev_set_master_mute(struct audio_hw_device *dev, bool mute)
{
    struct tiny_audio_device *adev = (struct tiny_audio_device *)dev;
    int ret=-ENOSYS;
    pthread_mutex_lock(&adev->lock);
    if(mute!=adev->master_mute){
        LOG_I("adev_set_master_mute:%d",mute);
        adev->master_mute = mute;
    }
    pthread_mutex_unlock(&adev->lock);
    return 0;
}

static int adev_get_master_mute(struct audio_hw_device *dev, bool *mute)
{
    struct tiny_audio_device *adev = (struct tiny_audio_device *)dev;
    pthread_mutex_lock(&adev->lock);
    *mute = adev->master_mute;
    pthread_mutex_unlock(&adev->lock);
    return 0;
}

static int adev_set_mic_mute(struct audio_hw_device *dev, bool mute)
{
    struct tiny_audio_device *adev = (struct tiny_audio_device *)dev;
    int ret=-ENOSYS;
    pthread_mutex_lock(&adev->lock);
    if(mute!=adev->mic_mute){
        LOG_I("adev_set_mic_mute:%d",mute);
        if(is_usecase(adev->dev_ctl,UC_CALL)){
            ret=set_voice_ul_mute(adev->dev_ctl,mute);
            if(ret==0){
                LOG_I("adev_set_mic_mute:%d",mute);
            }else{
                LOG_W("adev_set_mic_mute failed:%d :%d",mute,adev->mic_mute);
            }
        }
        adev->mic_mute = mute;
    }
    pthread_mutex_unlock(&adev->lock);
    return 0;
}

static int adev_get_mic_mute(const struct audio_hw_device *dev, bool *state)
{
    struct tiny_audio_device *adev = (struct tiny_audio_device *)dev;
    pthread_mutex_lock(&adev->lock);
    *state = adev->mic_mute;
    pthread_mutex_unlock(&adev->lock);
    return 0;
}

static size_t adev_get_input_buffer_size(const struct audio_hw_device *dev,
        const struct audio_config *config)
{
    size_t size;
    int channel_count = popcount(config->channel_mask);

    if (check_input_parameters
        (config->sample_rate, config->format, channel_count) != 0) {
        return 0;
    }

    return get_input_buffer_size((struct tiny_audio_device *)dev,
                                 config->sample_rate, config->format,
                                 channel_count);
}

static int in_add_audio_effect(const struct audio_stream *stream,
                               effect_handle_t effect)
{
    return 0;
}

static int in_remove_audio_effect(const struct audio_stream *stream,
                                  effect_handle_t effect)
{
    return 0;
}

int adev_open_input_stream(struct audio_hw_device *dev,
                                  audio_io_handle_t handle,
                                  audio_devices_t devices,
                                  struct audio_config *config,
                                  struct audio_stream_in **stream_in,
                                  audio_input_flags_t flags,
                                  const char *address __unused,
                                  audio_source_t source )
{
    struct tiny_audio_device *ladev = (struct tiny_audio_device *)dev;
    struct tiny_stream_in *in;
    struct audio_control *control = NULL;
    int pcm_devices;
    int ret = 0;
    int channel_count = popcount(config->channel_mask);
    control = ladev->dev_ctl;

    LOG_I
    ("adev_open_input_stream,devices=0x%x,sample_rate=%d, channel_count=%d flags:%x source:%d",
     devices, config->sample_rate, channel_count,flags,source);

    if (check_input_parameters
        (config->sample_rate, config->format, channel_count) != 0) {
        LOG_E("adev_open_input_stream:check_input_parameters error");
        return -EINVAL;
    }

    if(config->sample_rate!=control->pcm_handle.record[AUD_PCM_MM_NORMAL].rate){
        LOG_E("adev_open_input_stream sample_rate:%d is not support",config->sample_rate);
     //   config->sample_rate = control->pcm_handle.record[AUD_PCM_MM_NORMAL].rate;
    //  return -1;
    }

    in = (struct tiny_stream_in *)calloc(1, sizeof(struct tiny_stream_in));
    if (!in) {
        LOG_E("adev_open_input_stream alloc fail, size:%d",
              sizeof(struct tiny_stream_in));
        return -ENOMEM;
    }
    memset(in, 0, sizeof(struct tiny_stream_in));
    pthread_mutex_init(&in->lock, NULL);
    in->stream.common.get_sample_rate = in_get_sample_rate;
    in->stream.common.set_sample_rate = in_set_sample_rate;
    in->stream.common.get_buffer_size = in_get_buffer_size;
    in->stream.common.get_channels = in_get_channels;
    in->stream.common.get_format = in_get_format;
    in->stream.common.set_format = in_set_format;
    in->stream.common.standby = in_standby;
    in->stream.common.dump = in_dump;
    in->stream.common.set_parameters = in_set_parameters;
    in->stream.common.get_parameters = in_get_parameters;
    in->stream.common.add_audio_effect = in_add_audio_effect;
    in->stream.common.remove_audio_effect = in_remove_audio_effect;
    in->stream.set_gain = in_set_gain;
    in->stream.read = in_read;
    in->stream.get_input_frames_lost = in_get_input_frames_lost;
#ifdef AUDIO_HAL_ANDROID_N_API
    in->stream.get_capture_position = in_get_capture_position;
#endif

    in->requested_rate = config->sample_rate;
    in->requested_channels = channel_count;

    in->resampler = NULL;
    in->standby_fun=do_normal_inputput_standby;

    in->source=source;
    in-> config = (struct pcm_config *) malloc(sizeof(struct pcm_config));
    if(!in-> config){
        ALOGE(" adev_open_input_stream malloc failed");
        goto err;
    }

    pthread_mutex_lock(&ladev->lock);

    if(AUDIO_SOURCE_VOICE_CALL==in->source){
        in->audio_app_type =AUDIO_HW_APP_CALL_RECORD;
    }else if((AUDIO_SOURCE_FM_TUNER==in->source) ||(true==ladev->fm_record)){
        in->audio_app_type =AUDIO_HW_APP_FM_RECORD;
    }else if(AUDIO_SOURCE_CAMCORDER==in->source){
        in->audio_app_type =AUDIO_HW_APP_NORMAL_RECORD;
    }else if(true==ladev->voip_start){
        in->audio_app_type =AUDIO_HW_APP_VOIP_RECORD;
    }else if(AUDIO_DEVICE_IN_ALL_SCO == devices){
        in->audio_app_type =AUDIO_HW_APP_BT_RECORD;
    }else{
        in->audio_app_type =AUDIO_HW_APP_NORMAL_RECORD;
    }
    pthread_mutex_unlock(&ladev->lock);

    ret = dev_ctl_get_in_pcm_config(control, in->audio_app_type, &pcm_devices, in->config);
    if(ret != 0 ){
        goto err;
    }

    if (in->requested_rate) {
        in->pop_mute_bytes =
            RECORD_POP_MIN_TIME * in->requested_rate / 1000 *
            audio_stream_frame_size((const struct audio_stream
                                     *)(&(in->stream).common));
    }
    in->dev = ladev;
    in->standby = true;
    in->devices = devices;
    in->pop_mute = true;

    *stream_in = &in->stream;

     audio_add_input(ladev,in);
     LOG_I
    ("adev_open_input_stream,devices=0x%x,sample_rate=%d, channel_count=%d",
     devices, config->sample_rate, in->config->channels);
    LOG_D("Successfully, adev_open_input_stream.");
    return 0;

err:

    LOG_E("Failed(%d), adev_open_input_stream.", ret);
    if (in->buffer) {
        free(in->buffer);
        in->buffer = NULL;
    }
    if (in->resampler) {
        release_resampler(in->resampler);
        in->resampler = NULL;
    }
    if (in->proc_buf) {
        free(in->proc_buf);
        in->proc_buf = NULL;
    }
    if(in->config){
        free(in->config);
        in->config = NULL;
    }
    in->proc_buf_size=0;

    free(in);
    *stream_in = NULL;
    return ret;
}

static void adev_close_input_stream(struct audio_hw_device *dev,
                                    struct audio_stream_in *stream)
{
    struct tiny_stream_in *in = (struct tiny_stream_in *)stream;

    LOG_D("adev_close_input_stream");
    in_standby(&stream->common);
    audio_del_input(dev,in);

    if (in->resampler) {
        free(in->buffer);
        release_resampler(in->resampler);
        in->resampler=NULL;
    }

    if (in->nr_buffer) {
        free(in->nr_buffer);
        in->nr_buffer = NULL;
    }

    if (in->proc_buf) {
        free(in->proc_buf);
        in->proc_buf = NULL;
    }
    in->proc_buf_size = 0;

    if(in->config){
        free(in->config);
        in->config = NULL;
    }
    LOG_I("%s %d",__func__,__LINE__);
    free(stream);
    return;
}

static int adev_dump(const audio_hw_device_t *device, int fd)
{
    return 0;
}

static void close_all_stream(void *dev){
    struct tiny_stream_out *out;
    struct tiny_stream_in *in;
    struct listnode *item;
    struct listnode *item2;
    struct tiny_audio_device *adev = (struct tiny_audio_device *)dev;
    LOG_I("close_all_stream stream_status:0x%x",adev->stream_status)
    pthread_mutex_lock(&adev->lock);
    if(!list_empty(&adev->active_out_list)){
        list_for_each_safe(item, item2,&adev->active_out_list){
            out = node_to_item(item, struct tiny_stream_out, node);
            pthread_mutex_unlock(&adev->lock);
            adev_close_output_stream(adev,out);
            pthread_mutex_lock(&adev->lock);
        }
    }
    if(!list_empty(&adev->active_input_list)){
        list_for_each_safe(item, item2,&adev->active_input_list){
            in = node_to_item(item, struct tiny_stream_in, node);
            pthread_mutex_unlock(&adev->lock);
            adev_close_input_stream(adev,in);
            pthread_mutex_lock(&adev->lock);
        }
    }
    pthread_mutex_unlock(&adev->lock);
    LOG_I("close_all_stream:exit");
}
static int adev_close(hw_device_t *device)
{
    struct tiny_audio_device *adev = (struct tiny_audio_device *)device;
    LOG_E("adev_close");
    free_audio_control(adev->dev_ctl);
    free(adev->dev_ctl);
    free_audio_param(&adev->audio_param);
    stream_routing_manager_close(adev->dev_ctl);
    force_all_standby(adev);
    dsp_ctrl_close(adev->dev_ctl->agdsp_ctl);
    free(device);
    return 0;
}

static uint32_t adev_get_supported_devices(const struct audio_hw_device *dev)
{
    return (		/* OUT */
               AUDIO_DEVICE_OUT_EARPIECE |
               AUDIO_DEVICE_OUT_SPEAKER |
               AUDIO_DEVICE_OUT_WIRED_HEADSET |
               AUDIO_DEVICE_OUT_WIRED_HEADPHONE |
               AUDIO_DEVICE_OUT_AUX_DIGITAL |
               AUDIO_DEVICE_OUT_ANLG_DOCK_HEADSET |
               AUDIO_DEVICE_OUT_DGTL_DOCK_HEADSET |
               AUDIO_DEVICE_OUT_ALL_SCO |
               AUDIO_DEVICE_OUT_FM | AUDIO_DEVICE_OUT_DEFAULT |
               /* IN */
               AUDIO_DEVICE_IN_COMMUNICATION |
               AUDIO_DEVICE_IN_AMBIENT |
               AUDIO_DEVICE_IN_BUILTIN_MIC |
               AUDIO_DEVICE_IN_WIRED_HEADSET |
               AUDIO_DEVICE_IN_AUX_DIGITAL |
               AUDIO_DEVICE_IN_BACK_MIC |
               AUDIO_DEVICE_IN_BLUETOOTH_SCO_HEADSET |
               AUDIO_DEVICE_IN_ALL_SCO | AUDIO_DEVICE_IN_VOICE_CALL |
               //AUDIO_DEVICE_IN_LINE_IN | //zzjtodo
               AUDIO_DEVICE_IN_DEFAULT);
}

/*
    Read audproc params from nv and config.
    return value: TRUE:success, FALSE:failed
*/
bool init_rec_process(struct audio_stream_in *stream, int sample_rate)
{
    bool ret = false;
    struct tiny_stream_in *in = (struct tiny_stream_in *)stream;
    struct tiny_audio_device *adev = in->dev;
    struct audio_record_proc_param *param_data=NULL;

    LOG_I("init_rec_process");

    if(true==adev->dev_ctl->config.record.record_process_enable) {
        int size = 0;
        int buf_size = 0;
        size = in->config->period_size;
        size = ((size + 15) / 16) * 16;
        buf_size = size * 2 * sizeof(short);
        if (in->proc_buf_size < (size_t) buf_size) {
            if (in->proc_buf)
            { free(in->proc_buf); }
            in->proc_buf = malloc(buf_size);
            if (!in->proc_buf) {
                LOG_E("init_rec_process:%d",__LINE__);
                return false;
            }
            in->proc_buf_size = buf_size;
        }
    }

    param_data=(struct audio_record_proc_param *)get_ap_record_param(adev->dev_ctl->audio_param,adev->dev_ctl->in_devices);

    if(NULL==param_data){
      LOG_I("audio record process param is null");
      return false;
    }

    LOG_D("init_rec_process AUDPROC_initDp DP_input_gain:%x COMPRESSOR_threshold:%x DP_lcf_gain_r:%x",
        param_data->dp_control.DP_input_gain,param_data->dp_control.COMPRESSOR_threshold,param_data->dp_control.DP_lcf_gain_r);
    ret = AUDPROC_initDp(&param_data->dp_control, sample_rate);
    if(true!=ret){
        LOG_W("init_rec_process AUDPROC_initDp failed");
        return false;
    }
    LOG_D("init_rec_process AUDPROC_initRecordEq RECORDEQ_sw_switch:%x RECORDEQ_band_para[2]:%x RECORDEQ_band_para[5]:%x",
        param_data->record_eq.RECORDEQ_sw_switch,param_data->record_eq.RECORDEQ_band_para[2].gain,param_data->record_eq.RECORDEQ_band_para[5].gain);

    ret = AUDPROC_initRecordEq(&param_data->record_eq, sample_rate);
    if(true!=ret){
        LOG_W("init_rec_process AUDPROC_initRecordEq failed");
    }

    return ret;
}


int aud_dsp_assert_set(void * dev, bool asserted)
{
    struct tiny_audio_device *adev  = dev;
    if(asserted) {
        adev->is_agdsp_asserted = true;
    }
    else {
        adev->is_agdsp_asserted = false;
        if(adev->call_mode  ==AUDIO_MODE_IN_CALL) {
            send_cmd_to_dsp_thread(adev->dev_ctl->agdsp_ctl,AUDIO_CTL_START_VOICE,NULL);
        }
    }
    return 0;
}

int aud_rec_do_process(void *buffer, size_t bytes, void *tmp_buffer,
                              size_t tmp_buffer_bytes)
{
    int16_t *temp_buf = NULL;
    size_t read_bytes = bytes;
    unsigned int dest_count = 0;
    temp_buf = (int16_t *) tmp_buffer;
    if (temp_buf && (tmp_buffer_bytes >= 2)) {
        do {
            if (tmp_buffer_bytes <= bytes) {
                read_bytes = tmp_buffer_bytes;
            } else {
                read_bytes = bytes;
            }
            bytes -= read_bytes;
            //LOG_V("aud_rec_do_process");
            AUDPROC_ProcessDp((int16_t *) buffer,
                              (int16_t *) buffer, read_bytes >> 1,
                              temp_buf, temp_buf, &dest_count);
            memcpy(buffer, temp_buf, read_bytes);
            buffer = (uint8_t *) buffer + read_bytes;
        } while (bytes);
    } else {
        LOG_E("temp_buf malloc failed.(len=%d)", (int)read_bytes);
        return -1;
    }
    return 0;
}

static int adev_open(const hw_module_t *module, const char *name,
                     hw_device_t **device)
{
    struct tiny_audio_device *adev;
    int ret;
    LOG_I("adev_open");
    if (strcmp(name, AUDIO_HARDWARE_INTERFACE) != 0) {
        return -EINVAL;
    }
    adev = calloc(1, sizeof(struct tiny_audio_device));
    if (!adev) {
        LOG_E("malloc tiny_audio_device failed, size:%d",
              sizeof(struct tiny_audio_device));
        return -ENOMEM;
    }
    memset(adev, 0, sizeof(struct tiny_audio_device));

    adev->hw_device.common.tag = HARDWARE_DEVICE_TAG;
    adev->hw_device.common.version = AUDIO_DEVICE_API_VERSION_2_0;
    adev->hw_device.common.module = (struct hw_module_t *)module;
    adev->hw_device.common.close = adev_close;

    adev->hw_device.get_supported_devices = adev_get_supported_devices;
    adev->hw_device.init_check = adev_init_check;
    adev->hw_device.set_voice_volume = adev_set_voice_volume;
    adev->hw_device.set_master_volume = adev_set_master_volume;
    adev->hw_device.set_mode = adev_set_mode;
    adev->hw_device.set_master_mute = adev_set_master_mute;
    adev->hw_device.get_master_mute = adev_get_master_mute;
    adev->hw_device.set_mic_mute = adev_set_mic_mute;
    adev->hw_device.get_mic_mute = adev_get_mic_mute;
    adev->hw_device.set_parameters = adev_set_parameters;
    adev->hw_device.get_parameters = adev_get_parameters;
    adev->hw_device.get_input_buffer_size = adev_get_input_buffer_size;
    adev->hw_device.open_output_stream = adev_open_output_stream;
    adev->hw_device.close_output_stream = adev_close_output_stream;
    adev->hw_device.open_input_stream = adev_open_input_stream;
    adev->hw_device.close_input_stream = adev_close_input_stream;
    adev->hw_device.dump = adev_dump;

    adev->call_start = false;
    adev->call_mode = AUDIO_MODE_NORMAL;
    adev->call_status = VOICE_INVALID_STATUS;
    adev->stream_status = 0;
    adev->mic_mute = false;
    adev->low_power = false;
    adev->master_mute=false;

    pthread_mutex_init(&adev->lock, NULL);
    pthread_mutex_init(&adev->audio_param, NULL);

    adev->dev_ctl = init_audio_control(adev);
    if (adev->dev_ctl == NULL) {
        LOG_E("adev_open Init audio control failed ");
        goto error_malloc;
    }

    pthread_mutex_lock(&adev->lock);
    init_audio_param(adev);

    /* upload audio firmware to kernel */
    upload_audio_param_firmware(&adev->audio_param);

    pthread_mutex_unlock(&adev->lock);

    adev->audio_param.tunning.running=false;
    adev->audio_param.tunning.wire_connected=false;
    adev->audio_param.tunning.audio_config_xml=NULL;
#ifdef AUDIO_DEBUG
    start_audio_tunning_server(adev);
#endif

#ifdef LOCAL_SOCKET_SERVER
    start_audio_local_server(adev);
#endif

    *device = &adev->hw_device.common;

    ext_control_init(adev);
    modem_monitor_open(adev);
    list_init(&adev->active_out_list);
    list_init(&adev->active_input_list);
    audio_dsp_loop_open(adev);
    voice_open(adev);
    fm_open(adev);

    LOG_I("adev_open end");
    return 0;
error_malloc:
    if (adev)
    { free(adev); }
    LOG_E("adev_open error");
    return -EINVAL;
}


int sprd_audio_hal_init(const hw_module_t *module, const char *name,
                        hw_device_t **device)
{
    return adev_open(module, name, device);
}

static struct hw_module_methods_t hal_module_methods = {
    .open = adev_open,
};

struct audio_module HAL_MODULE_INFO_SYM = {
    .common = {
        .tag = HARDWARE_MODULE_TAG,
        .module_api_version = AUDIO_MODULE_API_VERSION_0_1,
        .hal_api_version = HARDWARE_HAL_API_VERSION,
        .id = AUDIO_HARDWARE_MODULE_ID,
        .name = "Spreadtrum Audio HW HAL",
        .author = "The Android Open Source Project",
        .methods = &hal_module_methods,
    },
};

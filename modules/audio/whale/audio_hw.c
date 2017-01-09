/*
    Copyright (C) 2012 The Android Open Source Project

    Licensed under the Apache License, Version 2.0 (the "License");
    you may not use this file except in compliance with the License.
    You may obtain a copy of the License at

        http://www.apache.org/licenses/LICENSE-2.0

    Unless required by applicable law or agreed to in writing, software
    distributed under the License is distributed on an "AS IS" BASIS,
    WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
    See the License for the specific language governing permissions and
    limitations under the License.
*/
#define LOG_TAG "audio_hw_primary"
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
#ifdef LOCAL_SOCKET_SERVER
#include "audiotester/local_socket.h"
#endif

extern int ext_control_init(struct tiny_audio_device *adev);
extern int get_snd_card_number(const char *card_name);
//int enable_device_gain(struct audio_control *ctl, int device, int usecase, int in_device);
int aud_rec_do_process(void *buffer, size_t bytes, void *tmp_buffer,
                              size_t tmp_buffer_bytes);
void force_all_standby(struct tiny_audio_device *adev);
static int do_output_standby(struct tiny_stream_out *out);
static int do_input_standby(struct tiny_stream_in *in);
void start_audio_tunning_server(struct tiny_audio_device *adev);

//#include "at_commands_generic.c"

extern int modem_monitor_open(void *arg);
extern bool audio_is_offload_support_format(audio_format_t format);
extern int audio_get_offload_codec_id(audio_format_t format);
extern int audio_start_compress_output(struct tiny_stream_out *out);
extern int audio_get_compress_metadata(struct tiny_stream_out *out,
                                       struct str_parms *parms);
extern int audio_send_offload_cmd(struct tiny_stream_out *out,
                                  AUDIO_OFFLOAD_CMD_T command);
extern void *audio_offload_thread_loop(void *param);
extern int audio_offload_create_thread(struct tiny_stream_out *out);
extern int audio_offload_destroy_thread(struct tiny_stream_out *out);
extern AUDIO_OFFLOAD_MIXER_CARD_T audio_get_offload_mixer_card(
    struct tiny_stream_out *out);
extern void dump_pcm(const void *pBuffer, size_t aInBufSize);
extern ssize_t out_write_compress(struct tiny_stream_out *out,
                                  const void *buffer,
                                  size_t bytes);
extern int out_offload_set_callback(struct audio_stream_out *stream,
                                    stream_callback_t callback, void *cookie);
extern int out_offload_pause(struct audio_stream_out *stream);
extern int out_offload_resume(struct audio_stream_out *stream);
extern int out_offload_drain(struct audio_stream_out *stream,
                             audio_drain_type_t type );
extern int out_offload_flush(struct audio_stream_out *stream);
extern int ext_contrtol_process(struct tiny_audio_device *adev,const char *cmd_string);

static int out_deinit_resampler(struct tiny_stream_out *out);
static int out_init_resampler(struct tiny_stream_out *out);
int send_cmd_to_dsp_thread(struct dsp_control_t *agdsp_ctl,int cmd,void * parameter);

static int _pcm_mixer(int16_t *buffer, uint32_t samples);
static ssize_t read_pcm_data(void *stream, void* buffer, size_t bytes);

int32_t GetAudio_InMode_number_from_device(int in_dev)
{
    int ret = 0;
    if (((in_dev & ~AUDIO_DEVICE_BIT_IN) & AUDIO_DEVICE_IN_BUILTIN_MIC)
        || ((in_dev & ~AUDIO_DEVICE_BIT_IN) & AUDIO_DEVICE_IN_BACK_MIC)) {
        ret = AUDIO_RECORD_MODE_HANDSFREE;
    } else if (((in_dev & ~AUDIO_DEVICE_BIT_IN) &
                AUDIO_DEVICE_IN_WIRED_HEADSET)
               || ((in_dev & ~AUDIO_DEVICE_BIT_IN) &
                   AUDIO_DEVICE_IN_BLUETOOTH_SCO_HEADSET)) {
        ret = AUDIO_RECORD_MODE_HEADSET;
    }
    return ret;
}


static ssize_t _pcm_read(void *stream, void* buffer,
        size_t bytes)
{
    int ret =0;
    struct audio_stream_in *stream_tmp=(struct audio_stream_in *)stream;
    struct tiny_stream_in *in = (struct tiny_stream_in *)stream;
    struct tiny_audio_device *adev = in->dev;
    size_t frames_rq = bytes / audio_stream_frame_size((const struct audio_stream *)(&stream_tmp->common));

    if(in->config->channels==1){
        frames_rq/=2;
    }

    ret = pcm_read(in->pcm, buffer, frames_rq*audio_stream_frame_size((const struct audio_stream *)(&stream_tmp->common)));
    if(0==ret){
        if(in->config->channels==1){
            _pcm_mixer(buffer, frames_rq);
            frames_rq*=2;
        }
    }

    if(adev->debug.record.vbc.hex_fd>0){
        write(adev->debug.record.vbc.hex_fd,buffer,
                               bytes);
    }

    return ret;
}


static record_nr_handle init_nr_process(struct audio_stream_in *stream)
{
    int audio_record_mode;

    struct tiny_stream_in *in = (struct tiny_stream_in *)stream;
    struct tiny_audio_device *adev = in->dev;
    struct audio_record_proc_param *proc_param = NULL;
    char *param_data = NULL;

    NR_CONTROL_PARAM_T *nr_control=NULL;
    audio_record_mode = GetAudio_InMode_number_from_device(adev->in_devices);

    LOG_I("init_nr_process");
    param_data = (char *)
                 adev->audio_param.param[SND_AUDIO_PARAM_RECORD_PROCESS_PROFILE].data;
    if(NULL == param_data) {
        LOG_I("audio record process param is null");
        return false;
    }

    proc_param =  (DP_CONTROL_PARAM_T *)(param_data + sizeof(
            struct audio_record_proc_param) * audio_record_mode);

    nr_control = &proc_param->nr_control;

    return AudioRecordNr_Init(nr_control, _pcm_read, (void *)in,
                                           in->requested_channels);
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

static int _pcm_mixer(int16_t *buffer, uint32_t samples)
{
    int i = samples;
    int j=0;
    int size=i*2;
    while(i--){
        buffer[--size]=buffer[i];
        buffer[--size]=buffer[i];
    }
    return 0;
}

void force_all_standby(struct tiny_audio_device *adev)
{
    struct tiny_stream_out *out;
    struct listnode *item;
    LOG_I("force_all_standby stream_status:0x%x",adev->stream_status)

    pthread_mutex_lock(&adev->lock);
    while (!list_empty(&adev->active_out_list)) {
        item = list_head(&adev->active_out_list);
        out = node_to_item(item, struct tiny_stream_out, node);
        LOG_I("force_all_standby type:%d out:%p fun:%p",out->audio_app_type,out,out->standby_fun);
        if(NULL!=out->standby_fun){
            out->standby_fun(adev,out,out->audio_app_type);
        }else{
            adev->stream_status &=~(1<<out->audio_app_type);
        }
        list_remove(item);
    }

    if((list_empty(&adev->active_out_list)) && (true == adev->low_power)){
        close_out_control(adev->dev_ctl);
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
        pthread_mutex_unlock(&adev->lock);
    }
    return ret;
}


int audio_add_input(struct tiny_audio_device *adev,struct tiny_stream_in *in,AUDIO_HW_APP_T audio_app_type,AUDIO_OUTPUT_STANDBY_FUN standby)
{
    LOG_I("audio_add_input type:%d out:%p fun:%p",in->audio_app_type,in,standby);
    in->audio_app_type=audio_app_type;
    in->standby_fun=standby;
    adev->stream_status |=(1<<in->audio_app_type);
    list_add_tail(&adev->active_input_list, &in->node);
    return 0;
}

int audio_add_output(struct tiny_audio_device *adev,struct tiny_stream_out *out,AUDIO_HW_APP_T audio_app_type,AUDIO_OUTPUT_STANDBY_FUN standby)
{
    LOG_I("audio_add_output type:%d out:%p fun:%p",out->audio_app_type,out,standby);
    out->audio_app_type=audio_app_type;
    out->standby_fun=standby;
    adev->stream_status |=(1<<audio_app_type);
    list_add_tail(&adev->active_out_list, &out->node);
    if((AUDIO_HW_APP_CALL==audio_app_type) ||
        (AUDIO_HW_APP_OFFLOAD==audio_app_type)) {
       select_audio_param(adev,false);
    }
    return 0;
}

static void do_normal_output_standby(struct tiny_audio_device *adev,void * out,AUDIO_HW_APP_T type)
{
    struct tiny_stream_out *normal_out=(struct tiny_stream_out *)out;
    LOG_I("do_normal_output_standby enter:%p standby:%d",normal_out->pcm,normal_out->standby);
    pthread_mutex_lock(&normal_out->lock);
    if (NULL!=normal_out->pcm) {
        LOG_I("do_normal_output_standby pcm_close:%p",normal_out->pcm);
        pcm_close(normal_out->pcm);
        normal_out->pcm = NULL;
    }
    normal_out->standby=true;
    if(AUDIO_HW_APP_VOIP==normal_out->audio_app_type){
        LOG_D("do_normal_output_standby AUDIO_HW_APP_VOIP");
        adev->stream_status &=~(1<<AUDIO_HW_APP_VOIP);
    }
    adev->stream_status &=~(1<<normal_out->audio_app_type);
    if(false==is_usecase(adev->dev_ctl,UC_CALL)){
        switch_vbc_iis_route(adev->dev_ctl,UC_MUSIC,false);
    }

    if((AUDIO_HW_APP_FAST!=normal_out->audio_app_type) && (AUDIO_HW_APP_DIRECT!=normal_out->audio_app_type)){
        LOG_W("do_normal_output_standby app type:%d",normal_out->audio_app_type);
        normal_out->audio_app_type=AUDIO_HW_APP_PRIMARY;
    }


    pthread_mutex_unlock(&normal_out->lock);
    LOG_I("do_normal_output_standby :%p %d exit",out,normal_out->audio_app_type);
}

static ssize_t normal_out_write(struct tiny_stream_out *out, const void *buffer,
                                size_t bytes)
{
    void *buf;
    size_t frame_size = 0;
    size_t in_frames = 0;
    size_t out_frames = 0;
    int ret=0;
    struct tiny_audio_device *adev = out->dev;

    struct pcm_config *config=NULL;
    frame_size = audio_stream_frame_size(&out->stream.common);
    in_frames = bytes / frame_size;
    out_frames = RESAMPLER_BUFFER_SIZE / frame_size;

    if(out->pcm){
        if(out->config->rate !=out->requested_rate){
            out->resampler->resample_from_input(out->resampler,
                    (int16_t *)buffer,&in_frames,
                    (int16_t *) out->buffer,&out_frames);
            buf = out->buffer;
        }else{
            out_frames = in_frames;
            buf = (void *)buffer;
        }

        if((out->config->channels==1) &&(frame_size == 4)){
            frame_size=2;
            LOG_D("normal_out_write chanage channle");
            pcm_mixer(buf, out_frames*frame_size);
        }

        ret = pcm_mmap_write(out->pcm, (void *)buf,
                         out_frames * frame_size);
        if(0==ret){
            LOG_D("normal_out_write out frames  is %d bytes:%d", out_frames,bytes);
            if(adev->debug.playback.normal.hex_fd>0){
                write(adev->debug.playback.normal.hex_fd,buf,out_frames * frame_size);
            }
        }else{
            LOG_E("normal_out_write ret:0x%x pcm:0x%p", ret,out->pcm);
            if (ret < 0) {
                if (out->pcm) {
                    LOG_W("normal_out_write warning:%d, (%s)", ret,
                          pcm_get_error(out->pcm));
                }
                pthread_mutex_unlock(&out->lock);
                force_out_standby(adev,out->audio_app_type);
                pthread_mutex_lock(&out->lock);
            }
        }
    }else{
        LOG_E("out_new_write_audio pcm is null");
        out->standby = true;
        usleep(out_frames * 1000 * 1000 / out->config->rate);
    }
    return bytes;
}

static int start_output_stream(struct tiny_stream_out *out)
{
    struct tiny_audio_device *adev = out->dev;
    int pcm_devices=-1;
    int ret = 0;
    struct audio_control *control = NULL;
    AUDIO_HW_APP_T audio_app_type=AUDIO_HW_APP_PRIMARY;
    control = adev->control;

    out->low_power = 1;
    adev->out_devices = out->devices;
    LOG_I("start_output_stream device=%d flags:0x%x stream_status:0x%x app:%d", adev->out_devices,out->flags,adev->stream_status,out->audio_app_type);

    if(false==is_usecase(adev->dev_ctl,UC_CALL)){
            LOG_I("start_output_stream voip_start:%d",adev->voip_start);
            out->devices=adev->out_devices;
            if(true==adev->voip_start){
                switch_vbc_iis_route(adev->dev_ctl,UC_VOIP,true);
            }else{
                switch_vbc_iis_route(adev->dev_ctl,UC_MUSIC,true);
        }
    }

    switch(out->audio_app_type){
        case AUDIO_HW_APP_VAUDIO:
            LOG_I("start_output_stream UC_CALL");
            pcm_devices=adev->dev_ctl->pcm_handle.playback_devices[AUD_PCM_VOIP];
            out->config = &control->pcm_handle.play[AUD_PCM_VOIP];
            break;
        case AUDIO_HW_APP_VOIP:
            LOG_I("start_output_stream AUD_PCM_VOIP");
            pcm_devices=adev->dev_ctl->pcm_handle.playback_devices[AUD_PCM_VOIP];
            out->config = &control->pcm_handle.play[AUD_PCM_VOIP];
            break;
        case AUDIO_HW_APP_PRIMARY:
            if(adev->stream_status & (1<<AUDIO_HW_APP_OFFLOAD)){
                LOG_W("start_output_stream offload do not support normal playback");
                goto error;
            }

            if(true==adev->voip_start){
                LOG_I("start_output_stream AUDIO_HW_APP_VOIP");
                out->audio_app_type = AUDIO_HW_APP_VOIP;
                audio_app_type = AUDIO_HW_APP_VOIP;
                pcm_devices=adev->dev_ctl->pcm_handle.playback_devices[AUD_PCM_VOIP];
                out->config = &control->pcm_handle.play[AUD_PCM_VOIP];
            }else{
                LOG_I("start_output_stream AUD_PCM_MM_NORMAL");
                pcm_devices=adev->dev_ctl->pcm_handle.playback_devices[AUD_PCM_MM_NORMAL];
                out->config = &control->pcm_handle.play[AUD_PCM_MM_NORMAL];
                audio_app_type = AUDIO_HW_APP_PRIMARY;
            }
            break;
        case AUDIO_HW_APP_DIRECT:
            LOG_I("start_output_stream AUDIO_HW_APP_DIRECT");
            pcm_devices=adev->dev_ctl->pcm_handle.playback_devices[AUD_PCM_MM_NORMAL];
            out->config = &control->pcm_handle.play[AUD_PCM_MM_NORMAL];
            audio_app_type=AUDIO_HW_APP_DIRECT;
            break;
        case AUDIO_HW_APP_FAST:
            if(adev->stream_status & (1<<AUDIO_HW_APP_OFFLOAD)){
                LOG_W("start_output_stream offload do not support offload playback");
                goto error;
            }
            LOG_I("start_output_stream AUDIO_HW_APP_FAST");
            pcm_devices=adev->dev_ctl->pcm_handle.playback_devices[AUD_PCM_MM_FAST];
            out->config = &control->pcm_handle.play[AUD_PCM_MM_FAST];
            audio_app_type=AUDIO_HW_APP_FAST;
            break;
        default:
            LOG_E("start_output_stream stream type:0x%x",out->audio_app_type);
            goto error;
            break;
    }

    adev->stream_status |=(1<<audio_app_type);
    out->devices= adev->out_devices;
    select_devices_new(adev->dev_ctl,out->devices,0);

    if(NULL==out->buffer){
        out->buffer = malloc(RESAMPLER_BUFFER_SIZE);
        if (!out->buffer) {
            LOG_E("start_output_stream: alloc fail, size: %d",
                  RESAMPLER_BUFFER_SIZE);
            goto error;
        }
        memset(out->buffer, 0, RESAMPLER_BUFFER_SIZE);
    }

    if(NULL!=out->pcm){
        LOG_E("start_output_stream pcm err:%x",out->pcm);
        pcm_close(out->pcm);
        out->pcm=NULL;
    }
    out->pcm =
        pcm_open(0, pcm_devices, PCM_OUT | PCM_MMAP | PCM_NOIRQ, out->config);

    if (!pcm_is_ready(out->pcm)) {
        LOG_E("%s:cannot open pcm : %s", __func__,
              pcm_get_error(out->pcm));
        goto error;
    } else {
        if((out->config->rate !=out->requested_rate)&&(NULL==out->resampler)){
            LOG_I("start_output_stream create_resampler");
            if(0!=out_init_resampler(out)){
                goto error;
            }
            out->resampler->reset(out->resampler);
        }

        if(NULL!=out->buffer){
            memset(out->buffer, 0, RESAMPLER_BUFFER_SIZE);
        }

        LOG_I("start_output_stream pcm_open devices:%d pcm:%p %d %d",pcm_devices,out->pcm
            ,out->config->rate,out->requested_rate);
    }
    out->standby=false;
    audio_add_output(adev,out,audio_app_type,do_normal_output_standby);
    set_usecase(adev->dev_ctl,UC_MUSIC,1);
    select_audio_param(adev,true);

    return 0;

error:

    adev->stream_status &=~(1<<audio_app_type);

    if(out->pcm) {
        pcm_close(out->pcm);
        out->pcm = NULL;
    }
//  close_out_control(adev->dev_ctl);
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
    struct audio_control *control = adev->control;
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

    if(out->audio_app_type == AUDIO_HW_APP_OFFLOAD) {
        return out->offload_format;
    }

    return AUDIO_FORMAT_PCM_16_BIT;
}

static int out_set_format(struct audio_stream *stream, audio_format_t format)
{
    return 0;
}

/* must be called with hw device and output stream mutexes locked */
static int do_output_standby(struct tiny_stream_out *out)
{
    struct tiny_audio_device *adev = out->dev;
    LOG_I("do_output_standby enter stream_status:0x%x out:%p",adev->stream_status,out);
    struct listnode *item;

    pthread_mutex_lock(&adev->lock);
    if(!list_empty(&adev->active_out_list)){
        list_for_each(item, &adev->active_out_list){
            if(node_to_item(item, struct tiny_stream_out, node)==out){
                if(NULL!=out->standby_fun){
                    out->standby_fun(adev,out,out->audio_app_type);
                }else{
                    adev->stream_status &=~(1<<out->audio_app_type);
                }
                if(adev->stream_status ==0){
                    dsp_sleep_ctrl(&adev->agdsp_ctl,false);
                }
                list_remove(item);
                break;
            }
        }
    }
    if((list_empty(&adev->active_out_list)) && (true == adev->low_power)){
        close_out_control(adev->dev_ctl);
    }
    pthread_mutex_unlock(&adev->lock);
    LOG_I("do_output_standby exit:0x%x",adev->stream_status)
    return 0;
}

void force_out_standby(struct tiny_audio_device *adev,AUDIO_HW_APP_T audio_app_type)
{
    struct tiny_stream_out *out;
    struct listnode *item;
    LOG_I("force_out_standby stream_status:0x%x",adev->stream_status)

    pthread_mutex_lock(&adev->lock);
    if(!list_empty(&adev->active_out_list)){
        list_for_each(item, &adev->active_out_list){
            out = node_to_item(item, struct tiny_stream_out, node);
            if(out->audio_app_type==audio_app_type){
                if((AUDIO_HW_APP_CALL==audio_app_type) ||
                    (AUDIO_HW_APP_OFFLOAD==audio_app_type)) {
                    clear_audio_param(&adev->audio_param);
                }

                if(NULL!=out->standby_fun){
                    out->standby_fun(adev,out,out->audio_app_type);
                }else{
                    adev->stream_status &=~(1<<out->audio_app_type);
                }

                if(adev->stream_status ==0){
                    dsp_sleep_ctrl(&adev->agdsp_ctl,false);
                }

                list_remove(item);
                break;
             }
        }
    }
    pthread_mutex_unlock(&adev->lock);
    LOG_I("force_out_standby:exit");
}

static int out_standby(struct audio_stream *stream)
{
    struct tiny_stream_out *out = (struct tiny_stream_out *)stream;
    int status=-1;
    status = do_output_standby(out);
    return status;
}

static int out_dump(const struct audio_stream *stream, int fd)
{
    return 0;
}

static int out_set_parameters(struct audio_stream *stream, const char *kvpairs)
{
    struct tiny_stream_out *out = (struct tiny_stream_out *)stream;
    struct tiny_audio_device *adev = out->dev;
    struct str_parms *parms;
    char *str;
    char value[32];
    int ret, val = 0;
    static int cur_mode = 0;

    LOG_I("[out_set_parameters], kvpairs=%s devices:0x%x  mode:%d ",
          kvpairs, adev->out_devices, adev->call_mode);

    parms = str_parms_create_str(kvpairs);

    ret =
        str_parms_get_str(parms, AUDIO_PARAMETER_STREAM_ROUTING, value,
                          sizeof(value));
    if (ret >= 0) {
        val = atoi(value);
        pthread_mutex_lock(&adev->lock);
        pthread_mutex_lock(&out->lock);
        if (val != 0 && out->devices != (unsigned int)val) {
            out->devices = val;
        }
        adev->out_devices = out->devices;
        pthread_mutex_unlock(&out->lock);
        pthread_mutex_unlock(&adev->lock);
        select_devices_new(adev->dev_ctl,adev->out_devices,0);

        if (is_usecase(adev->dev_ctl,UC_LOOP |UC_CALL|UC_VOIP)) {
            if((adev->out_devices ==AUDIO_DEVICE_OUT_BLUETOOTH_SCO_HEADSET)
                || (adev->out_devices ==AUDIO_DEVICE_OUT_BLUETOOTH_SCO)){
                adev->in_devices=0;
                close_in_control(adev->dev_ctl);
            }else{
                if(adev->out_devices !=AUDIO_DEVICE_OUT_WIRED_HEADSET){
                    if(adev->dev_ctl->codec.switch_route==1){
                        adev->in_devices=AUDIO_DEVICE_IN_BUILTIN_MIC;
                    }else if(adev->dev_ctl->codec.switch_route==2){
                        adev->in_devices=AUDIO_DEVICE_IN_BACK_MIC;
                    }else if(adev->dev_ctl->codec.switch_route==3){
                        adev->in_devices=AUDIO_DEVICE_IN_BUILTIN_MIC|AUDIO_DEVICE_IN_BACK_MIC;
                    }
                }else{
                    adev->in_devices=AUDIO_DEVICE_IN_WIRED_HEADSET;
                }
            }
            select_devices_new(adev->dev_ctl,adev->in_devices,1);
        }
    }

    if (out->audio_app_type == AUDIO_HW_APP_OFFLOAD) {
        audio_get_compress_metadata(out, parms);
    }

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

    return (out->config->period_size * out->config->period_count * 1000) /
           out->config->rate;
}

 static int out_set_volume(struct audio_stream_out *stream, float left,
                           float right)
{
    struct tiny_stream_out *out = (struct tiny_stream_out *)stream;
    struct tiny_audio_device *adev = out->dev;

    struct mixer_ctl *offload_dg = 0;
    int mdg_arr[2] = {0};
    int max = 0;

    if (out->audio_app_type == AUDIO_HW_APP_OFFLOAD) {
     offload_dg = mixer_get_ctl_by_name(adev->dev_ctl->mixer, "OFFLOAD DG Set");
     if(offload_dg) {
         max = mixer_ctl_get_range_max(offload_dg);
         mdg_arr[0] = max * left;
         mdg_arr[1] = max * right;
         LOG_I("out_set_volume left=%f,right=%f, max=%d", left, right, max);
         return mixer_ctl_set_array(offload_dg, (void *)mdg_arr, 2);
     } else {
         LOG_E("out_set_volume cannot get offload_dg ctrl");
     }
    }
    return -ENOSYS;
}

static bool out_bypass_data(struct tiny_stream_out *out, uint32_t frame_size,
                            uint32_t sample_rate, size_t bytes)
{
    struct tiny_audio_device *adev = out->dev;
    if(out->audio_app_type == AUDIO_HW_APP_OFFLOAD)
        return false;
    if((is_usecase(adev->dev_ctl,UC_LOOP |UC_FM |UC_VOIP))
        ||((adev->stream_status & (1<<AUDIO_HW_APP_OFFLOAD)) &&( (out->audio_app_type == AUDIO_HW_APP_PRIMARY) || (out->audio_app_type ==AUDIO_HW_APP_FAST))))
    {
        LOG_I
        ("out_bypass_data %p %p 0x%x stream_status:0x%x",out,out->pcm,adev->dev_ctl->usecase,adev->stream_status);
        usleep((int64_t) bytes * 1000000 / frame_size / sample_rate);
        return true;
    }
    return false;
}

static int out_get_presentation_position(const struct audio_stream_out *stream,
        uint64_t *frames, struct timespec *timestamp)
{
    struct tiny_stream_out *out = (struct tiny_stream_out *)stream;
    struct pcm *pcm = NULL;
    int ret = -1;
    pthread_mutex_lock(&out->lock);
    pcm = out->pcm;
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
    pthread_mutex_unlock(&out->lock);
    //LOG_E("out_get_presentation_position ret %d",ret);
    return ret;
}

static ssize_t out_write(struct audio_stream_out *stream, const void *buffer,
                         size_t bytes)
{
    int ret = 0;
    bool low_power;
    size_t frame_size = 0;
    size_t in_frames = 0;
    size_t out_frames = 0;
    void *buf;
    struct tiny_stream_out *out = (struct tiny_stream_out *)stream;
    struct tiny_audio_device *adev = out->dev;
    AUDIO_OFFLOAD_MIXER_CARD_T card = AUDIO_OFFLOAD_MIXER_INVALID;

    pthread_mutex_lock(&adev->lock);
    LOG_V("out_write out:%p stream_status:0x%x call_mode:%d usecase:0x%x audio_app_type:%x",
          out, adev->stream_status, adev->call_mode, adev->dev_ctl->usecase,
          out->audio_app_type)

    if(out->audio_app_type == AUDIO_HW_APP_OFFLOAD) {
        adev->offload_on = 1;
    }
    if (out_bypass_data(out, audio_stream_frame_size(&stream->common),
        out_get_sample_rate(&stream->common), bytes)) {
        pthread_mutex_unlock(&adev->lock);
        if(NULL!=out->pcm){
            do_output_standby(out);
        }
        return bytes;
    }
    pthread_mutex_unlock(&adev->lock);
    pthread_mutex_lock(&out->lock);

    low_power = adev->low_power;
    switch(out->audio_app_type){
        case AUDIO_HW_APP_OFFLOAD:
            if (true==out->standby){
                force_out_standby(adev,AUDIO_HW_APP_PRIMARY);
                dsp_sleep_ctrl(&adev->agdsp_ctl,true);
                ret = audio_start_compress_output(out);
                if (ret != 0) {
                    goto exit;
                }
                out->standby = false;
            }
            ret = out_write_compress(out, buffer, bytes);
            break;
        case  AUDIO_HW_APP_FAST:
        case AUDIO_HW_APP_DIRECT:
        case AUDIO_HW_APP_VOIP:
            if (true==out->standby){
                force_out_standby(adev,AUDIO_HW_APP_PRIMARY);
                dsp_sleep_ctrl(&adev->agdsp_ctl,true);
                ret = start_output_stream(out);
                if (ret != 0) {
                    goto exit;
                }
                out->standby = false;
            }
            ret = normal_out_write(out, buffer, bytes);
            break;
        case AUDIO_HW_APP_PRIMARY:
            if (true==out->standby){
                pthread_mutex_lock(&adev->lock);
                dsp_sleep_ctrl(&adev->agdsp_ctl,true);
                ret = start_output_stream(out);
                pthread_mutex_unlock(&adev->lock);
                if (ret != 0) {
                    goto exit;
                }
                out->standby = false;
                ret=bytes;
                goto exit;
            }
            ret = normal_out_write(out, buffer, bytes);
            break;
        default:
            ret=-1;
            LOG_E("out_write audio_app_type is err:0x%x",out->audio_app_type);
            break;
    }
exit:
    pthread_mutex_unlock(&out->lock);
    return ret;
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
    return 0;
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
    LOG_I("in_deinit_resampler");

    if (in->resampler) {
        release_resampler(in->resampler);
        in->resampler = NULL;
    }
    if (in->buffer) {
        free(in->buffer);
        in->buffer = NULL;
    }

    if (in->nr_buffer) {
        free(in->nr_buffer);
        in->nr_buffer = NULL;
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
        audio_stream_frame_size(&in->stream.common);

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


    if(NULL==in->nr_buffer){
        in->nr_buffer = malloc(size);
    }

    if (!in->nr_buffer) {
        LOG_E("in_init_resampler: alloc fail, size: %d", size);
        ret = -ENOMEM;
        goto err;
    } else {
        memset(in->nr_buffer, 0, size);
    }

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
static int audio_bt_sco_duplicate_start(struct tiny_audio_device *adev,
                                        bool enable)
{
    int ret = 0;
    LOG_E("bt sco : %s duplicate thread %s", __func__,
          (enable ? "start" : "stop"));
    pthread_mutex_lock(&adev->bt_sco_manager.dup_mutex);
    if (enable != adev->bt_sco_manager.dup_count) {
        adev->bt_sco_manager.dup_count = enable;

        pthread_mutex_lock(&adev->bt_sco_manager.cond_mutex);
        adev->bt_sco_manager.dup_need_start = enable;
        pthread_cond_signal(&adev->bt_sco_manager.cond);
        pthread_mutex_unlock(&adev->bt_sco_manager.cond_mutex);

        LOG_E("bt sco : %s %s before wait", __func__,
              (enable ? "start" : "stop"));
        sem_wait(&adev->bt_sco_manager.dup_sem);
        LOG_E("bt sco : %s %s after wait", __func__,
              (enable ? "start" : "stop"));
    }
    if (enable && (adev->bt_sco_state & BT_SCO_DOWNLINK_OPEN_FAIL)) {
        adev->bt_sco_manager.dup_count = !enable;
        adev->bt_sco_manager.dup_need_start = !enable;
        ret = -1;
    } else {
        ret = 0;
    }
    pthread_mutex_unlock(&adev->bt_sco_manager.dup_mutex);
    return ret;
}

static void audio_bt_sco_thread_destory(struct tiny_audio_device *adev)
{
    int ret = 0;

    adev->bt_sco_manager.thread_is_exit = true;
    LOG_E("bt sco : duplicate thread destory before");
    ret = pthread_join(adev->bt_sco_manager.dup_thread, NULL);
    LOG_E("bt sco : duplicate thread destory ret is %d", ret);
    adev->bt_sco_manager.dup_thread = NULL;

    pthread_mutex_destroy(&adev->bt_sco_manager.dup_mutex);
    pthread_mutex_destroy(&adev->bt_sco_manager.cond_mutex);
    pthread_cond_destroy(&adev->bt_sco_manager.cond);
    sem_destroy(&adev->bt_sco_manager.dup_sem);
}

static void do_normal_inputput_standby(struct tiny_audio_device *adev,void * in,AUDIO_HW_APP_T type)
{
    struct tiny_stream_in *normal_in=(struct tiny_stream_in *)in;
    LOG_I("do_normal_inputput_standby enter");
    pthread_mutex_lock(&normal_in->lock);
    LOG_I("do_normal_inputput_standby:%p",normal_in->pcm);
    if (NULL!=normal_in->pcm) {
        LOG_I("do_normal_inputput_standby pcm_close:%p",normal_in->pcm);
        pcm_close(normal_in->pcm);
        normal_in->pcm = NULL;
    }

    if (NULL!=normal_in->proc_buf) {
        free(normal_in->proc_buf);
        normal_in->proc_buf = NULL;
    }
    normal_in->proc_buf_size=0;

    in_deinit_resampler(in);

    if (normal_in->active_rec_proc) {
        AUDPROC_DeInitDp();
        normal_in->active_rec_proc = false;

        if(normal_in->rec_nr_handle) {
            AudioRecordNr_Deinit(normal_in->rec_nr_handle);
            normal_in->rec_nr_handle = NULL;
        }
    }

    normal_in->standby=true;

    if(AUDIO_HW_APP_FM==normal_in->audio_app_type){
        LOG_D("do_normal_output_standby AUDIO_HW_APP_FM");
    }

    adev->stream_status &=~(1<<normal_in->audio_app_type);
    if(false==is_usecase(adev->dev_ctl,UC_CALL)){
        switch_vbc_iis_route(adev->dev_ctl,UC_MM_RECORD,false);
    }

    normal_in->audio_app_type=AUDIO_HW_APP_NORMAL_RECORD;

    pthread_mutex_unlock(&normal_in->lock);
    LOG_I("do_normal_inputput_standby exit");
    set_usecase(adev->dev_ctl,UC_MM_RECORD,0);
}

/* must be called with hw device and input stream mutexes locked */
static int start_input_stream(struct tiny_stream_in *in)
{
    struct tiny_audio_device *adev = in->dev;
    int pcm_devices=-1;
    int ret = 0;
    struct audio_control *control = NULL;
    AUDIO_HW_APP_T audio_app_type=AUDIO_HW_APP_NORMAL_RECORD;

    control = adev->control;

    adev->in_devices = in->devices;

    if(true==is_usecase(adev->dev_ctl,UC_LOOP)){
        LOG_E("start_input_stream usecase:0x%x",adev->dev_ctl->usecase);
        return -1;
    }

    in->config=NULL;

    if(false==is_usecase(adev->dev_ctl,UC_CALL)){
        LOG_I("start_input_stream voip_start:%d",adev->voip_start);
        if(true==adev->voip_start){
            switch_vbc_iis_route(adev->dev_ctl,UC_VOIP,true);
        }if(true==adev->fm_record){
            switch_vbc_iis_route(adev->dev_ctl,UC_MM_RECORD,true);
        }else{
            switch_vbc_iis_route(adev->dev_ctl,UC_MM_RECORD,true);
        }
    }

    if(true==is_usecase(adev->dev_ctl,UC_CALL)){
        LOG_I("start_input_stream UC_CALL");
        pcm_devices=adev->dev_ctl->pcm_handle.record_devices[AUD_PCM_CALL];
        in->config = &control->pcm_handle.record[AUD_PCM_CALL];
        audio_app_type=AUDIO_HW_APP_CALL;
    }else if (in->is_voip) {
        LOG_I("start_input_stream AUD_PCM_VOIP");
        pcm_devices=adev->dev_ctl->pcm_handle.record_devices[AUD_PCM_VOIP];
        in->config = &control->pcm_handle.record[AUD_PCM_VOIP];
        audio_app_type=AUDIO_HW_APP_VOIP;
    } else if (in->is_fm) {
        LOG_I("start_input_stream AUD_PCM_DIGITAL_FM");
        pcm_devices=adev->dev_ctl->pcm_handle.record_devices[AUD_PCM_DIGITAL_FM];
        in->config = &control->pcm_handle.record[AUD_PCM_DIGITAL_FM];
        audio_app_type=AUDIO_HW_APP_FM;
    } else if (in->is_bt_sco) {
        LOG_I("start_input_stream AUD_PCM_BT_VOIP");
        pcm_devices=adev->dev_ctl->pcm_handle.record_devices[AUD_PCM_BT_VOIP];
        in->config = &control->pcm_handle.record[AUD_PCM_BT_VOIP];
    }else {
        LOG_I("start_input_stream AUD_PCM_MM_NORMAL");
        pcm_devices=adev->dev_ctl->pcm_handle.record_devices[AUD_PCM_MM_NORMAL];
        in->config = &control->pcm_handle.record[AUD_PCM_MM_NORMAL];
    }

    in->audio_app_type=audio_app_type;
    adev->stream_status |=(1<<in->audio_app_type);
    select_audio_param(adev,true);
    in->devices= adev->in_devices;
    select_devices_new(adev->dev_ctl,in->devices,1);

    if(true==adev->config.record_process_enable){
        in->active_rec_proc = init_rec_process(in, in->requested_rate);
    }else{
        in->active_rec_proc=false;
    }

    LOG_I("start_input_stream rate:%d channel:%d",in->config->rate,in->config->channels);
    if((in->config->rate == 48000) && (true==adev->config.record_nr_enable)  && (true==adev->config.record_process_enable)){
        in->rec_nr_handle = init_nr_process(in);
    }else{
        in->rec_nr_handle =NULL;
    }

    in->pcm =
        pcm_open(0, pcm_devices, PCM_IN, in->config);

    if (!pcm_is_ready(in->pcm)) {
        LOG_E("start_input_stream:cannot open pcm : %s",
              pcm_get_error(in->pcm));
        goto error;
    } else {
        if(in->config->rate !=in->requested_rate){
            LOG_I("start_input_stream create_resampler");
            ret = in_init_resampler(in);
            if (ret) {
                LOG_E(": in_init_resampler failed");
                goto error;
            }
            in->resampler->reset(in->resampler);
        }else{
            if(in->resampler!=NULL){
                in_deinit_resampler(in);
            }
            in->resampler=NULL;
        }
        LOG_I("start_input_stream pcm_open devices:%d pcm:%p %d requested_rate:%d",pcm_devices,in->pcm
            ,in->config->rate,in->requested_rate);
    }
    in->standby=0;

    if (NULL!=in->resampler) {
        in->resampler->reset(in->resampler);
        in->frames_in = 0;
    }

    audio_add_input(adev,in,audio_app_type,do_normal_inputput_standby);
    set_usecase(adev->dev_ctl,UC_MM_RECORD,1);
    return 0;

error:

    adev->stream_status &=~(1<<in->audio_app_type);

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
    size = in->config->period_size;
    size = ((size + 15) / 16) * 16;

    return size * in->config->channels * sizeof(short);
}

static uint32_t in_get_channels(const struct audio_stream *stream)
{
    struct tiny_stream_in *in = (struct tiny_stream_in *)stream;

    if (in->config->channels == 1) {
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
static int do_input_standby(struct tiny_stream_in *in)
{
    struct tiny_audio_device *adev = in->dev;
    LOG_I("do_inputput_standby stream_status:0x%x",adev->stream_status)
    struct listnode *item=NULL;

    pthread_mutex_lock(&adev->lock);
    if(!list_empty(&adev->active_input_list)){
        list_for_each(item, &adev->active_input_list){
            if(node_to_item(item, struct tiny_stream_in, node)==in){
                LOG_I("do_input_standby %p type:%d",in,in->audio_app_type);
                if(NULL!=in->standby_fun){
                    in->standby_fun(adev,in,in->audio_app_type);
                }else{
                    adev->stream_status &=~(1<<in->audio_app_type);
                }
                list_remove(item);
                break;
            }
        }
        LOG_I("do_inputput_standby");
    }

    if(list_empty(&adev->active_input_list)){
        close_in_control(adev->dev_ctl);
    }
    pthread_mutex_unlock(&adev->lock);
    return 0;
}

void force_in_standby(struct tiny_audio_device *adev,AUDIO_HW_APP_T audio_app_type)
{
    struct tiny_stream_in *in;
    struct listnode *item;
    LOG_I("force_in_standby stream_status:0x%x",adev->stream_status)

    pthread_mutex_lock(&adev->lock);
    if(!list_empty(&adev->active_out_list)){
        list_for_each(item, &adev->active_out_list){
            in = node_to_item(item, struct tiny_stream_in, node);
            if(in->audio_app_type==audio_app_type){
                if(NULL!=in->standby_fun){
                    in->standby_fun(adev,in,in->audio_app_type);
                }else{
                    adev->stream_status &=~(1<<in->audio_app_type);
                }
                list_remove(item);
                break;
             }
        }
    }

    if(list_empty(&adev->active_input_list)){
        close_in_control(adev->dev_ctl);
        dsp_sleep_ctrl(&adev->agdsp_ctl,false);
    }

    pthread_mutex_unlock(&adev->lock);
    LOG_I("force_in_standby:exit");
}


static int in_standby(struct audio_stream *stream)
{
    int status=0;
    struct tiny_stream_in *in = (struct tiny_stream_in *)stream;
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
    struct str_parms *parms;
    char *str;
    char value[32];
    int ret, val = 0;

    LOG_I("[in_set_parameters], kvpairs=%s in_devices:0x%x  mode:%d ",
          kvpairs, adev->in_devices, adev->call_mode);
    parms = str_parms_create_str(kvpairs);

    pthread_mutex_lock(&adev->lock);
    pthread_mutex_lock(&in->lock);
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
        if(in->is_fm){
            LOG_I("FM is recording");
        }else{
            if (in->devices != (audio_devices_t) val) {
                in->devices = val;
                adev->in_devices = in->devices;
                select_devices_new(adev->dev_ctl,val,1);
            }
        }
    }

    pthread_mutex_unlock(&in->lock);
    pthread_mutex_unlock(&adev->lock);

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
                                           audio_stream_frame_size((const struct audio_stream *)(&in->stream.common));

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
                if(adev->debug.record.nr.hex_fd>0){
                    write(adev->debug.record.nr.hex_fd,(void *)in->nr_buffer,period_size);
                }
                in->read_status =ret;
            }else{
                in->read_status = _pcm_read(in, in->buffer, period_size);
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
                (int16_t *) ((char *) buffer +frames_wr *audio_stream_frame_size(
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
                       audio_stream_frame_size((const struct
                                                audio_stream
                                                *)(&in->stream.
                                                   common)),
                       buf.raw,
                       buf.frame_count *
                       audio_stream_frame_size((const struct
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

static bool in_bypass_data(struct tiny_stream_in *in, uint32_t frame_size,
                           uint32_t sample_rate, void *buffer, size_t bytes)
{
    struct tiny_audio_device *adev = in->dev;
#if 0
    if ((adev->dev_ctl->voice_handle.call_start
         && !adev->dev_ctl->voice_handle.call_connected)
        || (adev->dev_ctl->voice_handle.call_prestop)) {
        LOG_W
        ("in_bypass_data, call_start=%d, call_connected=%d call_prestop=%d",
         adev->dev_ctl->voice_handle.call_start,
         adev->dev_ctl->voice_handle.call_connected,
         adev->dev_ctl->voice_handle.call_prestop);
        memset(buffer, 0, bytes);
        usleep((int64_t) bytes * 1000000 / frame_size / sample_rate);
        return true;
    } else 
#endif
    {
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
    size_t frames_rq = bytes / audio_stream_frame_size((const struct audio_stream *)(&stream_tmp->common));

    if (in->resampler != NULL) {
        LOG_D("in_read %d start frames_rq:%d",bytes,frames_rq);
        if(in->config->channels==1){
            frames_rq/=2;
        }
        ret = read_frames(in, buffer, frames_rq);
        if (ret != frames_rq) {
            LOG_E("ERR:in_read0");
            ret = -1;
        } else {
            if(in->config->channels==1){
                _pcm_mixer(buffer, frames_rq);
                frames_rq*=2;
            }
            ret = 0;
        }
        LOG_D("in_read end:%d",ret);
    } else {
            if((in->config->rate == 48000) && (in->rec_nr_handle) && (in->active_rec_proc)){
                ret = AudioRecordNr_Proc(in->rec_nr_handle,buffer, bytes);
                if(ret <0){
                    LOG_E("AudioRecordNr_Proc  error");
                }
                if(adev->debug.record.nr.hex_fd>0){
                    write(adev->debug.record.nr.hex_fd,(void *)in->nr_buffer,bytes);
                }
            }
            else{
                ret = _pcm_read(in->pcm, buffer, bytes);
                if(in->active_rec_proc){
                    aud_rec_do_process(buffer, bytes, in->proc_buf, in->proc_buf_size);
                    if(adev->debug.record.process.hex_fd>0){
                        write(adev->debug.record.process.hex_fd,buffer,bytes);
                    }
                }
            }
    }
    return ret;
}

static ssize_t in_read(struct audio_stream_in *stream, void *buffer,
                       size_t bytes)
{
    int ret = 0;
    struct tiny_stream_in *in = (struct tiny_stream_in *)stream;
    struct tiny_audio_device *adev = in->dev;

    int frames_rq =bytes /audio_stream_frame_size((const struct audio_stream *)(&stream->common));

    LOG_D("in_read stream_status:%d call_mode%d usecase:%d",adev->stream_status,adev->call_mode,adev->dev_ctl->usecase);

    pthread_mutex_lock(&adev->lock);

    if (in_bypass_data(in,audio_stream_frame_size((const struct audio_stream *)(&stream->common)),
         in_get_sample_rate(&stream->common), buffer, bytes)) {
        pthread_mutex_unlock(&adev->lock);
        return bytes;
    }

    if(true == adev->voip_start){
        in->is_voip=true;
    }else{
        in->is_voip=false;
    }

    if(true == adev->fm_record){
        in->is_fm=true;
    }else{
        in->is_fm=false;
    }

    pthread_mutex_unlock(&adev->lock);
    pthread_mutex_lock(&in->lock);

    if (in->standby) {
        force_out_standby(adev,AUDIO_HW_APP_PRIMARY);
        force_out_standby(adev,AUDIO_HW_APP_FAST);
        dsp_sleep_ctrl(&adev->agdsp_ctl,true);
        ret = start_input_stream(in);
        if (ret < 0) {
            LOG_E("start_input_stream error ret=%d", ret);
            pthread_mutex_unlock(&adev->lock);
            goto exit;
        }
        in->standby = 0;
    }

    if(NULL == in->pcm){
        LOG_E("in_read pcm err");
        ret=-1;
    }else{
        ret = read_pcm_data(in, buffer, bytes);
    }

exit:

    if (ret < 0){
        if (in->pcm) {
            LOG_W("in_read, warning: ret=%d, (%s)", ret,
                  pcm_get_error(in->pcm));
        }

        LOG_I("in_read do_normal_inputput_standby:%p", in->pcm);
        if (NULL != in->pcm) {
            LOG_I("in_read  do_normal_inputput_standby pcm_close:%p", in->pcm);
            pcm_close(in->pcm);
            in->pcm = NULL;
        }

        if (NULL != in->proc_buf) {
            free(in->proc_buf);
            in->proc_buf = NULL;
        }

        in->proc_buf_size = 0;

        in_deinit_resampler(in);

        if (in->active_rec_proc) {
            AUDPROC_DeInitDp();
            in->active_rec_proc = false;
        }

        if(in->rec_nr_handle) {
            AudioRecordNr_Deinit(in->rec_nr_handle);
            in->rec_nr_handle = NULL;
        }

        in->standby = true;
        memset(buffer, 0, bytes);
        usleep((int64_t) bytes * 1000000 / audio_stream_frame_size((
                    const struct audio_stream *)(&stream->common)) / in->requested_rate);
    } else    {
        LOG_D("in_read:%d", bytes);
        if(adev->debug.record.normal.hex_fd > 0) {
            write(adev->debug.record.normal.hex_fd, buffer, bytes);
        }
    }

    pthread_mutex_unlock(&in->lock);
    ret = bytes;
    return ret;
}

static uint32_t in_get_input_frames_lost(struct audio_stream_in *stream)
{
    return 0;
}

static int adev_open_output_stream(struct audio_hw_device *dev,
                                   audio_io_handle_t handle,
                                   audio_devices_t devices,
                                   audio_output_flags_t flags,
                                   struct audio_config *config,
                                   struct audio_stream_out **stream_out)
{
    struct tiny_audio_device *ladev = (struct tiny_audio_device *)dev;
    struct tiny_stream_out *out;
    struct audio_control *control = NULL;
    int ret;
    control = ladev->control;

    LOG_I("%s, devices = %d flags:0x%x rate:%d", __func__, devices,flags,config->sample_rate);

    if(0 ==  (flags & AUDIO_OUTPUT_FLAG_COMPRESS_OFFLOAD)){
        if(config->sample_rate!=control->pcm_handle.record[AUD_PCM_MM_NORMAL].rate){
            LOG_E("adev_open_input_stream sample_rate:%d is not support",config->sample_rate);
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

    memset(out, 0, sizeof(struct tiny_stream_out));
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

        out->config = &(control->pcm_handle.play[AUD_PCM_OFF_LOAD]);

        LOG_I("compress_config fragment_size:0x%x fragments:0x%x",
            out->compress_config.fragment_size,out->compress_config.fragments);

        if (flags & AUDIO_OUTPUT_FLAG_NON_BLOCKING)
        { out->is_offload_nonblocking = 1; }
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

        if(flags & AUDIO_OUTPUT_FLAG_DIRECT){
            out->audio_app_type = AUDIO_HW_APP_DIRECT;
            out->config = &(control->pcm_handle.play[AUD_PCM_MM_NORMAL]);
        }else if(flags & AUDIO_OUTPUT_FLAG_FAST){
            out->audio_app_type = AUDIO_HW_APP_FAST;
            out->config = &(control->pcm_handle.play[AUD_PCM_MM_FAST]);
        }else{
            out->audio_app_type = AUDIO_HW_APP_PRIMARY;
            out->config = &(control->pcm_handle.play[AUD_PCM_MM_NORMAL]);
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

    LOG_I("adev_open_output_stream Successful audio_app_type:%d out:%p",out->audio_app_type,out);
    *stream_out = &out->stream;
    return 0;

err_open:
    LOG_E("Error adev_open_output_stream");
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

int set_dsp_volume(struct audio_control *ctl,int volume){
    int ret=0;
    LOG_I("set_dsp_volume:%d",volume);

    if(ctl->usecase & UC_CALL) {
        if(NULL==ctl->dsp_volume_ctl){
            ctl->dsp_volume_ctl=mixer_get_ctl_by_name(ctl->mixer, "VBC_VOLUME");;
        }

        if(NULL==ctl->dsp_volume_ctl){
            LOG_E("set_dsp_volume failed,can not get mixer:VBC_VOLUME");
            return -1;
        }

        ret=mixer_ctl_set_value(ctl->dsp_volume_ctl, 0, volume);

        if (ret != 0) {
            LOG_E("set_dsp_volume Failed volume:%d\n",volume);
        }else{
            ctl->voice_volume = volume;
        }
    }
    return ret;
}

int adev_set_voice_volume(struct audio_hw_device *dev, float volume)
{
    struct tiny_audio_device *adev = (struct tiny_audio_device *)dev;
    pthread_mutex_lock(&adev->lock);
    adev->voice_volume = volume;
    adev->dev_ctl->voice_volume = (int)(volume * 6 + 1);
    if(adev->dev_ctl->voice_volume >= 6){
        adev->dev_ctl->voice_volume = 6;
    }

    LOG_I("adev_set_voice_volume volume:%f level:%d", volume,adev->dev_ctl->voice_volume);

    /*Send at command to dsp side */
    set_dsp_volume(adev->dev_ctl,adev->dev_ctl->voice_volume);

    pthread_mutex_unlock(&adev->lock);
    return 0;
}

static int adev_set_master_volume(struct audio_hw_device *dev, float volume)
{
    return -ENOSYS;
}

static int adev_set_mode(struct audio_hw_device *dev, audio_mode_t mode)
{
    struct tiny_audio_device *adev = (struct tiny_audio_device *)dev;
    int ret=0;
    LOG_I("adev_set_mode:%d adev mode:%d",mode,adev->call_mode);
    if(true==is_usecase(adev->dev_ctl,UC_LOOP)){
        ret=-1;
        LOG_E("adev_set_mode looping now, can not start voice call");
        return ret;
    }

    ret=0;
    pthread_mutex_lock(&adev->lock);
    if (adev->call_mode != mode) {
        adev->call_mode = mode;
        pthread_mutex_unlock(&adev->lock);
        switch(mode){
            case AUDIO_MODE_INVALID:
                send_cmd_to_dsp_thread(&adev->agdsp_ctl,AUDIO_CTL_STOP_VOICE,NULL);
                break;
            case AUDIO_MODE_CURRENT:
                break;
            case AUDIO_MODE_NORMAL:
                send_cmd_to_dsp_thread(&adev->agdsp_ctl,AUDIO_CTL_STOP_VOICE,NULL);
                break;
            case AUDIO_MODE_RINGTONE:
                break;
            case AUDIO_MODE_IN_CALL:
                send_cmd_to_dsp_thread(&adev->agdsp_ctl,AUDIO_CTL_START_VOICE,NULL);
                break;
            case AUDIO_MODE_IN_COMMUNICATION:
                break; 
            default:
                send_cmd_to_dsp_thread(&adev->agdsp_ctl,AUDIO_CTL_STOP_VOICE,NULL);
                break; 
        }
    }else{
        pthread_mutex_unlock(&adev->lock);
    }

    return ret;
}

int ext_setVoiceMode(void * dev,struct str_parms *parms,int mode, char * val){
    adev_set_mode(dev,mode);
    return 0;
}

static int adev_set_master_mute(struct audio_hw_device *dev, bool mute)
{
    struct tiny_audio_device *adev = (struct tiny_audio_device *)dev;
    int ret=0;
    bool master_mute;

    pthread_mutex_lock(&adev->lock);
    master_mute = adev->master_mute;
    pthread_mutex_unlock(&adev->lock);

    if(mute!=adev->master_mute){
        if(adev->dev_ctl->usecase & UC_CALL) {

            LOG_I("adev_set_master_mute:%d",mute);

            if(NULL==adev->dev_ctl->voice_ul_mute_ctl){
                LOG_E("set_dsp_volume failed,can not get mixer:VBC_DL_MUTE");
                goto exit;
            }

            if(mute){
                ret=mixer_ctl_set_enum_by_string(adev->dev_ctl->voice_dl_mute_ctl, "enable");
            }else{
                ret=mixer_ctl_set_enum_by_string(adev->dev_ctl->voice_dl_mute_ctl, "disable");
            }

            if (ret != 0) {
                LOG_E("adev_set_master_mute Failed :%d\n",ret);
            }
        }

        pthread_mutex_lock(&adev->lock);
        adev->master_mute = mute;
        pthread_mutex_unlock(&adev->lock);

    }

exit:
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

static int adev_set_mic_mute(struct audio_hw_device *dev, bool state)
{
    struct tiny_audio_device *adev = (struct tiny_audio_device *)dev;
    int ret=0;
    bool mic_mute;

    pthread_mutex_lock(&adev->lock);
    mic_mute = adev->mic_mute;
    pthread_mutex_unlock(&adev->lock);

    if(state!=mic_mute){
        if(true==is_usecase(adev->dev_ctl,UC_CALL)) {
            LOG_I("adev_set_mic_mute:%d",state);

            if(NULL==adev->dev_ctl->voice_ul_mute_ctl){
                LOG_E("adev_set_mic_mute failed,can not get mixer:VBC_UL_MUTE");
                goto exit;
            }

            if(state){
                ret=mixer_ctl_set_enum_by_string(adev->dev_ctl->voice_ul_mute_ctl, "enable");
            }else{
                ret=mixer_ctl_set_enum_by_string(adev->dev_ctl->voice_ul_mute_ctl, "disable");
            }

            if (ret != 0) {
                LOG_E("adev_set_mic_mute Failed :%d\n",ret);
            }
        }else{
            LOG_I("adev_set_mic_mute usecase:%d",adev->dev_ctl->usecase);
        }

        pthread_mutex_lock(&adev->lock);
        adev->mic_mute = state;
        pthread_mutex_unlock(&adev->lock);

    }else{
        LOG_I("adev_set_mic_mute mic_mute:%d",adev->mic_mute);
    }
exit:
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

static int adev_open_input_stream(struct audio_hw_device *dev,
                                  audio_io_handle_t handle,
                                  audio_devices_t devices,
                                  struct audio_config *config,
                                  struct audio_stream_in **stream_in)
{
    struct tiny_audio_device *ladev = (struct tiny_audio_device *)dev;
    struct tiny_stream_in *in;
    struct audio_control *control = NULL;
    int ret = 0;
    int channel_count = popcount(config->channel_mask);
    control = ladev->control;

    LOG_I
    ("adev_open_input_stream,devices=0x%x,sample_rate=%d, channel_count=%d",
     devices, config->sample_rate, channel_count);

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

    in->requested_rate = config->sample_rate;
    in->requested_channels = channel_count;
    in->config=&(control->pcm_handle.record[AUD_PCM_MM_NORMAL]);


    if (in->requested_rate) {
        in->pop_mute_bytes =
            RECORD_POP_MIN_TIME * in->requested_rate / 1000 *
            audio_stream_frame_size((const struct audio_stream
                                     *)(&(in->stream).common));
    }
    in->dev = ladev;
    in->standby = 1;
    in->devices = devices;
    in->pop_mute = true;
    in->is_voip = false;
    in->is_fm = false;

    *stream_in = &in->stream;
     LOG_V
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

    if (in->resampler) {
        free(in->buffer);
        release_resampler(in->resampler);
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

    free(stream);
    return;
}

static int adev_dump(const audio_hw_device_t *device, int fd)
{
    return 0;
}

static int adev_close(hw_device_t *device)
{
    struct tiny_audio_device *adev = (struct tiny_audio_device *)device;
    LOG_E("adev_close");
    audio_bt_sco_thread_destory(adev);
    free_audio_control(adev->dev_ctl);
    stream_routing_manager_close(adev->dev_ctl);
    force_all_standby(adev);
    dsp_ctrl_close(&adev->agdsp_ctl);
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
    int ret = 0;
    int audio_record_mode;
    struct audio_record_proc_param * proc_param=NULL;
    char *param_data=NULL;
    DP_CONTROL_PARAM_T *ctrl_param_ptr = NULL;
    RECORDEQ_CONTROL_PARAM_T *eq_param_ptr = NULL;
    struct tiny_stream_in *in = (struct tiny_stream_in *)stream;
    struct tiny_audio_device *adev = in->dev;

    audio_record_mode = GetAudio_InMode_number_from_device(adev->in_devices);

    LOG_I("init_rec_process");

    if((true==adev->config.record_process_enable) &&(in->active_rec_proc) ) {
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

    param_data=(char *)adev->audio_param.param[SND_AUDIO_PARAM_RECORD_PROCESS_PROFILE].data;
    if(NULL==param_data){
      LOG_I("audio record process param is null");
      return false;
    }

    param_data +=  (sizeof(struct audio_record_proc_param)*audio_record_mode);


    eq_param_ptr=(RECORDEQ_CONTROL_PARAM_T *)param_data;
    ctrl_param_ptr=(DP_CONTROL_PARAM_T *)(param_data+sizeof(RECORDEQ_CONTROL_PARAM_T));

    ret |= AUDPROC_initDp(ctrl_param_ptr, sample_rate);
    ret |= AUDPROC_initRecordEq(eq_param_ptr, sample_rate);
    return ret;
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
    ret= parse_audio_debug(adev);

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

    adev->call_mode = AUDIO_MODE_NORMAL;
    adev->stream_status = 0;
    adev->out_devices = AUDIO_DEVICE_NONE;
    adev->in_devices = AUDIO_DEVICE_NONE;
    adev->mic_mute = false;
    adev->low_power = false;
    adev->loop_delay=2000;

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

    start_audio_tunning_server(adev);
#ifdef LOCAL_SOCKET_SERVER
    start_audio_local_server(adev);
#endif

    *device = &adev->hw_device.common;

    adev->loop_delay=2500;
    adev->loop_type=2;

    adev->config.record_nr_enable=true;
    adev->config.record_process_enable =true;

    ext_control_init(adev);
    modem_monitor_open(adev);
    dsp_ctrl_open(adev);
    list_init(&adev->active_out_list);
    list_init(&adev->active_input_list);

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

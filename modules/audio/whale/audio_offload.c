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
#define LOG_TAG "audio_hw_offload"

#include <errno.h>
#include <pthread.h>
#include <stdint.h>
#include <sys/time.h>
#include <sys/stat.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/mman.h>
#include <cutils/log.h>
#include <cutils/str_parms.h>
#include <cutils/properties.h>
#include <expat.h>
#include "audio_control.h"
#include "audio_offload.h"
#include "tinycompress/tinycompress.h"
#include "compress_util.h"


/*
 Extern function declaration
*/

/*
 Function declaration
*/
void audio_stop_compress_output(struct tiny_stream_out *out);

/*
 Function implement
*/
bool audio_is_offload_support_format(audio_format_t format)
{
    if (format == AUDIO_FORMAT_MP3 ||
        format == AUDIO_FORMAT_AAC) {
        return true;
    }
    return false;
}

int audio_get_offload_codec_id(audio_format_t format)
{
    int id = 0;

    switch (format) {
    case AUDIO_FORMAT_MP3:
        id = SND_AUDIOCODEC_MP3;
        break;
    case AUDIO_FORMAT_AAC:
        id = SND_AUDIOCODEC_AAC;
        break;
    default:
        LOG_E("%s: audio format (%d) is not supported ", __func__, format);
    }

    return id;
}

struct compress {
    int fd;
    unsigned int flags;
    char error[128];
    struct compr_config *config;
    int running;
    int max_poll_wait_ms;
    int nonblocking;
    unsigned int gapless_metadata;
    unsigned int next_track;
};

static void do_offload_output_standby(struct tiny_audio_device *adev,void * out,AUDIO_HW_APP_T type)
{
    int usecase = UC_UNKNOWN;
    struct tiny_stream_out *offload_out=(struct tiny_stream_out *)out;

    if(offload_out->standby==true){
        LOG_W("do_offload_output_standby exit");
        return ;
    }
    offload_out->standby=true;
    usecase = stream_type_to_usecase(offload_out->audio_app_type);

    set_mdg_mute(adev->dev_ctl,usecase,true);

    audio_stop_compress_output(offload_out);
    offload_out->gapless_mdata.encoder_delay = 0;
    offload_out->gapless_mdata.encoder_padding = 0;
    if (offload_out->compress != NULL) {
        LOG_E("do_offload_output_standby audio_offload compress_close");
        compress_close(offload_out->compress);
        offload_out->compress = NULL;
    }

    if(adev->pcm_modem_dl) {
        pcm_close(adev->pcm_modem_dl);
        adev->pcm_modem_dl = NULL;
    }

    LOG_I("do_offload_output_standby start exit");
    adev->audio_outputs_state &= ~AUDIO_OUTPUT_DESC_OFFLOAD;
    adev->stream_status &=~(1<<offload_out->audio_app_type);
    adev->offload_on = 0;
    set_usecase(adev->dev_ctl, usecase, false);
    set_audioparam(adev->dev_ctl,PARAM_USECASE_CHANGE,NULL,false);
}

int audio_start_compress_output(struct audio_stream_out *out_p)
{
    int ret = 0;
    int usecase = UC_UNKNOWN;
    struct tiny_stream_out *out = (struct tiny_stream_out *) out_p;
    struct tiny_audio_device *adev = out->dev;

    out->devices= adev->dev_ctl->out_devices;
    usecase = stream_type_to_usecase(out->audio_app_type);
    ret = set_usecase(adev->dev_ctl, usecase, true);
    if(ret < 0) {
        goto Err;
    }
    out->pcm = NULL;    /*set pcm to NULL to avoid pcm_write after starting offload-playback*/
    if(!adev->pcm_modem_dl) {
        LOG_I("audio_start_compress_output pcm_modem_dl %d rate:%d stop_threshold:%x port:%d pcm;%x",
            adev->dev_ctl->pcm_handle.playback_devices[AUD_PCM_OFF_LOAD],
            out->config->rate,
            out->config->stop_threshold,
            adev->dev_ctl->pcm_handle.compress.devices,
            adev->dev_ctl->pcm_handle.playback_devices[AUD_PCM_OFF_LOAD]);

        adev->pcm_modem_dl= pcm_open(adev->dev_ctl->cards.s_tinycard, adev->dev_ctl->pcm_handle.playback_devices[AUD_PCM_OFF_LOAD], PCM_OUT, out->config);
        LOG_I("audio_start_compress_output pcm_modem_dl start end");
        if (!pcm_is_ready(adev->pcm_modem_dl)) {
           LOG_E("%s: cannot open pcm_modem_dl : %s", __func__, pcm_get_error(adev->pcm_modem_dl));
           pcm_close(adev->pcm_modem_dl);
           adev->pcm_modem_dl = NULL;
           goto Err;
        }
        //pcm_start(adev->pcm_modem_dl);
    }
    out->compress = compress_open(adev->dev_ctl->cards.s_tinycard, adev->dev_ctl->pcm_handle.compress.devices,
                                   COMPRESS_IN, &out->compress_config);

    if(NULL==out->compress){
        LOG_E("%s: compress_open ERR:%s ", __func__, compress_get_error(out->compress));
        goto Err;
    }
    
    if (out->compress && !is_compress_ready(out->compress)) {
        struct compress *compress_tmp = out->compress;
        LOG_E("%s: err:%s fd:%d return ", __func__, compress_get_error(out->compress),compress_tmp->fd);
        compress_close(out->compress);
        out->compress = NULL;
        goto Err;
    }

    LOG_I("%s: compress_open out compress:%p app_type:%d", __func__,out->compress,out->audio_app_type);

    if (out->audio_offload_callback) {   
        compress_nonblock(out->compress, out->is_offload_nonblocking); 
    }

    set_mdg_mute(adev->dev_ctl,usecase,false);
    select_devices_new(adev->dev_ctl,out->audio_app_type,out->devices,false,false,true);
    out->standby_fun = do_offload_output_standby;
    out->standby = false;
    out->audio_app_type = AUDIO_HW_APP_OFFLOAD;
    set_offload_volume(adev->dev_ctl,adev->dev_ctl->music_volume,adev->dev_ctl->music_volume);
    set_audioparam(adev->dev_ctl,PARAM_USECASE_DEVICES_CHANGE,NULL,false);
    return 0;
Err:
    LOG_E("audio_start_compress_output err");
    adev->stream_status &=~(1<<AUDIO_HW_APP_OFFLOAD);
    if(adev->pcm_modem_dl!=NULL){
        pcm_close(adev->pcm_modem_dl);
        adev->pcm_modem_dl = NULL;
    }
    if(out->compress!=NULL){
        compress_close(out->compress);
        out->compress = NULL;
    }
    set_usecase(adev->dev_ctl, usecase, false);
    return -1;
}

void audio_stop_compress_output(struct tiny_stream_out *out)
{
    struct tiny_audio_device *adev = out->dev;
    LOG_I("%s in, audio_app_type:%d, audio_offload_state:%d ",
          __func__, out->audio_app_type, out->audio_offload_state);

    out->audio_offload_state = AUDIO_OFFLOAD_STATE_STOPED;
    out->is_offload_compress_started = false;
    out->is_offload_need_set_metadata =
        true;  /* need to set metadata to driver next time */
    if (out->compress != NULL) {
        compress_stop(out->compress);
        /* wait for finishing processing the command */
        while (out->is_audio_offload_thread_blocked) {
            LOG_I("audio_stop_compress_output wait");
            pthread_cond_wait(&out->audio_offload_cond, &out->lock);
        }
    }

    if(adev->pcm_modem_dl) {
        pcm_stop(adev->pcm_modem_dl);
    }
}

int audio_get_compress_metadata(struct audio_stream_out *out_p,
                                struct str_parms *parms)
{
    int ret = 0;
    char value[32];
    struct tiny_stream_out *out = (struct tiny_stream_out *) out_p;
    struct compr_gapless_mdata tmp_compress_metadata;

    if (!out || !parms) {
        return -1;
    }
    /* get meta data from audio framework */
    ret = str_parms_get_str(parms, AUDIO_OFFLOAD_CODEC_DELAY_SAMPLES, value,
                            sizeof(value));
    if (ret >= 0) {
        tmp_compress_metadata.encoder_delay = atoi(value);
    } else {
        return -1;
    }

    ret = str_parms_get_str(parms, AUDIO_OFFLOAD_CODEC_PADDING_SAMPLES, value,
                            sizeof(value));
    if (ret >= 0) {
        tmp_compress_metadata.encoder_padding = atoi(value);
    } else {
        return -1;
    }

    out->gapless_mdata = tmp_compress_metadata;
    out->is_offload_need_set_metadata = true;
    LOG_I("%s successfully, new encoder_delay: %u, encoder_padding: %u ",
          __func__, out->gapless_mdata.encoder_delay, out->gapless_mdata.encoder_padding);

    return 0;
}

int audio_send_offload_cmd(struct audio_stream_out *out_p,
                           AUDIO_OFFLOAD_CMD_T command)
{
    struct tiny_stream_out *out = (struct tiny_stream_out *) out_p;
    struct audio_offload_cmd *cmd = (struct audio_offload_cmd *)calloc(1,
                                    sizeof(struct audio_offload_cmd));

    LOG_D("%s, cmd:%d, offload_state:%d ",
          __func__, command, out->audio_offload_state);
    /* add this command to list, then send signal to offload thread to process the command list */
    cmd->cmd = command;
    list_add_tail(&out->audio_offload_cmd_list, &cmd->node);
    pthread_cond_signal(&out->audio_offload_cond);
    return 0;
}

void *audio_offload_thread_loop(struct audio_stream_out *param)
{
    struct tiny_stream_out *out = (struct tiny_stream_out *) param;
    struct listnode *item;

    static long time1=0, time2=0;

    /* init the offload state */
    out->audio_offload_state = AUDIO_OFFLOAD_STATE_STOPED;
    out->is_offload_compress_started = false;

    setpriority(PRIO_PROCESS, 0, ANDROID_PRIORITY_AUDIO);
    set_sched_policy(0, SP_FOREGROUND);
    prctl(PR_SET_NAME, (unsigned long)"Audio Offload Thread", 0, 0, 0);

    LOG_I("%s in", __func__);
    pthread_mutex_lock(&out->lock);
    for (;;) {
        bool need_send_callback = false;
        struct audio_offload_cmd *cmd = NULL;
        stream_callback_event_t event;

        LOG_D("%s, offload_cmd_list:%d, offload_state:%d",
              __func__, list_empty(&out->audio_offload_cmd_list), out->audio_offload_state);

        /*
            If the command list is not empty, don't need to wait for new, just process it.
            Otherwise, wait for new command.
        */
        if (list_empty(&out->audio_offload_cmd_list)) {
            LOG_I("audio_offload_thread_loop audio_offload_cmd_list is empty,wait");
            pthread_cond_wait(&out->audio_offload_cond, &out->lock);
            continue;
        }
        /* get the command from the list, then process the command */
        item = list_head(&out->audio_offload_cmd_list);
        cmd = node_to_item(item, struct audio_offload_cmd, node);
        list_remove(item);
        LOG_I("%s, offload_state:%d, offload_cmd:%d, out->compress:%p ",
              __func__, out->audio_offload_state, cmd->cmd, out->compress);

        if (cmd->cmd == AUDIO_OFFLOAD_CMD_EXIT) {
            LOG_I("audio_offload_thread_loop AUDIO_OFFLOAD_CMD_EXIT");
            free(cmd);
            break;
        }
        if (out->compress == NULL) {
            LOG_I("audio_offload_thread_loop:compress is null");
            pthread_cond_signal(&out->audio_offload_cond);
            continue;
        }
        out->is_audio_offload_thread_blocked = true;
        pthread_mutex_unlock(&out->lock);
        need_send_callback = false;
        switch(cmd->cmd) {
        case AUDIO_OFFLOAD_CMD_WAIT_FOR_BUFFER:
            time1=getCurrentTimeUs();
            compress_wait(out->compress, -1);
            time2=getCurrentTimeUs()-time1;
 //           if(time2>60*1000){
                LOG_D("audio_offload_thread_loop compress_wait:%dus",time2);
 //           }
            need_send_callback = true;
            event = STREAM_CBK_EVENT_WRITE_READY;
            break;
        case AUDIO_OFFLOAD_CMD_PARTIAL_DRAIN:
            compress_next_track(out->compress);
            compress_partial_drain(out->compress);
            need_send_callback = true;
            event = STREAM_CBK_EVENT_DRAIN_READY;
            break;
        case AUDIO_OFFLOAD_CMD_DRAIN:
            compress_drain(out->compress);
            need_send_callback = true;
            event = STREAM_CBK_EVENT_DRAIN_READY;
            break;
        default:
            LOG_E("%s unknown command received: %d", __func__, cmd->cmd);
            break;
        }
        LOG_I("audio_offload_thread_loop try lock");
        pthread_mutex_lock(&out->lock);
        out->is_audio_offload_thread_blocked = false;
        /* send finish processing signal to awaken where is waiting for this information */
        pthread_cond_signal(&out->audio_offload_cond);
        if (need_send_callback && out->audio_offload_callback) {
            out->audio_offload_callback(event, NULL, out->audio_offload_cookie);
        }
        free(cmd);
    }

    LOG_I("audio_offload_thread_loop:start exit");
    pthread_cond_signal(&out->audio_offload_cond);
    /* offload thread loop exit, free the command list */
    while (!list_empty(&out->audio_offload_cmd_list)) {
        item = list_head(&out->audio_offload_cmd_list);
        list_remove(item);
        free(node_to_item(item, struct audio_offload_cmd, node));
    }
    pthread_mutex_unlock(&out->lock);
    LOG_I("audio_offload_thread_loop:exit");
    return NULL;
}


int audio_offload_create_thread(struct audio_stream_out  *out_p)
{
   struct tiny_stream_out *out = (struct tiny_stream_out *) out_p;
    pthread_cond_init(&out->audio_offload_cond, (const pthread_condattr_t *) NULL);
    list_init(&out->audio_offload_cmd_list);
    pthread_create(&out->audio_offload_thread, (const pthread_attr_t *) NULL,
                   audio_offload_thread_loop, out);
    LOG_I("%s, successful, id:%lu ", __func__, out->audio_offload_thread);
    return 0;
}

int audio_offload_destroy_thread(struct audio_stream_out  *out_p)
{
    struct tiny_stream_out *out = (struct tiny_stream_out *) out_p;
    LOG_I("audio_offload_destroy_thread enter");
    pthread_mutex_lock(&out->lock);
    audio_stop_compress_output(out);
    /* send command to exit the thread_loop */
    audio_send_offload_cmd(out, AUDIO_OFFLOAD_CMD_EXIT);
    pthread_mutex_unlock(&out->lock);
    pthread_join(out->audio_offload_thread, (void **) NULL);
    pthread_cond_destroy(&out->audio_offload_cond);
    LOG_I("audio_offload_destroy_thread exit");
    return 0;
}

AUDIO_OFFLOAD_MIXER_CARD_T audio_get_offload_mixer_card( struct audio_stream_out  *out_p)
{
    AUDIO_OFFLOAD_MIXER_CARD_T card = AUDIO_OFFLOAD_MIXER_INVALID;
    struct tiny_stream_out *out = (struct tiny_stream_out * )out_p;
    struct tiny_audio_device *adev = out->dev;

    if(out->audio_app_type == AUDIO_HW_APP_OFFLOAD) {
        if(adev->audio_outputs_state == AUDIO_OUTPUT_DESC_PRIMARY) {
            LOG_D("%s, offload-playback write, standby all primary output ", __func__);
            //    force_all_standby(adev);
        } else { //AUDIO_OFFLOAD_STATE_PLAYING
            //go on
        }
        adev->audio_outputs_state |= AUDIO_OUTPUT_DESC_OFFLOAD;
        card = AUDIO_OFFLOAD_MIXER_CARD1;
    }
    if(out->audio_app_type == AUDIO_HW_APP_PRIMARY) {
        if(adev->audio_outputs_state & AUDIO_OUTPUT_DESC_OFFLOAD) {
            LOG_D("%s, primary-playback write, mix-data to dsp, audio_outputs_state:0x%x ",
                  __func__, adev->audio_outputs_state);
            /* To do: mix data to cp */
            card = AUDIO_OFFLOAD_MIXER_CARD2;
        } else {
            card = AUDIO_OFFLOAD_MIXER_CARD0;
        }
        adev->audio_outputs_state |= AUDIO_OUTPUT_DESC_PRIMARY;
    }
    LOG_I("%s: %d, out. audio_app_type:%d, audio_outputs_state:0x%x ",
          __func__, card, out->audio_app_type, adev->audio_outputs_state);
    return card;
}

void dump_pcm(const void *pBuffer, size_t aInBufSize)
{
    FILE *fp = fopen("/data/local/media/leodump.wav", "ab");
    fwrite(pBuffer, 1, aInBufSize, fp);
    fclose(fp);
}

ssize_t out_write_compress(struct audio_stream_out* out_p, const void *buffer,
                           size_t bytes)
{
    int ret = 0;
    struct tiny_stream_out *out = (struct tiny_stream_out * )out_p;
    struct tiny_audio_device *adev = out->dev;
    LOG_D("%s: want to write buffer (%d bytes) to compress device, offload_state:%d ",
          __func__, bytes, out->audio_offload_state);

    if (out->is_offload_need_set_metadata) {
        LOG_W("%s: need to send new metadata to driver ", __func__);
        compress_set_gapless_metadata(out->compress, &out->gapless_mdata);
        out->is_offload_need_set_metadata = 0;
    }

    //dump_pcm(buffer, bytes);
    ret = offload_write(out->compress, buffer, bytes);
    LOG_D("%s: finish writing buffer (%d bytes) to compress device, and return %d",
          __func__, bytes, ret);
    /* neet to wait for ring buffer to ready for next read or write */
    if (ret >= 0 && ret < (ssize_t)bytes) {
        audio_send_offload_cmd(out, AUDIO_OFFLOAD_CMD_WAIT_FOR_BUFFER);
    }

    if (!out->is_offload_compress_started) {
        int result=-1;
        if(!adev->pcm_modem_dl) {
            adev->pcm_modem_dl = pcm_open(adev->dev_ctl->cards.s_tinycard, adev->dev_ctl->pcm_handle.playback_devices[AUD_PCM_OFF_LOAD],
                                          PCM_OUT, &(adev->dev_ctl->pcm_handle.play[AUD_PCM_OFF_LOAD]));

            if (!pcm_is_ready(adev->pcm_modem_dl)) {
                LOG_E("%s: cannot open pcm_modem_dl : %s", __func__,
                      pcm_get_error(adev->pcm_modem_dl));
                pcm_close(adev->pcm_modem_dl);
                adev->pcm_modem_dl = NULL;
                return -1;
            }
        }
        result = pcm_start(adev->pcm_modem_dl);
        LOG_W("pcm out write warning:%d, (%s)", result,
                  pcm_get_error(adev->pcm_modem_dl));
        compress_start(out->compress);
        out->is_offload_compress_started = true;
        out->audio_offload_state = AUDIO_OFFLOAD_STATE_PLAYING;
    }
    return ret;
}

int out_offload_set_callback(struct audio_stream_out *stream,
                             stream_callback_t callback, void *cookie)
{
    struct tiny_stream_out *out = (struct tiny_stream_out *)stream;

    LOG_D("%s in, audio_app_type:%d callback:%p cookie:%p", __func__, out->audio_app_type,
        callback,cookie);
    pthread_mutex_lock(&out->lock);
    out->audio_offload_callback = callback;
    out->audio_offload_cookie = cookie;
    pthread_mutex_unlock(&out->lock);
    return 0;
}

int out_offload_pause(struct audio_stream_out *stream)
{
    struct tiny_stream_out *out = (struct tiny_stream_out *)stream;

    int status = -ENOSYS;
    LOG_I("%s in, audio_app_type:%d, audio_offload_state:%d ",
          __func__, out->audio_app_type, out->audio_offload_state);

    if (out->audio_app_type == AUDIO_HW_APP_OFFLOAD) {
        pthread_mutex_lock(&out->lock);
        if (out->compress != NULL
            && out->audio_offload_state == AUDIO_OFFLOAD_STATE_PLAYING) {
            status = compress_pause(out->compress);
            out->audio_offload_state = AUDIO_OFFLOAD_STATE_PAUSED;
        }
        pthread_mutex_unlock(&out->lock);
    }
    return status;
}

int out_offload_resume(struct audio_stream_out *stream)
{
    struct tiny_stream_out *out = (struct tiny_stream_out *)stream;
    struct tiny_audio_device *adev = out->dev;

    int ret = -ENOSYS;
    LOG_I("%s in, audio_app_type:%d, audio_offload_state:%d ",
          __func__, out->audio_app_type, out->audio_offload_state);

    if (out->audio_app_type == AUDIO_HW_APP_OFFLOAD) {

        if(!adev->pcm_modem_dl) {
            adev->pcm_modem_dl = pcm_open(adev->dev_ctl->cards.s_tinycard, adev->dev_ctl->pcm_handle.playback_devices[AUD_PCM_OFF_LOAD],
                                          PCM_OUT, &(adev->dev_ctl->pcm_handle.play[AUD_PCM_OFF_LOAD]));
            if (!pcm_is_ready(adev->pcm_modem_dl)) {
                LOG_E("%s: cannot open pcm_modem_dl : %s", __func__,
                      pcm_get_error(adev->pcm_modem_dl));
                pcm_close(adev->pcm_modem_dl);
                adev->pcm_modem_dl = NULL;
            }
            pcm_start(adev->pcm_modem_dl);
            LOG_E("%s: pcm_start", __func__);
        }

        ret = 0;
        pthread_mutex_lock(&out->lock);
        if (out->compress != NULL
            && out->audio_offload_state == AUDIO_OFFLOAD_STATE_PAUSED) {
            ret = compress_resume(out->compress);
            out->audio_offload_state = AUDIO_OFFLOAD_STATE_PLAYING;
        }
        pthread_mutex_unlock(&out->lock);
    }
    return ret;
}

int out_offload_drain(struct audio_stream_out *stream, audio_drain_type_t type )
{
    struct tiny_stream_out *out = (struct tiny_stream_out *)stream;

    int ret = -ENOSYS;
    LOG_I("%s in, audio_app_type:%d, audio_offload_state:%d, type:%d ",
          __func__, out->audio_app_type, out->audio_offload_state, type);

    if (out->audio_app_type == AUDIO_HW_APP_OFFLOAD) {
        pthread_mutex_lock(&out->lock);
        if (type == AUDIO_DRAIN_EARLY_NOTIFY) {
            ret = audio_send_offload_cmd(out, AUDIO_OFFLOAD_CMD_PARTIAL_DRAIN);
        } else {
            ret = audio_send_offload_cmd(out, AUDIO_OFFLOAD_CMD_DRAIN);
        }
        pthread_mutex_unlock(&out->lock);
    }
    LOG_I("out_offload_drain exit");
    return ret;
}

int out_offload_flush(struct audio_stream_out *stream)
{
    struct tiny_stream_out *out = (struct tiny_stream_out *)stream;
    LOG_I("%s in, audio_app_type:%d, audio_offload_state:%d ",
          __func__, out->audio_app_type, out->audio_offload_state);

    if (out->audio_app_type == AUDIO_HW_APP_OFFLOAD) {
        pthread_mutex_lock(&out->lock);
        if(out->standby == false){
            audio_stop_compress_output(out);
        }
        pthread_mutex_unlock(&out->lock);
        LOG_I("out_offload_flush exit");
        return 0;
    }
    return -ENOSYS;
}

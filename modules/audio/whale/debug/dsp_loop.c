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
        num_read=pcm_read(pcm, buffer, size);
        LOG_D("dsp_loop_rx_thread read:0x%x req:0x%x",num_read,size);
        if (!num_read){
            bytes_read += size;
            if(adev->debug.record.dsploop.hex_fd>0){
                write(adev->debug.record.dsploop.hex_fd,buffer,size);
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
    while(1){
        num_read=ring_buffer_get(out->ring_buf, (void *)buffer, size);
        LOG_I("dsp_loop_tx_thread read:0x%x req:0x%x",num_read,size);

        if(num_read > 0){
            no_data_count=0;
            ret = pcm_write(pcm, buffer,num_read);
            if (ret) {
                int ret = 0;
                LOG_E("dsp_loop_tx_thread Error playing sample:%s\n",pcm_get_error(pcm));
                ret = pcm_close(pcm);
                out->pcm = pcm_open(0, adev->dev_ctl->pcm_handle.playback_devices[AUD_PCM_DSP_LOOP],
                        PCM_OUT , &adev->dev_ctl->pcm_handle.play[AUD_PCM_DSP_LOOP]);
                if (!pcm || !pcm_is_ready(out->pcm)) {
                    LOG_E("dsp_loop_tx_thread Unable to open PCM device %u (%s)\n",
                          0, pcm_get_error(out->pcm));
                    goto ERR;
                }
                pcm=out->pcm;
            }else{
                write_count+=num_read;
                LOG_I("dsp_loop_tx_thread write:0x%x total:0x%x",num_read,write_count);
                if(adev->debug.playback.dsploop.hex_fd>0){
                    write(adev->debug.playback.dsploop.hex_fd,buffer,num_read);
                }
            }
        }else{
            usleep(100*1000);
            no_data_count++;
            LOG_I("dsp_loop_tx_thread no data read");
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

static void audio_dsp_loop_standby(struct tiny_audio_device *adev,void * loop_stream,AUDIO_HW_APP_T type)
{
    struct tiny_stream_out *dsp_loop=(struct tiny_stream_in *)loop_stream;
    struct loop_ctl_t *ctl=NULL;
    struct dsp_loop_t *loop=&adev->loop_ctl;
    int wait_count=30;

    LOG_I("audio_dsp_loop_stop enter");
    if(false==loop->state){
        LOG_W("audio_dsp_loop_stop failed, loop is not work");
        return -1;
    }

    /* first:close record pcm devices */
    loop->in.is_exit=true;
    pthread_join(&loop->in.thread,  NULL);
    usleep(100*1000);

    LOG_I("audio_dsp_loop_stop %d",__LINE__);
    loop->out.is_exit=true;
    pthread_join(&loop->out.thread, NULL);

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

    switch_vbc_iis_route(adev->control,UC_LOOP,false);
    loop->state=false;

    dsp_loop->standby=true;
    adev->stream_status &=~(1<<dsp_loop->audio_app_type);

    pthread_mutex_destroy(&(loop->out.lock));
    pthread_mutex_destroy(&(loop->in.lock));

    if(NULL!=dsp_loop){
        pthread_mutex_destroy(&(dsp_loop->lock));
        free(dsp_loop);
    }

    set_usecase(adev->dev_ctl,UC_LOOP,false);

    clear_audio_param(&adev->audio_param);
    LOG_I("audio_dsp_loop_standby exit");

    return 0;
}

static int audio_dsp_loop_start(struct tiny_audio_device *adev,struct dsp_loop_t *loop){
    struct ring_buffer *ring_buf=NULL;
    struct pcm_config * in_config=NULL;
    struct tiny_stream_out *dsp_loop=NULL;
    int size=0;
    int min_buffer_size=0;
    int dsp_zero_size=0;
    int delay_size=0;

    if(true==loop->state){
        LOG_W("audio_dsp_loop_start failed, loop is working now");
        return -1;
    }

    set_usecase(adev->dev_ctl,UC_LOOP,true);

    dsp_loop=(struct tiny_stream_out *)calloc(1,
                                sizeof(struct tiny_stream_out));
    dsp_loop->dev=adev;
    dsp_loop->standby=false;
    dsp_loop->audio_app_type=AUDIO_HW_APP_DSP_LOOP;

    in_config=&(adev->dev_ctl->pcm_handle.record[AUD_PCM_DSP_LOOP]);

    size=calculation_ring_buffer_size(adev->loop_delay*16,in_config);
    min_buffer_size=(TEST_AUDIO_LOOP_MIN_DATA_MS *in_config->rate*in_config->channels*2/1000);
    if(size<min_buffer_size){
        size=calculation_ring_buffer_size(TEST_AUDIO_LOOP_MIN_DATA_MS,in_config);
    }

    /* dsp process frame buffer size is 122 word(244bytes),
       the delay buffer size must be a multiple of dsp frame buffer */
    delay_size=adev->loop_delay*in_config->rate*in_config->channels*2/1000;
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

    select_devices_new(adev->dev_ctl,adev->in_devices,1);
    select_devices_new(adev->dev_ctl,adev->out_devices,0);

    loop->in.is_exit=false;
    loop->out.is_exit=false;

    sem_init(&loop->in.sem, 0, 1);
    sem_init(&loop->out.sem, 0, 1);

    apply_dsploop_control(&(adev->control->route.dsploop_ctl),adev->loop_type);
    switch_vbc_iis_route(adev->control,UC_LOOP,true);

    LOG_I("audio_dsp_loop_start out devices:%d rate:%d",
        adev->dev_ctl->pcm_handle.playback_devices[AUD_PCM_DSP_LOOP],
        adev->dev_ctl->pcm_handle.play[AUD_PCM_DSP_LOOP].rate);

    /*
        dsp loop test needs to open the record pcm devices first and
        then open the playback pcm devices.
    */
    loop->out.pcm  =
        pcm_open(0, adev->dev_ctl->pcm_handle.playback_devices[AUD_PCM_DSP_LOOP],
        PCM_OUT , &adev->dev_ctl->pcm_handle.play[AUD_PCM_DSP_LOOP]);

    if (!pcm_is_ready(loop->out.pcm)) {
        LOG_E("audio_dsp_loop_start:cannot open pcm : %s",
              pcm_get_error(loop->out.pcm));
        goto error;
    }

    LOG_I("audio_dsp_loop_start in devices:%d rate:%d",
        adev->dev_ctl->pcm_handle.record_devices[AUD_PCM_DSP_LOOP],
        adev->dev_ctl->pcm_handle.record[AUD_PCM_DSP_LOOP].rate);

    loop->in.pcm  =
        pcm_open(0, adev->dev_ctl->pcm_handle.record_devices[AUD_PCM_DSP_LOOP],
        PCM_IN,  &adev->dev_ctl->pcm_handle.record[AUD_PCM_DSP_LOOP]);

    if (!pcm_is_ready(loop->in.pcm)) {
        LOG_E("start_input_stream:cannot open pcm : %s",
              pcm_get_error(loop->in.pcm));
        goto error;
    }

    if(pthread_create(&loop->out.thread, NULL, dsp_loop_tx_thread, (void *)&(loop->out))) {
        LOG_E("audio_out_devices_test creating tx thread failed !!!!");
        goto error;
    }

    if(pthread_create(&loop->in.thread, NULL, dsp_loop_rx_thread, (void *)&(loop->in))){
        LOG_E("audio_out_devices_test creating rx thread failed !!!!");
        goto error;
    }

    loop->state=true;
	dsp_loop->audio_app_type = AUDIO_HW_APP_DSP_LOOP;
    audio_add_output(adev,dsp_loop,AUDIO_HW_APP_DSP_LOOP,audio_dsp_loop_standby);
    adev->stream_status |=(1<<dsp_loop->audio_app_type);

    select_audio_param(adev,true);

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

    if(NULL!=dsp_loop){
        pthread_mutex_destroy(&(dsp_loop->lock));
        free(dsp_loop);
    }

    set_usecase(adev->dev_ctl,UC_LOOP,false);
    switch_vbc_iis_route(adev->control,UC_LOOP,false);
    loop->state=false;

    LOG_I("audio_dsp_loop_start failed");
    return -1;
}

int audio_dsp_loop(void * dev,struct str_parms *parms,bool is_start){
    int ret=0;
    struct tiny_audio_device * adev=(struct tiny_audio_device *)dev;

    if(true==is_start){
        force_all_standby(adev);
        pthread_mutex_lock(&adev->lock);
        ret=audio_dsp_loop_start(adev,&adev->loop_ctl);
        pthread_mutex_unlock(&adev->lock);
    }else{
        force_out_standby(adev,AUDIO_HW_APP_DSP_LOOP);
    }

    return ret;
}

int set_dsploop_type(void * dev,struct str_parms *parms,int type, char * val){
    struct tiny_audio_device *adev=(struct tiny_audio_device *)dev;
    LOG_I("set_audioloop_type:0x%x",type);

    pthread_mutex_lock(&adev->lock);
    adev->loop_type=type;
    pthread_mutex_unlock(&adev->lock);
    return 0;
}

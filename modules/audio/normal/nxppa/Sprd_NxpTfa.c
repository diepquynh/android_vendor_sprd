#define LOG_TAG "nxptfa"
#include "Sprd_NxpTfa.h"
#include <audio_utils/resampler.h>
#define CurentSamleRate 44100
#define TransDataMaxLen 1280*8

struct nxppa_res
{
    pthread_t *writewordthread;
    sem_t Sem_WriteWord;
    int StopWriteEmpty;
    struct pcm *pcm_tfa;
    int is_inited;
    int is_speakon;
    int is_clock_on;
    struct resampler_itfe  *resampler;
    char * buffer_resampler;
    pthread_mutex_t lock;
};



 void * NxpTfa_FillEmptyWord(struct nxppa_res * nxp_res)
{
        int write_count = 0;
        int ret = 0;
        int wait_notified = 0;
        int buffer_size = pcm_get_buffer_size(nxp_res->pcm_tfa);
        char *buffer = malloc(buffer_size);
        if(!buffer) {
            return -1;
        }
        memset(buffer,0,sizeof(buffer));

        while(!nxp_res->StopWriteEmpty)
        {
            ret = pcm_mmap_write(nxp_res->pcm_tfa, buffer, buffer_size);
            if(!ret) {
                write_count++;
                if((write_count > 2) &&(!wait_notified)){
                    wait_notified = 1;
                    sem_post(&nxp_res->Sem_WriteWord);
                }
            }
        }
        ALOGE("Stop Write Empty Word \n");
        free(buffer);
        return NULL;
}


int  NxpTfa_ClockInit(struct nxppa_res * nxp_res)
{
    int ret = 0;
    ret = pthread_create(&nxp_res->writewordthread, NULL, NxpTfa_FillEmptyWord, nxp_res);
     if (ret)
     {
        ALOGE("%s: pthread_create falied, code is %s", __func__, strerror(errno));
        return ret;
     }
     sem_wait(&nxp_res->Sem_WriteWord);
     nxp_res->is_clock_on = 1;
     return 0;
}

int  NxpTfa_ClockDeInit(struct nxppa_res * nxp_res)
{
    nxp_res->StopWriteEmpty= 1;
    if(nxp_res->writewordthread) {
        pthread_join(nxp_res->writewordthread, (void **)NULL);
        nxp_res->writewordthread = NULL;
    }
    nxp_res->is_clock_on = 0;
    return 0;
}

nxp_pa_handle NxpTfa_Open(struct NxpTpa_Info *info)
{
    int card = 0;
    int ret = 0;
    struct nxppa_res * nxp_res = NULL;
    if(!info) {
        return NULL;
    }
    ALOGE("peter: NxpTfa_Open in");
    nxp_res =malloc(sizeof(struct nxppa_res));//
    if(!nxp_res) {
        return NULL;
    }
    memset(nxp_res, 0, sizeof(struct nxppa_res));
    pthread_mutex_init(&nxp_res->lock, NULL);

    nxp_res->buffer_resampler = malloc(20*1024);
    if(!nxp_res->buffer_resampler ) {
        return NULL;
    }
    if(nxp_res->resampler == NULL) {
        ret = create_resampler( 44100,
                    48000,
                    2,
                    RESAMPLER_QUALITY_DEFAULT,
                    NULL,
                    &nxp_res->resampler);
        if (ret != 0) {
            nxp_res->resampler = NULL;
            goto error;
        }
    }

    if(!info->pCardName) {
        goto error;
    }
    card =  get_snd_card_number(info->pCardName);
    if(card < 0) {
        goto error;
    }
    nxp_res->pcm_tfa=pcm_open(card, info->dev , PCM_OUT|PCM_MMAP|PCM_NOIRQ, &info->pcmConfig);
    if(!pcm_is_ready(nxp_res->pcm_tfa)) {
        goto error;
    }

    sem_init(&nxp_res->Sem_WriteWord,0,0);
    nxp_res->StopWriteEmpty=0;
    nxp_res->is_clock_on=0;
    nxp_res->is_inited=0;
    nxp_res->is_speakon=0;

    ret = NxpTfa_ClockInit(nxp_res);
    if(ret) {
        goto error;
    }
    ret = tfa9890_init();
    if (ret) {
        ALOGD ("Device Tfa9890 Init failed, ret: %d \n", ret);
        goto error;
    }
    tfa9890_setSamplerate(info->pcmConfig.rate);
    nxp_res->is_inited = 1;
    tfa9890_SpeakerOn();
    nxp_res->is_speakon = 1;
//add for call
    NxpTfa_ClockDeInit(nxp_res);
    return (nxp_pa_handle)nxp_res;
    free(nxp_res);
error:
    NxpTfa_Close( nxp_res );
    return NULL;

}

int NxpTfa_Write( nxp_pa_handle handle , const void *data, unsigned int count)
{
    int ret = 0;
    int OutSize;
    size_t out_frames = 0;
    size_t in_frames = count/4;
    int16_t  * buffer = data;
    struct nxppa_res * nxp_res = NULL;
    if(handle == NULL)
        return -1;
    nxp_res = (struct nxppa_res *) handle;
    ALOGE("peter:NxpTfa_Write in");
    pthread_mutex_lock(&nxp_res->lock);
    if(nxp_res->is_clock_on) {
        NxpTfa_ClockDeInit(nxp_res);
    }
    out_frames = in_frames*2;
    if(nxp_res->resampler && nxp_res->buffer_resampler) {
        //ALOGE("peter 1: NxpTfa_Write in_frames %d,out_frames %d", in_frames, out_frames);
        nxp_res->resampler->resample_from_input(nxp_res->resampler,
                    (int16_t *)buffer,
                    &in_frames,
                    (int16_t *)nxp_res->buffer_resampler,
                    &out_frames);
        //ALOGE("peter 2: NxpTfa_Write in_frames %d,out_frames %d", in_frames, out_frames);
    }
    ret = pcm_mmap_write(nxp_res->pcm_tfa, nxp_res->buffer_resampler, out_frames*4);
    //ret = pcm_mmap_write(nxp_res->pcm_tfa, data, count);
    pthread_mutex_unlock(&nxp_res->lock);
    ALOGE("peter:NxpTfa_Write out");
    return ret;
}


void  NxpTfa_Close( nxp_pa_handle handle )
{
    struct nxppa_res * nxp_res = NULL;
    if(handle == NULL)
        return -1;
    nxp_res = (struct nxppa_res *) handle;
    pthread_mutex_lock(&nxp_res->lock);

    ALOGE("peter: NxpTfa_Close in");
#if 0
    if(!nxp_res->is_clock_on) {
        NxpTfa_ClockInit(nxp_res);
        ALOGD("tfa--NxpTfa_Close close the click!\n");
        }
#endif
    if(nxp_res->is_speakon) {
        tfa9890_SpeakerOff();
        ALOGD("tfa--NxpTfa_Close  tfa9890_SpeakerOff!\n");
    }
    if(nxp_res->is_inited) {
        tfa9890_deinit();
        ALOGD("tfa--NxpTfa_Close  Deinit!\n");
    }
    if(nxp_res->is_clock_on){
    NxpTfa_ClockDeInit(nxp_res);
    }
    if(nxp_res->pcm_tfa) {
        pcm_close(nxp_res->pcm_tfa);
    }
    if(nxp_res->resampler) {
        release_resampler(nxp_res->resampler);
        nxp_res->resampler= NULL;
    }
    if(nxp_res->buffer_resampler) {
        free(nxp_res->buffer_resampler);
        nxp_res->buffer_resampler = NULL;
    }
    sem_destroy (&nxp_res->Sem_WriteWord);
    pthread_mutex_unlock(&nxp_res->lock);
    free(nxp_res);

}












#include "sprd_cp_control.h"

#define LOG_TAG "sprd_cp_control"

struct pcm_config pcm_config_mixplayback = {
    .channels = 2,
    .rate = 8000,
    .period_size = 320,
    .period_count = 4,
    .format = PCM_FORMAT_S16_LE,
};

static int set_cp_mixer_type(struct mixer *cp_mixer, int mixer_type);
static void *mixer_to_cp_thread(void *args);

static void *mixer_to_cp_thread(void *args){
    struct cp_control_res *rt_res =  (struct cp_control_res *)args;
    struct pcm *pcm;
    char *buffer;
    unsigned int size;
    int playback_fd;
    int num_read,num_read_all;
    int fix_data_size = sizeof(s_pcm_mono);
    int ret = 0;
    int card = -1;
    bool enable=false;

    if(rt_res){
        card= rt_res->card;
        if(card < 0) {
            ALOGE("YAYE %s error! card :%d is invalid", __func__, rt_res->card);
            return NULL;
        }
    }

    pthread_mutex_lock(&rt_res->lock);
    while(rt_res->init){
        pthread_mutex_unlock(&rt_res->lock);
        pcm = NULL;
        buffer = NULL;
        num_read_all = 0;
        size = 0;

        ALOGD("YAYE %s wait", __func__);
        sem_wait(&rt_res->sem);
        ALOGD("YAYE %s running", __func__);

        pthread_mutex_lock(&rt_res->lock);
        if(!rt_res->init)
            break;
        pthread_mutex_unlock(&rt_res->lock);

        pcm = pcm_open(card, 0, PCM_OUT| PCM_MMAP |PCM_NOIRQ |PCM_MONOTONIC, &rt_res->config);
        if (!pcm || !pcm_is_ready(pcm)) {
            ALOGE("YAYE %s  Unable to open PCM device %u (%s)\n", __func__, card, pcm_get_error(pcm));
            goto next;
        }

        size = pcm_frames_to_bytes(pcm, pcm_get_buffer_size(pcm));
        buffer = malloc(size);
        if (!buffer) {
            ALOGE("YAYE %s  Unable to allocate %d bytes\n", __func__, size);
            goto next;
        }
        ALOGD("YAYE %s write entry!!buffer size:%d", __func__, size);

        pthread_mutex_lock(&rt_res->lock);
        rt_res->fd=open(AUDIO_MIXER_TO_CP_PCM_FILE, O_RDONLY);
        if(rt_res->fd<0)
            rt_res->fd=open(AUDIO_MIXER_TO_CP_PCM_FILE_ROOT, O_RDONLY);
        if(rt_res->fd<0){
            ALOGE("YAYE %s unable to open file %s!!!", __func__, AUDIO_MIXER_TO_CP_PCM_FILE_ROOT);
        }
        while(rt_res->enable){
            pthread_mutex_unlock(&rt_res->lock);
            if(rt_res->fd > 0){
                num_read = read(rt_res->fd, buffer, size);
                ALOGD("YAYE %s read_data num:%d", __func__, num_read);
                if(num_read<=0){
                    goto next;
                }else
                    ret = pcm_mmap_write(pcm, buffer,num_read);
            }
            else {
                int i = 0;
                num_read = 0;
                do{
                    memcpy(buffer+num_read, s_pcm_mono, fix_data_size);
                    num_read += fix_data_size;
                    num_read_all += fix_data_size;
                    if(num_read_all>size)
                        break;
                }
                while((num_read + fix_data_size)<size);

                ret = pcm_mmap_write(pcm, buffer,num_read);
                ALOGD("YAYE %s write fixed data num_read:%d ret:%d", __func__, num_read, ret);
                if(num_read_all>size)
                    goto next;
            }

            if (ret) {
                int ret = 0;
                ALOGE("YAYE %s Error playing 1 sample:%s\n", __func__,pcm_get_error(pcm));
                ret = pcm_close(pcm);
                pcm = pcm_open(card, 0, PCM_OUT| PCM_MMAP |PCM_NOIRQ |PCM_MONOTONIC, &rt_res->config);
                if (!pcm || !pcm_is_ready(pcm)) {
                    ALOGE("YAYE %s Error Unable to open PCM device %u (%s)\n", __func__,
                          0, pcm_get_error(pcm));
                    goto next;
                }
            }
            ALOGD("YAYE %s write ok", __func__);
            pthread_mutex_lock(&rt_res->lock);
        }
        pthread_mutex_unlock(&rt_res->lock);

        next:
            if(buffer) {
                free(buffer);
                buffer = NULL;
            }

            if(pcm) {
                pcm_close(pcm);
                pcm = NULL;
            }

            if(rt_res->fd > 0) {
                close(rt_res->fd);
                rt_res->fd = -1;
            }

            ALOGD("YAYE %s write finish entry!!", __func__);
            pthread_mutex_lock(&rt_res->lock);
            set_cp_mixer_type(rt_res->cp_mixer, 0);
            ALOGD("YAYE %s write finish exit:%d", __func__);
            rt_res->enable = 0;
            sem_post(&rt_res->sem_end);
    }

    pthread_mutex_unlock(&rt_res->lock);
    return NULL;
}

static int set_cp_mixer_type(struct mixer *cp_mixer, int mixer_type){
    int ret=0;
    struct mixer_ctl *ctl1;

    ALOGD("YAYE %s mixer is %d", __func__, cp_mixer);
    ctl1 = mixer_get_ctl_by_name(cp_mixer,"PCM MIXER Route");
    mixer_ctl_set_value(ctl1, 0, mixer_type);//mixer_type 0: dl 1:ul 2:ul&&dl

    ALOGD("YAYE %s ctl1:%x mixer_type:%d", __func__, ctl1, mixer_type);

    return 0;
}

int cp_mixer_enable(struct cp_control_res * record_tone_info, struct mixer *cp_mixer, int card){
    int ret=0;
    struct cp_control_res * rt_res = record_tone_info;

    if(!rt_res){
        ALOGD("YAYE %s error, not been opend yet!!", __func__);
        return -1;
    }

    pthread_mutex_lock(&rt_res->lock);
    rt_res->card = card;
    if(NULL == rt_res->thread){
        ALOGD("YAYE %s first entry, card:%d!!", __func__, card);
        if(pthread_create(&rt_res->thread, NULL, mixer_to_cp_thread, (void *)rt_res)){
            ALOGE("YAYE %s creating failed !!!!", __func__);
            return -1;
        }
    }

    ALOGD("YAYE %s entry, card:%d!!", __func__, card);
    if(rt_res->enable ||rt_res->waiting_for_end){
        ALOGD("YAYE %s been enabled yet!!", __func__);
        pthread_mutex_unlock(&rt_res->lock);
        return 0;
    }

    sem_destroy(&rt_res->sem_end);
    sem_init(&rt_res->sem_end, 0, 0);
    set_cp_mixer_type(cp_mixer, 1);
    rt_res->cp_mixer = cp_mixer;
    rt_res->enable = 1;
    pthread_mutex_unlock(&rt_res->lock);
    sem_post(&rt_res->sem);
    ALOGD("YAYE %s EXIT!!", __func__);
    return 0;
}

int cp_mixer_disable(struct cp_control_res * record_tone_info, struct mixer *cp_mixer){
    int ret=0;
    struct cp_control_res * rt_res = record_tone_info;

    if(!rt_res){
        ALOGD("YAYE %s error, not been opend yet!!", __func__);
        return -1;
    }
    pthread_mutex_lock(&rt_res->lock);
    ALOGD("YAYE %s entry!!", __func__);
    if(!rt_res->enable){
        ALOGD("YAYE %s not been enabled yet!!", __func__);
        pthread_mutex_unlock(&rt_res->lock);
        return 0;
    }

    rt_res->enable = 0;
    rt_res->waiting_for_end = 1;
    pthread_mutex_unlock(&rt_res->lock);

    sem_wait(&rt_res->sem_end);
    pthread_mutex_lock(&rt_res->lock);
    rt_res->waiting_for_end = 0;
    pthread_mutex_unlock(&rt_res->lock);

    ALOGD("YAYE %s EXIT!!", __func__);
    return ret;
}

struct cp_control_res * cp_mixer_open(void){
    int ret=0;
    struct cp_control_res * rt_res = NULL;

    rt_res = malloc(sizeof(struct cp_control_res));
    if(!rt_res) {
        ret = -1;
        goto EXIT;
    }
    memset(rt_res, 0, sizeof(struct cp_control_res));
    pthread_mutex_init(&rt_res->lock, NULL);

    rt_res->config = pcm_config_mixplayback;
    rt_res->init = 1;

    sem_init(&rt_res->sem, 0, 0);
    sem_init(&rt_res->sem_end, 0, 0);

    return rt_res;

EXIT:
    if(rt_res)
        free(rt_res);
    ALOGD("YAYE %s error!!", __func__);
    return NULL;
}

int cp_mixer_close(struct cp_control_res * record_tone_info){
    int ret=0;
    struct cp_control_res * rt_res = record_tone_info;

    if(!rt_res) {
        ALOGD("YAYE %s not opend yet!!", __func__);
        return 0;
    }

    cp_mixer_disable(rt_res, rt_res->cp_mixer);
    sem_post(&rt_res->sem);
    pthread_mutex_lock(&rt_res->lock);
    ALOGD("YAYE %s entry get lock!!", __func__);
    rt_res->init = 0;
    pthread_mutex_unlock(&rt_res->lock);
    ALOGD("YAYE %s entry release lock!!", __func__);

    if(rt_res->thread){
    ret = pthread_join(rt_res->thread, NULL);
    sem_destroy(&rt_res->sem);
    sem_destroy(&rt_res->sem_end);
    rt_res->thread = (pthread_t)NULL;
    }
    pthread_mutex_destroy(&rt_res->lock);

    if(rt_res)
        free(rt_res);
    ALOGD("YAYE %s exit!!", __func__);

    return ret;
}

int cp_mixer_is_enable(struct cp_control_res * record_tone_info){
    int ret=0;

    if(record_tone_info){
    pthread_mutex_lock(&record_tone_info->lock);
        if(record_tone_info->enable){
            ret = 1;
        }
    pthread_mutex_unlock(&record_tone_info->lock);
    }

    return ret;
}

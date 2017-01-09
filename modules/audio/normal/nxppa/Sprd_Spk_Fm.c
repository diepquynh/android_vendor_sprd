#define LOG_TAG "sprdspkfm"
#include "Sprd_Spk_Fm.h"
#include "fmrecordhal_api.h"
#include <utils/Log.h>


struct pcm_config fm_play_config = {
    .channels = 2,
    .rate = 44100,
    .period_size = (160*4*2),
    .period_count = 4,
    .format = PCM_FORMAT_S16_LE,
};
typedef struct t_nxppa_res
{
    pthread_t pTthread;
    struct pcm *pcm_tfa;
    int is_start;
    int is_stop;
    sem_t           sem_stop;
    fmtrackhandle FmTrack_handle;
    FMPcmHandler HalSource_handle;
    pthread_mutex_t FmLock;
}T_NXPPA_RES;

void * SprdFm_Spk_threadentry(T_NXPPA_RES * nxp_fm_res)
{
    unsigned int size;
    char *buffer;
    int status;
    int flag = 0;
    int frames=0;
    int is_block = 1;
    int bytes_read = 0;
    int MinBufferSize = 2560;

    size = fm_play_config.period_size;
    buffer = malloc(size);
    if (NULL == buffer)
    {
        ALOGE("Sprdfm: Calloc buffer fail!");
        goto ERROR;
    }

    nxp_fm_res->FmTrack_handle = FmTrack_Open(fm_play_config.rate,MinBufferSize, 2);
    if( nxp_fm_res->FmTrack_handle == NULL)
    {
        ALOGE("Sprdfm: FmTrack init error");
        goto ERROR;
    }

    status = FmTrack_Start(nxp_fm_res->FmTrack_handle);
    if(0 != status)
    {
        ALOGE("Sprdfm: FmTrack_Start fail!");
        goto ERROR;
    }

    ALOGE("SprdFm_Spk_threadentry Loop start");

    //FILE *pFmFile=fopen("/data/local/media/e","wb+");
    while(!nxp_fm_res->is_stop)
    {
        if(nxp_fm_res->is_start)
        {
            bytes_read = fm_pcm_read(nxp_fm_res->HalSource_handle, buffer, size, is_block, 2);
            if(bytes_read == 0)
            {
                ALOGE("Sprdfm:Read Pcm_Tfa_Fm Error! ret = %d", status);
                usleep(20000);
                continue;
            }
            frames += bytes_read/4;
            if((frames >= MinBufferSize) && (is_block == 1)) {
                is_block = 0;
            }
            //fwrite(buffer,size,1,pFmFile);
            status = FmTrack_Write(nxp_fm_res->FmTrack_handle, buffer, bytes_read, 1);
            if(status < 0)
            {
                ALOGE("Sprdfm:FmTrack Write ERR status = %d", status);
                continue;
            }
        }
        else
        {
            usleep(10000);
        }
    }
    if(nxp_fm_res->FmTrack_handle) {
        FmTrack_Close(nxp_fm_res->FmTrack_handle);
        nxp_fm_res->FmTrack_handle = NULL;
    }
    ALOGE("SprdFm:RW thread exit!");
    if (buffer != NULL)
        free(buffer);
   sem_post(&nxp_fm_res->sem_stop);
   ALOGE("SprdFm_Spk_threadentry: ok return");
   return NULL;
ERROR:
    ALOGE("SprdFm_Spk_threadentry: failed return");
     if(nxp_fm_res->FmTrack_handle) {
        FmTrack_Close(nxp_fm_res->FmTrack_handle);
        nxp_fm_res->FmTrack_handle = NULL;
    }
    return NULL;
}

int NxpTfa_Fm_Start(fm_handle pFmRes)
{
    int card = 0;
    int ret = 0;
    int status;

    T_NXPPA_RES *nxp_fm_res = NULL;
    nxp_fm_res = (T_NXPPA_RES *)pFmRes;
    nxp_fm_res->is_stop = 0;
    ALOGE("sprdfm: NxpTfa_Fm_Start in!");
    pthread_mutex_lock(&nxp_fm_res->FmLock);
    if(nxp_fm_res->is_start ==1) {
        pthread_mutex_unlock(&nxp_fm_res->FmLock);
        return 0;
    }

    nxp_fm_res->HalSource_handle = fm_pcm_open(fm_play_config.rate, fm_play_config.channels, fm_play_config.period_size*4, 2);
    if(!nxp_fm_res->HalSource_handle)
    {
        ALOGE("Sprdfm: %s,open pcm fail, code is %s", __func__, strerror(errno));
        goto Error;
    }

    if(nxp_fm_res->pTthread == 0) {
        ret = pthread_create(&nxp_fm_res->pTthread, NULL, SprdFm_Spk_threadentry, nxp_fm_res);
        if (ret != 0)
        {
            ALOGE("Sprdfm: %s,pthread_create falied, code is %s", __func__, strerror(errno));
            goto Error;
        }
        nxp_fm_res->is_start = 1;
    }
    pthread_mutex_unlock(&nxp_fm_res->FmLock);
    ALOGE("Sprdfm: NxpTfa_Fm_Start out");
    return 0;
Error:
    ALOGE("Sprdfm: NxpTfa_Fm_Start out error");
    if(nxp_fm_res->HalSource_handle) {
        fm_pcm_close(nxp_fm_res->HalSource_handle);
        nxp_fm_res->HalSource_handle = NULL;
    }
    pthread_mutex_unlock(&nxp_fm_res->FmLock);
    return -1;
}

int NxpTfa_Fm_Stop( fm_handle handle)
{
    int rc = 0;
    if(NULL == handle)
    {
        ALOGE("Sprdfm: NxpTfa_Fm_Stop ,fm not start");
        return 0;
    }
    T_NXPPA_RES * nxp_fm_res = NULL;
    nxp_fm_res =(T_NXPPA_RES *)handle;
    pthread_mutex_lock(&nxp_fm_res->FmLock);
    ALOGE("Sprdfm:NxpTfa_Fm_Stop in,nxp_fm_res->is_start= %d",nxp_fm_res->is_start);
    if(1 != nxp_fm_res->is_start) {
        pthread_mutex_unlock(&nxp_fm_res->FmLock);
        ALOGE("Sprdfm: NxpTfa_Fm_Stop error");
        return -1;
    }

    nxp_fm_res->is_stop = 1;
    sem_wait(&nxp_fm_res->sem_stop);
    pthread_join(nxp_fm_res->pTthread, (void **)NULL);
    nxp_fm_res->pTthread = 0;
    if(nxp_fm_res->HalSource_handle) {
        fm_pcm_close(nxp_fm_res->HalSource_handle);
        nxp_fm_res->HalSource_handle = NULL;
    }
    nxp_fm_res->is_start = 0;
    pthread_mutex_unlock(&nxp_fm_res->FmLock);
    ALOGE("Sprdfm: NxpTfa_Fm_Stop out");
    return 0;
}


fm_handle NxpTfa_Fm_Open(void)
{
    int ret = 0;
    T_NXPPA_RES * nxp_fm_res = NULL;
    ALOGE("Sprdfm: NxpTfa_Fm_Open in");
    nxp_fm_res = malloc(sizeof(T_NXPPA_RES));
    memset(nxp_fm_res, 0, sizeof(T_NXPPA_RES));
    if (ret) {
        ALOGE("Sprdfm:sem_init nxp_fm_res->is_stop, code is %s", strerror(errno));
        return ret;
    }
    ret = sem_init(&nxp_fm_res->sem_stop, 0, 0);
    if (ret) {
        ALOGE("Sprdfm:sem_init nxp_fm_res->is_stopsssssss, code is %s", strerror(errno));
        return ret;
    }
    pthread_mutex_init(&nxp_fm_res->FmLock, NULL);
    ALOGE("Sprdfm: NxpTfa_Fm_Open out ok");
    return (fm_handle)nxp_fm_res;
}

int NxpTfa_Fm_Close(fm_handle handle)
{
    ALOGE("Sprdfm: NxpTfa_Fm_Close in");
    int ret = 0;
    T_NXPPA_RES * nxp_fm_res = handle;
    pthread_mutex_lock(&nxp_fm_res->FmLock);
    ret = sem_destroy(&nxp_fm_res->sem_stop);
    if (ret) {
        ALOGE("Sprdfm:sem_init falied, code is %s", strerror(errno));
        return ret;
    }
    free(nxp_fm_res);
    ALOGE("Sprdfm: NxpTfa_Fm_Close out");
    return 0;
}


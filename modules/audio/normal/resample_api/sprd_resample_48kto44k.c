#define LOG_TAG "sprdresample"
#include"sprd_resample_48kto44k.h"
#include<stdio.h>
#include <utils/Log.h>

#define SRC_SAMPLE_RATE     48000
#define DEST_SAMPLE_RATE    44100
#define NR_UNIT 1920
#define UNIT NR_UNIT

typedef struct tResample48kto44k{
    TRANSAM44K_CONTEXT_T  *pTrans44K;
    void *pTransInLeftBuffer;
    void *pTransInRightBuffer;
    void *pTransOutBuffer;
    void *pTransReadBuffer;
    void *pTransStoreBuffer;//storage the left data
    int storage_bytes;
    callback read_data;
    void *data_handle;
    int channel;
}T_TRANSFORM48K_TO_44K;

transform_handle SprdSrc_To_44K_Init(callback get_proc_data,void *data_handle, int request_channel)
{
    int dwRc = 0;
    T_TRANSFORM48K_TO_44K *pTransform=NULL;
    pTransform = (T_TRANSFORM48K_TO_44K *) malloc(sizeof(T_TRANSFORM48K_TO_44K));
    if(NULL == pTransform)
        goto err;
    memset(pTransform, 0, sizeof(T_TRANSFORM48K_TO_44K));

    pTransform->pTrans44K = (void *)malloc(sizeof(TRANSAM44K_CONTEXT_T));
    if(NULL == pTransform->pTrans44K)
        goto err;
    memset(pTransform->pTrans44K, 0, sizeof(TRANSAM44K_CONTEXT_T));

    //in,left and right buffer
    pTransform->pTransInLeftBuffer = (void *)malloc(UNIT);
    if(NULL == pTransform->pTransInLeftBuffer)
        goto err;
    memset(pTransform->pTransInLeftBuffer, 0 , UNIT);

    pTransform->pTransInRightBuffer = (void *)malloc(UNIT);
    if(NULL == pTransform->pTransInRightBuffer)
        goto err;
    memset(pTransform->pTransInRightBuffer, 0 ,UNIT);

    //out buffer
    pTransform->pTransOutBuffer = (void *)malloc(UNIT*2);
    if(NULL == pTransform->pTransOutBuffer)
        goto err;
    memset(pTransform->pTransOutBuffer, 0, UNIT*2);

    pTransform->pTransReadBuffer = (void *)malloc(UNIT);
    if(NULL == pTransform->pTransReadBuffer)
        goto err;
    memset(pTransform->pTransReadBuffer, 0 ,UNIT);

    pTransform->pTransStoreBuffer = (short *)malloc(UNIT*2);//give enough
    if(NULL == pTransform->pTransStoreBuffer)
        goto err;
    memset(pTransform->pTransStoreBuffer, 0, UNIT*2);

    ALOGW("SprdSrc_To_441K_Init in");
    dwRc = MP344K_InitParam(pTransform->pTrans44K, SRC_SAMPLE_RATE, UNIT);
    if(dwRc < 0)
    {
        ALOGE("Sprd_NxpTfa: SRC_to_44K_Init fail!");
        goto err;
    }
    pTransform->storage_bytes=0;

    pTransform->read_data  = get_proc_data;
    pTransform->data_handle = data_handle;
    pTransform->channel = request_channel;
    ALOGW("SprdSrc_To_44K_Init: dwRc: %d ",dwRc);
    return (void *)pTransform;
err:
    if(pTransform)
    {
        if(pTransform->pTransOutBuffer)
            free(pTransform->pTransOutBuffer);
        if(pTransform->pTransInLeftBuffer)
            free(pTransform->pTransInLeftBuffer);
        if(pTransform->pTransInRightBuffer)
            free(pTransform->pTransInRightBuffer);
        if(pTransform->pTransReadBuffer)
            free(pTransform->pTransReadBuffer);
        if(pTransform->pTransStoreBuffer)
            free(pTransform->pTransStoreBuffer);
        if(pTransform->pTrans44K)
            free(pTransform->pTrans44K);
        free(pTransform);
    }
    return NULL;
}


int SprdSrc_To_44K_DeInit(transform_handle handle)
{
    int dwRc = 0;
    T_TRANSFORM48K_TO_44K *pTransform=NULL;
    pTransform = (T_TRANSFORM48K_TO_44K *)handle;
    if(pTransform)
    {
        if(pTransform->pTrans44K)
        {
            dwRc = MP344K_DeInitParam(pTransform->pTrans44K);
            if(dwRc < 0)
            {
                ALOGE("Sprd_NxpTfa: SprdSrc_48K_DeInit fail!");
            }
            free(pTransform->pTrans44K);
        }
        if(pTransform->pTransOutBuffer)
            free(pTransform->pTransOutBuffer);
        if(pTransform->pTransInLeftBuffer)
            free(pTransform->pTransInLeftBuffer);
        if(pTransform->pTransInRightBuffer)
            free(pTransform->pTransInRightBuffer);
        if(pTransform->pTransReadBuffer)
            free(pTransform->pTransReadBuffer);
        if(pTransform->pTransStoreBuffer)
            free(pTransform->pTransStoreBuffer);
        free(pTransform);
    }
    return dwRc;
}

static int SprdSrc_To_44K_proc(transform_handle handle,void *pInBuf , int in_bytes,int channel)
{
    int i, count, OutSize;
    T_TRANSFORM48K_TO_44K *pTransform=NULL;
    pTransform = (T_TRANSFORM48K_TO_44K *)handle;
    TRANSAM44K_CONTEXT_T *pTrans44K = pTransform->pTrans44K;

    short *pSrcLBuf = (short *)(pTransform->pTransInLeftBuffer);
    short *pSrcRBuf = (short *)(pTransform->pTransInRightBuffer);
    short *pOutBuf = (short*)(pTransform->pTransOutBuffer);
    short *pSrcBuf = (short *)pInBuf;

       if(channel == 1)
    {
        for (i = 0; i < in_bytes/2; i++) /*compute L/R Channel*/
        {
            pSrcLBuf[i] = pSrcBuf[i];
            pSrcRBuf[i] = pSrcBuf[i];
        }
        count = MP3_Play44k(pTrans44K, pSrcLBuf, pSrcRBuf, in_bytes/2, SRC_SAMPLE_RATE, pOutBuf, pOutBuf+in_bytes/2);
        for (i = 0; i <count; i++)
        {
            pSrcBuf[i] = pOutBuf[i];
        }
        OutSize = count * 2;
    }
    else
    {
        for (i = 0; i < in_bytes/4; i++) /*compute L/R Channel*/
        {
            pSrcLBuf[i] = pSrcBuf[i*2];
            pSrcRBuf[i] = pSrcBuf[i*2+1];
        }
        count = MP3_Play44k(pTrans44K, pSrcLBuf, pSrcRBuf, in_bytes/4, SRC_SAMPLE_RATE, pOutBuf, pOutBuf+in_bytes/4);
        for ( i = 0;  i < count; i++)
        {
            pSrcBuf[i*2] = pOutBuf[i];
            pSrcBuf[i*2+1] = pOutBuf[i + in_bytes/4];
        }
        OutSize = count * 4;
    }
    ALOGE("SprdSrc_To_44K in_bytes =%d ,count =%d ", in_bytes, count);
    return OutSize;
}

int SprdSrc_To_44K(transform_handle handle, void *buffer , int bytes)
{
    int ret;
    int transfered_size,out_size;
    int in_unit = UNIT;
    T_TRANSFORM48K_TO_44K *pTransform=NULL;
    pTransform = (T_TRANSFORM48K_TO_44K *)handle;
    void *pReadBuffer = pTransform->pTransReadBuffer;
    int new_storage_size;
    int channel = pTransform->channel;

    if(pTransform->storage_bytes < bytes)
    {
        memcpy((char *)buffer,(char *)pTransform->pTransStoreBuffer, pTransform->storage_bytes);
        transfered_size = pTransform->storage_bytes;
        pTransform->storage_bytes = 0;
        while(1)
        {
            ret = pTransform->read_data(pTransform->data_handle, pReadBuffer,  in_unit);
            if(ret <0)
            {
               ALOGE("get nr data error");
               return -1;;
            }
            out_size = SprdSrc_To_44K_proc(pTransform, pReadBuffer, in_unit, channel);
            if( (bytes-transfered_size) > out_size)
            {
                memcpy((char *)buffer+transfered_size, (char*)pReadBuffer,out_size);
                transfered_size+=out_size;
            }
            else
            {
                memcpy((char *)buffer+transfered_size, (char*)pReadBuffer,bytes-transfered_size );
                //storage the bytes
                new_storage_size = transfered_size+ out_size-bytes;//update the storage size
                memcpy((char *)pTransform->pTransStoreBuffer, (char *)pReadBuffer+(out_size - new_storage_size), new_storage_size);
                pTransform->storage_bytes = new_storage_size;
                ALOGW("pTransform->storage_bytes = %d",pTransform->storage_bytes);
                break;
            }
        }
    }
    else
    {
        memcpy((char *)buffer, (char *)pTransform->pTransReadBuffer, bytes);
        pTransform->storage_bytes = pTransform->storage_bytes -bytes;
        memcpy((char *)pTransform->pTransReadBuffer, (char *)pTransform->pTransReadBuffer+bytes, pTransform->storage_bytes);
    }
    return ret;
}
